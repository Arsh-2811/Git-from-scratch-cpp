#include "headers/diff.h"
#include "headers/refs.h"
#include "headers/objects.h"
#include "headers/utils.h"
#include "headers/index.h"

#include <iostream>
#include <set>

std::string get_workdir_sha(const std::string& path) {
    try {
        std::string content = read_file(path);
        // If read_file returns empty for non-existent file, compute_sha1 handles it.
        // If read_file throws, we catch it below.
        return compute_sha1(content);
    } catch (const std::exception& e) {
        std::cerr << "Warning: Cannot read/hash workdir file " << path << ": " << e.what() << std::endl;
        return ""; // Return empty on error
    }
}

void read_tree_recursive(const std::string& tree_sha1, const std::string& current_path, std::map<std::string, std::string>& contents) {
    if (tree_sha1.empty()) return;

    try {
        ParsedObject parsed_obj = read_object(tree_sha1);
        if (parsed_obj.type != "tree") {
            std::cerr << "Warning: Expected tree object, got " << parsed_obj.type << " for SHA " << tree_sha1 << std::endl;
            return;
        }
        const auto& tree = std::get<TreeObject>(parsed_obj.data);

        for (const auto& entry : tree.entries) {
            std::string full_path = current_path.empty() ? entry.name : current_path + "/" + entry.name;
            if (entry.mode == "40000") {
                read_tree_recursive(entry.sha1, full_path, contents);
            } else {
                contents[full_path] = entry.sha1;
            }
        }
    } catch (const std::exception& e) {
        std::cerr << "Warning: Failed to read tree object " << tree_sha1 << ": " << e.what() << std::endl;
    }
}

std::map<std::string, std::string> read_tree_contents(const std::string& tree_sha1) {
    std::map<std::string, std::string> contents;
    read_tree_recursive(tree_sha1, "", contents);
    return contents;
}

std::map<std::string, StatusEntry> get_repository_status() {
    std::map<std::string, StatusEntry> status_map;
    std::set<std::string> all_paths; // Collect all relevant paths here

    // 1. Get HEAD commit's tree contents {path: sha1}
    std::map<std::string, std::string> head_tree_contents;
    std::optional<std::string> head_commit_sha = resolve_ref("HEAD");
    if (head_commit_sha) {
        try {
            ParsedObject commit_obj = read_object(*head_commit_sha);
            if (commit_obj.type == "commit") {
                std::string tree_sha = std::get<CommitObject>(commit_obj.data).tree_sha1;
                if (!tree_sha.empty()) { // Handle commits without trees? (Shouldn't happen often)
                    head_tree_contents = read_tree_contents(tree_sha);
                    for (const auto& pair : head_tree_contents) {
                        all_paths.insert(pair.first);
                    }
                }
            }
        } catch (...) { /* Ignore errors reading HEAD commit */ }
    }

    // 2. Read the index {path: {stage: IndexEntry}}
    IndexMap index = read_index();
    std::map<std::string, IndexEntry> index_stage0; // Simplified view {path: Entry} for stage 0
    bool has_conflicts = false;
    for (const auto& path_pair : index) {
        all_paths.insert(path_pair.first);
        bool path_conflicted = false;
        for (const auto& stage_pair : path_pair.second) {
            if (stage_pair.first > 0) {
                has_conflicts = true;
                path_conflicted = true;
            }
        }
        auto stage0_it = path_pair.second.find(0);
        if (stage0_it != path_pair.second.end()) {
            index_stage0[path_pair.first] = stage0_it->second;
            if (path_conflicted) {
                 status_map[path_pair.first].index_status = FileStatus::Conflicted;
            }
        } else if (path_conflicted) {
             status_map[path_pair.first].index_status = FileStatus::Conflicted;
        }
    }

    // 3. Scan Working Directory for *existing* files and add their paths
    // We will calculate workdir SHAs *later* only when needed for comparison.
    std::set<std::string> workdir_existing_paths;
    try {
        if (!fs::exists(".")) {
            throw std::runtime_error("Current working directory does not exist.");
        }
        for (auto it = fs::recursive_directory_iterator("."), end = fs::recursive_directory_iterator(); it != end; ++it) {
            fs::path current_path_fs = it->path();
            std::string generic_rel_path;
            try {
                fs::path repo_root = fs::current_path();
                fs::path rel_path = fs::relative(current_path_fs, repo_root);
                generic_rel_path = rel_path.generic_string();
                if (generic_rel_path.empty() || generic_rel_path == ".") continue;
            } catch (...) { continue; }

            // Ignore logic
            bool ignored = false;
            if (generic_rel_path == GIT_DIR || generic_rel_path.rfind(GIT_DIR + "/", 0) == 0) {
                ignored = true;
            }
            // TODO: Add .gitignore parsing

            if (ignored) {
                if (it->is_directory()) it.disable_recursion_pending();
                continue;
            }

            if (it->is_regular_file() || it->is_symlink()) {
                all_paths.insert(generic_rel_path); // Add path to the master set
                workdir_existing_paths.insert(generic_rel_path); // Note that it exists
            }
        }
    } catch (const fs::filesystem_error& e) {
        throw std::runtime_error("Filesystem error during status scan: " + std::string(e.what()));
    }


    // 4. Iterate through all unique paths and determine status
    for (const std::string& path : all_paths) {
        // Get state flags
        bool in_head = (head_tree_contents.count(path) > 0);
        bool in_index0 = (index_stage0.count(path) > 0);
        bool in_workdir = (workdir_existing_paths.count(path) > 0); // Check existence first

        // Get SHAs (only compute workdir SHA if needed)
        std::string head_sha = in_head ? head_tree_contents[path] : "";
        std::string index_sha = in_index0 ? index_stage0[path].sha1 : "";
        std::string workdir_sha = ""; // Initialize empty

        // Get existing status entry (might already be marked Conflicted)
        StatusEntry& entry = status_map[path];
        entry.path = path; // Ensure path is set

        // Skip further checks if already marked conflicted
        if (entry.index_status == FileStatus::Conflicted) {
             continue;
        }

        // Determine Index vs HEAD status (same as before)
        if (in_index0 && in_head) {
            if (index_sha != head_sha) entry.index_status = FileStatus::ModifiedStaged;
        } else if (in_index0 && !in_head) {
            entry.index_status = FileStatus::AddedStaged;
        } else if (!in_index0 && in_head) {
            entry.index_status = FileStatus::DeletedStaged;
        }


        // Determine Workdir vs Index status
        if (in_index0) { // Path is tracked by the index (stage 0)
             if (in_workdir) {
                 workdir_sha = get_workdir_sha(path); // Compute SHA only if file exists and is tracked
                 if (workdir_sha.empty()) { // Handle hashing error
                     entry.workdir_status = FileStatus::ModifiedWorkdir; // Assume modified if error
                 } else if (workdir_sha != index_sha) {
                      entry.workdir_status = FileStatus::ModifiedWorkdir;
                 }
                 // else Unmodified (default)

                  // <<<--- DEBUG OUTPUT (moved here) --- >>>
                 if (path == "file1.txt" || path == "file2.txt") {
                     std::cout << "DEBUG_STATUS: Path=" << path
                               << " InWorkDir=YES IndexSHA=" << index_sha
                               << " WorkdirSHA=" << workdir_sha
                               << " Match=" << (!workdir_sha.empty() && index_sha == workdir_sha)
                               << " Status=" << static_cast<int>(entry.workdir_status) << std::endl;
                 }
                 // <<<--- END DEBUG --- >>>

             } else { // In index, but not in workdir
                  entry.workdir_status = FileStatus::DeletedWorkdir;
                  // <<<--- DEBUG OUTPUT --- >>>
                  if (path == "file1.txt" || path == "file2.txt") {
                      std::cout << "DEBUG_STATUS: Path=" << path
                                << " InWorkDir=NO IndexSHA=" << index_sha
                                << " WorkdirSHA=N/A"
                                << " Status=" << static_cast<int>(entry.workdir_status) << std::endl;
                  }
                  // <<<--- END DEBUG --- >>>
             }
        } else { // Path is not tracked by the index (stage 0)
             if (in_workdir) {
                  entry.workdir_status = FileStatus::AddedWorkdir; // Untracked
                  // <<<--- DEBUG OUTPUT --- >>>
                   if (path == "file1.txt" || path == "file2.txt") {
                       workdir_sha = get_workdir_sha(path); // Compute SHA for debug info
                       std::cout << "DEBUG_STATUS: Path=" << path
                                 << " InWorkDir=YES IndexSHA=N/A"
                                 << " WorkdirSHA=" << workdir_sha
                                 << " Status=" << static_cast<int>(entry.workdir_status) << std::endl;
                   }
                   // <<<--- END DEBUG --- >>>
             }
             // else: Not in index and not in workdir (relevant only if in HEAD, handled by index_status)
        }

    } // End loop through all_paths

    return status_map;
}
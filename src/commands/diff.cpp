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

// --- Tree Reading ---

// Recursive helper for read_tree_contents
void read_tree_recursive(const std::string& tree_sha1, const std::string& current_path_prefix, std::map<std::string, std::string>& contents) {
    if (tree_sha1.empty()) return;

    try {
        // Read the tree object itself
        // NOTE: Using read_object here which parses fully. Could optimize by just getting content.
        ParsedObject parsed_obj = read_object(tree_sha1);
        if (parsed_obj.type != "tree") {
            // This could happen if a commit points to a non-tree object
            std::cerr << "Warning: Expected tree object, got " << parsed_obj.type << " for SHA " << tree_sha1 << std::endl;
            return;
        }
        const auto& tree_obj = std::get<TreeObject>(parsed_obj.data); // Get the parsed TreeObject

        // Iterate through entries in *this* tree
        for (const auto& entry : tree_obj.entries) {
            // Construct full path relative to the root
            std::string full_path = current_path_prefix.empty() ? entry.name : current_path_prefix + "/" + entry.name;

            if (entry.mode == "40000") { // It's a subdirectory (subtree)
                // Recurse into the subtree
                read_tree_recursive(entry.sha1, full_path, contents);
            } else { // It's a blob (file) or symlink
                // Store the blob/symlink SHA associated with the full path
                contents[full_path] = entry.sha1;
                // If mode was needed: contents[full_path] = {entry.sha1, entry.mode};
            }
        }
    } catch (const std::exception& e) {
        // If tree object doesn't exist or is corrupt, stop recursion for this path
        std::cerr << "Warning: Failed to read or parse tree object " << tree_sha1.substr(0, 7) << ": " << e.what() << std::endl;
    }
}

// Public function to get flattened tree contents
std::map<std::string, std::string> read_tree_contents(const std::string& tree_sha1) {
    std::map<std::string, std::string> contents;
    read_tree_recursive(tree_sha1, "", contents); // Start recursion with empty prefix
    return contents;
}

void read_tree_full_recursive(const std::string& tree_sha1, const std::string& current_path_prefix, std::map<std::string, TreeEntry>& contents) {
    if (tree_sha1.empty()) return;
    try {
        ParsedObject parsed_obj = read_object(tree_sha1);
        if (parsed_obj.type != "tree") {
            std::cerr << "Warning: Expected tree object, got " << parsed_obj.type << " for SHA " << tree_sha1 << std::endl;
            return;
        }
        const auto& tree_obj = std::get<TreeObject>(parsed_obj.data);

        for (const auto& entry : tree_obj.entries) {
            std::string full_path = current_path_prefix.empty() ? entry.name : current_path_prefix + "/" + entry.name;
            if (entry.mode == "40000") { // Subdirectory
                // Recursively read the subtree
                read_tree_full_recursive(entry.sha1, full_path, contents);
            } else { // Blob or Symlink
                // Store the full TreeEntry associated with the full path
                TreeEntry full_path_entry = entry; // Copy entry data
                full_path_entry.name = full_path; // Use the full path as the effective name/key
                contents[full_path] = full_path_entry;
            }
        }
    } catch (const std::exception& e) {
        std::cerr << "Warning: Failed to read or parse tree object " << tree_sha1.substr(0, 7) << " for full read: " << e.what() << std::endl;
    }
}

// *** NEW: Public function for full tree map ***
std::map<std::string, TreeEntry> read_tree_full(const std::string& tree_sha1) {
    std::map<std::string, TreeEntry> contents;
    read_tree_full_recursive(tree_sha1, "", contents);
    return contents;
}

std::map<std::string, StatusEntry> get_repository_status() {
    std::map<std::string, StatusEntry> status_map;
    std::set<std::string> all_paths;

    // 1. Get HEAD commit's tree contents {path: sha1}
    std::map<std::string, std::string> head_tree_contents;
    std::optional<std::string> head_commit_sha = resolve_ref("HEAD");
    std::cout << "DEBUG_STATUS: Resolving HEAD commit..." << std::endl; // Keep this
    if (head_commit_sha) {
        std::cout << "DEBUG_STATUS: HEAD commit SHA = " << *head_commit_sha << std::endl; // Keep this
        try {
            ParsedObject commit_obj = read_object(*head_commit_sha);
            if (commit_obj.type == "commit") {
                std::string tree_sha = std::get<CommitObject>(commit_obj.data).tree_sha1;
                 std::cout << "DEBUG_STATUS: HEAD tree SHA = " << tree_sha << std::endl; // Keep this
                if (!tree_sha.empty()) {
                    head_tree_contents = read_tree_contents(tree_sha); // Call recursive read
                    // *** ADD THIS DEBUG BLOCK ***
                    std::cout << "DEBUG_STATUS: Read HEAD tree contents. Size = " << head_tree_contents.size() << std::endl;
                    for (const auto& pair : head_tree_contents) {
                         std::cout << "DEBUG_STATUS:   HEAD Tree Entry: " << pair.first << " -> " << pair.second.substr(0,7) << std::endl;
                    }
                    // *** END ADDED DEBUG BLOCK ***
                    for (const auto& pair : head_tree_contents) {
                        all_paths.insert(pair.first);
                    }
                } else {
                     std::cout << "DEBUG_STATUS: HEAD commit has empty tree SHA!" << std::endl;
                }
            } else {
                 std::cout << "DEBUG_STATUS: HEAD resolved object is not a commit (" << commit_obj.type << ")" << std::endl;
            }
        } catch (const std::exception& e) {
            std::cerr << "DEBUG_STATUS: Exception reading HEAD commit/tree: " << e.what() << std::endl;
         }
    } else {
         std::cout << "DEBUG_STATUS: HEAD could not be resolved to a commit." << std::endl;
    }


    // 2. Read the index {path: {stage: IndexEntry}}
    // ... (Index reading logic as before, populating index_stage0 and all_paths) ...
    IndexMap index = read_index();
    std::map<std::string, IndexEntry> index_stage0;
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
    // ... (Workdir scanning logic as before, populating workdir_existing_paths and all_paths) ...
    std::set<std::string> workdir_existing_paths;
    try {
        // ... (recursive_directory_iterator loop as before) ...
         if (!fs::exists(".")) { throw std::runtime_error("CWD does not exist."); }
        for (auto it = fs::recursive_directory_iterator("."), end = fs::recursive_directory_iterator(); it != end; ++it) {
            // ... (path calculation, ignore logic) ...
             fs::path current_path_fs = it->path();
            std::string generic_rel_path;
            try {
                fs::path repo_root = fs::current_path();
                fs::path rel_path = fs::relative(current_path_fs, repo_root);
                generic_rel_path = rel_path.generic_string();
                if (generic_rel_path.empty() || generic_rel_path == ".") continue;
            } catch (...) { continue; }
             bool ignored = false;
            if (generic_rel_path == GIT_DIR || generic_rel_path.rfind(GIT_DIR + "/", 0) == 0) ignored = true;
            if (ignored) { if (it->is_directory()) it.disable_recursion_pending(); continue; }

            if (it->is_regular_file() || it->is_symlink()) {
                all_paths.insert(generic_rel_path);
                workdir_existing_paths.insert(generic_rel_path);
            }
        }
    } catch (const fs::filesystem_error& e) { /* ... */ }


    // 4. Iterate through all unique paths and determine status
    // ... (Status determination logic using head_tree_contents, index_stage0, workdir_existing_paths as finalized in previous step) ...
    for (const std::string& path : all_paths) {
        bool in_head = (head_tree_contents.count(path) > 0);
        bool in_index0 = (index_stage0.count(path) > 0);
        bool in_workdir = (workdir_existing_paths.count(path) > 0);

        std::cout << "DEBUG_STATUS: Checking path='" << path
                  << "' | in_head=" << in_head
                  << " | in_index0=" << in_index0
                  << " | in_workdir=" << in_workdir << std::endl;

        // Get SHAs
        std::string head_sha = in_head ? head_tree_contents.at(path) : ""; // Use .at for clarity? No, [] is fine.
        std::string index_sha = in_index0 ? index_stage0.at(path).sha1 : "";
        std::string workdir_sha = ""; // Calculated later if needed

        // *** Initialize status explicitly for this path ***
        FileStatus current_index_status = FileStatus::Unmodified; // Default
        FileStatus current_workdir_status = FileStatus::Unmodified; // Default

        // Check if already marked conflicted during index read
         bool already_conflicted = false;
         if (status_map.count(path) && status_map[path].index_status == FileStatus::Conflicted) {
             already_conflicted = true;
             current_index_status = FileStatus::Conflicted; // Preserve conflict status
         }

        if (!already_conflicted) {
            // Determine Index vs HEAD status
            if (in_index0 && in_head) {
                if (index_sha != head_sha) current_index_status = FileStatus::ModifiedStaged;
            } else if (in_index0 && !in_head) {
                current_index_status = FileStatus::AddedStaged;
            } else if (!in_index0 && in_head) {
                current_index_status = FileStatus::DeletedStaged;
            }
            std::cout << "DEBUG_STATUS:   -> IndexStatus=" << static_cast<int>(current_index_status) << std::endl;

            // Determine Workdir vs Index status
            if (in_index0) {
                if (in_workdir) {
                    workdir_sha = get_workdir_sha(path);
                    if (workdir_sha.empty() || workdir_sha != index_sha) {
                        current_workdir_status = FileStatus::ModifiedWorkdir;
                    }
                } else { current_workdir_status = FileStatus::DeletedWorkdir; }
            } else { if (in_workdir) { current_workdir_status = FileStatus::AddedWorkdir; } }
            std::cout << "DEBUG_STATUS:   -> WorkdirStatus=" << static_cast<int>(current_workdir_status) << std::endl;
        } else {
             std::cout << "DEBUG_STATUS:   -> Path was pre-marked Conflicted." << std::endl;
        }

        // Update the map entry for this path
        status_map[path].path = path;
        status_map[path].index_status = current_index_status;
        status_map[path].workdir_status = current_workdir_status;
    }

    return status_map;
}
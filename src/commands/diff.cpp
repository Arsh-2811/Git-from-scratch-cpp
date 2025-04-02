#include "headers/diff.h"
#include "headers/refs.h"
#include "headers/objects.h"
#include "headers/utils.h"
#include "headers/index.h"

#include <iostream>
#include <set>

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
    std::set<std::string> processed_paths;

    std::map<std::string, std::string> head_tree_contents;
    std::optional<std::string> head_commit_sha = resolve_ref("HEAD");
    if (head_commit_sha) {
        try {
            ParsedObject commit_obj = read_object(*head_commit_sha);
            if (commit_obj.type == "commit") {
                 std::string tree_sha = std::get<CommitObject>(commit_obj.data).tree_sha1;
                 head_tree_contents = read_tree_contents(tree_sha);
            } else {
                 std::cerr << "Warning: HEAD resolved to a non-commit object: " << *head_commit_sha << std::endl;
            }
        } catch (const std::exception& e) {
            std::cerr << "Warning: Could not read HEAD commit object (" << *head_commit_sha << "): " << e.what() << std::endl;
        }
    }

    IndexMap index = read_index();

    std::set<std::string> all_paths = {};
    for (const auto& pair : head_tree_contents) all_paths.insert(pair.first);
    for (const auto& pair : index) all_paths.insert(pair.first);


    for(const std::string& path : all_paths) {
        processed_paths.insert(path);
        auto head_it = head_tree_contents.find(path);
        auto index_stages_it = index.find(path);

        bool in_head = (head_it != head_tree_contents.end());
        bool in_index = (index_stages_it != index.end() && !index_stages_it->second.empty());

        if (in_index) {
            bool conflicted = false;
            for(const auto& stage_pair : index_stages_it->second) {
                if (stage_pair.first > 0) {
                    conflicted = true;
                    break;
                }
            }
            if (conflicted) {
                status_map[path].index_status = FileStatus::Conflicted;
            } else {
                auto stage0_it = index_stages_it->second.find(0);
                if (stage0_it == index_stages_it->second.end()) {
                    std::cerr << "Warning: Path " << path << " in index but no stage 0 found and not conflicted." << std::endl;
                    continue; // Should not happen in normal operation
                }
                const IndexEntry& index_entry = stage0_it->second;

                if (in_head) {
                    if (head_it->second != index_entry.sha1) {
                        status_map[path].index_status = FileStatus::ModifiedStaged;
                    } else {
                        status_map[path].index_status = FileStatus::Unmodified; // Assume mode matches if SHA matches
                    }
                } else {
                    status_map[path].index_status = FileStatus::AddedStaged;
                }
            }

        } else {
            if (in_head) {
                status_map[path].index_status = FileStatus::DeletedStaged;
            } else {

            }
        }
        status_map[path].path = path;
    }
    try {
        if (!fs::exists(".")) {
            throw std::runtime_error("Current working directory does not exist.");
        }

        // TODO : Confirm the working of this code
        for (auto it = fs::recursive_directory_iterator("."), end = fs::recursive_directory_iterator(); it != end; ++it) {
            fs::path current_path_fs = it->path();
            std::string current_path = current_path_fs.lexically_relative(fs::current_path()).generic_string();

            bool ignored = false;
            if (current_path == GIT_DIR || current_path.rfind(GIT_DIR + "/", 0) == 0) {
                ignored = true;
            }

            if (ignored) {
                if (it->is_directory()) {
                    
                    it.disable_recursion_pending();
                }
                continue;
            }

            if (it->is_regular_file() || it->is_symlink()) {
                processed_paths.insert(current_path);
                auto index_stages_it = index.find(current_path);
                bool in_index = (index_stages_it != index.end() && !index_stages_it->second.empty());
                bool index_conflicted = false;
                if (in_index) {
                    for (const auto& stage_pair : index_stages_it->second) {
                        if (stage_pair.first > 0) index_conflicted = true;
                    }
                }


                if (in_index && !index_conflicted) {
                    auto stage0_it = index_stages_it->second.find(0);
                    if (stage0_it == index_stages_it->second.end()) {
                        continue;
                    }
                    const IndexEntry& index_entry = stage0_it->second;

                    try {
                        std::string workdir_content = read_file(current_path);
                        std::string workdir_sha1 = compute_sha1(workdir_content);

                        if (index_entry.sha1 != workdir_sha1) {
                            status_map[current_path].workdir_status = FileStatus::ModifiedWorkdir;
                        } else {
                            status_map[current_path].workdir_status = FileStatus::Unmodified;
                        }
                    } catch (const std::exception& e) {
                        std::cerr << "Warning: Cannot read/hash workdir file " << current_path << ": " << e.what() << std::endl;
                        status_map[current_path].workdir_status = FileStatus::ModifiedWorkdir;
                    }

                } else if (!in_index) {
                    status_map[current_path].workdir_status = FileStatus::AddedWorkdir;
                }
                status_map[current_path].path = current_path;
             }
        }
    } catch (const fs::filesystem_error& e) {
        throw std::runtime_error("Filesystem error during status scan: " + std::string(e.what()));
    }

    for(const auto& index_pair : index) {
        const std::string& path = index_pair.first;

        auto stage0_it = index_pair.second.find(0);
        if (stage0_it != index_pair.second.end()) {
            if (!file_exists(path)) {
                processed_paths.insert(path);
                status_map[path].workdir_status = FileStatus::DeletedWorkdir;
                status_map[path].path = path;
            }
        }
    }
    return status_map;
}
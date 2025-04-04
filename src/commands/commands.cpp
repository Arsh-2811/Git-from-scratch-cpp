#include "headers/commands.h"
#include "headers/diff.h"
#include "headers/refs.h"
#include "headers/objects.h"
#include "headers/utils.h"
#include "headers/index.h"

#include <iostream>
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <set>
#include <queue>
#include <map>

int handle_init() {
    fs::path git_dir_path = GIT_DIR;
    fs::path objects_path = OBJECTS_DIR;
    fs::path refs_path = REFS_DIR;

    if (fs::exists(git_dir_path)) {
        if (fs::exists(objects_path) && fs::exists(refs_path) && fs::exists(git_dir_path / "HEAD")) {
            std::cerr << "Reinitialized existing Git repository in " << fs::absolute(git_dir_path).string() << std::endl;
            return 0;
        } else {
            std::cerr << "Error: '" << GIT_DIR << "' already exists but is not a valid repository." << std::endl;
            return 1;
        }
    }
    try {
        fs::create_directory(git_dir_path);
        fs::create_directories(objects_path / "info");
        fs::create_directories(objects_path / "pack");
        fs::create_directories(refs_path / "heads");
        fs::create_directories(refs_path / "tags");
        fs::create_directories(git_dir_path / "info");

        std::ofstream head_file(git_dir_path / "HEAD");
        if (!head_file) throw std::runtime_error("Failed to create HEAD file.");
        head_file << "ref: refs/heads/main\n";
        head_file.close();

        std::ofstream config_file(git_dir_path / "config");
        if (!config_file) throw std::runtime_error("Failed to create config file.");
        config_file << "[core]\n"
                    << "\trepositoryformatversion = 0\n"
                    << "\tfilemode = true\n" // Adjust based on OS/needs
                    << "\tbare = false\n"
                    << "\tlogallrefupdates = true\n";
        config_file.close();

        std::ofstream desc_file(git_dir_path / "description");
        if (!desc_file) throw std::runtime_error("Failed to create description file.");
        desc_file << "Unnamed repository; edit this file 'description' to name the repository.\n";
        desc_file.close();

        std::ofstream exclude_file(git_dir_path / "info" / "exclude");
        if (!exclude_file) throw std::runtime_error("Failed to create info/exclude file.");
        exclude_file << "# git ls-files --others --exclude-from=.mygit/info/exclude\n";
        exclude_file << "# Lines that start with '#' are comments.\n";
        exclude_file << "# For a project mostly in C, you might want to ignore\n";
        exclude_file << "# generated files and binaries:\n";
        exclude_file << "# *.[oa]\n";
        exclude_file << "# *~\n";
        exclude_file.close();

        std::cout << "Initialized empty Git repository in " << fs::absolute(git_dir_path).string() << std::endl;
        return 0;
    } catch (const fs::filesystem_error& e) {
        std::cerr << "Error creating Git directory structure: " << e.what() << std::endl;
        return 1;
    } catch (const std::runtime_error& e) {
        std::cerr << "Error initializing repository: " << e.what() << std::endl;
        return 1;
    }
}

bool add_single_file_to_index(const std::string& file_path_to_add, IndexMap& index_map) {
    try {
        // Basic ignore check (can be expanded with .gitignore later)
        if (file_path_to_add == GIT_DIR || file_path_to_add.rfind(GIT_DIR + "/", 0) == 0) {
            // std::cout << "DEBUG_ADD: Ignoring path within .mygit: " << file_path_to_add << std::endl;
            return true; // Skipping is considered success in this context
        }

        // 1. Read file content
        std::string content = read_file(file_path_to_add);
        // Note: read_file might return empty if file doesn't exist or is empty.
        // hash_and_write_object handles empty content correctly (creates empty blob).

        // 2. Create blob object (writes if not exists)
        std::string sha1 = hash_and_write_object("blob", content);

        // 3. Get file mode
        mode_t mode_raw = get_file_mode(file_path_to_add);
         if (mode_raw == 0) {
              std::cerr << "Warning: Could not determine mode for file: " << file_path_to_add << ". Using default 100644." << std::endl;
              mode_raw = 0100644; // Default to non-executable
         }
         // Skip adding directories themselves to index (Git doesn't store empty dirs)
         if (mode_raw == 0040000) {
             return true; // Successfully skipped directory placeholder
         }

        std::stringstream ss;
        ss << std::oct << mode_raw;
        std::string mode_str = ss.str();

        // 4. Prepare index entry (stage 0)
        IndexEntry entry;
        entry.mode = mode_str;
        entry.sha1 = sha1;
        entry.stage = 0;
        entry.path = file_path_to_add; // Use the relative path passed in

        // 5. Add/Update entry in the index map
        remove_entry(index_map, file_path_to_add, 1);
        remove_entry(index_map, file_path_to_add, 2);
        remove_entry(index_map, file_path_to_add, 3);
        add_or_update_entry(index_map, entry);
        // std::cout << "DEBUG_ADD: Added/Updated " << file_path_to_add << std::endl;

    } catch (const std::exception& e) {
        std::cerr << "Error adding file '" << file_path_to_add << "': " << e.what() << std::endl;
        return false; // Indicate error for this specific file
    }
    return true; // Success for this file
}

int handle_add(const std::vector<std::string>& files_to_add) {
    if (files_to_add.empty()) {
        std::cerr << "Nothing specified, nothing added." << std::endl;
        std::cerr << "Maybe you wanted to say 'mygit add .'?" << std::endl;
        return 1;
    }

    IndexMap index = read_index(); // Read index once
    std::set<std::string> processed_files; // Track files actually added/attempted
    bool errors_encountered = false;

    std::vector<std::string> paths_to_process = files_to_add; // Start with arguments

    // --- Expand directories ---
    // We might need a temporary vector to avoid modifying while iterating if we used range-based for
    std::vector<std::string> final_file_list;
    std::vector<std::string> current_paths = files_to_add; // Start with arguments

    // Basic '.' expansion (add everything in current dir)
     if (std::find(current_paths.begin(), current_paths.end(), ".") != current_paths.end()) {
         std::cout << "Expanding '.' ..." << std::endl;
         // Remove '.' itself
         current_paths.erase(std::remove(current_paths.begin(), current_paths.end(), "."), current_paths.end());
         // Add contents of current directory (respecting ignores later)
         try {
              for (const auto& entry : fs::directory_iterator(".")) {
                   // Add top-level files/dirs from current directory
                   current_paths.push_back(entry.path().generic_string());
              }
         } catch (const fs::filesystem_error& e) {
              std::cerr << "Warning: Failed to iterate current directory for '.': " << e.what() << std::endl;
         }
     }


    // Expand directories recursively and collect file paths
    for (const std::string& path_arg : current_paths) {
         fs::path current_fs_path = path_arg;
         std::string relative_path = current_fs_path.lexically_normal().generic_string();

        if (!fs::exists(current_fs_path)) {
            std::cerr << "fatal: pathspec '" << relative_path << "' did not match any files" << std::endl;
            errors_encountered = true; // Mark error but continue processing others
            continue;
        }

        if (fs::is_directory(current_fs_path)) {
             // --- RECURSIVE DIRECTORY ADD ---
             try {
                // Iterate recursively
                 for (auto it = fs::recursive_directory_iterator(current_fs_path), end = fs::recursive_directory_iterator(); it != end; ++it) {
                     fs::path sub_path_fs = it->path();
                     std::string sub_relative_path;
                      try { // Robust relative path calculation
                         sub_relative_path = fs::relative(sub_path_fs, fs::current_path()).generic_string();
                          if (sub_relative_path.empty() || sub_relative_path == ".") continue;
                      } catch (...) { continue; }

                     // Basic ignore check (can be expanded)
                     bool ignored = false;
                     if (sub_relative_path == GIT_DIR || sub_relative_path.rfind(GIT_DIR + "/", 0) == 0) {
                         ignored = true;
                     }
                     // TODO: Add .gitignore check here

                     if (ignored) {
                         if (it->is_directory()) {
                             it.disable_recursion_pending(); // Don't go into ignored dirs
                         }
                         continue;
                     }

                     // Add files to the final list
                     if (it->is_regular_file() || it->is_symlink()) {
                         final_file_list.push_back(sub_relative_path);
                     }
                 }
             } catch (const fs::filesystem_error& e) {
                  std::cerr << "Warning: Error iterating directory '" << relative_path << "': " << e.what() << std::endl;
                  errors_encountered = true;
             }
        } else if (fs::is_regular_file(current_fs_path) || fs::is_symlink(current_fs_path)) {
             // It's a file/link directly specified
             final_file_list.push_back(relative_path);
        } else {
             std::cerr << "Warning: Skipping unsupported file type: '" << relative_path << "'" << std::endl;
        }
    } // End loop through initial arguments


    // --- Process the final list of files ---
    std::cout << "Adding " << final_file_list.size() << " file(s) to index..." << std::endl;
    for (const std::string& final_file_path : final_file_list) {
         // Use the helper function to add each file
         if (!add_single_file_to_index(final_file_path, index)) {
             errors_encountered = true; // Track if any individual add failed
         }
    }


    // --- Write index if changes were attempted ---
    // Only write if the final list wasn't empty, even if errors occurred for some files
    if (!final_file_list.empty()) {
        try {
            write_index(index);
        } catch (const std::exception& e) {
            std::cerr << "Error writing index file: " << e.what() << std::endl;
            return 1; // Fail hard if index cannot be written
        }
    } else if (!errors_encountered) {
         std::cerr << "Nothing specified, nothing added." << std::endl; // Case where only non-existent files were given
         return 1;
    }

    // Return 0 if successful or partially successful (Git often returns 0 even if some paths fail)
    // Return 1 only for major errors like cannot write index, or maybe if *all* paths failed?
    // Let's return 0 unless index write fails.
    return errors_encountered ? 1 : 0; // Optionally return 1 if any error occurred
}


// --- rm ---
int handle_rm(const std::vector<std::string>& files_to_remove, bool cached_mode) {
    if (files_to_remove.empty()) {
        std::cerr << "Nothing specified, nothing removed." << std::endl;
        return 1;
    }

     IndexMap index = read_index();
     bool changes_made = false;

     for (const std::string& filepath_arg : files_to_remove) {
          fs::path filepath = filepath_arg;
          std::string relative_path = filepath.lexically_normal().generic_string();

          // Check if file is actually in the index
          auto it = index.find(relative_path);
          if (it == index.end() || it->second.empty()) {
               std::cerr << "fatal: pathspec '" << relative_path << "' did not match any files" << std::endl;
               continue; // Git continues
          }

           // Remove entry from index (all stages)
           remove_entry(index, relative_path, -1); // -1 removes all stages
           changes_made = true;

           // If not --cached, remove from working directory
           if (!cached_mode) {
               try {
                   if (file_exists(relative_path)) {
                       if (!fs::remove(relative_path)) {
                            std::cerr << "Warning: Failed to remove file from working directory: " << relative_path << std::endl;
                       }
                   }
                   // TODO: Handle directory removal if 'rm -r' is implemented
               } catch (const fs::filesystem_error& e) {
                    std::cerr << "Warning: Error removing file from working directory '" << relative_path << "': " << e.what() << std::endl;
               }
           }
     }


    if (changes_made) {
        try {
            write_index(index);
        } catch (const std::exception& e) {
            std::cerr << "Error writing index file: " << e.what() << std::endl;
            return 1;
        }
    } else {
         // If no files matched, git rm exits with non-zero status
        return 1;
    }

    return 0;
}

// --- Recursive Helper for write-tree ---
// Takes index entries relevant to a specific directory level (relative paths)
// Returns the SHA1 of the created tree object for this level
std::string build_tree_recursive(const std::vector<IndexEntry>& entries_for_level) {
    std::cout << "DEBUG_BUILD_TREE: ENTERING build_tree_recursive. Entry count = " << entries_for_level.size() << std::endl;
    if (!entries_for_level.empty()) {
        std::cout << "DEBUG_BUILD_TREE: First entry path = " << entries_for_level[0].path << std::endl;
    }

    // Maps to hold entries categorized for this level
    std::map<std::string, TreeEntry> files_in_level; // key=basename, value=TreeEntry
    std::map<std::string, std::vector<IndexEntry>> dirs_in_level; // key=dirname, value=vector of entries for that dir

    // Categorize entries passed to this function
    for (const IndexEntry& entry : entries_for_level) {
        size_t slash_pos = entry.path.find('/');
        if (slash_pos == std::string::npos) { // File/link/placeholder at this level
             if (entry.mode == "100644" || entry.mode == "100755" || entry.mode == "120000") {
                 // Use entry.path as the key since it's just the basename here
                 files_in_level[entry.path] = {entry.mode, entry.path, entry.sha1};
                 std::cout << "DEBUG_BUILD_TREE: Categorized FILE: " << entry.path << std::endl;
             } else {
                 std::cout << "DEBUG_BUILD_TREE: Ignoring non-file/link entry at this level: " << entry.path << " Mode=" << entry.mode << std::endl;
             }
        } else { // Belongs in a subdirectory
            std::string dir_name = entry.path.substr(0, slash_pos);
            std::string rest_of_path = entry.path.substr(slash_pos + 1);

            IndexEntry sub_entry = entry;
            sub_entry.path = rest_of_path; // Adjust path for recursive call
            dirs_in_level[dir_name].push_back(sub_entry);
            std::cout << "DEBUG_BUILD_TREE: Categorized DIR entry: " << dir_name << "/" << rest_of_path << std::endl;
        }
    }

    // ===> PROBLEM AREA LIKELY HERE <===
    // Build the TreeEntry vector for the tree object *at this level*
    std::vector<TreeEntry> current_level_tree_entries;

    // Add entries for the files directly in this directory
    for (const auto& pair : files_in_level) {
        current_level_tree_entries.push_back(pair.second);
        std::cout << "DEBUG_BUILD_TREE: Added file entry to current level: " << pair.second.name << std::endl;
    }

    // Recursively build subdirectories and add entries for them
    for (auto const& [dir_name, subdir_entries] : dirs_in_level) { // Use structured binding (C++17)
         if (subdir_entries.empty()){
             std::cerr << "Warning: Directory map has empty entry list for " << dir_name << " - skipping." << std::endl;
             continue;
         }
        std::cout << "DEBUG_BUILD_TREE: Recursing into directory: " << dir_name << " with " << subdir_entries.size() << " sub-entries." << std::endl;
        std::string sub_tree_sha = build_tree_recursive(subdir_entries); // Recursive call
        if (sub_tree_sha.empty()){
             std::cerr << "Warning: Recursive call for directory " << dir_name << " returned empty SHA - skipping." << std::endl;
             continue;
        }
        // Add entry for the SUBDIRECTORY itself
        current_level_tree_entries.push_back({"40000", dir_name, sub_tree_sha});
         std::cout << "DEBUG_BUILD_TREE: Added directory entry to current level: " << dir_name << " -> " << sub_tree_sha.substr(0,7) << std::endl;
    }
    // ===> END PROBLEM AREA LIKELY HERE <===

     // Debug: How many entries are we passing to format?
     std::cout << "DEBUG_BUILD_TREE: Passing " << current_level_tree_entries.size() << " entries to format_tree_content." << std::endl;

    // Format, hash, and write the tree for this level
    std::string tree_content = format_tree_content(current_level_tree_entries); // Use the simplified version for now
    std::cout << "DEBUG_BUILD_TREE: Level content size=" << tree_content.size() << std::endl;

    std::string result_sha = hash_and_write_object("tree", tree_content);
    std::cout << "DEBUG_BUILD_TREE: hash_and_write_object returned SHA = " << result_sha << std::endl;
    return result_sha;
}

// --- write-tree ---
int handle_write_tree() {
    IndexMap index = read_index();
    if (index.empty()) {
        // Git allows writing an empty tree, useful for initial commits
        // std::cerr << "Error: Index is empty. Nothing to write." << std::endl;
        // return 1;
    }

    // Check for unmerged entries (conflicts)
    std::vector<IndexEntry> root_entries;
    for (const auto& path_pair : index) {
        bool conflicted = false;
        for (const auto& stage_pair : path_pair.second) {
            if (stage_pair.first > 0) {
                conflicted = true;
                break;
            }
        }
        if (conflicted) {
             std::cerr << "error: Path '" << path_pair.first << "' is unmerged." << std::endl;
             std::cerr << "fatal: Cannot write tree with unmerged paths." << std::endl;
             return 1;
        }
        // Add stage 0 entry if present
        auto stage0_it = path_pair.second.find(0);
        if (stage0_it != path_pair.second.end()) {
             root_entries.push_back(stage0_it->second);
        }
    }

    try {
        // Start recursive build from the root
        std::string root_tree_sha = build_tree_recursive(root_entries);
        std::cout << root_tree_sha << std::endl; // Output the ROOT tree SHA
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "Error writing tree object: " << e.what() << std::endl;
        return 1;
    }
}


// --- read-tree ---
int handle_read_tree(const std::string& tree_sha_prefix, bool update_workdir, bool merge_mode) {
    // 1. Resolve tree SHA
    std::optional<std::string> tree_sha_opt = resolve_ref(tree_sha_prefix);
    if (!tree_sha_opt) { std::cerr << "fatal: Not a valid tree object name: " << tree_sha_prefix << std::endl; return 1; }
    std::string tree_sha = *tree_sha_opt;

    // 2. Get target tree contents (recursively)
    // Stores { "full/path": "sha1" }
    std::map<std::string, std::string> target_tree_contents;
    // We also need mode info for checkout, so let's modify read_tree_contents or use a different approach
    // For simplicity now, let's re-parse the tree during checkout logic.
    // We still need the flat list for comparison.
     try {
         // Ensure it's actually a tree first
         ParsedObject root_obj = read_object(tree_sha);
         if(root_obj.type != "tree") {
              std::cerr << "fatal: Object " << tree_sha << " is not a tree." << std::endl;
              return 1;
         }
        target_tree_contents = read_tree_contents(tree_sha); // Get flat list for comparison/index update
     } catch (const std::exception& e) {
         std::cerr << "fatal: Failed to read target tree " << tree_sha << ": " << e.what() << std::endl;
         return 1;
     }


    // 3. Read current index (needed for comparison if updating workdir)
    IndexMap old_index_map = read_index(); // Read before modifying
    IndexMap new_index_map; // Build the new index state

    // 4. Populate the new index map based on target tree
    // This requires reading the tree structure again to get modes.
    // Could optimize by having read_tree_contents return mode info too.
    std::function<void(const std::string&, const std::string&)> populate_index_recursive =
        [&](const std::string& current_tree_sha, const std::string& path_prefix)
    {
         try {
            ParsedObject tree_obj_parsed = read_object(current_tree_sha);
            if (tree_obj_parsed.type != "tree") return; // Skip non-trees
            const auto& tree_data = std::get<TreeObject>(tree_obj_parsed.data);

            for (const auto& entry : tree_data.entries) {
                std::string full_path = path_prefix.empty() ? entry.name : path_prefix + "/" + entry.name;
                if (entry.mode == "40000") { // Subtree
                    populate_index_recursive(entry.sha1, full_path);
                } else { // Blob or Symlink
                    IndexEntry new_entry;
                    new_entry.mode = entry.mode;
                    new_entry.path = full_path;
                    new_entry.sha1 = entry.sha1;
                    new_entry.stage = 0;
                    add_or_update_entry(new_index_map, new_entry);
                }
            }
         } catch (...) { /* Ignore errors reading subtrees */ }
    };

    populate_index_recursive(tree_sha, ""); // Populate new_index_map

    // 5. Update working directory if requested (-u)
    if (update_workdir) {
        std::cout << "Updating workdir to match tree " << tree_sha.substr(0, 7) << "..." << std::endl;
        IndexMap old_index_map = read_index(); // Read old index ONLY if updating workdir
        std::set<std::string> processed_paths;

        // 5a. Deletions
        std::map<std::string, IndexEntry> old_index_stage0;
         for(const auto& pair : old_index_map) {
              if(pair.second.count(0)) old_index_stage0[pair.first] = pair.second.at(0);
         }
        for (const auto& old_pair : old_index_stage0) {
            const std::string& path = old_pair.first;
            if (new_index_map.find(path) == new_index_map.end()) {
                try {
                   if (file_exists(path)) {
                       std::cout << "  Deleting " << path << std::endl;
                       fs::remove(path);
                       processed_paths.insert(path);
                   }
                } catch (...) { /* Warning */ }
            }
        }

        // 5b. Additions/Updates
        for (const auto& new_pair : new_index_map) {
            const std::string& path = new_pair.first;
            const IndexEntry& new_entry = new_pair.second.at(0);
            processed_paths.insert(path);
            bool needs_update = false;
            if (!file_exists(path)) { needs_update = true; }
            else { /* Check SHA and mode */
                try {
                     std::string current_sha = get_workdir_sha(path); // Use helper
                     if (current_sha.empty() || current_sha != new_entry.sha1) {
                          needs_update = true;
                     } else {
                          mode_t current_mode_raw = get_file_mode(path);
                          std::stringstream ss; ss << std::oct << current_mode_raw;
                          std::string current_mode_str = ss.str();
                          if (current_mode_str != new_entry.mode && current_mode_raw != 0) {
                               needs_update = true;
                               std::cout << "  Updating mode for " << path << std::endl;
                          }
                     }
                } catch (...) { needs_update = true; }
            }

            if (needs_update) {
                try {
                     std::cout << "  Checking out " << path << std::endl;
                     ensure_parent_directory_exists(path);
                     ParsedObject blob_obj = read_object(new_entry.sha1);
                     if (blob_obj.type != "blob") { /* Warning */ continue; }
                     const std::string& content = std::get<BlobObject>(blob_obj.data).content;
                     write_file(path, content);
                     set_file_executable(path, new_entry.mode == "100755");
                } catch (const std::exception& e) { /* Error */ /* Maybe return 1 here? */ }
            }
        }
        // Workdir update logic finished.

        // *** Since workdir update IS implemented now, we only return 1 on actual error. ***
        // *** Comment out or remove the explicit return 1 for unimplemented feature ***
        // std::cerr << "Warning: read-tree working directory update (-u) is not fully robust yet." << std::endl;
        // return 1; // Remove this line if the basic implementation above is considered done

    } 


    // 6. Write the final index (reflecting the target tree)
    try {
        // If merging, this logic needs to be different (3-way merge)
        if (merge_mode) {
             std::cerr << "Error: read-tree merge update logic not fully implemented." << std::endl;
             return 1; // Don't write index in merge mode yet
        }
        write_index(new_index_map); // Write the index reflecting the target tree
    } catch (const std::exception& e) {
        std::cerr << "Error writing final index: " << e.what() << std::endl;
        return 1;
    }

    return 0; // Success
}

// --- status ---
int handle_status() {
    // 1. Get Current Branch Name
    std::string head_content = read_head();
    std::string branch_name;
     bool detached = false;
     if (head_content.rfind("ref: refs/heads/", 0) == 0) {
         branch_name = head_content.substr(16); // Length of "ref: refs/heads/"
     } else if (head_content.length() == 40 && head_content.find_first_not_of("0123456789abcdef") == std::string::npos) {
         branch_name = "HEAD detached at " + head_content.substr(0, 7);
         detached = true;
     } else {
         branch_name = "HEAD (unknown state)";
     }
     std::cout << "On branch " << branch_name << std::endl;

     // Check for MERGE_HEAD
     std::string merge_head_path = GIT_DIR + "/MERGE_HEAD";
     if (file_exists(merge_head_path)) {
          std::cout << "\nYou have unmerged paths." << std::endl;
          std::cout << "  (fix conflicts and run \"mygit commit\")" << std::endl;
     }


    // 2. Calculate Status
    std::map<std::string, StatusEntry> status;
    try {
        status = get_repository_status();
    } catch (const std::exception& e) {
        std::cerr << "Error getting repository status: " << e.what() << std::endl;
        return 1;
    }

    // 3. Print Status Report
    std::vector<std::string> staged_changes;     // Modified, Added, Deleted (Index vs HEAD)
    std::vector<std::string> unstaged_changes;   // Modified, Deleted (Workdir vs Index)
    std::vector<std::string> untracked_files;
    std::vector<std::string> conflicted_files;

    for (const auto& pair : status) {
         const StatusEntry& entry = pair.second;
         std::string status_line;

         // Check Conflicts first
         if (entry.index_status == FileStatus::Conflicted) {
             conflicted_files.push_back("  both modified:   " + entry.path); // Simple message
             continue; // Don't report other statuses for conflicted files
         }


         // --- Changes to be committed (Index vs HEAD) ---
         switch(entry.index_status) {
             case FileStatus::ModifiedStaged: staged_changes.push_back("  modified:   " + entry.path); break;
             case FileStatus::AddedStaged:    staged_changes.push_back("  new file:   " + entry.path); break;
             case FileStatus::DeletedStaged:  staged_changes.push_back("  deleted:    " + entry.path); break;
             default: break; // Unmodified or Conflicted (handled above)
         }

         // --- Changes not staged for commit (Workdir vs Index) ---
          switch(entry.workdir_status) {
             case FileStatus::ModifiedWorkdir: unstaged_changes.push_back("  modified:   " + entry.path); break;
             case FileStatus::DeletedWorkdir:  unstaged_changes.push_back("  deleted:    " + entry.path); break;
             case FileStatus::AddedWorkdir:    untracked_files.push_back("  " + entry.path); break; // Untracked
             default: break; // Unmodified
         }
    }

    //  if (conflicted_files.empty() && staged_changes.empty() && unstaged_changes.empty() && untracked_files.empty()) {
    //      std::cout << "nothing to commit, working tree clean" << std::endl;
    //  } else {
    //      if (!staged_changes.empty()) {
    //          std::cout << "\nChanges to be committed:" << std::endl;
    //          std::cout << "  (use \"mygit rm --cached <file>...\" to unstage)" << std::endl;
    //          for(const auto& s : staged_changes) std::cout << "\033[32m" << s << "\033[0m" << std::endl; // Green
    //      }
    //       if (!conflicted_files.empty()) {
    //          std::cout << "\nUnmerged paths:" << std::endl;
    //          std::cout << "  (use \"mygit add <file>...\" to mark resolution)" << std::endl;
    //          for(const auto& s : conflicted_files) std::cout << "\033[31m" << s << "\033[0m" << std::endl; // Red
    //      }
    //      if (!unstaged_changes.empty()) {
    //          std::cout << "\nChanges not staged for commit:" << std::endl;
    //          std::cout << "  (use \"mygit add <file>...\" to update what will be committed)" << std::endl;
    //           std::cout << "  (use \"mygit restore <file>...\" to discard changes in working directory - NOT IMPLEMENTED)" << std::endl; // Placeholder for restore/checkout
    //          for(const auto& s : unstaged_changes) std::cout << "\033[31m" << s << "\033[0m" << std::endl; // Red
    //      }
    //       if (!untracked_files.empty()) {
    //          std::cout << "\nUntracked files:" << std::endl;
    //          std::cout << "  (use \"mygit add <file>...\" to include in what will be committed)" << std::endl;
    //          for(const auto& s : untracked_files) std::cout << "\033[31m" << s << "\033[0m" << std::endl; // Red
    //      }
    //  }


    // return 0;
    bool changes_present = !staged_changes.empty() || !unstaged_changes.empty() || !conflicted_files.empty() || !untracked_files.empty();

    if (!changes_present) { // Check if ALL lists are empty
        std::cout << "nothing to commit, working tree clean" << std::endl;
    } else {
        if (!staged_changes.empty()) {
            std::cout << "\nChanges to be committed:" << std::endl;
            std::cout << "  (use \"mygit rm --cached <file>...\" to unstage)" << std::endl;
            for(const auto& s : staged_changes) std::cout << "\033[32m" << s << "\033[0m" << std::endl; // Green
        }
        if (!conflicted_files.empty()) {
            std::cout << "\nUnmerged paths:" << std::endl;
            std::cout << "  (use \"mygit add <file>...\" to mark resolution)" << std::endl;
            for(const auto& s : conflicted_files) std::cout << "\033[31m" << s << "\033[0m" << std::endl; // Red
        }
        if (!unstaged_changes.empty()) {
            std::cout << "\nChanges not staged for commit:" << std::endl;
            std::cout << "  (use \"mygit add <file>...\" to update what will be committed)" << std::endl;
            std::cout << "  (use \"mygit restore <file>...\" to discard changes in working directory - NOT IMPLEMENTED)" << std::endl;
            for(const auto& s : unstaged_changes) std::cout << "\033[31m" << s << "\033[0m" << std::endl; // Red
        }
        if (!untracked_files.empty()) {
            std::cout << "\nUntracked files:" << std::endl;
            std::cout << "  (use \"mygit add <file>...\" to include in what will be committed)" << std::endl;
            for(const auto& s : untracked_files) std::cout << "\033[31m" << s << "\033[0m" << std::endl; // Red
        }
    }
    // --- End Modified Printing Logic ---

    return 0;
}


// --- log ---
int handle_log(bool graph_mode) {
    std::optional<std::string> head_sha_opt = resolve_ref("HEAD");
    if (!head_sha_opt) {
        std::cerr << "fatal: your current branch '...' does not have any commits yet" << std::endl; // Improve branch name
        return 1;
    }

    std::string start_sha = *head_sha_opt;
    std::set<std::string> visited; // Prevent infinite loops and re-processing
    std::queue<std::string> q;     // Use queue for breadth-first-like traversal
    std::map<std::string, std::vector<std::string>> adj; // For graph edges: child -> {parents}
     std::map<std::string, std::string> node_labels; // For graph node labels

    q.push(start_sha);
    visited.insert(start_sha);

    while (!q.empty()) {
        std::string current_sha = q.front();
        q.pop();

        try {
            ParsedObject parsed_obj = read_object(current_sha);
            if (parsed_obj.type != "commit") {
                std::cerr << "Warning: Expected commit object, got " << parsed_obj.type << " for SHA " << current_sha << std::endl;
                continue;
            }
            const auto& commit = std::get<CommitObject>(parsed_obj.data);

            // --- Regular Log Output ---
            if (!graph_mode) {
                 std::cout << "\033[33mcommit " << current_sha << "\033[0m" << std::endl; // Yellow SHA
                  if (commit.parent_sha1s.size() > 1) {
                     std::cout << "Merge:";
                     for(size_t i = 0; i < commit.parent_sha1s.size(); ++i) {
                          std::cout << " " << commit.parent_sha1s[i].substr(0, 7);
                     }
                     std::cout << std::endl;
                  }
                 std::cout << "Author: " << commit.author_info << std::endl;
                 // Could parse date/time for nicer formatting
                 std::cout << "Date:   " << commit.committer_info.substr(commit.committer_info.find_last_of('>') + 2) << std::endl; // Simple date extraction
                 std::cout << std::endl;
                 // Indent message
                 std::istringstream message_stream(commit.message);
                 std::string msg_line;
                 while(std::getline(message_stream, msg_line)) {
                     std::cout << "    " << msg_line << std::endl;
                 }
                 std::cout << std::endl;
            }

             // --- Graph Mode Data Collection ---
             if (graph_mode) {
                 std::ostringstream label_ss;
                 label_ss << current_sha.substr(0, 7) << "\\n"
                          << commit.author_info.substr(0, commit.author_info.find('<')) << "\\n" // Just name
                          << commit.message.substr(0, commit.message.find('\n')); // First line of message
                 node_labels[current_sha] = label_ss.str();

                 for (const auto& parent_sha : commit.parent_sha1s) {
                     adj[current_sha].push_back(parent_sha); // Store edge: current -> parent
                     if (visited.find(parent_sha) == visited.end()) {
                         visited.insert(parent_sha);
                         q.push(parent_sha);
                     }
                 }
                  // Also add commits with no parents (root) to adjacency list
                  if (commit.parent_sha1s.empty() && adj.find(current_sha) == adj.end()) {
                       adj[current_sha] = {};
                  }
             } else {
                 // Add parents to queue for regular log
                 for (const auto& parent_sha : commit.parent_sha1s) {
                      if (visited.find(parent_sha) == visited.end()) {
                         visited.insert(parent_sha);
                         q.push(parent_sha);
                      }
                 }
             }


        } catch (const std::exception& e) {
            std::cerr << "Error reading commit object " << current_sha << ": " << e.what() << std::endl;
             // Continue processing other commits in the queue? Or stop?
             // Let's continue.
        }
    }


     // --- Graph Mode Output ---
     if (graph_mode) {
         std::cout << "digraph git_log {" << std::endl;
         std::cout << "  node [shape=box, style=rounded];" << std::endl;

         // Print nodes
         for (const auto& pair : node_labels) {
              std::cout << "  \"" << pair.first << "\" [label=\"" << pair.second << "\"];" << std::endl;
         }

         // Print edges
         for (const auto& pair : adj) {
             const std::string& child = pair.first;
             for (const std::string& parent : pair.second) {
                  std::cout << "  \"" << child << "\" -> \"" << parent << "\";" << std::endl;
             }
         }

          // Add branch pointers
          for (const std::string& branch : list_branches()) {
              std::optional<std::string> branch_sha = resolve_ref(branch);
              if (branch_sha && node_labels.count(*branch_sha)) { // Only point if commit was visited
                   std::cout << "  \"branch_" << branch << "\" [label=\"" << branch << "\", shape=box, style=filled, color=lightblue];" << std::endl;
                   std::cout << "  \"branch_" << branch << "\" -> \"" << *branch_sha << "\";" << std::endl;
              }
          }
          // Add tag pointers (simplified - doesn't dereference annotated tags fully here)
          for (const std::string& tag : list_tags()) {
               std::optional<std::string> tag_sha = resolve_ref(tag); // resolve_ref handles dereferencing
               if (tag_sha && node_labels.count(*tag_sha)) {
                   std::cout << "  \"tag_" << tag << "\" [label=\"" << tag << "\", shape=ellipse, style=filled, color=lightyellow];" << std::endl;
                   std::cout << "  \"tag_" << tag << "\" -> \"" << *tag_sha << "\";" << std::endl;
               }
          }
           // Add HEAD pointer
           std::string head_val = read_head();
           std::optional<std::string> head_target_sha = resolve_ref("HEAD");
            if (head_target_sha && node_labels.count(*head_target_sha)) {
                 std::cout << "  \"HEAD\" [shape=box, style=filled, color=lightgreen];" << std::endl;
                 std::cout << "  \"HEAD\" -> \"" << *head_target_sha << "\";" << std::endl;
            }


         std::cout << "}" << std::endl;
     }


    return 0;
}


// --- branch ---
int handle_branch(const std::vector<std::string>& args) {
     // No args: List branches
     if (args.empty()) {
         std::string current_head = read_head();
         std::string current_branch_ref;
          if (current_head.rfind("ref: ", 0) == 0) {
              current_branch_ref = current_head.substr(5);
          }

         std::vector<std::string> branches = list_branches();
         for (const std::string& branch : branches) {
              std::string prefix = "  ";
              if (!current_branch_ref.empty() && get_branch_ref(branch) == current_branch_ref) {
                   prefix = "* "; // Mark current branch
                   std::cout << "\033[32m"; // Green color
              }
              std::cout << prefix << branch << std::endl;
              if (!current_branch_ref.empty() && get_branch_ref(branch) == current_branch_ref) {
                   std::cout << "\033[0m"; // Reset color
              }
         }
         return 0;
     }

     // Create branch: mygit branch <name> [<start_point>]
     std::string branch_name = args[0];
     std::string start_point = "HEAD";
     if (args.size() > 1) {
         start_point = args[1];
     }
     if (args.size() > 2) {
         std::cerr << "Usage: mygit branch [<name> [<start_point>]]" << std::endl;
         return 1;
     }

     // Check if branch name is valid
     try {
         get_branch_ref(branch_name); // Throws if invalid chars
     } catch (const std::invalid_argument& e) {
         std::cerr << "fatal: '" << branch_name << "' is not a valid branch name: " << e.what() << std::endl;
         return 1;
     }

     // Check if branch already exists
     if (file_exists(GIT_DIR + "/" + get_branch_ref(branch_name))) {
          std::cerr << "fatal: A branch named '" << branch_name << "' already exists." << std::endl;
          return 1;
     }


     // Resolve start point to a commit SHA
     std::optional<std::string> start_sha_opt = resolve_ref(start_point);
     if (!start_sha_opt) {
          std::cerr << "fatal: Not a valid object name: '" << start_point << "'" << std::endl;
          return 1;
     }
     std::string start_sha = *start_sha_opt;

      // Ensure the resolved start_sha points to a commit object (optional but good practice)
      try {
          ParsedObject obj = read_object(start_sha);
          if (obj.type != "commit") {
               std::cerr << "fatal: '" << start_point << "' (which resolved to " << start_sha.substr(0,7)
                         << ") is not a commit object." << std::endl;
               return 1;
          }
      } catch (const std::exception& e) {
           std::cerr << "fatal: Failed to read object '" << start_point << "' (" << start_sha.substr(0,7) << "): " << e.what() << std::endl;
           return 1;
      }


     // Create the new branch ref file pointing to the commit
     try {
        update_ref(get_branch_ref(branch_name), start_sha);
     } catch (const std::exception& e) {
          std::cerr << "Error creating branch '" << branch_name << "': " << e.what() << std::endl;
          return 1;
     }

    return 0;
}


// --- tag --- (Simplified: handles list, create lightweight, create annotated basic)
int handle_tag(const std::vector<std::string>& args) {
    // No args: List tags
    if (args.empty()) {
        std::vector<std::string> tags = list_tags();
        for (const std::string& tag : tags) {
            std::cout << tag << std::endl;
        }
        return 0;
    }

    // Parse flags (basic)
    bool annotate = false;
    std::string message = "";
    std::string tag_name = "";
    std::string object_ref = "HEAD";
    int arg_idx = 0;

    while (arg_idx < args.size()) {
        std::string arg = args[arg_idx];
         if (arg == "-a") {
             annotate = true;
             arg_idx++;
         } else if (arg == "-m") {
              if (arg_idx + 1 < args.size()) {
                  message = args[arg_idx + 1];
                  arg_idx += 2;
              } else {
                   std::cerr << "Error: -m option requires a message argument." << std::endl;
                   return 1;
              }
         } else if (tag_name.empty()) {
             // First non-flag argument is tag name
             tag_name = arg;
             arg_idx++;
         } else {
              // Second non-flag argument is object ref
              object_ref = arg;
              arg_idx++;
              // Check for extra arguments
              if (arg_idx < args.size()) {
                   std::cerr << "Usage: mygit tag [-a [-m <msg>]] <name> [<object>]" << std::endl;
                   return 1;
              }
         }
    }

    if (tag_name.empty()) {
        std::cerr << "Usage: mygit tag [-a [-m <msg>]] <name> [<object>]" << std::endl;
        return 1;
    }
     if (annotate && message.empty()) {
         std::cerr << "Error: Annotated tags require a message via -m or an editor (editor not implemented)." << std::endl;
         return 1;
     }


     // Validate tag name
     try {
         get_tag_ref(tag_name); // Throws if invalid chars
     } catch (const std::invalid_argument& e) {
         std::cerr << "fatal: '" << tag_name << "' is not a valid tag name: " << e.what() << std::endl;
         return 1;
     }

      // Check if tag already exists
     if (file_exists(GIT_DIR + "/" + get_tag_ref(tag_name))) {
          std::cerr << "fatal: tag '" << tag_name << "' already exists." << std::endl;
          return 1;
     }

     // Resolve object reference
     std::optional<std::string> object_sha_opt = resolve_ref(object_ref);
     if (!object_sha_opt) {
          std::cerr << "fatal: Not a valid object name: '" << object_ref << "'" << std::endl;
          return 1;
     }
      std::string object_sha = *object_sha_opt;


     // Determine type of the target object
     std::string object_type;
      try {
           ParsedObject target_obj = read_object(object_sha);
           object_type = target_obj.type;
      } catch (const std::exception& e) {
            std::cerr << "fatal: Failed to read target object '" << object_ref << "' (" << object_sha.substr(0,7) << "): " << e.what() << std::endl;
            return 1;
      }


     // Create tag
      try {
          if (annotate) {
              // Create annotated tag object
              std::string tagger = get_user_info() + " " + get_current_timestamp_and_zone();
              std::string tag_content = format_tag_content(object_sha, object_type, tag_name, tagger, message);
              std::string tag_object_sha = hash_and_write_object("tag", tag_content);
              // Create ref pointing to the tag *object*
              update_ref(get_tag_ref(tag_name), tag_object_sha);

          } else {
              // Create lightweight tag (ref points directly to the target object)
              update_ref(get_tag_ref(tag_name), object_sha);
          }
      } catch (const std::exception& e) {
          std::cerr << "Error creating tag '" << tag_name << "': " << e.what() << std::endl;
          return 1;
      }


    return 0;
}

std::set<std::string> get_commit_ancestors(const std::string& start_sha, int limit = 1000) {
    std::set<std::string> ancestors;
    std::queue<std::string> q;
    std::set<std::string> visited; // Track visited within this traversal

    if (start_sha.empty()) return ancestors;

    q.push(start_sha);
    visited.insert(start_sha);
    ancestors.insert(start_sha); // Include the start commit itself

    int count = 0;
    while (!q.empty() && count++ < limit) {
        std::string current = q.front();
        q.pop();
        try {
            ParsedObject obj = read_object(current); // read_object handles prefix resolution
            if (obj.type == "commit") {
                const auto& commit = std::get<CommitObject>(obj.data);
                for (const auto& p : commit.parent_sha1s) {
                    if (visited.find(p) == visited.end()) {
                        visited.insert(p);
                        ancestors.insert(p);
                        q.push(p);
                    }
                }
            }
        } catch (...) { /* Ignore read errors during ancestry trace */ }
    }
    return ancestors;
}

// Revised merge base finder
std::optional<std::string> find_merge_base(const std::string& sha1_a, const std::string& sha1_b) {
    // ... (edge cases) ...
    if (sha1_a.empty() || sha1_b.empty()) return std::nullopt;
    if (sha1_a == sha1_b) return sha1_a;

    std::set<std::string> ancestors_a = get_commit_ancestors(sha1_a);
    if (ancestors_a.empty()) return std::nullopt;

    // Check if B is an ancestor of A
    if (ancestors_a.count(sha1_b)) {
        std::cout << "DEBUG_BASE: B (" << sha1_b.substr(0,7) << ") is ancestor of A (" << sha1_a.substr(0,7) << "). Base is B." << std::endl;
        return sha1_b;
    }

    std::set<std::string> ancestors_b = get_commit_ancestors(sha1_b);
    if (ancestors_b.empty()) return std::nullopt;

    // Check if A is an ancestor of B
    if (ancestors_b.count(sha1_a)) {
        std::cout << "DEBUG_BASE: A (" << sha1_a.substr(0,7) << ") is ancestor of B (" << sha1_b.substr(0,7) << "). Base is A." << std::endl;
        return sha1_a;
    }

    // Find common ancestor by walking back from B
    std::queue<std::string> q_b;
    std::set<std::string> visited_b;
    q_b.push(sha1_b);
    visited_b.insert(sha1_b);
    int count = 0;
    const int limit = 1000;

    while(!q_b.empty() && count++ < limit) {
        std::string current = q_b.front();
        q_b.pop();
        if (ancestors_a.count(current)) {
            std::cout << "DEBUG_BASE: Found common ancestor by walking from B: " << current.substr(0,7) << std::endl;
            return current; // Found first common ancestor
        }
        // Add parents
        try {
            ParsedObject obj = read_object(current);
            if (obj.type == "commit") {
                for (const auto& p : std::get<CommitObject>(obj.data).parent_sha1s) {
                     if (visited_b.find(p) == visited_b.end()) {
                        visited_b.insert(p); q_b.push(p);
                    }
                }
            }
        } catch (...) { /* Ignore read errors */ }
    }

    std::cout << "DEBUG_BASE: No common ancestor found." << std::endl;
    return std::nullopt;
}

int handle_merge(const std::string& branch_to_merge) {
    // 1. Status Check TODO
    // 2. Resolve SHAs
    std::optional<std::string> head_sha_opt = resolve_ref("HEAD");
    if (!head_sha_opt) { std::cerr << "Error: Cannot merge, HEAD does not point to a commit." << std::endl; return 1; }
    std::string head_sha = *head_sha_opt;

    std::optional<std::string> theirs_sha_opt = resolve_ref(branch_to_merge);
    if (!theirs_sha_opt) { std::cerr << "fatal: '" << branch_to_merge << "' does not point to a commit" << std::endl; return 1; }
    std::string theirs_sha = *theirs_sha_opt;

    // std::cout << "DEBUG_MERGE: head_sha=" << head_sha.substr(0,7) << ", theirs_sha=" << theirs_sha.substr(0,7) << std::endl;

    if (head_sha == theirs_sha) {
        std::cout << "Already up to date." << std::endl;
        return 0; // Correct return for this case
    }

    // 3. Find merge base
    std::optional<std::string> base_sha_opt = find_merge_base(head_sha, theirs_sha);
    if (!base_sha_opt) {
        std::cerr << "fatal: Could not find a common ancestor." << std::endl;
        return 1; // Correct return
    }
    std::string base_sha = *base_sha_opt;
    // std::cout << "DEBUG_MERGE: base_sha=" << base_sha.substr(0, 7) << std::endl;

    // 4. Handle scenarios
    if (base_sha == theirs_sha) {
        // std::cout << "DEBUG_MERGE: Scenario -> Already up-to-date (base == theirs)" << std::endl;
        std::cout << "Already up to date." << std::endl;
        return 0; // Correct return for this case
    } else if (base_sha == head_sha) {
        // std::cout << "DEBUG_MERGE: Scenario -> Fast-forward (base == head)" << std::endl;
        // --- Fast-forward merge ---
        std::cout << "Updating " << head_sha.substr(0, 7) << ".." << theirs_sha.substr(0, 7) << std::endl;
        std::cout << "Fast-forward" << std::endl;

        std::string theirs_tree_sha;
        try {
             ParsedObject theirs_commit = read_object(theirs_sha);
             if (theirs_commit.type != "commit") throw std::runtime_error("Target is not a commit");
             theirs_tree_sha = std::get<CommitObject>(theirs_commit.data).tree_sha1;
         } catch (const std::exception& e) { std::cerr << "Error reading target commit object " << theirs_sha << ": " << e.what() << std::endl; return 1; }

        int read_tree_ret = handle_read_tree(theirs_tree_sha, true, false);
        // Check the return code from read_tree
        if (read_tree_ret != 0) {
            // Expected path because -u is not implemented
            std::cerr << "Error updating index/workdir during fast-forward. Merge aborted." << std::endl;
            return 1; // Return failure *as expected by test*
        } else {
            std::cout << "Fast-forward merge successful. Updating HEAD..." << std::endl; // Indicate success
            std::string head_ref = read_head();
            if (head_ref.rfind("ref: ", 0) == 0) {
                std::string current_branch_ref = head_ref.substr(5);
                update_ref(current_branch_ref, theirs_sha); // Update branch
            } else {
                update_head(theirs_sha); // Update detached HEAD
            }
           return 0; // <<< RETURN 0 for SUCCESSFUL FF merge
        }
        // This part is now unreachable if read_tree returns 1 correctly
        // return 0;
    } else {
        // std::cout << "DEBUG_MERGE: Scenario -> True Merge (base != head and base != theirs)" << std::endl;
        // --- True merge (3-way merge required) ---
        std::cerr << "fatal: True merge logic is not implemented yet." << std::endl;
        return 1; // Return failure *as expected by test*
    }
}

int handle_commit(const std::string& message) {
    if (message.empty()) {
        std::cerr << "Aborting commit due to empty commit message." << std::endl;
        return 1;
    }

    // 1. Check for merge conflicts first
    IndexMap current_index = read_index();
    bool merge_in_progress = false;
    std::string merge_head_sha;
    std::string merge_head_path = GIT_DIR + "/MERGE_HEAD";
    // ... (conflict check logic as before) ...
    for (const auto& path_pair : current_index) {
         for (const auto& stage_pair : path_pair.second) {
             if (stage_pair.first > 0) { /* Conflict error */ return 1; }
         }
     }


    // 2. Write index to a tree object
    std::string tree_sha1;
    std::vector<IndexEntry> root_entries; // Define here
    try {
        IndexMap index_for_commit = read_index();
        // --- Get Stage 0 entries ---
        for (const auto& path_pair : index_for_commit) {
           auto stage0_it = path_pair.second.find(0);
           if (stage0_it != path_pair.second.end()) {
               root_entries.push_back(stage0_it->second);

               std::cout << "DEBUG_COMMIT: Adding root entry path=" << stage0_it->second.path << " sha=" << stage0_it->second.sha1.substr(0,7) << std::endl;
           }

           for (const auto& stage_pair : path_pair.second) { 
                if (stage_pair.first > 0) { 
                    /* Conflict error */ 
                    return 1;
                } 
            }
        }
        // --- End Get Stage 0 entries ---

        // *** Check if index is truly empty ***
        if (root_entries.empty() && !merge_in_progress) {
             // Check if HEAD exists and points to the known empty tree
             std::optional<std::string> current_head_commit = resolve_ref("HEAD");
             if (current_head_commit) {
                  try {
                       ParsedObject head_commit_obj = read_object(*current_head_commit);
                       if(head_commit_obj.type == "commit") {
                           std::string head_tree_sha = std::get<CommitObject>(head_commit_obj.data).tree_sha1;
                           if (head_tree_sha == "da39a3ee5e6b4b0d3255bfef95601890afd80709" || head_tree_sha.empty()) {
                               // Index is empty and HEAD tree is empty - nothing to commit
                               std::cerr << "nothing to commit (empty index matches empty HEAD tree)" << std::endl;
                               return 0;
                           }
                       }
                  } catch (...) { /* Ignore errors reading HEAD */ }
             }
            tree_sha1 = "da39a3ee5e6b4b0d3255bfef95601890afd80709"; // Use known empty tree SHA
            std::cout << "DEBUG_COMMIT: Index is empty, setting tree SHA to empty: " << tree_sha1 << std::endl;
        } else if (!root_entries.empty()){
            // Only call build_tree if there are entries
            tree_sha1 = build_tree_recursive(root_entries);
        } else {
            // Merge in progress with potentially empty index? How should this work?
            // Let's assume for now merge implies non-empty index, or fail earlier.
             tree_sha1 = "da39a3ee5e6b4b0d3255bfef95601890afd80709";
             std::cout << "DEBUG_COMMIT: Merge in progress with empty index? Setting tree SHA to empty: " << tree_sha1 << std::endl;
        }


        // tree_sha1 = build_tree_recursive(root_entries); // Get root tree SHA
        std::cout << "DEBUG_COMMIT: Final calculated tree SHA = " << tree_sha1 << std::endl;

        if (tree_sha1.empty()) {
            std::cerr << "FATAL_DEBUG: Tree SHA became empty unexpectedly!" << std::endl; return 1;
       }

    } catch (const std::exception& e) {
        std::cerr << "Error creating tree object for commit: " << e.what() << std::endl;
        return 1;
    }

    // 3. Determine parent commit(s)
    std::vector<std::string> parent_sha1s;
    std::optional<std::string> head_parent_sha = resolve_ref("HEAD");
    std::cout << "DEBUG_COMMIT: Parent SHA = " << (head_parent_sha ? *head_parent_sha : "None") << std::endl; // *** DEBUG PRINT ***

    // Check if tree hasn't changed
    if (head_parent_sha) {
        bool tree_changed = true;
        std::string parent_tree_sha = "";
        try {
            ParsedObject parent_commit_obj = read_object(*head_parent_sha);
            if (parent_commit_obj.type == "commit") {
                parent_tree_sha = std::get<CommitObject>(parent_commit_obj.data).tree_sha1;
                std::cout << "DEBUG_COMMIT: Parent Tree SHA = " << parent_tree_sha << std::endl; // *** DEBUG PRINT ***
                if (parent_tree_sha == tree_sha1 && !merge_in_progress) {
                    tree_changed = false;
                }
            }
        } catch (...) { /* Ignore errors reading parent */ }

        if (!tree_changed) {
            // ... (print "nothing to commit" logic as before) ...
            std::string head_ref_str = read_head();
            std::string branch_name = "HEAD";
            if (head_ref_str.rfind("ref: refs/heads/", 0) == 0) branch_name = head_ref_str.substr(16);
            else if (!head_ref_str.empty()) branch_name = "HEAD detached at " + head_ref_str.substr(0,7);
            std::cerr << "On branch " << branch_name << std::endl;
            std::cerr << "nothing to commit, working tree clean" << std::endl;
            return 0;
        }
        // If tree changed or initial commit, add parent SHA
        parent_sha1s.push_back(*head_parent_sha);
    } // Else: Initial commit (parent_sha1s remains empty)


    // Add merge parent if applicable
    if (merge_in_progress) {
        if (head_parent_sha && *head_parent_sha == merge_head_sha) { /* Warning */ }
        parent_sha1s.push_back(merge_head_sha);
    }

    // 4. Get author/committer info and time
    std::string author = get_user_info() + " " + get_current_timestamp_and_zone();
    std::string committer = author;

    // 5. Format and write commit object
    std::cout << "DEBUG_COMMIT: Formatting commit with tree=" << tree_sha1 << ", #parents=" << parent_sha1s.size() << std::endl; // *** DEBUG PRINT ***
    std::string commit_content = format_commit_content(tree_sha1, parent_sha1s, author, committer, message);
    std::string commit_sha1 = hash_and_write_object("commit", commit_content);
    std::cout << "DEBUG_COMMIT: Created commit object SHA = " << commit_sha1 << std::endl; // *** DEBUG PRINT ***


    // 6. Update HEAD reference
    std::string head_ref = read_head();
    if (head_ref.rfind("ref: ", 0) == 0) {
        std::string current_branch_ref = head_ref.substr(5);
        update_ref(current_branch_ref, commit_sha1);
    } else {
        update_head(commit_sha1);
    }

    // 7. Clean up MERGE_HEAD if it was a merge commit
    if (merge_in_progress) { /* ... remove MERGE_HEAD ... */ }

    // 8. Output commit info
    std::string branch_name_display = "HEAD";
    if (head_ref.rfind("ref: refs/heads/", 0) == 0) branch_name_display = head_ref.substr(16);
    else if (!head_ref.empty() && head_ref.find("ref: ") != 0) branch_name_display = "HEAD detached at " + head_ref.substr(0,7);

    std::cout << "[" << branch_name_display
              << (parent_sha1s.empty() ? " (root-commit)" : "") // Use parent_sha1s empty check
              << " " << commit_sha1 << "] " // Print full commit SHA
              << message.substr(0, message.find('\n')) << std::endl;

    return 0;
}

int handle_checkout(const std::string& target_ref) {
    std::cout << "Switching to '" << target_ref << "'..." << std::endl;

    // 1. Safety Check: Ensure workdir/index is clean
    // TODO: Implement a more robust check. For now, check basic status.
    // This requires get_repository_status to be reasonably fast.
     std::map<std::string, StatusEntry> current_status;
     try {
         current_status = get_repository_status();
         bool dirty = false;
         for(const auto& pair : current_status) {
             // Check for any staged changes or unstaged workdir changes
              if (pair.second.index_status != FileStatus::Unmodified ||
                 (pair.second.workdir_status != FileStatus::Unmodified && pair.second.workdir_status != FileStatus::AddedWorkdir))
              {
                    // Allow AddedWorkdir (untracked files) to remain
                    if (pair.second.index_status == FileStatus::Conflicted) {
                         std::cerr << "error: You have unmerged paths." << std::endl;
                         std::cerr << "hint: Fix them up in the work tree, and then use 'mygit add <file>'." << std::endl;
                         return 1;
                    }
                    if(pair.second.workdir_status != FileStatus::AddedWorkdir){
                         dirty = true;
                         std::cerr << "error: Your local changes to the following files would be overwritten by checkout:" << std::endl;
                         std::cerr << "  " << pair.first << std::endl;
                         // List only first dirty file for brevity
                         break;
                    }

              }
         }
         if(dirty){
              std::cerr << "Please commit your changes or stash them before you switch branches." << std::endl;
              std::cerr << "Aborting" << std::endl;
              return 1;
         }
     } catch (const std::exception& e) {
          std::cerr << "Error checking repository status before checkout: " << e.what() << std::endl;
          return 1;
     }


    // 2. Resolve target ref to a commit SHA
    std::optional<std::string> target_sha_opt = resolve_ref(target_ref);
    if (!target_sha_opt) {
        std::cerr << "fatal: pathspec '" << target_ref << "' did not match any file(s) known to git" << std::endl;
        return 1;
    }
    std::string target_sha = *target_sha_opt;

    // 3. Read the target commit and get its tree
    std::string target_tree_sha;
    try {
        ParsedObject target_commit_obj = read_object(target_sha);
        if (target_commit_obj.type != "commit") {
             std::cerr << "fatal: Reference '" << target_ref << "' (" << target_sha.substr(0,7)
                       << ") is not a commit object." << std::endl;
             return 1;
        }
        target_tree_sha = std::get<CommitObject>(target_commit_obj.data).tree_sha1;
         if (target_tree_sha.empty()){
             // Handle commit with no tree (e.g. corrupt repo?)
             std::cerr << "Warning: Target commit " << target_sha.substr(0,7) << " has no associated tree." << std::endl;
             // Proceed with empty tree? Or fail? Let's proceed cautiously.
             target_tree_sha = "4b825dc642cb6eb9a060e54bf8d69288fbee4904"; // Known empty tree SHA
         }

    } catch (const std::exception& e) {
        std::cerr << "fatal: Failed to read target commit object '" << target_sha.substr(0,7) << "': " << e.what() << std::endl;
        return 1;
    }


    // 4. Update index and working directory using read-tree -u
    int read_tree_ret = handle_read_tree(target_tree_sha, true /* update workdir */, false /* no merge mode */);
    if (read_tree_ret != 0) {
         std::cerr << "Error updating index/workdir during checkout. Checkout aborted partially." << std::endl;
         // State might be inconsistent here!
         return 1;
    }


    // 5. Update HEAD
    std::string new_head_value;
    bool is_branch = false;
    // Check if target_ref corresponds to a known branch
    std::string potential_branch_ref = get_branch_ref(target_ref);
    std::string direct_ref_val = read_ref_direct(potential_branch_ref);
     if (!direct_ref_val.empty() && resolve_ref(target_ref) == target_sha_opt) {
         // It looks like a branch name that resolves to the target commit
         new_head_value = "ref: " + potential_branch_ref;
         is_branch = true;
     } else {
         // Detached HEAD - point directly to commit SHA
         new_head_value = target_sha;
         is_branch = false;
     }

    try {
        update_head(new_head_value); // update_head handles symbolic/direct internally now
    } catch (const std::exception& e) {
         std::cerr << "Error updating HEAD during checkout: " << e.what() << std::endl;
         // Index/workdir updated, but HEAD didn't! Bad state.
         return 1;
    }

    // 6. Output confirmation message
    if (is_branch) {
        std::cout << "Switched to branch '" << target_ref << "'" << std::endl;
    } else {
        std::cout << "Note: switching to '" << target_ref << "'." << std::endl;
        std::cout << "You are in 'detached HEAD' state..." << std::endl;
        // Add more advice like git does
    }

    return 0;
}



int handle_cat_file(const std::string& operation, const std::string& sha1_prefix) {
    // Add check for valid operation early
    if (operation != "-t" && operation != "-s" && operation != "-p") {
        // Error handled in main dispatch, but could check here too
        throw std::invalid_argument("error: invalid option '" + operation + "'");
    }

    try {
        // Resolve prefix first using find_object from objects.cpp/h
        std::string full_sha = find_object(sha1_prefix); // Reuse the finder
        ParsedObject object = read_object(full_sha); // Use the already-parsing read_object

        if (operation == "-t") {
            std::cout << object.type << std::endl;
        } else if (operation == "-s") {
            // Need to get size from the *original* header, not parsed content size
            // Let's modify read_object slightly OR re-read/decompress just for size?
            // Easier: get size from the parsed object if possible (for blobs),
            // or recalculate for trees/commits if needed?
            // Git cat-file -s gives the size from the header "type <size>\0..."
            // Let's modify ParsedObject to store this original size.

            // --> Requires modification in objects.h/cpp: Add original_size to ParsedObject struct
            // --> And populate it in read_object() in objects.cpp before parsing content.
            // Assuming ParsedObject has original_size:
             std::cerr << "Warning: cat-file -s requires ParsedObject.original_size to be implemented." << std::endl;
             // For now, use the parsed content size as an approximation
             if (object.type == "blob") std::cout << std::get<BlobObject>(object.data).content.size() << std::endl;
             else if (object.type == "tree") std::cout << std::get<TreeObject>(object.data).entries.size() << " entries (approx size)" << std::endl; // Not quite right
             else if (object.type == "commit") std::cout << std::get<CommitObject>(object.data).message.size() << " msg size (approx)" << std::endl; // Not right
             else if (object.type == "tag") std::cout << std::get<TagObject>(object.data).message.size() << " msg size (approx)" << std::endl; // Not right
             // Fallback:
             // std::cout << object.size << std::endl; // Use the size field parsed initially? YES.

             std::cout << object.size << std::endl; // Use the parsed size field

        } else if (operation == "-p") {
            // Pretty-print based on type
            if (object.type == "blob") {
                 std::cout << std::get<BlobObject>(object.data).content;
                 // Add trailing newline like git if missing?
                  if (!std::get<BlobObject>(object.data).content.empty() && std::get<BlobObject>(object.data).content.back() != '\n') {
                      std::cout << std::endl;
                  }
            } else if (object.type == "tree") {
                 const auto& tree = std::get<TreeObject>(object.data);
                 for(const auto& entry : tree.entries) {
                     std::string type_str = (entry.mode == "40000") ? "tree" : "blob"; // Simple guess
                     // Need to read object type properly if needed
                     printf("%06s %s %s\t%s\n", entry.mode.c_str(), type_str.c_str(), entry.sha1.c_str(), entry.name.c_str());
                 }
            } else if (object.type == "commit") {
                 // Re-format roughly like git cat-file -p commit
                 const auto& commit = std::get<CommitObject>(object.data);
                 std::cout << "tree " << commit.tree_sha1 << std::endl;
                 for(const auto& p : commit.parent_sha1s) std::cout << "parent " << p << std::endl;
                 std::cout << "author " << commit.author_info << std::endl;
                 std::cout << "committer " << commit.committer_info << std::endl;
                 std::cout << std::endl;
                 std::cout << commit.message << std::endl; // Assume message includes necessary newlines
            } else if (object.type == "tag") {
                 const auto& tag = std::get<TagObject>(object.data);
                 std::cout << "object " << tag.object_sha1 << std::endl;
                 std::cout << "type " << tag.type << std::endl;
                 std::cout << "tag " << tag.tag_name << std::endl;
                 std::cout << "tagger " << tag.tagger_info << std::endl;
                 std::cout << std::endl;
                 std::cout << tag.message << std::endl;
            } else {
                 // Should not happen if read_object worked
                 std::cerr << "Error: Unknown object type for pretty-print: " << object.type << std::endl;
                 return 1;
            }
        }
        return 0; // Success
    } catch (const std::exception& e) {
        std::cerr << "fatal: " << e.what() << std::endl; // Mimic git's fatal prefix
        return 1; // Failure
    }
}

// --- hash-object --- (Based on previous version)
int handle_hash_object(const std::string& filename, const std::string& type, bool write_mode) {
    // Validate type (simple check)
    if (type != "blob" && type != "commit" && type != "tree" && type != "tag") {
        std::cerr << "Error: Invalid object type '" << type << "'" << std::endl;
        return 1;
    }

    try {
        std::string content = read_file(filename);
        // Use hash_and_write_object which handles writing if write_mode is true
        // AND returns the correct content SHA.
        // We need to adapt hash_and_write_object based on write_mode.
        // OR, calculate SHA first, then call write logic if needed.

        std::string content_sha = compute_sha1(content);

        if (write_mode) {
             // Construct object data and write it
             std::string object_data = type + " " + std::to_string(content.size()) + '\0' + content;
             std::string path = get_object_path(content_sha); // Path uses content sha
              if (!file_exists(path)) {
                  std::vector<unsigned char> compressed = compress_data(object_data);
                  ensure_object_directory_exists(content_sha);
                  write_file(path, compressed);
              }
        }
        // Always print the content SHA
        std::cout << content_sha << std::endl;
        return 0; // Success
    } catch (const std::exception& e) {
        std::cerr << "Error hashing object '" << filename << "': " << e.what() << std::endl;
        return 1; // Failure
    }
}

int handle_rev_parse(const std::vector<std::string>& args) {
    if (args.size() != 1) {
         std::cerr << "Usage: mygit rev-parse <ref>" << std::endl;
         return 1;
    }
    std::string ref_name = args[0];
    try {
         std::optional<std::string> sha = resolve_ref(ref_name);
         if (sha) {
              std::cout << *sha << std::endl;
              return 0;
         } else {
              std::cerr << ref_name << ": unknown revision or path not in the working tree." << std::endl;
              return 128;
         }
    } catch (const std::exception& e) {
         std::cerr << "Error rev-parse: " << e.what() << std::endl;
         return 1;
    }
}
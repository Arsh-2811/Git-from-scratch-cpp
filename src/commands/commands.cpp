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

int handle_add(const std::vector<std::string>& files_to_add) {
    if (files_to_add.empty()) {
        std::cerr << "Nothing specified, nothing added." << std::endl;
        std::cerr << "Maybe you wanted to say 'mygit add .'?" << std::endl; // Helpful message
        return 1;
    }

    IndexMap index = read_index(); // Read index once

    for (const std::string& filepath_arg : files_to_add) {
         // Handle '.' (add all modified/new in current dir - needs expansion)
         // Handle directories (recurse and add files) - needs expansion
         // For now, assume individual files are passed

        fs::path filepath = filepath_arg;
         // Normalize path? Git stores relative paths.
         std::string relative_path = filepath.lexically_normal().generic_string(); // Make separators consistent


        if (!file_exists(relative_path)) {
            std::cerr << "fatal: pathspec '" << relative_path << "' did not match any files" << std::endl;
            // Should we continue or fail? Git continues.
            continue;
        }

         if (fs::is_directory(relative_path)) {
             std::cerr << "Warning: Adding directories is not fully implemented yet. Skipping '" << relative_path << "'" << std::endl;
             // TODO: Recurse into directory and add contained files
             continue;
         }

        try {
            // 1. Read file content
            std::string content = read_file(relative_path);

            // 2. Create blob object (writes if not exists)
            std::string sha1 = hash_and_write_object("blob", content);

            // 3. Get file mode (handle non-existence from get_file_mode?)
            mode_t mode_raw = get_file_mode(relative_path);
             if (mode_raw == 0) {
                  std::cerr << "Warning: Could not determine mode for file: " << relative_path << ". Using default 100644." << std::endl;
                  mode_raw = 0100644;
             }
            std::string mode_str = std::to_string(mode_raw);


            // 4. Prepare index entry (stage 0)
            IndexEntry entry;
            entry.mode = mode_str;
            entry.sha1 = sha1;
            entry.stage = 0;
            entry.path = relative_path;
            // TODO: Populate stat data if implementing optimizations

            // 5. Add/Update entry in the index map
            // If the file was conflicted (stages > 0), adding it resolves the conflict
            // by removing the higher stages and placing the new version at stage 0.
            remove_entry(index, relative_path, 1);
            remove_entry(index, relative_path, 2);
            remove_entry(index, relative_path, 3);
            add_or_update_entry(index, entry);


        } catch (const std::exception& e) {
            std::cerr << "Error adding file '" << relative_path << "': " << e.what() << std::endl;
            // Continue to next file? Or return error? Git often continues.
        }
    }

    try {
        write_index(index); // Write index back once at the end
    } catch (const std::exception& e) {
        std::cerr << "Error writing index file: " << e.what() << std::endl;
        return 1; // Fail if index cannot be written
    }

    return 0;
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


// --- write-tree ---
int handle_write_tree() {
    IndexMap index = read_index();
    if (index.empty()) {
        std::cerr << "Error: Index is empty. Nothing to write." << std::endl;
        return 1;
    }

    // Check for unmerged entries (conflicts)
     for (const auto& path_pair : index) {
         for (const auto& stage_pair : path_pair.second) {
             if (stage_pair.first > 0) {
                 std::cerr << "error: Path '" << path_pair.first << "' is unmerged." << std::endl;
                 std::cerr << "fatal: Cannot write tree with unmerged paths." << std::endl;
                 return 1;
             }
         }
     }


    // Convert index (stage 0) entries to TreeEntry format
    std::vector<TreeEntry> tree_entries;
    for (const auto& path_pair : index) {
         auto stage0_it = path_pair.second.find(0);
         if (stage0_it != path_pair.second.end()) {
            const IndexEntry& idx_entry = stage0_it->second;
            // We only handle flat trees here. Building nested trees from paths like "dir/file.txt"
            // requires recursive tree construction.
            // TODO: Implement nested tree building if paths contain '/'.
             if (idx_entry.path.find('/') != std::string::npos) {
                 std::cerr << "Error: write-tree currently only supports flat directories (no '/'). Path: " << idx_entry.path << std::endl;
                 return 1; // Fail for now
             }

             tree_entries.push_back({idx_entry.mode, idx_entry.path, idx_entry.sha1});
         }
    }


    try {
        // Format and write the tree object
        std::string tree_content = format_tree_content(tree_entries);
        std::string tree_sha1 = hash_and_write_object("tree", tree_content);

        std::cout << tree_sha1 << std::endl; // Output the tree SHA
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
     if (!tree_sha_opt) {
         std::cerr << "fatal: Not a valid tree object name: " << tree_sha_prefix << std::endl;
         return 1;
     }
     std::string tree_sha = *tree_sha_opt;


     // 2. Read and parse the tree object
     std::map<std::string, TreeEntry> tree_map; // Use map for easier lookup
     try {
        ParsedObject parsed_obj = read_object(tree_sha);
        if (parsed_obj.type != "tree") {
            std::cerr << "fatal: Object " << tree_sha << " is not a tree." << std::endl;
            return 1;
        }
        const auto& tree = std::get<TreeObject>(parsed_obj.data);
         for(const auto& entry : tree.entries) {
             // TODO: Handle recursive read-tree if needed (Git usually reads just one level?)
             // For now, assume we only care about the entries in *this* tree.
             if (entry.mode != "40000") { // Only process files/links, not subtrees directly
                tree_map[entry.name] = entry;
             } else {
                 std::cerr << "Warning: read-tree currently ignores sub-directories found in tree " << tree_sha << std::endl;
             }
         }

     } catch (const std::exception& e) {
          std::cerr << "fatal: Failed to read tree " << tree_sha << ": " << e.what() << std::endl;
          return 1;
     }

     // 3. Read current index
     IndexMap index = read_index();

     // 4. Update index based on mode
     if (merge_mode) {
         // *** Merge Logic (Simplified) ***
         // This is highly simplified. Real 'git read-tree -m' performs complex
         // 3-way merge checks involving HEAD, the new tree, and potentially another tree.
         // We'll just overlay the new tree onto the index for now.
         std::cerr << "Warning: read-tree merge mode (-m) is highly simplified." << std::endl;
          for (const auto& pair : tree_map) {
             const TreeEntry& tree_entry = pair.second;
             IndexEntry idx_entry;
             idx_entry.mode = tree_entry.mode;
             idx_entry.path = tree_entry.name; // Assumes flat paths
             idx_entry.sha1 = tree_entry.sha1;
             idx_entry.stage = 0; // Simple overlay - overwrite stage 0
             add_or_update_entry(index, idx_entry);
         }
     } else {
         // *** Overwrite Mode ***
         index.clear(); // Clear the existing index
          for (const auto& pair : tree_map) {
              const TreeEntry& tree_entry = pair.second;
              IndexEntry idx_entry;
              idx_entry.mode = tree_entry.mode;
              idx_entry.path = tree_entry.name;
              idx_entry.sha1 = tree_entry.sha1;
              idx_entry.stage = 0;
              add_or_update_entry(index, idx_entry);
          }
     }


     // 5. Write the updated index
     try {
        write_index(index);
     } catch (const std::exception& e) {
          std::cerr << "Error writing updated index: " << e.what() << std::endl;
          return 1;
     }


     // 6. Update working directory if requested (-u option in git, passed as update_workdir)
     if (update_workdir) {
         std::cerr << "Warning: read-tree working directory update (-u) is not implemented." << std::endl;
         // TODO: Implement workdir update:
         // - Compare old index (before clear/merge) and new index.
         // - Delete files removed from index.
         // - Checkout/update files modified or added in the new index.
         // - Handle conflicts if merge_mode resulted in them.
         return 1; // Return error as feature is missing
     }


    return 0;
}


// --- commit ---
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
     if (file_exists(merge_head_path)) {
         merge_in_progress = true;
         merge_head_sha = read_file(merge_head_path);
         if (!merge_head_sha.empty() && merge_head_sha.back() == '\n') merge_head_sha.pop_back();
         // Basic validation of merge head sha
          if (merge_head_sha.length() != 40 || merge_head_sha.find_first_not_of("0123456789abcdef") != std::string::npos) {
              std::cerr << "Error: Invalid SHA-1 found in MERGE_HEAD: " << merge_head_sha << std::endl;
              return 1;
          }
     }

     for (const auto& path_pair : current_index) {
         for (const auto& stage_pair : path_pair.second) {
             if (stage_pair.first > 0) {
                 std::cerr << "error: Committing is not possible because you have unmerged files." << std::endl;
                 std::cerr << "hint: Fix them up in the work tree, and then use 'mygit add <file>'." << std::endl;
                 std::cerr << "fatal: Exiting because of unmerged files." << std::endl;
                 return 1;
             }
         }
     }


    // 2. Write index to a tree object
    std::string tree_sha1;
    try {
         // This re-reads the index, slightly inefficient but ensures consistency
         int write_tree_ret = handle_write_tree(); // Reuse write-tree logic
         if (write_tree_ret != 0) {
             std::cerr << "Error: Failed to write tree before commit." << std::endl;
             return 1;
         }
          // We need the SHA printed by handle_write_tree. This is awkward.
          // Refactor write-tree logic into a reusable function returning the SHA.

          // --- Replicated/Refactored write-tree logic ---
          IndexMap index_for_commit = read_index(); // Read again after conflict check
          std::vector<TreeEntry> tree_entries;
          for (const auto& path_pair : index_for_commit) {
             auto stage0_it = path_pair.second.find(0);
             if (stage0_it != path_pair.second.end()) {
                  if (stage0_it->second.path.find('/') != std::string::npos) { // Check from write-tree
                      std::cerr << "Error: commit currently only supports flat directories. Path: " << stage0_it->second.path << std::endl;
                      return 1;
                  }
                 tree_entries.push_back({stage0_it->second.mode, stage0_it->second.path, stage0_it->second.sha1});
             }
         }
          std::string tree_content = format_tree_content(tree_entries);
          tree_sha1 = hash_and_write_object("tree", tree_content);
          // --- End refactored logic ---

    } catch (const std::exception& e) {
         std::cerr << "Error creating tree object for commit: " << e.what() << std::endl;
         return 1;
    }


    // 3. Determine parent commit(s)
    std::vector<std::string> parent_sha1s;
    std::optional<std::string> head_parent_sha = resolve_ref("HEAD");
    if (head_parent_sha) {
        // Check if index matches HEAD tree - if so, nothing to commit
        try {
            ParsedObject parent_commit_obj = read_object(*head_parent_sha);
            if (parent_commit_obj.type == "commit") {
                std::string parent_tree_sha = std::get<CommitObject>(parent_commit_obj.data).tree_sha1;
                if (parent_tree_sha == tree_sha1 && !merge_in_progress) {
                    std::cerr << "On branch " << read_head() /* Improve branch reporting */ << std::endl;
                    std::cerr << "nothing to commit, working tree clean" << std::endl;
                    return 0; // Nothing changed
                }
            }
        } catch (...) { /* Ignore errors reading parent here */ }

        parent_sha1s.push_back(*head_parent_sha);
    } // Else: Initial commit, no parents


     // Add merge parent if applicable
     if (merge_in_progress) {
         if (head_parent_sha && *head_parent_sha == merge_head_sha) {
              std::cerr << "Warning: Attempting merge commit with HEAD == MERGE_HEAD. This shouldn't happen." << std::endl;
              // Proceeding anyway, might result in duplicate parent
         }
         parent_sha1s.push_back(merge_head_sha);
         // Ensure parents are unique and sorted? Git seems to list HEAD first, then merge parent.
         // Let's keep the order: HEAD parent, then MERGE_HEAD parent.
     }


    // 4. Get author/committer info and time
    std::string author = get_user_info() + " " + get_current_timestamp_and_zone();
    std::string committer = author; // Use same for simplicity, Git allows different

    // 5. Format and write commit object
    std::string commit_content = format_commit_content(tree_sha1, parent_sha1s, author, committer, message);
    std::string commit_sha1 = hash_and_write_object("commit", commit_content);

    // 6. Update HEAD reference
    std::string head_ref = read_head();
    if (head_ref.rfind("ref: ", 0) == 0) {
        // Update the branch HEAD points to
        std::string current_branch_ref = head_ref.substr(5);
        update_ref(current_branch_ref, commit_sha1);
    } else {
        // Detached HEAD - update HEAD directly
        update_head(commit_sha1);
    }

     // 7. Clean up MERGE_HEAD if it was a merge commit
     if (merge_in_progress) {
         try {
             fs::remove(merge_head_path);
         } catch (const fs::filesystem_error& e) {
             std::cerr << "Warning: Failed to remove MERGE_HEAD file: " << e.what() << std::endl;
         }
     }


    // 8. Output commit info
     std::cout << "[" << head_ref.substr(head_ref.find_last_of('/') + 1) /* Simple branch name */
               << (head_parent_sha ? "" : " (root-commit)") // Indicate initial commit
               << " " << commit_sha1.substr(0, 7) << "] " << message.substr(0, message.find('\n')) << std::endl;


    return 0;
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

     if (conflicted_files.empty() && staged_changes.empty() && unstaged_changes.empty() && untracked_files.empty()) {
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
              std::cout << "  (use \"mygit restore <file>...\" to discard changes in working directory - NOT IMPLEMENTED)" << std::endl; // Placeholder for restore/checkout
             for(const auto& s : unstaged_changes) std::cout << "\033[31m" << s << "\033[0m" << std::endl; // Red
         }
          if (!untracked_files.empty()) {
             std::cout << "\nUntracked files:" << std::endl;
             std::cout << "  (use \"mygit add <file>...\" to include in what will be committed)" << std::endl;
             for(const auto& s : untracked_files) std::cout << "\033[31m" << s << "\033[0m" << std::endl; // Red
         }
     }


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

std::optional<std::string> find_merge_base_naive(const std::string& sha1_a, const std::string& sha1_b) {
    std::set<std::string> ancestors_a;
    std::queue<std::string> q_a;

    q_a.push(sha1_a);
    ancestors_a.insert(sha1_a);

    int count = 0;
    const int limit = 1000;

    while(!q_a.empty() && count++ < limit) {
        std::string current = q_a.front();
        q_a.pop();
        try {
            ParsedObject obj = read_object(current);
            if (obj.type == "commit") {
                const auto& commit = std::get<CommitObject>(obj.data);
                for (const auto& p : commit.parent_sha1s) {
                    if (ancestors_a.find(p) == ancestors_a.end()) {
                        ancestors_a.insert(p);
                        q_a.push(p);
                    }
                }
            }
        } catch (...) { /* Ignore read errors */ }
    }

    std::queue<std::string> q_b;
    std::set<std::string> visited_b;
    q_b.push(sha1_b);
    visited_b.insert(sha1_b);
    count = 0;

    while(!q_b.empty() && count++ < limit) {
        std::string current = q_b.front();
        q_b.pop();

        if (ancestors_a.count(current)) {
            return current;
        }

        try {
            ParsedObject obj = read_object(current);
            if (obj.type == "commit") {
                const auto& commit = std::get<CommitObject>(obj.data);
                for (const auto& p : commit.parent_sha1s) {
                    if (visited_b.find(p) == visited_b.end()) {
                        visited_b.insert(p);
                        q_b.push(p);
                    }
                }
            }
        } catch (...) { /* Ignore read errors */ }
    }

    return std::nullopt;
}


int handle_merge(const std::string& branch_to_merge) {
    std::optional<std::string> head_sha_opt = resolve_ref("HEAD");
    if (!head_sha_opt) {
        std::cerr << "Error: Cannot merge, HEAD does not point to a commit." << std::endl;
        return 1;
    }
    std::string head_sha = *head_sha_opt;

    std::optional<std::string> theirs_sha_opt = resolve_ref(branch_to_merge);
    if (!theirs_sha_opt) {
        std::cerr << "fatal: '" << branch_to_merge << "' does not point to a commit" << std::endl;
        return 1;
    }
    std::string theirs_sha = *theirs_sha_opt;

    if (head_sha == theirs_sha) {
        std::cout << "Already up to date." << std::endl;
        return 0;
    }

    std::optional<std::string> base_sha_opt = find_merge_base_naive(head_sha, theirs_sha);
    if (!base_sha_opt) {
        base_sha_opt = find_merge_base_naive(theirs_sha, head_sha);
    }
    if (!base_sha_opt) {
        std::cerr << "fatal: Could not find a common ancestor for merging." << std::endl;
        return 1;
    }
    std::string base_sha = *base_sha_opt;

    if (base_sha == theirs_sha) {
        std::cout << "Already up to date." << std::endl;
        return 0;
    } else if (base_sha == head_sha) {
        std::cout << "Updating " << head_sha.substr(0, 7) << ".." << theirs_sha.substr(0, 7) << std::endl;

        std::string theirs_tree_sha;
        try {
            ParsedObject theirs_commit = read_object(theirs_sha);
            if (theirs_commit.type != "commit") throw std::runtime_error("Target is not a commit");
            theirs_tree_sha = std::get<CommitObject>(theirs_commit.data).tree_sha1;
        } catch (const std::exception& e) {
            std::cerr << "Error reading target commit object " << theirs_sha << ": " << e.what() << std::endl;
            return 1;
        }

        std::cout << "Fast-forward" << std::endl;
        int read_tree_ret = handle_read_tree(theirs_tree_sha, true /* update workdir */, false /* no merge mode */);
        if (read_tree_ret != 0) {
            std::cerr << "Error updating index/workdir during fast-forward. Merge aborted." << std::endl;
            return 1;
        }

        std::string head_ref = read_head();
        if (head_ref.rfind("ref: ", 0) == 0) {
            std::string current_branch_ref = head_ref.substr(5);
            update_ref(current_branch_ref, theirs_sha);
        } else {
            update_head(theirs_sha);
        }
        std::cout << "Merge successful (fast-forward)." << std::endl;

    } else {
        std::cout << "Starting 3-way merge (NOT IMPLEMENTED)" << std::endl;
        std::cerr << "fatal: True merge logic is not implemented yet." << std::endl;
        return 1;
    }
    return 0;
}
#include <iostream>
#include <string>
#include <vector>

#include "headers/commands.h"
#include "headers/utils.h"

void print_usage() {
    std::cerr << "Usage: mygit <command> [<args>...]" << std::endl;
    std::cerr << std::endl;
    std::cerr << "Available commands:" << std::endl;
    std::cerr << "  init              Create an empty Git repository or reinitialize an existing one" << std::endl;
    std::cerr << "  add <file>...     Add file contents to the index" << std::endl;
    std::cerr << "  rm [--cached] <file>..." << std::endl;
    std::cerr << "                    Remove files from the working tree and from the index" << std::endl;
    std::cerr << "  commit -m <msg>   Record changes to the repository" << std::endl;
    std::cerr << "  status            Show the working tree status" << std::endl;
    std::cerr << "  log [<ref>] [--graph]" << std::endl;
    std::cerr << "  branch            List, create, or delete branches" << std::endl;
    std::cerr << "  branch <name> [<start>] Create a new branch" << std::endl;
    // std::cerr << "  branch -d <name>  Delete a branch" << std::endl; // Add delete later
    std::cerr << "  checkout <branch|commit> Switch branches or restore working tree files" << std::endl;
    std::cerr << "  tag               List tags" << std::endl;
    std::cerr << "  tag [-a [-m <msg>]] <name> [<obj>]" << std::endl;
    std::cerr << "                    Create a tag object" << std::endl;
    std::cerr << "  write-tree        Create a tree object from the current index" << std::endl;
    std::cerr << "  read-tree <tree-ish> Read tree information into the index" << std::endl; // Add flags later
    std::cerr << "  merge <branch>    Join two or more development histories together" << std::endl;
    // Add cat-file, hash-object back if needed for low-level operations
    std::cerr << "  rev-parse <ref>   Resolve ref name to SHA-1" << std::endl;
    std::cerr << "  cat-file (-t | -s | -p) <object>" << std::endl;
    std::cerr << "                    Provide content or type and size information for repository objects" << std::endl;
    std::cerr << "  hash-object [-w] [-t <type>] <file>" << std::endl;
    std::cerr << "                    Compute object ID and optionally create an object from a file" << std::endl;
    std::cerr << "  ls-tree [-r] <tree-ish>" << std::endl;
    std::cerr << "                    List the contents of a tree object" << std::endl;
}

std::vector<std::string> collect_args(int start_index, int argc, char* argv[]) {
    std::vector<std::string> args;
    for (int i = start_index; i < argc; ++i) {
        args.push_back(argv[i]);
    }
    return args;
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        print_usage();
        return 1;
    }

    std::string command = argv[1];

    if (command != "init" && !fs::exists(GIT_DIR)) {
        std::cerr << "fatal: not a git repository (or any of the parent directories): " << GIT_DIR << std::endl;
        return 1;
    }

    try {
        if (command == "init") {
            if (argc != 2) {
                std::cerr << "Usage: mygit init" << std::endl; return 1;
            }
            return handle_init();
        }
        else if (command == "add") {
            if (argc < 3) {
                std::cerr << "Usage: mygit add <file>..." << std::endl; return 1;
            }
            return handle_add(collect_args(2, argc, argv));
        }
        else if (command == "rm") {
            bool cached = false;
            std::vector<std::string> files;
            for (int i = 2; i < argc; ++i) {
                if (std::string(argv[i]) == "--cached") {
                    cached = true;
                } else {
                    files.push_back(argv[i]);
                }
            }
            if (files.empty()) {
                std::cerr << "Usage: mygit rm [--cached] <file>..." << std::endl; return 1;
            }
            return handle_rm(files, cached);
        } else if (command == "commit") {
            std::string message = "";
            if (argc == 4 && std::string(argv[2]) == "-m") {
                message = argv[3];
            } else {
                std::cerr << "Usage: mygit commit -m <message>" << std::endl;
                std::cerr << "(Editor support not implemented)" << std::endl;
                return 1;
            }
            return handle_commit(message);
        } else if (command == "status") {
            if (argc != 2) {
                std::cerr << "Usage: mygit status" << std::endl; return 1;
            }
            return handle_status();
        }
        else if (command == "log") {
            bool graph_mode = false;
            std::optional<std::string> start_ref_opt = std::nullopt;

            // Iterate through arguments after "log" (index 2 onwards)
            for (int i = 2; i < argc; ++i) {
                std::string arg = argv[i];
                if (arg == "--graph") {
                    if (graph_mode) { // Check for duplicate --graph flag
                        std::cerr << "Error: Duplicate --graph option provided." << std::endl;
                        print_usage(); // Show general usage
                        return 1;
                    }
                    graph_mode = true;
                } else {
                    // Assume it's the reference argument
                    if (start_ref_opt) { // Check if a ref has already been provided
                        std::cerr << "Error: Too many non-option arguments provided for log." << std::endl;
                        std::cerr << "Usage: mygit log [<ref>] [--graph]" << std::endl;
                        return 1;
                    }
                    start_ref_opt = arg; // Store the potential reference name
                }
            }
            return handle_log(graph_mode, start_ref_opt);
        } else if (command == "branch") {
            return handle_branch(collect_args(2, argc, argv));
        } else if (command == "tag") {
            return handle_tag(collect_args(2, argc, argv));
        } else if (command == "write-tree") {
            if (argc != 2) {
                std::cerr << "Usage: mygit write-tree" << std::endl; return 1;
            }
            return handle_write_tree();
        } else if (command == "read-tree") {
            if (argc != 3) {
                std::cerr << "Usage: mygit read-tree <tree-ish>" << std::endl; return 1;
            }
            return handle_read_tree(argv[2], false, false);
        } else if (command == "checkout") {
            if (argc != 3) {
                std::cerr << "Usage: mygit checkout <branch|commit>" << std::endl;
                // Add usage for checking out paths later maybe
                return 1;
            }
            return handle_checkout(argv[2]);
        } else if (command == "merge") {
            if (argc != 3) {
                std::cerr << "Usage: mygit merge <branch>" << std::endl; return 1;
            }
            return handle_merge(argv[2]);
        } else if (command == "cat-file") {
            if (argc != 4) {
                std::cerr << "Usage: mygit cat-file (-t | -s | -p) <object>" << std::endl;
                return 1;
            }
            std::string operation = argv[2];
            std::string sha1_prefix = argv[3];
            // Validation happens inside handle_cat_file now
            return handle_cat_file(operation, sha1_prefix);

        } else if (command == "hash-object") {
            bool write_mode = false;
            std::string type = "blob"; // Default type
            std::string filename;

            for (int i = 2; i < argc; ++i) {
                std::string arg = argv[i];
                if (arg == "-w") {
                    write_mode = true;
                } else if (arg == "-t") {
                    if (i + 1 < argc) { type = argv[++i]; }
                    else { /* Error: -t requires arg */ return 1; }
                } else if (filename.empty()) { filename = arg; }
                else { /* Error: too many args */ return 1; }
            }

            if (filename.empty()) { /* Error: Missing filename */ return 1; }

            return handle_hash_object(filename, type, write_mode);

        } else if (command == "rev-parse") {
            return handle_rev_parse(collect_args(2, argc, argv));
        } else if (command == "ls-tree") {
            return handle_ls_tree(collect_args(2, argc, argv));
        } else {
            std::cerr << "mygit: '" << command << "' is not a mygit command. See 'mygit --help' (or just 'mygit')." << std::endl;
            print_usage();
            return 1;
        }

    } catch (const std::exception& e) {
        std::cerr << "Fatal error: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}
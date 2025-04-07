# mygit: C++ Git Implementation Core

This directory contains the source code for `mygit`, a custom implementation of core Git functionality written in C++17. It serves as the foundational engine for the [Git from Scratch++](../README.md) project.

## Purpose

The primary goal of `mygit` is to provide a hands-on learning experience by rebuilding essential Git features from scratch. This includes understanding and implementing:

*   Git's object model (blobs, trees, commits, tags)
*   Content-addressable storage and SHA-1 hashing
*   Object compression (zlib)
*   The Index (staging area) mechanism
*   Reference management (branches, tags, HEAD)
*   Commit history traversal
*   Basic branching and merging logic

## Features Implemented

`mygit` provides the following command-line interface, mimicking standard Git commands:

| Command          | Description                                                                    |
| :--------------- | :----------------------------------------------------------------------------- |
| `init`           | Create/reinitialize an empty repository (`.mygit` directory)                   |
| `add <file>...`  | Add file contents to the index (staging area)                                  |
| `rm [--cached] <file>...` | Remove files from the index and optionally working directory            |
| `commit -m <msg>`| Record changes (staged in the index) to the repository                       |
| `status`         | Show the working tree status (changes vs index vs HEAD)                        |
| `log [<ref>] [--graph]` | Show commit logs (linear history and `--graph` DOT output)               |
| `branch`         | List branches                                                                  |
| `branch <name> [<start>]` | Create a new branch                                                    |
| `checkout <branch\|commit>` | Switch branches or restore working tree files (switch/detach HEAD)   |
| `tag`            | List tags                                                                      |
| `tag [-a [-m <msg>]] <name> [<obj>]` | Create a lightweight or annotated tag object                |
| `merge <branch>` | Merge branches (fast-forward and basic 3-way merge with conflict detection)    |
| `write-tree`     | Create a tree object from the current index                                    |
| `read-tree <tree-ish>` | Read tree information into the index                                     |
| `cat-file (-t\|-s\|-p) <object>` | Inspect Git objects (type, size, content)                       |
| `hash-object [-w] [-t <type>] <file>` | Compute object ID and optionally create blob from file     |
| `rev-parse <ref>`| Resolve ref names (branch, tag, HEAD, SHA) to full SHA-1                       |
| `ls-tree [-r] <tree-ish>` | List the contents of a tree object                                      |

*(Refer to `src/main.cpp` for the exact usage details printed by the tool).*

## Building

`mygit` uses CMake for building.

**Dependencies:**

*   A C++17 compliant compiler (e.g., GCC >= 7, Clang >= 5)
*   CMake (>= 3.10 recommended)
*   zlib development library (e.g., `zlib1g-dev` on Debian/Ubuntu, `zlib-devel` on Fedora/CentOS, or install via Homebrew/MacPorts on macOS)

**Build Steps:**

1.  **Navigate to this directory:**
    ```bash
    cd /path/to/Git-from-scratch-cpp/mygitImplementation
    ```
2.  **Configure using CMake:** (Create a build directory)
    ```bash
    cmake -S . -B build
    ```
    *   `-S .` specifies the source directory (current directory).
    *   `-B build` specifies the build directory (creates `./build/`).
3.  **Compile:**
    ```bash
    cmake --build build
    ```
4.  **Executable:** The compiled executable will be located at `build/src/mygit`.

## Usage

Once built, you can use `mygit` like the standard `git` command, but operating on its own `.mygit` directories.

```bash
# Example: Create a new repo, add a file, and commit
mkdir my-test-repo
cd my-test-repo

# Initialize using the built executable
/path/to/Git-from-scratch-cpp/mygitImplementation/build/src/mygit init
# Output: Initialized empty Git repository in .../.mygit/

echo "Hello MyGit" > hello.txt
/path/to/Git-from-scratch-cpp/mygitImplementation/build/src/mygit add hello.txt

/path/to/Git-from-scratch-cpp/mygitImplementation/build/src/mygit status
# Output: Changes to be committed... new file: hello.txt

/path/to/Git-from-scratch-cpp/mygitImplementation/build/src/mygit commit -m "Initial commit"
# Output: [main (root-commit) <sha>] Initial commit

/path/to/Git-from-scratch-cpp/mygitImplementation/build/src/mygit log
# Output: commit <sha> ... Author: ... Date: ... Initial commit
```

## Directory Structure

*   `CMakeLists.txt`: Main CMake build script for `mygit`.
*   `include/headers/`: Public header files defining interfaces for commands, objects, index, refs, diff, and utilities.
*   `src/`: Source code implementation.
    *   `CMakeLists.txt`: CMake script specific to the source files.
    *   `main.cpp`: Entry point, command-line argument parsing, and command dispatching.
    *   `commands/`: Implementation files for each `mygit` command and related logic (objects.cpp, index.cpp, refs.cpp, diff.cpp, utils.cpp, commands.cpp).
*   `test_mygit.sh`: A basic shell script for testing `mygit` commands.

## Core Concepts Implemented

*   **Objects:** Blobs, Trees, Commits, Tags stored in `.mygit/objects/`.
*   **Hashing:** SHA-1 used for content addressing.
*   **Compression:** zlib used to compress object files.
*   **Index:** The staging area implemented via `.mygit/index`.
*   **Refs:** Branches (`.mygit/refs/heads/`), Tags (`.mygit/refs/tags/`), and HEAD (`.mygit/HEAD`).
*   **History Traversal:** Following parent pointers in commit objects for `log`.
*   **Merging:** Fast-forward and basic 3-way merge base detection and file-level comparison.

## Testing

A basic test script `test_mygit.sh` is included. It performs a sequence of `mygit` commands to verify basic functionality. Run it from the `mygitImplementation` directory *after* building:

```bash
./test_mygit.sh /path/to/Git-from-scratch-cpp/mygitImplementation/build/src/mygit
```

This testing is rudimentary and could be significantly expanded.
# 🛠️ MyGit: Building Git from Scratch in C++ ✨

[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg?style=for-the-badge)](https://opensource.org/licenses/MIT)

A deep dive into the internals of Git by re-implementing its core functionalities from the ground up using C++17. This project is primarily an educational endeavor to understand how version control systems work under the hood.

**This is NOT intended to be a full replacement for the official Git**, but rather a learning tool and a demonstration of implementing complex system concepts.

---

## 📚 Why Build Git Again?

*   **Demystify Version Control:** To truly understand how Git tracks changes, manages branches, and stores history.
*   **Hands-On Learning:** Gain practical experience with file system manipulation, hashing (SHA-1), data compression (Zlib), object storage, and graph traversal in C++.
*   **C++ Practice:** Apply modern C++ techniques (C++17 filesystem, smart pointers, STL containers, etc.) to a substantial project.
*   **Appreciation:** Develop a deeper appreciation for the elegance and efficiency of the real Git.

---

## ✨ Features Implemented

This implementation currently supports a significant subset of core Git commands:

**Repository Initialization:**
*   `mygit init`: Creates the necessary `.mygit` directory structure (`objects`, `refs`, `HEAD`, etc.).

**Basic Workflow:**
*   `mygit add <file>...`: Stages changes from the working directory to the index. (Handles files, basic subdirectory paths).
*   `mygit rm [--cached] <file>...`: Removes files from the index and optionally the working directory.
*   `mygit commit -m <message>`: Creates a commit object capturing the state of the index, linking it to parent commits. Supports merge commits (multiple parents).
*   `mygit status`: Shows the status of the working directory and staging area (untracked, modified, staged changes).

**History & Inspection:**
*   `mygit log [--graph]`: Displays the commit history. `--graph` outputs a DOT representation for visualization (e.g., using Graphviz).
*   `mygit cat-file (-t | -s | -p) <object>`: Inspects Git objects (blob, tree, commit, tag) by type, size, or content.
*   `mygit hash-object [-w] [-t <type>] <file>`: Computes object IDs (SHA-1) and optionally writes blob objects to the store.

**Branching & Tagging:**
*   `mygit branch`: Lists existing branches.
*   `mygit branch <name> [<start_point>]`: Creates a new branch.
*   `mygit tag`: Lists existing tags.
*   `mygit tag [-a [-m <msg>]] <name> [<object>]`: Creates lightweight or annotated tags.

**Index & Tree Manipulation:**
*   `mygit write-tree`: Creates a tree object representing the current index state (supports subdirectories recursively).
*   `mygit read-tree <tree-ish>`: Reads a tree object into the index (overwrite mode).

**Merging (Basic):**
*   `mygit merge <branch>`:
    *   Detects and performs fast-forward merges (updates branch pointer, index, *and basic workdir update*).
    *   Detects non-fast-forward merges but aborts (3-way merge logic is *not* implemented).
*   `mygit checkout <branch|commit>`:
    *   Switches `HEAD` to the specified branch or commit.
    *   Updates the index and working directory to match the target commit's state (handles subdirectories).
    *   Includes basic safety checks for uncommitted changes.

---

## 💻 Tech Stack

*   **Language:** C++17
*   **Build System:** CMake (version 3.10+)
*   **Libraries:**
    *   OpenSSL (for SHA-1 hashing)
    *   Zlib (for object compression/decompression)
    *   Standard C++ Library (including `<filesystem>`)

---

## 🚀 Getting Started

### Prerequisites

You'll need a C++17 compatible compiler (GCC 7+ or Clang 5+), CMake, and the development libraries for OpenSSL and Zlib.

**On Debian/Ubuntu:**
```bash
sudo apt-get update
sudo apt-get install build-essential cmake libssl-dev zlib1g-dev
```

**On Fedora/CentOS/RHEL:**
```bash
sudo dnf update
sudo dnf install gcc-c++ cmake openssl-devel zlib-devel
```

**On macOS (using Homebrew):**
```bash
brew update
brew install cmake openssl zlib pkg-config
# Note: You might need to help CMake find Homebrew's OpenSSL
# export PKG_CONFIG_PATH="/usr/local/opt/openssl@3/lib/pkgconfig" # Or relevant path
```

### Building

1.  **Clone the repository:**
    ```bash
    git clone https://github.com/Arsh-2811/Git-from-scratch-cpp.git
    cd mygit
    ```
2.  **Create a build directory:**
    ```bash
    cmake -S . -B build
    ```
    *   *(macOS Homebrew Note)* If CMake has trouble finding OpenSSL, you might need:
        ```bash
        cmake -S . -B build -DOPENSSL_ROOT_DIR=$(brew --prefix openssl@3)
        ```
3.  **Compile:**
    ```bash
    cmake --build build
    ```
4.  **Executable:** The `mygit` executable will be located in the `build/` directory. You can run it from there (`./build/mygit`) or add it to your PATH.

### Running

```bash
# Navigate to a directory where you want to create a test repository
mkdir my_test_project && cd my_test_project

# Initialize a MyGit repository
../build/mygit init # Adjust path to executable if needed

# Create and add a file
echo "Hello MyGit!" > hello.txt
../build/mygit add hello.txt

# Check status
../build/mygit status

# Commit
../build/mygit commit -m "Initial commit"

# See log
../build/mygit log
```

---

## 🛠️ Usage Examples

*(Assuming `mygit` is in your PATH or you use the relative path)*

**Basic Workflow:**
```sh
mygit init
echo "File content v1" > file.txt
mkdir src
echo "int main() {}" > src/main.cpp
mygit status
mygit add file.txt src/main.cpp
mygit status
mygit commit -m "Add initial files"
echo "File content v2" >> file.txt
mygit status
mygit diff # (If/when implemented)
mygit add file.txt
mygit commit -m "Update file.txt"
mygit log
```

**Branching:**
```sh
mygit branch                  # List branches
mygit branch my-feature       # Create branch 'my-feature'
mygit checkout my-feature     # Switch to 'my-feature'
echo "New feature" > feature.txt
mygit add feature.txt
mygit commit -m "Add feature file"
mygit checkout main           # Switch back to 'main'
mygit log --graph             # View history with branches (use Graphviz)
```

**Tagging:**
```sh
mygit tag v1.0                # Create lightweight tag at current HEAD
mygit tag v0.9 <commit_sha>   # Create lightweight tag on specific commit
mygit tag -a -m "Release 1.1" v1.1 # Create annotated tag
mygit tag                     # List tags
```

**Low-Level Inspection:**
```sh
# Get SHA of a file's content
mygit hash-object file.txt

# Get SHA and write blob object
BLOB_SHA=$(mygit hash-object -w file.txt)

# Inspect the blob
mygit cat-file -t $BLOB_SHA  # Output: blob
mygit cat-file -s $BLOB_SHA  # Output: <size>
mygit cat-file -p $BLOB_SHA  # Output: <content of file.txt>

# Write current index to a tree and get its SHA
TREE_SHA=$(mygit write-tree)

# Inspect the tree
mygit cat-file -p $TREE_SHA
```

---

## 🚦 Current Status & Limitations

*   ✅ Core object model (blobs, trees, commits, tags) implemented.
*   ✅ Object storage (`.mygit/objects`) with Zlib compression.
*   ✅ Index/Staging area (`.mygit/index`) implemented (simple text format).
*   ✅ Ref management (`.mygit/HEAD`, `.mygit/refs/`) implemented.
*   ✅ Most basic commands (`init`, `add`, `rm`, `commit`, `status`, `log`, `branch`, `tag`, `checkout`) are functional for single files and basic subdirectory structures.
*   ✅ Recursive tree writing/reading supported.
*   ✅ Basic workdir updates implemented for `checkout` and `read-tree -u`.
*   ⚠️ Merge implementation only detects fast-forward or non-fast-forward scenarios. **True 3-way merging logic is missing.** Merging fails for non-FF cases.
*   ❌ No `.gitignore` support.
*   ❌ Performance optimizations (like index stat caching, packfiles) are not implemented. `status` might be slow on large repos.
*   ❌ No networking (remotes, clone, fetch, push, pull).
*   ❌ Limited error handling and user feedback compared to official Git.
*   ❌ Complex history manipulation (rebase, cherry-pick, stash) not implemented.
*   ❌ Diff output is not implemented (only status reporting).

---

## 🗺️ Roadmap / Future Plans

-   [ ] **True 3-Way Merge:** Implement content-level merging and conflict handling.
-   [ ] **Diff Implementation:** Generate unified diff output for `mygit diff` and `mygit diff --staged`.
-   [ ] **`.gitignore`:** Add support for ignoring files.
-   [ ] **`reset` Command:** Implement `--soft`, `--mixed`, and `--hard` modes.
-   [ ] **`checkout <paths>`:** Allow restoring specific files.
-   [ ] **Index Performance:** Add stat data caching.
-   [ ] **Binary Index Format:** Switch to the optimized binary index.
-   [ ] **Subdirectory Handling:** Further robustness testing and potential optimizations for nested trees.
-   [ ] **Packfiles & GC:** Implement object packing and garbage collection.
-   [ ] **Remotes:** Add basic remote tracking, fetch, and maybe push.
-   [ ] **More Commands:** `rebase`, `stash`, `cherry-pick`, etc.
-   [ ] **Improved Error Handling & User Feedback.**

---

## 🙏 Contributing

This is primarily a learning project, but suggestions, bug reports, and discussions are welcome! If you'd like to contribute:

1.  Fork the repository.
2.  Create a new branch (`git checkout -b feature/your-feature`).
3.  Make your changes.
4.  Commit your changes (`git commit -m 'Add some feature'`).
5.  Push to the branch (`git push origin feature/your-feature`).
6.  Open a Pull Request.

Please open an issue first to discuss significant changes.

---

## 📜 License

This project is licensed under the MIT License
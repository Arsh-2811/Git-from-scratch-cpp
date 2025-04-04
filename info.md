# MyGit C++ Implementation - Technical Documentation

**Version:** (Specify date or version if applicable)

**Purpose:** This document provides a detailed technical overview of the MyGit C++ implementation. It is intended for developers working on or seeking to understand the internals of this specific project. It covers core data structures, module responsibilities, command workflows, and key technical choices.

---

## Table of Contents

1.  [Core Concepts](#1-core-concepts)
    *   [Git Objects](#git-objects-blob-tree-commit-tag)
    *   [The Index (Staging Area)](#the-index-staging-area)
    *   [References (Refs) & HEAD](#references-refs--head)
    *   [Repository Structure (`.mygit/`)](#repository-structure-mygit)
2.  [Module Breakdown](#2-module-breakdown)
    *   [`utils.*`](#utils)
    *   [`objects.*`](#objects)
    *   [`index.*`](#index)
    *   [`refs.*`](#refs)
    *   [`diff.*`](#diff)
    *   [`commands.*`](#commands)
    *   [`main.cpp`](#maincpp)
3.  [Command Implementation Details](#3-command-implementation-details)
    *   [`mygit init`](#mygit-init)
    *   [`mygit add`](#mygit-add)
    *   [`mygit rm`](#mygit-rm)
    *   [`mygit commit`](#mygit-commit)
    *   [`mygit status`](#mygit-status)
    *   [`mygit log`](#mygit-log)
    *   [`mygit branch`](#mygit-branch)
    *   [`mygit checkout`](#mygit-checkout)
    *   [`mygit tag`](#mygit-tag)
    *   [`mygit write-tree`](#mygit-write-tree)
    *   [`mygit read-tree`](#mygit-read-tree)
    *   [`mygit merge`](#mygit-merge)
    *   [Plumbing Commands (`cat-file`, `hash-object`, `rev-parse`)](#plumbing-commands)
4.  [Dependencies & Build](#4-dependencies--build)
5.  [Known Limitations & Simplifications](#5-known-limitations--simplifications)

---

## 1. Core Concepts

### Git Objects (Blob, Tree, Commit, Tag)

*   **Concept:** The fundamental units of data storage in Git. Immutable and content-addressable via SHA-1 hashes.
*   **Format:** All objects internally follow `type<space>content_size<null_byte>content`.
*   **Storage:** Stored in `.mygit/objects/`. Typically compressed using Zlib. The path is derived from the object's SHA-1 (e.g., `objects/ab/cdef...`). *Note: MyGit currently uses the content SHA for blob/tree paths and object SHAs for commit/tag paths, consistent with Git.*
*   **Implementation (`objects.cpp`):**
    *   **Blob:** Represents file content. `hash_and_write_object` calculates SHA-1 of raw content, writes `blob size\0content` object if needed. `read_object` parses this, `BlobObject` struct holds content.
    *   **Tree:** Represents directory snapshot. Contains sorted entries: `mode<space>name<NUL>binary_sha1`. `format_tree_content` serializes this binary format from `TreeEntry` structs. `parse_tree_content` parses the binary data. `TreeObject` struct holds `std::vector<TreeEntry>`. Recursive trees are handled by `build_tree_recursive` (in `commands.cpp`) which calls `hash_and_write_object` for subtrees.
    *   **Commit:** Links history. Contains tree SHA, parent SHA(s), author/committer info, message. `format_commit_content` creates the text representation. `parse_commit_content` parses it. `CommitObject` struct holds parsed data.
    *   **Tag (Annotated):** Points to another object. Contains target object SHA/type, tag name, tagger info, message. `format_tag_content` and `parse_tag_content` handle the text format. `TagObject` struct holds data.
*   **Libraries/Functions:**
    *   `<openssl/sha.h>`: `SHA1()` for hashing.
    *   `<zlib.h>`: `compressBound`, `compress`, `inflateInit`, `inflate`, `inflateEnd` (via `compress_data`, `decompress_chunk` in `utils.cpp`).
    *   `std::string`, `std::vector<unsigned char>`, `std::stringstream`, `memchr`.
    *   `objects.h`: Structs (`BlobObject`, `TreeEntry`, etc.), `ParsedObject` (using `std::variant`).
    *   `objects.cpp`: `read_object`, `write_object`, `hash_and_write_object`, `parse_*`, `format_*`.
    *   `utils.cpp`: `compute_sha1`, `compress_data`, `decompress_chunk`, `hex_to_sha1`, `sha1_to_hex`.

### The Index (Staging Area)

*   **Concept:** A file acting as a cache and staging area, decoupling the working directory from the commit history. Tracks files intended for the next commit.
*   **Storage:** `.mygit/index`.
*   **MyGit Format:** Simple text file, one entry per line: `mode<space>sha1<space>stage<tab>path\n`.
    *   `mode`: Octal file mode string (e.g., "100644").
    *   `sha1`: 40-char hex SHA-1 of the corresponding *blob* object's content.
    *   `stage`: 0 (normal), 1 (base), 2 (ours), 3 (theirs) for merge conflicts.
    *   `path`: Relative path from repository root.
*   **Implementation (`index.cpp`):**
    *   `IndexEntry` struct: Holds data for one line.
    *   `IndexMap` typedef: `std::map<std::string, std::map<int, IndexEntry>>` for in-memory representation (path -> stage -> entry).
    *   `read_index()`: Parses the text file into `IndexMap`. Returns empty map if file doesn't exist.
    *   `write_index()`: Writes the `IndexMap` back to the file *atomically* (writes to `.mygit/index.tmp`, then `fs::rename`). Uses a simple lock file (`.mygit/index.lock`) during write. Sorts entries by path then stage before writing.
    *   `add_or_update_entry()`, `remove_entry()`: Helpers to modify the in-memory `IndexMap`.
*   **Libraries/Functions:** `std::ifstream`, `std::ofstream`, `std::map`, `std::string` processing (`find`, `substr`), `split_string` (from `utils.cpp`), `fs::rename`, `fs::remove`.

### References (Refs) & HEAD

*   **Concept:** Pointers used to identify specific commits (branches, tags) or the current checkout state (HEAD).
*   **Storage:** Text files within `.mygit/refs/` or the `.mygit/HEAD` file.
*   **Format:**
    *   Direct: Contains a single 40-character SHA-1 followed by a newline.
    *   Symbolic: Contains `ref:<space>path/to/another/ref\n` (e.g., `ref: refs/heads/main`).
*   **`HEAD`:** Can be direct (detached HEAD state) or symbolic (checked out on a branch).
*   **Implementation (`refs.cpp`):**
    *   `read_ref_direct()`: Reads the raw content of a ref file.
    *   `update_ref()`: Atomically updates/creates a ref file (using a `.lock` file). Takes content and a `symbolic` flag. *Crucially, it assumes the caller formats the content correctly (e.g., adding `ref: `)*.
    *   `resolve_ref()`: The core resolution logic. Takes a ref name (e.g., "main", "HEAD", "v1.0") or SHA prefix. Tries resolving in order: HEAD, `refs/heads/`, `refs/tags/`, direct SHA prefix lookup in `objects/`. Handles symbolic refs recursively (with depth limit). Dereferences annotated tags to return the commit SHA. Returns `std::optional<std::string>`.
    *   `read_head()`: Helper calling `read_ref_direct("HEAD")`.
    *   `update_head()`: Helper calling `update_ref` for "HEAD", correctly formatting the value based on whether it looks like a ref path or a SHA.
    *   `list_branches()`, `list_tags()`: Use `fs::recursive_directory_iterator` on `refs/heads` and `refs/tags`.
    *   `get_branch_ref()`, `get_tag_ref()`: Construct full ref paths, perform basic name validation.
*   **Libraries/Functions:** `std::ifstream`, `std::ofstream`, `std::filesystem`, `std::string` processing, `std::optional`, `read_object` (for tag dereferencing), `find_object` (for SHA prefix lookup).

### Repository Structure (`.mygit/`)

*   **`.mygit/`**: Root directory created by `mygit init`.
*   **`objects/`**: Stores blob, tree, commit, tag objects.
    *   `objects/[0-9a-f]{2}/[0-9a-f]{38}`: Path to individual object files.
*   **`refs/`**: Stores references.
    *   `refs/heads/`: Branch pointers.
    *   `refs/tags/`: Tag pointers.
*   **`HEAD`**: Current checkout state (symbolic ref or commit SHA).
*   **`index`**: Staging area (created by first `add`).
*   **`config`**: Basic configuration.
*   **`description`**: Placeholder.
*   **`info/exclude`**: Placeholder exclude patterns.
*   **`MERGE_HEAD`**: Temporary file created during a merge conflict, stores the SHA of the commit being merged ('theirs'). Deleted by `commit` upon successful merge completion.

---

## 2. Module Breakdown

### `utils.*`

*   **Purpose:** General-purpose helper functions used across modules.
*   **Key Functions:**
    *   File I/O: `read_file`, `write_file` (overloads for string/vector), `file_exists`.
    *   Directory Handling: `ensure_directory_exists`, `ensure_parent_directory_exists`.
    *   Hashing/Compression Wrappers: `compute_sha1`, `compress_data`, `decompress_chunk`.
    *   SHA Conversion: `sha1_to_hex`, `hex_to_sha1`.
    *   File Metadata: `get_file_mode`, `set_file_executable`.
    *   Time/User Info: `get_current_timestamp_and_zone`, `get_user_info`.
    *   String Utils: `split_string`.
*   **Libraries Used:** `<filesystem>`, `<fstream>`, `<sstream>`, `<iomanip>`, `<chrono>`, `<ctime>`, `<sys/stat.h>`, `<openssl/sha.h>`, `<zlib.h>`.

### `objects.*`

*   **Purpose:** Handles reading, writing, parsing, and formatting of Git objects.
*   **Key Data Structures:** `BlobObject`, `TreeEntry`, `TreeObject`, `CommitObject`, `TagObject`, `ParsedObject` (`std::variant`).
*   **Key Functions:**
    *   `read_object()`: Reads compressed data, decompresses, parses header, calls specific `parse_*` function based on type, returns `ParsedObject`. Handles SHA prefix resolution via `find_object`.
    *   `write_object()`: Low-level write of already compressed data. High-level overload takes type/content, formats, compresses, writes.
    *   `hash_and_write_object()`: Computes **content SHA** for blobs/trees. Writes the *formatted object* (header+content) to the path derived from the **content SHA**. Returns the **content SHA**. (Handles commit/tag objects similarly, but consistency needs review - Git usually references commits/tags by their *object* SHA).
    *   `parse_blob/tree/commit/tag_content()`: Parse raw decompressed content string into specific structs. `parse_tree_content` handles the binary format.
    *   `format_tree/commit/tag_content()`: Format data from structs into the string representation needed for writing/hashing. `format_tree_content` handles sorting and binary serialization.
    *   `read_tree_contents()`: Recursively reads a tree, returns flat map `{path: sha}`.
    *   `read_tree_full()`: Recursively reads a tree, returns map `{path: TreeEntry}` (includes mode).
    *   `find_object()`: Resolves a unique object SHA from a prefix by scanning `objects/`.
*   **Libraries Used:** As per `utils.*`, plus `<variant>`, `<map>`, `<algorithm>`.

### `index.*`

*   **Purpose:** Manages the staging area (`.mygit/index`).
*   **Key Data Structures:** `IndexEntry` struct, `IndexMap` typedef.
*   **Key Functions:**
    *   `read_index()`: Reads and parses the text index file.
    *   `write_index()`: Atomically writes the in-memory map back to the file (using temp file + rename and a lock file).
    *   `add_or_update_entry()`, `remove_entry()`: Modify the map.
*   **Libraries Used:** `<fstream>`, `<map>`, `<vector>`, `<string>`, `<algorithm>`, `<filesystem>`.

### `refs.*`

*   **Purpose:** Manages references (`HEAD`, branches, tags).
*   **Key Functions:**
    *   `read_ref_direct()`: Reads content of a specific ref file.
    *   `update_ref()`: Atomically writes/updates a ref file (handles symbolic flag, expects caller to format content). Uses a lock file.
    *   `resolve_ref()`: Resolves symbolic refs, branch/tag names, or SHA prefixes to a full commit/object SHA. Handles tag dereferencing.
    *   `read_head()`, `update_head()`: Specific helpers for `HEAD`.
    *   `list_branches()`, `list_tags()`: List refs in specific directories.
    *   `get_branch_ref()`, `get_tag_ref()`: Path construction and validation.
*   **Libraries Used:** `<fstream>`, `<filesystem>`, `<optional>`, `<string>`, `<vector>`, `<set>`. Depends on `objects.cpp` for tag dereferencing/prefix resolution.

### `diff.*`

*   **Purpose:** Calculates repository status and (eventually) differences.
*   **Key Data Structures:** `FileStatus` enum, `StatusEntry` struct.
*   **Key Functions:**
    *   `get_repository_status()`: Core logic comparing HEAD, Index, and Working Directory. Uses `resolve_ref`, `read_object`, `read_tree_contents`, `read_index`, `fs::recursive_directory_iterator`, `fs::relative`, `get_workdir_sha`. Returns `std::map<std::string, StatusEntry>`.
    *   `read_tree_contents()`: (Now defined in `objects.cpp`) Helper to get flat tree content map.
    *   `get_workdir_sha()`: Helper to read and hash a workdir file.
*   **Libraries Used:** Depends on all other modules. `<set>`, `<map>`, `<filesystem>`.

### `commands.*`

*   **Purpose:** Implements the logic for each user-facing MyGit command. Orchestrates calls to functions in other modules.
*   **Key Functions:** `handle_init`, `handle_add`, `handle_rm`, `handle_commit`, `handle_status`, `handle_log`, `handle_branch`, `handle_checkout`, `handle_tag`, `handle_write_tree`, `handle_read_tree`, `handle_merge`, `handle_cat_file`, `handle_hash_object`, `handle_rev_parse`. Also includes internal helpers like `build_tree_recursive`, `get_commit_ancestors`, `find_merge_base`, `add_single_file_to_index`.
*   **Libraries Used:** Depends on all other modules. Uses standard library containers (`vector`, `map`, `set`, `queue`), streams (`iostream`, `fstream`, `sstream`), `<functional>` (for `std::function` in `read-tree` helper).

### `main.cpp`

*   **Purpose:** Parses command-line arguments and dispatches to the appropriate `handle_*` function in `commands.cpp`.
*   **Key Functions:** `main`, `print_usage`, `collect_args`. Basic argument parsing using `argc`, `argv`. Includes top-level `try...catch` block.
*   **Libraries Used:** `<iostream>`, `<string>`, `<vector>`, `<stdexcept>`. Depends on `commands.h`.

---

## 3. Command Implementation Details

*(This section details the workflow for each command, mentioning key internal function calls)*

### `mygit init`

1.  Check if `.mygit` exists (handle re-init). (`fs::exists`)
2.  Create `.mygit` and subdirectories (`fs::create_directory`, `fs::create_directories`).
3.  Write initial `HEAD`, `config`, `description`, `info/exclude` files (`std::ofstream`).
4.  Prints success message.

### `mygit add <paths>...`

1.  Read current index (`read_index`).
2.  Initialize list of paths to process from arguments.
3.  Expand `.` argument by adding contents of current directory (`fs::directory_iterator`).
4.  Iterate through paths to process:
    *   If directory, recursively scan (`fs::recursive_directory_iterator`), calculate relative paths (`fs::relative`), perform basic ignore checks, add valid files to `final_file_list`.
    *   If file, add directly to `final_file_list`.
5.  Iterate through `final_file_list`:
    *   Call `add_single_file_to_index` for each file.
        *   `add_single_file_to_index`: `read_file`, `hash_and_write_object` (creates blob if needed), `get_file_mode`, format mode string (`std::stringstream << std::oct`), `remove_entry` (stages 1-3), `add_or_update_entry` (stage 0).
6.  Write updated index (`write_index`).

### `mygit rm [--cached] <files>...`

1.  Read index (`read_index`).
2.  Parse `--cached` flag.
3.  For each file argument:
    *   Check if path exists in index map.
    *   Remove all stages for path (`remove_entry`).
    *   If *not* `--cached`, remove from working directory (`fs::remove`).
4.  Write updated index (`write_index`).

### `mygit commit -m <message>`

1.  Check message presence.
2.  Check for `MERGE_HEAD` (`file_exists`, `read_file`).
3.  Read index (`read_index`), check for conflict stages (>0). Abort if conflicts found.
4.  Gather stage 0 entries from index.
5.  Build tree object (`build_tree_recursive`), get `tree_sha1`. Handle empty index case (use known empty tree SHA).
6.  Resolve `HEAD` (`resolve_ref`) to get `head_parent_sha`.
7.  If not merging and `tree_sha1` matches parent's tree SHA (`read_object` on parent, get tree SHA), print "nothing to commit" and exit.
8.  Determine parents (`head_parent_sha`, plus `merge_head_sha` if merging).
9.  Get author/committer info (`get_user_info`, `get_current_timestamp_and_zone`).
10. Format commit content (`format_commit_content`).
11. Write commit object (`hash_and_write_object`), get `commit_sha1`.
12. Update `HEAD` ref (`update_ref`/`update_head`).
13. If merging, remove `MERGE_HEAD` (`fs::remove`).
14. Print summary.

### `mygit status`

1.  Read `HEAD` (`read_head`), determine branch name/detached state.
2.  Call `get_repository_status`.
    *   Reads HEAD commit/tree (`resolve_ref`, `read_object`, `read_tree_contents`).
    *   Reads index (`read_index`).
    *   Scans workdir (`fs::recursive_directory_iterator`, `fs::relative`).
    *   Compares HEAD vs Index vs Workdir for all paths (`get_workdir_sha`).
    *   Returns `map<string, StatusEntry>`.
3.  Check for `MERGE_HEAD` (`file_exists`).
4.  Format and print status report based on the returned map and merge status, grouping into sections, using colors.

### `mygit log [--graph]`

1.  Resolve `HEAD` (`resolve_ref`).
2.  Graph traversal (BFS using `std::queue`, track `visited` in `std::set`) starting from HEAD.
3.  For each commit: `read_object`.
4.  If `--graph`: Store parent links (`adj` map) and node labels (`node_labels` map).
5.  If not `--graph`: Print formatted commit details.
6.  Add parents to queue.
7.  If `--graph`: After traversal, list branches/tags (`list_branches`/`list_tags`), resolve them (`resolve_ref`), then print DOT output using `adj`, `node_labels`, and ref info.

### `mygit branch [<name> [<start_point>]]`

1.  **List:** If no args, `read_head`, `list_branches`, print list, marking current.
2.  **Create:** Validate name (`get_branch_ref`). Check existence (`file_exists`). Resolve `start_point` (`resolve_ref`). Verify resolved SHA is a commit (`read_object`). Create/update ref file (`update_ref`).

### `mygit checkout <branch|commit>`

1.  **Safety Check:** Call `get_repository_status`, check for staged/unstaged changes (excluding untracked) or conflicts. Abort if dirty.
2.  Resolve target (`resolve_ref`).
3.  Get target commit's tree SHA (`read_object`).
4.  Call `handle_read_tree(target_tree_sha, true, false)` to update index & workdir. Abort if it fails.
5.  Determine if target was a branch name (`read_ref_direct`, compare resolved SHAs).
6.  Update `HEAD` (symbolic ref or direct SHA) using `update_head`.
7.  Print confirmation.

### `mygit tag [-a [-m <msg>]] <name> [<object>]`

1.  **List:** If no args, `list_tags`, print names.
2.  **Create:** Parse flags. Validate name (`get_tag_ref`). Check existence (`file_exists`). Resolve `object` (`resolve_ref`). Get target object `type` (`read_object`).
3.  **Lightweight:** `update_ref` pointing tag ref to object SHA.
4.  **Annotated:** Get tagger info, `format_tag_content`, `hash_and_write_object` to create tag object, `update_ref` pointing tag ref to tag object SHA.

### `mygit write-tree`

1.  Read index (`read_index`). Check for conflicts.
2.  Gather stage 0 entries.
3.  Call `build_tree_recursive` with stage 0 entries.
4.  Print the returned root tree SHA.

### `mygit read-tree <tree-ish>`

1.  Resolve `<tree-ish>` to `tree_sha` (`resolve_ref`, check type).
2.  Build `new_index_map` by recursively reading the target tree (`read_object`, `populate_index_recursive` helper).
3.  If `update_workdir` (`-u` flag):
    *   Read `old_index_map`.
    *   Compare old vs new index maps.
    *   Delete removed files (`fs::remove`).
    *   Check out changed/added files (`read_object` for blobs, `write_file`, `ensure_parent_directory_exists`, `set_file_executable`).
4.  If not `merge_mode`, write `new_index_map` to index file (`write_index`).

### `mygit merge <branch>`

1.  Perform safety checks (`get_repository_status`, `MERGE_HEAD` check).
2.  Resolve `HEAD` and `<branch>` to `head_sha`, `theirs_sha`.
3.  Find merge base (`find_merge_base`).
4.  Check for "Already up-to-date" (`base == theirs` or `head == theirs`).
5.  Check for Fast-Forward (`base == head`):
    *   Call `handle_read_tree(theirs_tree_sha, true, false)`.
    *   If successful (returns 0), update `HEAD` ref.
    *   If fails (returns 1, e.g., `-u` fails), report error and return 1.
6.  Else (Non-Fast-Forward / True Merge):
    *   Read base, ours, theirs trees (`read_tree_full`).
    *   Perform 3-way diff path by path, populate `merge_results` map. Detect conflicts.
    *   Update `new_index`: Stage 0 for clean, Stages 1/2/3 for conflicts.
    *   Update workdir: Apply clean changes, write conflict markers.
    *   Write `new_index` (`write_index`).
    *   If conflicts: Write `theirs_sha` to `MERGE_HEAD`, return 1.
    *   If no conflicts: Call `handle_commit` with merge message (returns 0).

### Plumbing Commands

*   **`cat-file`**: `resolve_ref`, `read_object`, `std::get` on variant, format output. Needs temporary re-read for `-s` size currently.
*   **`hash-object`**: `read_file`, `compute_sha1` (for non-write blob), `hash_and_write_object` (for `-w`).
*   **`rev-parse`**: `resolve_ref`, print result.

---

## 4. Dependencies & Build

*   **Compiler:** C++17 standard required (e.g., GCC 7+, Clang 5+).
*   **Build System:** CMake (3.10+).
*   **Libraries:**
    *   OpenSSL (Crypto library for SHA-1).
    *   Zlib (Compression library).
    *   Standard Library (`<filesystem>`, `<vector>`, `<string>`, `<map>`, `<set>`, `<queue>`, `<fstream>`, `<sstream>`, etc.).
*   **Build Process:** Standard CMake out-of-source build (`cmake -S . -B build`, `cmake --build build`).

---

## 5. Known Limitations & Simplifications

*   **Merge:** True 3-way *content* merging is not implemented; only conflicts are detected and marked. Conflict resolution requires manual editing followed by `mygit add` and `mygit commit`. Directory/file conflicts and mode/type conflicts are not handled.
*   **Index:** Uses simple text format (less efficient, potential parsing fragility). Lacks full stat data caching for performance.
*   **`.gitignore`:** Not implemented.
*   **Error Handling:** Basic error handling; may not recover gracefully from all file system or object store issues. Messages might be less informative than Git's.
*   **Performance:** No packfiles, no index caching. Likely slow on large repositories or complex histories. File I/O could be optimized.
*   **Features Missing:** Remotes, submodules, rebase, stash, cherry-pick, gc, hooks, detailed diff output, full `--option` parsing for most commands, sophisticated refspecs, configuration file reading (`.gitconfig`), etc.
*   **Object Model:** MyGit currently uses content SHA for blob/tree paths but commit/tag object SHA for commit/tag paths. Git uses content SHA for blobs and tree SHA for trees, while commits/tags are referenced by their *object* SHAs. The internal references (parent pointers, tree pointers) use the appropriate target object SHAs. The implementation of `hash_and_write_object` for commits/tags needs verification against this.
*   **Checkout/Read-Tree Workdir Update:** Basic implementation exists but may lack robustness in handling complex scenarios or errors.
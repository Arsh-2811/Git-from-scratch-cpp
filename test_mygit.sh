#!/bin/bash

# Extensive Test Script for mygit

# --- Configuration ---
MYGIT_CMD="mygit" # Assuming it's in PATH or aliased after build
# Or use: MYGIT_CMD="../build/mygit" # Relative path from test repo dir
TEST_DIR="mygit_test_repo"
VERBOSE=0 # Set to 1 for more detailed command output

# --- Colors for Output ---
COLOR_GREEN='\033[0;32m'
COLOR_RED='\033[0;31m'
COLOR_YELLOW='\033[0;33m'
COLOR_BLUE='\033[0;34m'
COLOR_RESET='\033[0m'

# --- State ---
TEST_PASSED=0
TEST_FAILED=0
CURRENT_TEST=""
LAST_CMD_OUTPUT=""
LAST_CMD_STATUS=0

# --- Helper Functions ---

# Function to run a command and capture output/status
run_cmd() {
    CURRENT_TEST="$1"
    shift # Remove the test description
    echo -e "${COLOR_BLUE}RUNNING [${CURRENT_TEST}]:${COLOR_YELLOW} ${MYGIT_CMD} $@${COLOR_RESET}"
    LAST_CMD_OUTPUT=$(${MYGIT_CMD} "$@" 2>&1)
    LAST_CMD_STATUS=$?
    if [ "$VERBOSE" -eq 1 ]; then
        echo "-------------------- Output ---------------------"
        echo "$LAST_CMD_OUTPUT"
        echo "-------------------- Status: $LAST_CMD_STATUS --------------"
    fi
}

# Check the exit status of the last command
check_status() {
    local expected_status=$1
    if [ $LAST_CMD_STATUS -eq $expected_status ]; then
        echo -e "  ${COLOR_GREEN}PASS:${COLOR_RESET} [${CURRENT_TEST}] - Expected status $expected_status"
        TEST_PASSED=$((TEST_PASSED + 1))
    else
        echo -e "  ${COLOR_RED}FAIL:${COLOR_RESET} [${CURRENT_TEST}] - Expected status $expected_status, got $LAST_CMD_STATUS"
        echo "       Output:"
        echo "$LAST_CMD_OUTPUT" | sed 's/^/       | /' # Indent output
        TEST_FAILED=$((TEST_FAILED + 1))
    fi
}

# Check if the last command's output contains a string
check_output_contains() {
    local expected_string="$1"
    # Use grep -q for quiet check
    if echo "$LAST_CMD_OUTPUT" | grep -qF -- "$expected_string"; then
        echo -e "  ${COLOR_GREEN}PASS:${COLOR_RESET} [${CURRENT_TEST}] - Output contains '$expected_string'"
        TEST_PASSED=$((TEST_PASSED + 1))
    else
        echo -e "  ${COLOR_RED}FAIL:${COLOR_RESET} [${CURRENT_TEST}] - Output does NOT contain '$expected_string'"
        echo "       Output:"
        echo "$LAST_CMD_OUTPUT" | sed 's/^/       | /'
        TEST_FAILED=$((TEST_FAILED + 1))
    fi
}

# Check if the last command's output DOES NOT contain a string
check_output_not_contains() {
    local unexpected_string="$1"
     # Use grep -q for quiet check
    if ! echo "$LAST_CMD_OUTPUT" | grep -qF -- "$unexpected_string"; then
        echo -e "  ${COLOR_GREEN}PASS:${COLOR_RESET} [${CURRENT_TEST}] - Output does not contain '$unexpected_string'"
        TEST_PASSED=$((TEST_PASSED + 1))
    else
        echo -e "  ${COLOR_RED}FAIL:${COLOR_RESET} [${CURRENT_TEST}] - Output CONTAINS '$unexpected_string'"
        echo "       Output:"
        echo "$LAST_CMD_OUTPUT" | sed 's/^/       | /'
        TEST_FAILED=$((TEST_FAILED + 1))
    fi
}

# Check if a file exists
check_file_exists() {
    local filepath="$1"
    if [ -e "$filepath" ]; then
        echo -e "  ${COLOR_GREEN}PASS:${COLOR_RESET} [${CURRENT_TEST}] - File '$filepath' exists"
        TEST_PASSED=$((TEST_PASSED + 1))
    else
        echo -e "  ${COLOR_RED}FAIL:${COLOR_RESET} [${CURRENT_TEST}] - File '$filepath' does NOT exist"
        TEST_FAILED=$((TEST_FAILED + 1))
    fi
}

# Check if a file does NOT exist
check_file_not_exists() {
    local filepath="$1"
    if [ ! -e "$filepath" ]; then
        echo -e "  ${COLOR_GREEN}PASS:${COLOR_RESET} [${CURRENT_TEST}] - File '$filepath' does not exist"
        TEST_PASSED=$((TEST_PASSED + 1))
    else
        echo -e "  ${COLOR_RED}FAIL:${COLOR_RESET} [${CURRENT_TEST}] - File '$filepath' EXISTS unexpectedly"
        TEST_FAILED=$((TEST_FAILED + 1))
    fi
}

# Check if a file contains a specific string
check_file_contains() {
    local filepath="$1"
    local expected_string="$2"
    if [ ! -f "$filepath" ]; then
         echo -e "  ${COLOR_RED}FAIL:${COLOR_RESET} [${CURRENT_TEST}] - Cannot check content, file '$filepath' does not exist"
         TEST_FAILED=$((TEST_FAILED + 1))
         return
    fi
    if grep -qF -- "$expected_string" "$filepath"; then
        echo -e "  ${COLOR_GREEN}PASS:${COLOR_RESET} [${CURRENT_TEST}] - File '$filepath' contains '$expected_string'"
        TEST_PASSED=$((TEST_PASSED + 1))
    else
        echo -e "  ${COLOR_RED}FAIL:${COLOR_RESET} [${CURRENT_TEST}] - File '$filepath' does NOT contain '$expected_string'"
        TEST_FAILED=$((TEST_FAILED + 1))
    fi
}

# Get FULL SHA from commit output
get_commit_full_sha() {
    # ... (as before) ...
     local sha=""
    sha=$(echo "$LAST_CMD_OUTPUT" | grep -oE '\[.*\s([a-f0-9]{40})\]' | sed -E 's/.* ([a-f0-9]{40})\]/\1/')
     if [ -z "$sha" ]; then
       sha=$(echo "$LAST_CMD_OUTPUT" | grep -oE '^\[.*\(.*\)\s+([a-f0-9]{40})\]|^\[.*\s([a-f0-9]{40})\]' | sed -E 's/^.* ([a-f0-9]{40})\]$/\1/')
     fi
     echo "$sha" | head -n 1
}

# Get FULL SHA from write-tree output
get_tree_full_sha() {
     echo "$LAST_CMD_OUTPUT" | grep -oE '^[a-f0-9]{40}$' | head -n 1
}

# Exit script if SHA capture fails
check_sha_captured() {
    local sha_var_name="$1"
    local sha_value="${!sha_var_name}" # Indirect variable reference
    if [ -z "$sha_value" ]; then
        echo -e "${COLOR_RED}FATAL: [${CURRENT_TEST}] - Failed to capture SHA for $sha_var_name. Exiting.${COLOR_RESET}"
        echo "Output was:"
        echo "$LAST_CMD_OUTPUT"
        exit 1
    fi
}

# Get Tree SHA from a Commit SHA using cat-file (Add cat-file back to mygit first!)
# OR simulate by reading commit object manually if needed for tests
get_tree_sha_from_commit() {
    local commit_sha="$1"
    if [ -z "$commit_sha" ]; then return 1; fi
    # Use cat-file command
    local output
    output=$(${MYGIT_CMD} cat-file -p "$commit_sha" 2>/dev/null)
    local status=$?
    if [ $status -ne 0 ]; then
        echo "ERROR: cat-file failed for $commit_sha" >&2 # Print error to stderr
        return 1
    fi
    echo "$output" | grep '^tree ' | sed -E 's/tree // '
}


# Helper to simulate checkout by updating HEAD and index
# Usage: simulate_checkout <branch_name_or_sha>
simulate_checkout() {
    local target="$1"
    local target_sha
    local target_tree_sha
    local target_ref_path

    echo "Simulating checkout $target..."

    # Resolve target to SHA using rev-parse
    run_cmd "resolve $target for checkout" rev-parse "$target"
    # Allow rev-parse to fail if target doesn't exist, handle it
    if [ $LAST_CMD_STATUS -ne 0 ]; then
        echo "ERROR: Failed to resolve '$target' using rev-parse during checkout simulation"
        exit 1
    fi
    target_sha="$LAST_CMD_OUTPUT"
    check_sha_captured "target_sha" # Exit if empty

    # Check if target is a branch name by checking ref existence
    target_ref_path=".mygit/refs/heads/$target"
    if [ -f "$target_ref_path" ]; then
        echo "ref: refs/heads/$target" > .mygit/HEAD
        echo "  (Updating HEAD to branch '$target')"
    else
        echo "$target_sha" > .mygit/HEAD # Detached HEAD
        echo "  (Updating HEAD to detached SHA '$target_sha')"
    fi

    # Get tree SHA for the target commit using updated helper
    target_tree_sha=$(get_tree_sha_from_commit "$target_sha")
     if [ -z "$target_tree_sha" ]; then
        echo "ERROR: Failed to get tree SHA for commit $target_sha during checkout simulation"
        exit 1
     fi
     echo "  (Target tree SHA: $target_tree_sha)"

    # Update index using read-tree (no workdir update)
    run_cmd "checkout: read-tree $target" read-tree "$target_tree_sha"
    check_status 0 # Expect success for index update
    echo "Checkout simulation complete (HEAD and index updated)."
}

# --- Cleanup Function ---
cleanup() {
    echo -e "\n${COLOR_YELLOW}Cleaning up test directory...${COLOR_RESET}"
    if [ "$(pwd)" = "$PWD_START/$TEST_DIR" ]; then
      cd .. > /dev/null 2>&1 # Avoid printing cd output
    fi
    rm -rf "$TEST_DIR"
}

# --- Trap for cleanup on exit ---
PWD_START=$(pwd) # Store starting directory
trap cleanup EXIT

# --- START TESTS ---

echo -e "${COLOR_YELLOW}===================================${COLOR_RESET}"
echo -e "${COLOR_YELLOW}       Starting mygit Tests        ${COLOR_RESET}"
echo -e "${COLOR_YELLOW}===================================${COLOR_RESET}"

# Setup test directory
if [ -d "$TEST_DIR" ]; then
    echo "Removing previous test directory."
    rm -rf "$TEST_DIR"
fi

mkdir "$TEST_DIR"
cd "$TEST_DIR" || exit 1
echo "Created test directory: $(pwd)"

# --- Test: init ---
echo -e "\n${COLOR_YELLOW}--- Testing: init ---${COLOR_RESET}"
run_cmd "init: First time" init
check_status 0
check_output_contains "Initialized empty Git repository"
check_file_exists ".mygit/HEAD"
check_file_exists ".mygit/config"
check_file_exists ".mygit/objects"
check_file_exists ".mygit/refs"
check_file_contains ".mygit/HEAD" "ref: refs/heads/main"

run_cmd "init: Re-initialize" init
check_status 0
check_output_contains "Reinitialized existing Git repository"


# --- Test: status (empty repo) ---
echo -e "\n${COLOR_YELLOW}--- Testing: status (empty repo) ---${COLOR_RESET}"
run_cmd "status: Empty repo" status
check_status 0
check_output_contains "On branch main"
check_output_contains "nothing to commit"
check_output_not_contains "Changes to be committed"
check_output_not_contains "Untracked files"


# --- Test: add, status, commit (basic workflow) ---
# (Keep this section as it was, it passed correctly)
echo -e "\n${COLOR_YELLOW}--- Testing: add, status, commit (basic workflow) ---${COLOR_RESET}"
echo "Creating file1.txt"
echo "Hello Git!" > file1.txt
run_cmd "status: Untracked file" status; check_status 0; check_output_contains "Untracked files:"; check_output_contains "file1.txt"
run_cmd "add: New file" add file1.txt; check_status 0; check_file_exists ".mygit/index"
run_cmd "status: Staged new file" status; check_status 0; check_output_contains "Changes to be committed:"; check_output_contains "new file:   file1.txt"; check_output_not_contains "Untracked files:"
run_cmd "commit: Initial commit" commit -m "Initial commit: Add file1.txt"; check_status 0; check_output_contains "Initial commit: Add file1.txt"
COMMIT1_SHA=$(get_commit_full_sha); check_sha_captured "COMMIT1_SHA"; echo "    -> Commit 1 SHA: $COMMIT1_SHA"
run_cmd "status: After first commit" status; check_status 0; check_output_contains "nothing to commit, working tree clean"
echo "Modifying file1.txt"; echo "Hello again!" >> file1.txt
run_cmd "status: Modified workdir" status; check_status 0; check_output_contains "Changes not staged for commit:"; check_output_contains "modified:   file1.txt"; check_output_not_contains "Changes to be committed:"
run_cmd "add: Modified file" add file1.txt; check_status 0
run_cmd "status: Staged modified file" status; check_status 0; check_output_contains "Changes to be committed:"; check_output_contains "modified:   file1.txt"; check_output_not_contains "Changes not staged for commit:"
echo "Creating file2.txt"; echo "Second file" > file2.txt
run_cmd "add: Second new file" add file2.txt; check_status 0
run_cmd "status: Multiple staged changes" status; check_status 0; check_output_contains "Changes to be committed:"; check_output_contains "modified:   file1.txt"; check_output_contains "new file:   file2.txt"
run_cmd "commit: Second commit" commit -m "Second commit: Modify file1, add file2"; check_status 0; check_output_contains "Second commit: Modify file1, add file2"
COMMIT2_SHA=$(get_commit_full_sha); check_sha_captured "COMMIT2_SHA"; echo "    -> Commit 2 SHA: $COMMIT2_SHA"
run_cmd "status: Clean after second commit" status; check_status 0; check_output_contains "nothing to commit, working tree clean"


# --- Test: log ---
# (Keep this section as it was)
echo -e "\n${COLOR_YELLOW}--- Testing: log ---${COLOR_RESET}"
run_cmd "log: Basic" log; check_status 0; check_output_contains "commit ${COMMIT2_SHA}"; check_output_contains "Second commit: Modify file1, add file2"; check_output_contains "commit ${COMMIT1_SHA}"; check_output_contains "Initial commit: Add file1.txt"
run_cmd "log: Graph" log --graph; check_status 0; check_output_contains "digraph git_log {"; check_output_contains "\"${COMMIT2_SHA}\""; check_output_contains "\"${COMMIT1_SHA}\""; check_output_contains "\"${COMMIT2_SHA}\" -> \"${COMMIT1_SHA}\""; check_output_contains "\"branch_main\""; check_output_contains "\"HEAD\""; echo "    -> Graph output generated. Use Graphviz 'dot' command for visual check."


# --- Test: rm ---
# (Keep this section as it was)
echo -e "\n${COLOR_YELLOW}--- Testing: rm ---${COLOR_RESET}"
run_cmd "rm: Tracked file" rm file2.txt; check_status 0; check_file_not_exists "file2.txt"
run_cmd "status: After rm" status; check_status 0; check_output_contains "Changes to be committed:"; check_output_contains "deleted:    file2.txt"
run_cmd "commit: Remove file2" commit -m "Remove file2.txt"; check_status 0
COMMIT3_SHA=$(get_commit_full_sha); check_sha_captured "COMMIT3_SHA"; echo "    -> Commit 3 SHA: $COMMIT3_SHA"
echo "Recreating file2.txt"; echo "Second file again" > file2.txt
run_cmd "add: Re-add file2" add file2.txt; check_status 0
run_cmd "commit: Re-add file2" commit -m "Re-add file2.txt"; check_status 0
COMMIT4_SHA=$(get_commit_full_sha); check_sha_captured "COMMIT4_SHA"; echo "    -> Commit 4 SHA: $COMMIT4_SHA"
run_cmd "rm: Cached" rm --cached file1.txt; check_status 0; check_file_exists "file1.txt"
run_cmd "status: After rm --cached" status; check_status 0; check_output_contains "Changes to be committed:"; check_output_contains "deleted:    file1.txt"; check_output_contains "Untracked files:"; check_output_contains "file1.txt"
run_cmd "commit: Remove file1 from index" commit -m "Remove file1.txt from index"; check_status 0
COMMIT5_SHA=$(get_commit_full_sha); check_sha_captured "COMMIT5_SHA"; echo "    -> Commit 5 SHA: $COMMIT5_SHA"
run_cmd "status: After rm --cached commit" status; check_status 0; check_output_contains "Untracked files:"; check_output_contains "file1.txt"; check_output_not_contains "Changes to be committed:"


# --- Test: branch ---
# (Keep this section as it was)
echo -e "\n${COLOR_YELLOW}--- Testing: branch ---${COLOR_RESET}"
run_cmd "add: Restore file1 for branching" add file1.txt; check_status 0
run_cmd "commit: Restore file1" commit -m "Restore file1"; check_status 0
COMMIT6_SHA=$(get_commit_full_sha); check_sha_captured "COMMIT6_SHA"; echo "    -> Commit 6 SHA: $COMMIT6_SHA"
run_cmd "branch: List initial" branch; check_status 0; check_output_contains "* main"; check_output_not_contains "feature1"
run_cmd "branch: Create feature1" branch feature1; check_status 0; check_file_exists ".mygit/refs/heads/feature1"
run_cmd "branch: List after create" branch; check_status 0; check_output_contains "* main"; check_output_contains "  feature1"
run_cmd "branch: Create feature2 from specific commit" branch feature2 $COMMIT4_SHA; check_status 0; check_file_exists ".mygit/refs/heads/feature2"
run_cmd "branch: List after specific create" branch; check_status 0; check_output_contains "* main"; check_output_contains "  feature1"; check_output_contains "  feature2"
run_cmd "branch: Invalid name" branch "bad/name"; check_status 1; check_output_contains "is not a valid branch name"
run_cmd "branch: Duplicate name" branch main; check_status 1; check_output_contains "branch named 'main' already exists"


# --- Test: tag ---
# (Keep this section as it was)
echo -e "\n${COLOR_YELLOW}--- Testing: tag ---${COLOR_RESET}"
run_cmd "tag: List empty" tag; check_status 0; check_output_not_contains "v1.0"
run_cmd "tag: Create lightweight v1.0" tag v1.0; check_status 0; check_file_exists ".mygit/refs/tags/v1.0"
run_cmd "tag: List after lightweight" tag; check_status 0; check_output_contains "v1.0"
run_cmd "tag: Create lightweight v0.1 on Commit 1" tag v0.1 $COMMIT1_SHA; check_status 0; check_file_exists ".mygit/refs/tags/v0.1"
run_cmd "tag: List after specific lightweight" tag; check_status 0; check_output_contains "v0.1"; check_output_contains "v1.0"
run_cmd "tag: Create annotated v1.1" tag -a -m "Release version 1.1" v1.1; check_status 0; check_file_exists ".mygit/refs/tags/v1.1"
run_cmd "tag: List after annotated" tag; check_status 0; check_output_contains "v0.1"; check_output_contains "v1.0"; check_output_contains "v1.1"
run_cmd "tag: Duplicate name" tag v1.0; check_status 1; check_output_contains "tag 'v1.0' already exists"
run_cmd "tag: Annotated no message" tag -a v1.2; check_status 1; check_output_contains "Annotated tags require a message"


# --- Test: write-tree / read-tree ---
# (Keep this section as it was)
echo -e "\n${COLOR_YELLOW}--- Testing: write-tree / read-tree ---${COLOR_RESET}"
run_cmd "status: Before write-tree" status; check_status 0; check_output_contains "nothing to commit"
run_cmd "write-tree: Current index" write-tree; check_status 0
TREE1_SHA=$(get_tree_full_sha); check_sha_captured "TREE1_SHA"; echo "    -> Tree 1 SHA: $TREE1_SHA"
echo "Creating file3.txt and adding"; echo "File three" > file3.txt
run_cmd "add: file3 for read-tree test" add file3.txt; check_status 0
run_cmd "status: Before read-tree" status; check_status 0; check_output_contains "new file:   file3.txt"
run_cmd "read-tree: Overwrite index with Tree 1" read-tree $TREE1_SHA; check_status 0
run_cmd "status: After read-tree (index updated)" status; check_status 0; check_output_not_contains "Changes to be committed:"; check_output_contains "Untracked files:"; check_output_contains "file3.txt"
rm file3.txt


# --- Test: merge ---
echo -e "\n${COLOR_YELLOW}--- Testing: merge ---${COLOR_RESET}"
# Current state: main is at COMMIT6
run_cmd "status: State before merge tests" status
check_status 0
check_output_contains "nothing to commit"

# Test 1: Merge branch pointing to same commit
run_cmd "merge: Same commit" merge main
check_status 0
check_output_contains "Already up to date."

# Test 2: Merge branch pointing to ancestor (Already up-to-date)
# feature2 points to COMMIT4, main points to COMMIT6. C4 is ancestor of C6.
run_cmd "merge: Ancestor commit" merge feature2
check_status 0
check_output_contains "Already up to date."

# Test 3: Fast-forward scenario
# Setup: main at COMMIT6. Create feature_ff at COMMIT6. Make commit on feature_ff.
run_cmd "branch: Create FF branch" branch feature_ff $COMMIT6_SHA; check_status 0
simulate_checkout feature_ff # Simulate checkout
echo "FF commit content" > ff_file.txt
run_cmd "add: ff_file" add ff_file.txt; check_status 0
run_cmd "commit: Commit on FF branch" commit -m "FF commit"; check_status 0
COMMIT7_SHA=$(get_commit_full_sha); check_sha_captured "COMMIT7_SHA"; echo "    -> Commit 7 (on feature_ff) SHA: $COMMIT7_SHA"
simulate_checkout main # Simulate checkout back
# Now main is at COMMIT6, feature_ff is ahead at COMMIT7. Base is COMMIT6.

# *** ADJUST EXPECTATIONS FOR FF MERGE ***
run_cmd "merge: Fast-forward" merge feature_ff # Renamed test slightly
check_status 0                                  # Expect SUCCESS (status 0)
check_output_contains "Fast-forward"            # Should still detect FF
check_output_contains "Fast-forward merge successful" # Check for success message
check_output_not_contains "Error updating index/workdir" # Should NOT contain error
# Add checks for HEAD update?
check_file_contains ".mygit/HEAD" "ref: refs/heads/main" # Check HEAD still points to main symbolically
run_cmd "resolve main after ff merge" rev-parse main # Check where main points now
check_status 0
check_output_contains "$COMMIT7_SHA" # Main should now point to C7


# --- Test 4: Non-fast-forward scenario ---
# Reset state: main at C6, feature2 at C4
echo "Resetting main to C6..."
# *** USE simulate_checkout ***
simulate_checkout main # Should already be on main, this just ensures index matches C6
# Make commit C8 on main (parent C6)
echo "Main diverge content" > main_diverge.txt
run_cmd "add: main_diverge" add main_diverge.txt; check_status 0
run_cmd "commit: Commit on main (diverging)" commit -m "Diverging commit on main"
check_status 0
COMMIT8_SHA=$(get_commit_full_sha)
check_sha_captured "COMMIT8_SHA"; echo "    -> Commit 8 (on main) SHA: $COMMIT8_SHA"
# Make commit C9 on feature2 (parent C4)
# *** USE simulate_checkout ***
simulate_checkout feature2 # Update HEAD and index to feature2 (C4)
echo "Feature2 diverge content" > feature2_change.txt
run_cmd "add: feature2 change" add feature2_change.txt; check_status 0
run_cmd "commit: Commit on feature2 (diverging)" commit -m "Diverging commit on feature2"
check_status 0
COMMIT9_SHA=$(get_commit_full_sha)
check_sha_captured "COMMIT9_SHA"; echo "    -> Commit 9 (on feature2) SHA: $COMMIT9_SHA"
# *** USE simulate_checkout ***
simulate_checkout main # Update HEAD and index to C8 state
# Now main is at C8, feature2 is at C9. Base should be C4.
run_cmd "merge: Non-fast-forward (expect fail)" merge feature2
check_status 1
check_output_contains "True merge logic is not implemented"


# --- Final Summary ---
echo -e "\n${COLOR_YELLOW}===================================${COLOR_RESET}"
echo -e "${COLOR_YELLOW}         Test Summary              ${COLOR_RESET}"
echo -e "${COLOR_YELLOW}===================================${COLOR_RESET}"
echo -e "Tests Passed: ${COLOR_GREEN}${TEST_PASSED}${COLOR_RESET}"
echo -e "Tests Failed: ${COLOR_RED}${TEST_FAILED}${COLOR_RESET}"

# Exit with error code if any tests failed
if [ $TEST_FAILED -gt 0 ]; then
    exit 1
else
    exit 0
fi
"
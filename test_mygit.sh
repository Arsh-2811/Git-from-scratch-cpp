#!/bin/bash

# Extensive Test Script for mygit

# --- Configuration ---
MYGIT_CMD="mygit" # Adjust path if needed
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
    # Run command, redirecting stderr to stdout to capture all output
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

# Get SHA from commit output (naive parsing)
get_commit_sha() {
    echo "$LAST_CMD_OUTPUT" | grep -oE '[a-f0-9]{7}' | head -n 1
}

# Get SHA from write-tree output
get_tree_sha() {
     echo "$LAST_CMD_OUTPUT" | grep -oE '^[a-f0-9]{40}$'
}


# --- Cleanup Function ---
cleanup() {
    echo -e "\n${COLOR_YELLOW}Cleaning up test directory...${COLOR_RESET}"
    rm -rf "$TEST_DIR"
}

# --- Trap for cleanup on exit ---
trap cleanup EXIT

# --- START TESTS ---

echo -e "${COLOR_YELLOW}===================================${COLOR_RESET}"
echo -e "${COLOR_YELLOW}       Starting mygit Tests        ${COLOR_RESET}"
echo -e "${COLOR_YELLOW}===================================${COLOR_RESET}"

# Setup test directory
rm -rf "$TEST_DIR"
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
echo -e "\n${COLOR_YELLOW}--- Testing: add, status, commit (basic workflow) ---${COLOR_RESET}"
echo "Creating file1.txt"
echo "Hello Git!" > file1.txt

run_cmd "status: Untracked file" status
check_status 0
check_output_contains "Untracked files:"
check_output_contains "file1.txt"

run_cmd "add: New file" add file1.txt
check_status 0
check_file_exists ".mygit/index" # Index should be created now

run_cmd "status: Staged new file" status
check_status 0
check_output_contains "Changes to be committed:"
check_output_contains "new file:   file1.txt"
check_output_not_contains "Untracked files:"

run_cmd "commit: Initial commit" commit -m "Initial commit: Add file1.txt"
check_status 0
check_output_contains "Initial commit: Add file1.txt"
COMMIT1_SHA=$(get_commit_sha)
echo "    -> Commit 1 SHA (short): $COMMIT1_SHA"

run_cmd "status: After first commit" status
check_status 0
check_output_contains "nothing to commit, working tree clean"

echo "Modifying file1.txt"
echo "Hello again!" >> file1.txt

run_cmd "status: Modified workdir" status
check_status 0
check_output_contains "Changes not staged for commit:"
check_output_contains "modified:   file1.txt"
check_output_not_contains "Changes to be committed:"

run_cmd "add: Modified file" add file1.txt
check_status 0

run_cmd "status: Staged modified file" status
check_status 0
check_output_contains "Changes to be committed:"
check_output_contains "modified:   file1.txt"
check_output_not_contains "Changes not staged for commit:"

echo "Creating file2.txt"
echo "Second file" > file2.txt

run_cmd "add: Second new file" add file2.txt
check_status 0

run_cmd "status: Multiple staged changes" status
check_status 0
check_output_contains "Changes to be committed:"
check_output_contains "modified:   file1.txt"
check_output_contains "new file:   file2.txt"

run_cmd "commit: Second commit" commit -m "Second commit: Modify file1, add file2"
check_status 0
check_output_contains "Second commit: Modify file1, add file2"
COMMIT2_SHA=$(get_commit_sha)
echo "    -> Commit 2 SHA (short): $COMMIT2_SHA"

run_cmd "status: Clean after second commit" status
check_status 0
check_output_contains "nothing to commit, working tree clean"


# --- Test: log ---
echo -e "\n${COLOR_YELLOW}--- Testing: log ---${COLOR_RESET}"
run_cmd "log: Basic" log
check_status 0
check_output_contains "commit ${COMMIT2_SHA}" # Check for short SHA fragment
check_output_contains "Second commit: Modify file1, add file2"
check_output_contains "commit ${COMMIT1_SHA}"
check_output_contains "Initial commit: Add file1.txt"

run_cmd "log: Graph" log --graph
check_status 0
check_output_contains "digraph git_log {"
check_output_contains "\"${COMMIT2_SHA}" # Check node exists (using short sha)
check_output_contains "\"${COMMIT1_SHA}"
check_output_contains "-> \"${COMMIT1_SHA}" # Check edge exists (using short sha)
check_output_contains "\"branch_main\""
check_output_contains "\"HEAD\""
echo "    -> Graph output generated. Use Graphviz 'dot' command for visual check."


# --- Test: rm ---
echo -e "\n${COLOR_YELLOW}--- Testing: rm ---${COLOR_RESET}"
run_cmd "rm: Tracked file" rm file2.txt
check_status 0
check_file_not_exists "file2.txt"

run_cmd "status: After rm" status
check_status 0
check_output_contains "Changes to be committed:"
check_output_contains "deleted:    file2.txt"

run_cmd "commit: Remove file2" commit -m "Remove file2.txt"
check_status 0
COMMIT3_SHA=$(get_commit_sha)
echo "    -> Commit 3 SHA (short): $COMMIT3_SHA"

echo "Recreating file2.txt"
echo "Second file again" > file2.txt
run_cmd "add: Re-add file2" add file2.txt
check_status 0
run_cmd "commit: Re-add file2" commit -m "Re-add file2.txt"
check_status 0
COMMIT4_SHA=$(get_commit_sha)
echo "    -> Commit 4 SHA (short): $COMMIT4_SHA"

run_cmd "rm: Cached" rm --cached file1.txt
check_status 0
check_file_exists "file1.txt" # File should still be in workdir

run_cmd "status: After rm --cached" status
check_status 0
check_output_contains "Changes to be committed:"
check_output_contains "deleted:    file1.txt"
check_output_contains "Untracked files:" # Original file1 is now untracked
check_output_contains "file1.txt"

run_cmd "commit: Remove file1 from index" commit -m "Remove file1.txt from index"
check_status 0
COMMIT5_SHA=$(get_commit_sha)
echo "    -> Commit 5 SHA (short): $COMMIT5_SHA"

run_cmd "status: After rm --cached commit" status
check_status 0
check_output_contains "Untracked files:"
check_output_contains "file1.txt"
check_output_not_contains "Changes to be committed:"


# --- Test: branch ---
echo -e "\n${COLOR_YELLOW}--- Testing: branch ---${COLOR_RESET}"
# Restore file1 to tracked state for branching tests
run_cmd "add: Restore file1 for branching" add file1.txt
check_status 0
run_cmd "commit: Restore file1" commit -m "Restore file1"
check_status 0
COMMIT6_SHA=$(get_commit_sha)
echo "    -> Commit 6 SHA (short): $COMMIT6_SHA"

run_cmd "branch: List initial" branch
check_status 0
check_output_contains "* main"
check_output_not_contains "feature1"

run_cmd "branch: Create feature1" branch feature1
check_status 0

run_cmd "branch: List after create" branch
check_status 0
check_output_contains "* main"
check_output_contains "  feature1"

# Create branch from older commit (Commit 4)
run_cmd "branch: Create feature2 from specific commit" branch feature2 $COMMIT4_SHA
check_status 0

run_cmd "branch: List after specific create" branch
check_status 0
check_output_contains "* main"
check_output_contains "  feature1"
check_output_contains "  feature2"

# Test invalid branch name
run_cmd "branch: Invalid name" branch "bad/name"
check_status 1
check_output_contains "is not a valid branch name"

# Test duplicate branch name
run_cmd "branch: Duplicate name" branch main
check_status 1
check_output_contains "branch named 'main' already exists"


# --- Test: tag ---
echo -e "\n${COLOR_YELLOW}--- Testing: tag ---${COLOR_RESET}"
run_cmd "tag: List empty" tag
check_status 0
check_output_not_contains "v1.0" # Check output is empty or contains nothing relevant

run_cmd "tag: Create lightweight v1.0" tag v1.0
check_status 0

run_cmd "tag: List after lightweight" tag
check_status 0
check_output_contains "v1.0"

# Tag an older commit
run_cmd "tag: Create lightweight v0.1 on Commit 1" tag v0.1 $COMMIT1_SHA
check_status 0

run_cmd "tag: List after specific lightweight" tag
check_status 0
check_output_contains "v0.1"
check_output_contains "v1.0"

run_cmd "tag: Create annotated v1.1" tag -a -m "Release version 1.1" v1.1
check_status 0

run_cmd "tag: List after annotated" tag
check_status 0
check_output_contains "v0.1"
check_output_contains "v1.0"
check_output_contains "v1.1"

# Test duplicate tag
run_cmd "tag: Duplicate name" tag v1.0
check_status 1
check_output_contains "tag 'v1.0' already exists"

# Test annotated tag requires message
run_cmd "tag: Annotated no message" tag -a v1.2
check_status 1
check_output_contains "Annotated tags require a message"


# --- Test: write-tree / read-tree ---
echo -e "\n${COLOR_YELLOW}--- Testing: write-tree / read-tree ---${COLOR_RESET}"
# Ensure index matches COMMIT6
run_cmd "status: Before write-tree" status
check_status 0
check_output_contains "nothing to commit"

run_cmd "write-tree: Current index" write-tree
check_status 0
TREE1_SHA=$(get_tree_sha)
echo "    -> Tree 1 SHA: $TREE1_SHA"
if [ -z "$TREE1_SHA" ]; then
     echo -e "  ${COLOR_RED}FAIL:${COLOR_RESET} [${CURRENT_TEST}] - Failed to capture tree SHA"
     TEST_FAILED=$((TEST_FAILED + 1))
fi

echo "Creating file3.txt and adding"
echo "File three" > file3.txt
run_cmd "add: file3 for read-tree test" add file3.txt
check_status 0

run_cmd "status: Before read-tree" status
check_status 0
check_output_contains "new file:   file3.txt"

# Read the previous tree back into the index (overwrite mode)
run_cmd "read-tree: Overwrite index with Tree 1" read-tree $TREE1_SHA
check_status 0 # Expect success even though -u isn't implemented

run_cmd "status: After read-tree (index updated)" status
check_status 0
check_output_not_contains "Changes to be committed:" # Index should match TREE1
check_output_contains "Untracked files:" # file3.txt was added but index reset
check_output_contains "file3.txt"

# Cleanup file3
rm file3.txt


# --- Test: merge (Fast Forward detection) ---
echo -e "\n${COLOR_YELLOW}--- Testing: merge (Fast Forward) ---${COLOR_RESET}"
# Setup: main is at COMMIT6. Create 'feature' branch pointing to COMMIT6. Make commit on feature.
run_cmd "branch: Create feature_ff" branch feature_ff $COMMIT6_SHA
check_status 0

# Simulate checkout and commit on feature_ff by directly creating ref
echo "Creating commit directly on feature_ff (simulated)"
echo "Feature FF change" > feature_ff_file.txt
run_cmd "add: feature_ff_file" add feature_ff_file.txt
check_status 0
run_cmd "commit: Commit on feature_ff" commit -m "Commit on feature_ff"
check_status 0
COMMIT7_SHA=$(get_commit_sha) # This commit is on 'main' currently
echo "    -> Commit 7 SHA (short): $COMMIT7_SHA"
# Manually update feature_ff branch to point to this new commit
run_cmd "branch: Update feature_ff pointer manually" \
    update-ref refs/heads/feature_ff $COMMIT7_SHA # Assuming we add an update-ref command or do this manually
# Since update-ref isn't a command, let's just test the merge logic expecting failure
# Reset main back to COMMIT6 to simulate being "behind" feature_ff
echo "Manually resetting main to $COMMIT6_SHA (requires update-ref or manual edit)"
${MYGIT_CMD} update-ref refs/heads/main $COMMIT6_SHA # NEED update-ref command for proper test
# Or maybe just proceed, assuming merge detects base correctly even if main moved

# Now main is at COMMIT7, feature_ff points to COMMIT6. Merge feature_ff -> Already up to date
run_cmd "merge: Already up-to-date" merge feature_ff
check_status 0
check_output_contains "Already up to date."

# Let's try the other way (requires modifying refs, hard in script without checkout/update-ref)
# Instead, let's just check if merging a *future* commit triggers FF *detection*
# Setup: main is at COMMIT7. Create branch 'future' pointing to COMMIT7. Commit again on main.
run_cmd "branch: Create future branch" branch future $COMMIT7_SHA
check_status 0
echo "Commit 8" > file8.txt
run_cmd "add: file8" add file8.txt
check_status 0
run_cmd "commit: Commit 8" commit -m "Commit 8"
check_status 0
COMMIT8_SHA=$(get_commit_sha)
echo "    -> Commit 8 SHA (short): $COMMIT8_SHA"
# Now main is at COMMIT8, future is at COMMIT7. Merge future -> already up to date
run_cmd "merge: Already up-to-date (2)" merge future
check_status 0
check_output_contains "Already up to date."


# Test FF detection where target branch is ahead (expects failure due to read-tree -u missing)
# Reset main to Commit 7, merge Commit 8 (which is ahead)
# echo "Manually resetting main to $COMMIT7_SHA"
# ./mygit update-ref refs/heads/main $COMMIT7_SHA # NEED update-ref
# run_cmd "merge: Fast-forward (expect fail on -u)" merge $COMMIT8_SHA
# check_status 1 # Should fail because read-tree -u fails
# check_output_contains "Fast-forward"
# check_output_contains "Error updating index/workdir"

# Due to lack of checkout/update-ref, proper FF testing is hard.
echo "    -> Skipping detailed Fast-Forward merge update test due to missing commands (checkout/update-ref)."


# --- Test: merge (Non-Fast Forward detection) ---
echo -e "\n${COLOR_YELLOW}--- Testing: merge (Non-Fast Forward) ---${COLOR_RESET}"
# Setup:
# 1. Current state: main is at COMMIT8
# 2. Create branch 'topic' pointing to COMMIT6
run_cmd "branch: Create topic branch" branch topic $COMMIT6_SHA
check_status 0
# 3. Make a commit on 'main' (already done - COMMIT8)
# 4. Make a commit on 'topic' (simulate by manually creating commit obj & updating ref - too complex for script)
# Simplified: Just try merging 'topic' into 'main'. Base is COMMIT6, main is COMMIT8, topic is COMMIT6.
# This merge should technically be FF main->topic, but let's see what happens.
# Let's try merging COMMIT4 (diverged earlier)
run_cmd "merge: Non-fast-forward (expect fail)" merge $COMMIT4_SHA
check_status 1 # Expect fail because 3-way merge isn't implemented
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
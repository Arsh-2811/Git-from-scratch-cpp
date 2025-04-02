#!/bin/bash

# Extensive Test Script for mygit

# --- Configuration ---
# Point back up one level from mygit_test_repo to mygit2, then into build/
MYGIT_CMD="mygit"
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
    # Use relative path MYGIT_CMD which works once we cd into TEST_DIR
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

# Get FULL SHA from commit output
get_commit_full_sha() {
    local sha=""
    # Look for the line like "[main <sha>] message" and extract sha
    sha=$(echo "$LAST_CMD_OUTPUT" | grep -oE '\[.*\s([a-f0-9]{40})\]' | sed -E 's/.* ([a-f0-9]{40})\]/\1/')
    # Fallback: Try finding 40-char SHA on its own line (e.g., from log)
     if [ -z "$sha" ]; then
       sha=$(echo "$LAST_CMD_OUTPUT" | grep -oE 'commit ([a-f0-9]{40})' | head -n 1 | sed -E 's/commit //')
     fi
     echo "$sha"
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
        exit 1
    fi
}

# --- Cleanup Function ---
cleanup() {
    echo -e "\n${COLOR_YELLOW}Cleaning up test directory...${COLOR_RESET}"
    # Go back to parent dir before removing TEST_DIR
    cd ..
    rm -rf "$TEST_DIR"
}

# --- Trap for cleanup on exit ---
# Use EXIT trap
trap cleanup EXIT

# --- START TESTS ---

echo -e "${COLOR_YELLOW}===================================${COLOR_RESET}"
echo -e "${COLOR_YELLOW}       Starting mygit Tests        ${COLOR_RESET}"
echo -e "${COLOR_YELLOW}===================================${COLOR_RESET}"

# Setup test directory
# Cleanup first in case previous run failed
if [ -d "$TEST_DIR" ]; then
    echo "Removing previous test directory."
    rm -rf "$TEST_DIR"
fi
mkdir "$TEST_DIR"
cd "$TEST_DIR" || exit 1 # Exit if cd fails
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
check_output_contains "nothing to commit" # Expect this now
check_output_not_contains "Changes to be committed"
check_output_not_contains "Untracked files" # Expect this now


# --- Test: add, status, commit (basic workflow) ---
echo -e "\n${COLOR_YELLOW}--- Testing: add, status, commit (basic workflow) ---${COLOR_RESET}"
echo "Creating file1.txt"
echo "Hello Git!" > file1.txt

run_cmd "status: Untracked file" status
check_status 0
check_output_contains "Untracked files:"
check_output_contains "file1.txt" # Expect filename now

run_cmd "add: New file" add file1.txt
check_status 0
check_file_exists ".mygit/index" # Index should be created now

run_cmd "status: Staged new file" status
check_status 0
check_output_contains "Changes to be committed:"
check_output_contains "new file:   file1.txt"
check_output_not_contains "Untracked files:" # Should not be untracked

run_cmd "commit: Initial commit" commit -m "Initial commit: Add file1.txt"
check_status 0
check_output_contains "Initial commit: Add file1.txt"
COMMIT1_SHA=$(get_commit_full_sha) # Capture FULL SHA
check_sha_captured "COMMIT1_SHA"
echo "    -> Commit 1 SHA: $COMMIT1_SHA"

run_cmd "status: After first commit" status
check_status 0
check_output_contains "nothing to commit, working tree clean" # Expect this

echo "Modifying file1.txt"
echo "Hello again!" >> file1.txt

run_cmd "status: Modified workdir" status
check_status 0
check_output_contains "Changes not staged for commit:"
check_output_contains "modified:   file1.txt" # Expect modified
check_output_not_contains "Changes to be committed:"

run_cmd "add: Modified file" add file1.txt
check_status 0

run_cmd "status: Staged modified file" status
check_status 0
check_output_contains "Changes to be committed:"
check_output_contains "modified:   file1.txt" # Expect modified staged
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
COMMIT2_SHA=$(get_commit_full_sha) # Capture FULL SHA
check_sha_captured "COMMIT2_SHA"
echo "    -> Commit 2 SHA: $COMMIT2_SHA"

run_cmd "status: Clean after second commit" status
check_status 0
check_output_contains "nothing to commit, working tree clean" # Expect this


# --- Test: log ---
echo -e "\n${COLOR_YELLOW}--- Testing: log ---${COLOR_RESET}"
run_cmd "log: Basic" log
check_status 0
check_output_contains "commit ${COMMIT2_SHA}" # Check for FULL SHA
check_output_contains "Second commit: Modify file1, add file2"
check_output_contains "commit ${COMMIT1_SHA}" # Check for FULL SHA
check_output_contains "Initial commit: Add file1.txt"

run_cmd "log: Graph" log --graph
check_status 0
check_output_contains "digraph git_log {"
check_output_contains "\"${COMMIT2_SHA}\"" # Check node exists (using full sha)
check_output_contains "\"${COMMIT1_SHA}\""
check_output_contains "\"${COMMIT2_SHA}\" -> \"${COMMIT1_SHA}\"" # Check edge exists (using full sha)
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
COMMIT3_SHA=$(get_commit_full_sha) # Capture FULL SHA
check_sha_captured "COMMIT3_SHA"
echo "    -> Commit 3 SHA: $COMMIT3_SHA"

echo "Recreating file2.txt"
echo "Second file again" > file2.txt
run_cmd "add: Re-add file2" add file2.txt
check_status 0
run_cmd "commit: Re-add file2" commit -m "Re-add file2.txt"
check_status 0
COMMIT4_SHA=$(get_commit_full_sha) # Capture FULL SHA
check_sha_captured "COMMIT4_SHA"
echo "    -> Commit 4 SHA: $COMMIT4_SHA"

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
COMMIT5_SHA=$(get_commit_full_sha) # Capture FULL SHA
check_sha_captured "COMMIT5_SHA"
echo "    -> Commit 5 SHA: $COMMIT5_SHA"

run_cmd "status: After rm --cached commit" status
check_status 0
check_output_contains "Untracked files:"
check_output_contains "file1.txt" # Expect file1.txt here
check_output_not_contains "Changes to be committed:"


# --- Test: branch ---
echo -e "\n${COLOR_YELLOW}--- Testing: branch ---${COLOR_RESET}"
# Restore file1 to tracked state for branching tests
run_cmd "add: Restore file1 for branching" add file1.txt
check_status 0
run_cmd "commit: Restore file1" commit -m "Restore file1"
check_status 0
COMMIT6_SHA=$(get_commit_full_sha) # Capture FULL SHA
check_sha_captured "COMMIT6_SHA"
echo "    -> Commit 6 SHA: $COMMIT6_SHA"

run_cmd "branch: List initial" branch
check_status 0
check_output_contains "* main"
check_output_not_contains "feature1"

run_cmd "branch: Create feature1" branch feature1
check_status 0
check_file_exists ".mygit/refs/heads/feature1" # Check ref file created

run_cmd "branch: List after create" branch
check_status 0
check_output_contains "* main"
check_output_contains "  feature1"

# Create branch from older commit (Commit 4) - USE FULL SHA
run_cmd "branch: Create feature2 from specific commit" branch feature2 $COMMIT4_SHA
check_status 0 # Should succeed now with check
check_file_exists ".mygit/refs/heads/feature2"

run_cmd "branch: List after specific create" branch
check_status 0
check_output_contains "* main"
check_output_contains "  feature1"
check_output_contains "  feature2" # Should be listed

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
check_file_exists ".mygit/refs/tags/v1.0"

run_cmd "tag: List after lightweight" tag
check_status 0
check_output_contains "v1.0"

# Tag an older commit - USE FULL SHA
run_cmd "tag: Create lightweight v0.1 on Commit 1" tag v0.1 $COMMIT1_SHA
check_status 0
check_file_exists ".mygit/refs/tags/v0.1"

run_cmd "tag: List after specific lightweight" tag
check_status 0
check_output_contains "v0.1"
check_output_contains "v1.0"

run_cmd "tag: Create annotated v1.1" tag -a -m "Release version 1.1" v1.1
check_status 0
check_file_exists ".mygit/refs/tags/v1.1" # Check ref exists (points to tag object)

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
TREE1_SHA=$(get_tree_full_sha) # Capture FULL SHA
check_sha_captured "TREE1_SHA"
echo "    -> Tree 1 SHA: $TREE1_SHA"

echo "Creating file3.txt and adding"
echo "File three" > file3.txt
run_cmd "add: file3 for read-tree test" add file3.txt
check_status 0

run_cmd "status: Before read-tree" status
check_status 0
check_output_contains "new file:   file3.txt"

# Read the previous tree back into the index (overwrite mode) - USE FULL SHA
run_cmd "read-tree: Overwrite index with Tree 1" read-tree $TREE1_SHA
check_status 0 # Expect success even though -u isn't implemented

run_cmd "status: After read-tree (index updated)" status
check_status 0
check_output_not_contains "Changes to be committed:" # Index should match TREE1
check_output_contains "Untracked files:" # file3.txt still exists in workdir but not in index
check_output_contains "file3.txt" # Should be listed as untracked

# Cleanup file3
rm file3.txt


# --- Test: merge ---
echo -e "\n${COLOR_YELLOW}--- Testing: merge ---${COLOR_RESET}"
# Current state: main is at COMMIT6
# Use FULL SHAs throughout merge tests

# Test 1: Merge branch pointing to same commit
run_cmd "merge: Same commit" merge main # Merging current branch
check_status 0
check_output_contains "Already up to date."

# Test 2: Merge branch pointing to ancestor (Already up-to-date)
run_cmd "merge: Ancestor commit" merge feature2 # feature2 points to COMMIT4, main points to COMMIT6
check_status 0
check_output_contains "Already up to date."

# Test 3: Fast-forward scenario detection
# Setup: main at COMMIT6. Create feature_ff at COMMIT6. Make commit on feature_ff.
run_cmd "branch: Create FF branch" branch feature_ff $COMMIT6_SHA
check_status 0
echo "FF commit" > ff_file.txt
run_cmd "add: ff_file" add ff_file.txt
check_status 0
# Manually update feature_ff ref to simulate checkout/commit
run_cmd "commit: Commit on FF branch (simulated)" commit -m "FF commit"
check_status 0
COMMIT7_SHA=$(get_commit_full_sha)
check_sha_captured "COMMIT7_SHA"
echo "    -> Commit 7 SHA: $COMMIT7_SHA"
echo "Simulating feature_ff branch update..."
echo "$COMMIT7_SHA" > .mygit/refs/heads/feature_ff
# Now main is at COMMIT6, feature_ff is ahead at COMMIT7. Base is COMMIT6.
run_cmd "merge: Fast-forward (expect fail on -u)" merge feature_ff
check_status 1 # Should fail because read-tree -u fails/isn't implemented
check_output_contains "Fast-forward" # Check FF detection message
check_output_contains "Error updating index/workdir" # Check expected error

# Reset HEAD manually back to COMMIT6 for next test (as FF merge didn't update it)
echo "Simulating main branch reset to COMMIT6..."
echo "$COMMIT6_SHA" > .mygit/refs/heads/main
${MYGIT_CMD} update_head "ref: refs/heads/main" # Use internal function if possible, else manual echo
echo "ref: refs/heads/main" > .mygit/HEAD

# Test 4: Non-fast-forward scenario detection
# Setup: main at COMMIT6. topic at COMMIT4. Make commit on main.
echo "Main commit diverge" > main_diverge.txt
run_cmd "add: main_diverge" add main_diverge.txt
check_status 0
run_cmd "commit: Commit on main (diverging)" commit -m "Diverging commit on main"
check_status 0
COMMIT8_SHA=$(get_commit_full_sha)
check_sha_captured "COMMIT8_SHA"
echo "    -> Commit 8 SHA: $COMMIT8_SHA"
# Now main is at COMMIT8, topic is at COMMIT4. Base should be COMMIT3?
# Let's use feature2 which also points to COMMIT4
run_cmd "merge: Non-fast-forward (expect fail)" merge feature2
check_status 1 # Expect fail because 3-way merge isn't implemented
check_output_contains "True merge logic is not implemented" # Check expected failure message

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
    # Cleanup happens via trap, just exit 0
    exit 0
fi
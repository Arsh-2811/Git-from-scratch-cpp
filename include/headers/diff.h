#ifndef DIFF_H
#define DIFF_H

#include "headers/objects.h"
#include <string>
#include <map>

enum class FileStatus {
    Unmodified,     // Matches HEAD and index
    ModifiedStaged, // Changed in index vs HEAD
    AddedStaged,    // Added to index (not in HEAD)
    DeletedStaged,  // Deleted from index (was in HEAD)
    ModifiedWorkdir,// Changed in workdir vs index
    DeletedWorkdir, // Deleted from workdir (was in index)
    AddedWorkdir,   // New file in workdir (not in index) - Untracked
    Conflicted      // Unmerged paths in index (stages > 0)
};

struct StatusEntry {
    std::string path;
    FileStatus index_status = FileStatus::Unmodified;   // Status Index vs. HEAD
    FileStatus workdir_status = FileStatus::Unmodified; // Status Workdir vs Index
};

std::string get_workdir_sha(const std::string& path);

std::map<std::string, StatusEntry> get_repository_status();

std::map<std::string, std::string> read_tree_contents(const std::string& tree_sha1);

std::map<std::string, TreeEntry> read_tree_full(const std::string &tree_sha1);

#endif
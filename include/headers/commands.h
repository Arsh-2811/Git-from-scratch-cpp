#ifndef COMMANDS_H
#define COMMANDS_H

#include <string>
#include <vector>

int handle_init();
int handle_add(const std::vector<std::string>& files_to_add);
int handle_rm(const std::vector<std::string>& files_to_remove, bool cached_mode);
int handle_commit(const std::string& message);
int handle_status();
int handle_log(bool graph_mode);

int handle_branch(const std::vector<std::string>& args);
int handle_tag(const std::vector<std::string>& args);

int handle_write_tree();
int handle_read_tree(const std::string& tree_sha, bool update_workdir, bool merge_mode);

int handle_merge(const std::string& branch_to_merge);

#endif
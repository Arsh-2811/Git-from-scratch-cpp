#ifndef REFS_H
#define REFS_H

#include <string>
#include <vector>
#include <optional>

void update_ref(const std::string& ref_name, const std::string& value, bool symbolic = false);

std::string read_ref_direct(const std::string& ref_name);

std::optional<std::string> resolve_ref(const std::string& ref_or_sha_prefix);

std::string read_head();

void update_head(const std::string& value);

std::string get_branch_ref(const std::string& branch_name);

std::string get_tag_ref(const std::string& tag_name);

std::vector<std::string> list_branches();

std::vector<std::string> list_tags();

bool delete_ref(const std::string& ref_name);

#endif
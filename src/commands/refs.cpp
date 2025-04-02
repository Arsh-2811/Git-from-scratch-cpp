#include "headers/refs.h"
#include "headers/objects.h"
#include "headers/utils.h"

#include <fstream>
#include <stdexcept>
#include <iostream>

const std::string HEAD_PATH = GIT_DIR + "/HEAD";
const std::string REFS_HEADS_DIR = REFS_DIR + "/heads";
const std::string REFS_TAGS_DIR = REFS_DIR + "/tags";

void update_ref(const std::string& ref_name, const std::string& value, bool symbolic) {
    if (ref_name.find("..") != std::string::npos || ref_name.find("~") != std::string::npos || ref_name.find("^") != std::string::npos) {
        throw std::invalid_argument("Invalid character in ref name: " + ref_name);
    }

    std::string full_path_str = GIT_DIR + "/" + ref_name;
    fs::path full_path(full_path_str);
    fs::path parent_dir = full_path.parent_path();

    ensure_directory_exists(parent_dir);

    std::string content_to_write = value;
    if (symbolic) {
        content_to_write = "ref: " + value + "\n";
    } else {
        if (value.length() != 40 || value.find_first_not_of("0123456789abcdef") != std::string::npos) {
            if (ref_name != "HEAD") {
            }
        }
         content_to_write += "\n";
    }

    std::string lock_path_str = full_path_str + ".lock";
     std::ofstream lock_file(lock_path_str);
    if (!lock_file) {
        throw std::runtime_error("Failed to acquire lock for ref: " + ref_name);
    }
    lock_file.close();

    try {
        write_file(full_path_str, content_to_write);
    } catch (...) {
        fs::remove(lock_path_str);
        throw;
    }
    fs::remove(lock_path_str);
}

std::string read_ref_direct(const std::string& ref_name) {
    std::string full_path = GIT_DIR + "/" + ref_name;
    if (!file_exists(full_path)) {
        return "";
    }
    try {
        std::string content = read_file(full_path);
        if (!content.empty() && content.back() == '\n') {
            content.pop_back();
        }
        return content;
    } catch (const std::exception& e) {
        std::cerr << "Warning: Failed to read ref '" << ref_name << "': " << e.what() << std::endl;
        return "";
    }
}

std::optional<std::string> resolve_ref(const std::string& ref_or_sha_prefix) {
    if (ref_or_sha_prefix.empty()) return std::nullopt;

    std::string current_ref = ref_or_sha_prefix;
    int recursion_depth = 0;
    const int max_depth = 10;

    while (recursion_depth++ < max_depth) {
        if (current_ref == "HEAD") {
            std::string head_content = read_head();
            if (head_content.rfind("ref: ", 0) == 0) {
                current_ref = head_content.substr(5);
                continue;
            } else {
                if (head_content.length() >= 4 && head_content.length() <= 40 && head_content.find_first_not_of("0123456789abcdef") == std::string::npos) {
                    try { return find_object(head_content); }
                    catch (const std::runtime_error&) { return std::nullopt; }
                } else { return std::nullopt; }
            }
        }

        if (current_ref.rfind("refs/", 0) == 0) {
            std::string sha1 = read_ref_direct(current_ref);
            if (!sha1.empty()) {
                if (sha1.rfind("ref: ", 0) == 0) {
                    current_ref = sha1.substr(5);
                    continue;
                }
                if (sha1.length() == 40 && sha1.find_first_not_of("0123456789abcdef") == std::string::npos) {
                    return sha1;
                } else {
                    std::cerr << "Warning: Malformed SHA found in ref file: " << current_ref << std::endl;
                    return std::nullopt;
                }
            }
        }
        if (current_ref.find('/') == std::string::npos) {
            std::string branch_ref_path = get_branch_ref(current_ref);
            std::string sha1 = read_ref_direct(branch_ref_path);
            if (!sha1.empty()) {
                if (sha1.rfind("ref: ", 0) == 0) {
                    current_ref = sha1.substr(5); continue;
                }
                if (sha1.length() == 40 && sha1.find_first_not_of("0123456789abcdef") == std::string::npos) {
                    return sha1;
                } else {
                    std::cerr << "Warning: Malformed SHA in ref: " << branch_ref_path << std::endl;
                    return std::nullopt;
                }
            }
        }

        if (current_ref.find('/') == std::string::npos) {
            std::string tag_ref_path = get_tag_ref(current_ref);
            std::string sha1 = read_ref_direct(tag_ref_path);
            if (!sha1.empty()) {
                if (sha1.rfind("ref: ", 0) == 0) {
                    current_ref = sha1.substr(5); continue;
                }
                if (sha1.length() == 40 && sha1.find_first_not_of("0123456789abcdef") == std::string::npos) {
                    try {
                        ParsedObject obj = read_object(sha1);
                        if (obj.type == "tag") return std::get<TagObject>(obj.data).object_sha1;
                        else return sha1;
                    } catch (const std::exception& e) {
                        std::cerr << "Warning: Failed to read object for tag ref '" << tag_ref_path << "': " << e.what() << std::endl;
                        return std::nullopt;
                    }
                } else {
                    std::cerr << "Warning: Malformed SHA in ref: " << tag_ref_path << std::endl;
                    return std::nullopt;
                }
            }
        }

        if (current_ref.length() >= 4 && current_ref.length() <= 40 && current_ref.find_first_not_of("0123456789abcdef") == std::string::npos) {
            try { return find_object(current_ref); }
            catch (const std::runtime_error&) {}
        }

        break;
    }

    if (recursion_depth >= max_depth) {
        std::cerr << "Warning: Maximum symbolic ref recursion depth exceeded for: " << ref_or_sha_prefix << std::endl;
    }

    return std::nullopt;
}

std::string read_head() {
    return read_ref_direct("HEAD");
}

void update_head(const std::string& value) {
    bool symbolic = (value.rfind("ref: ", 0) == 0);
    std::string content = value;
    if (!symbolic && value.rfind("refs/", 0) == 0) {
        symbolic = true;
        content = value;
    }
    update_ref("HEAD", content, symbolic);
}

std::string get_branch_ref(const std::string& branch_name) {
    if (branch_name.empty() || branch_name.find('/') != std::string::npos || branch_name == "." || branch_name == "..") {
         throw std::invalid_argument("Invalid branch name: " + branch_name);
    }
    return "refs/heads/" + branch_name;
}

std::string get_tag_ref(const std::string& tag_name) {
    if (tag_name.empty() || tag_name.find('/') != std::string::npos || tag_name == "." || tag_name == "..") {
        throw std::invalid_argument("Invalid tag name: " + tag_name);
    }
    return "refs/tags/" + tag_name;
}

std::vector<std::string> list_refs_in_dir(const std::string& dir_path_str) {
    std::vector<std::string> names;
    fs::path dir_path = fs::path(GIT_DIR) / dir_path_str;

    if (!fs::exists(dir_path) || !fs::is_directory(dir_path)) {
        return names;
    }

    try {
        for (const auto& entry : fs::recursive_directory_iterator(dir_path)) {
            if (entry.is_regular_file()) {
                fs::path relative_path = fs::relative(entry.path(), dir_path);
                names.push_back(relative_path.generic_string());
            }
        }
    } catch (const fs::filesystem_error& e) {
         std::cerr << "Warning: Error iterating directory " << dir_path << ": " << e.what() << std::endl;
    }
    std::sort(names.begin(), names.end());
    return names;
}

std::vector<std::string> list_branches() {
    return list_refs_in_dir("refs/heads");
}

std::vector<std::string> list_tags() {
    return list_refs_in_dir("refs/tags");
}

bool delete_ref(const std::string& ref_name) {
     std::string full_path_str = GIT_DIR + "/" + ref_name;
    try {
        if (fs::exists(full_path_str)) {
            return fs::remove(full_path_str);
        }
        return false; // Didn't exist
    } catch (const fs::filesystem_error& e) {
        std::cerr << "Error deleting ref '" << ref_name << "': " << e.what() << std::endl;
        return false;
    }
}
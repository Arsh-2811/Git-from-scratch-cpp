#include "headers/objects.h"
#include "headers/utils.h"

#include <stdexcept>
#include <sstream>
#include <vector>
#include <algorithm>
#include <cstring>
#include <iostream> 

#include <fstream>

#include <openssl/sha.h>

// std::string find_object(const std::string& sha1_prefix);
std::string get_object_path(const std::string& sha1);
void ensure_object_directory_exists(const std::string& sha1);

std::string get_object_path(const std::string& sha1) {
    if (sha1.length() != 40) {
        throw std::invalid_argument("Invalid SHA-1 length for path: " + sha1);
    }
    return OBJECTS_DIR + "/" + sha1.substr(0, 2) + "/" + sha1.substr(2);
}

void ensure_object_directory_exists(const std::string& sha1) {
    if (sha1.length() != 40) {
         throw std::invalid_argument("Invalid SHA-1 length for directory creation: " + sha1);
    }
    fs::path dir = fs::path(OBJECTS_DIR) / sha1.substr(0, 2);
    ensure_directory_exists(dir);
}

std::string find_object(const std::string& sha1_prefix) {
    if (sha1_prefix.length() < 4) {
        throw std::runtime_error("fatal: ambiguous argument '" + sha1_prefix + "': unknown revision or path not in the working tree.");
    }
    if (sha1_prefix.length() > 40) {
         throw std::runtime_error("fatal: Not a valid object name " + sha1_prefix);
    }

    std::string dir_path = OBJECTS_DIR + "/" + sha1_prefix.substr(0, 2);
    if (!fs::exists(dir_path) || !fs::is_directory(dir_path)) {
        throw std::runtime_error("fatal: Not a valid object name " + sha1_prefix);
    }

    std::string rest_prefix = sha1_prefix.substr(2);
    std::vector<std::string> matches;

    for (const auto& entry : fs::directory_iterator(dir_path)) {
        std::string filename = entry.path().filename().string();
        if (filename.rfind(rest_prefix, 0) == 0) {
             if (sha1_prefix.length() == 40 && filename.length() == 38) {
                 return sha1_prefix;
             }
             matches.push_back(sha1_prefix.substr(0, 2) + filename);
        }
    }

    if (matches.empty()) {
        throw std::runtime_error("fatal: Not a valid object name " + sha1_prefix);
    }
    if (matches.size() > 1) {
        if (sha1_prefix.length() < 40) {
            throw std::runtime_error("fatal: ambiguous argument '" + sha1_prefix + "': multiple possibilities");
        } else {
            throw std::runtime_error("fatal: internal error - multiple objects found for full SHA: " + sha1_prefix);
        }
    }
    return matches[0];
}

void write_object(const std::string& sha1, const std::vector<unsigned char>& compressed_data) {
     try {
        ensure_object_directory_exists(sha1);
        std::string path = get_object_path(sha1);
        if (file_exists(path)) {
            return;
        }
        write_file(path, compressed_data);
    } catch (const std::exception& e) {
        throw std::runtime_error("Failed to write object " + sha1 + ": " + e.what());
    }
}

void write_object(const std::string& sha1, const std::string& object_type, const std::string& content) {
    std::string object_data = object_type + " " + std::to_string(content.size()) + '\0' + content;
    std::string computed_sha1 = compute_sha1(object_data);
    if (sha1 != computed_sha1) {
        throw std::logic_error("Internal error: SHA1 mismatch during write. Expected " + sha1 + ", got " + computed_sha1);
    }

    std::vector<unsigned char> compressed = compress_data(object_data);
    write_object(sha1, compressed);
}

std::string hash_and_write_object(const std::string& type, const std::string& content) {
    std::string object_data = type + " " + std::to_string(content.size()) + '\0' + content;
    std::string sha1 = compute_sha1(object_data);
    std::string path = get_object_path(sha1);

    if (!file_exists(path)) {
        std::vector<unsigned char> compressed = compress_data(object_data);
        write_object(sha1, compressed);
    }
    return sha1;
}

ParsedObject read_object(const std::string& sha1_prefix_or_full) {
    std::string sha1 = find_object(sha1_prefix_or_full);
    std::string path = get_object_path(sha1);

    std::ifstream file(path, std::ios::binary | std::ios::ate);
    if (!file) {
        throw std::runtime_error("Failed to open object file: " + path);
    }

    std::streamsize compressed_size = file.tellg();
    if (compressed_size < 0) {
        file.close();
        throw std::runtime_error("Failed to get size of object file: " + path);
    }
    file.seekg(0, std::ios::beg);
    std::vector<unsigned char> compressed_data(compressed_size);
    if (compressed_size > 0 && !file.read(reinterpret_cast<char*>(compressed_data.data()), compressed_size)) {
        file.close();
        throw std::runtime_error("Failed to read object file: " + path);
    }
    file.close();


    std::string decompressed_data;
    try {
        decompressed_data = decompress_chunk(compressed_data);
    } catch (const std::runtime_error& e) {
        throw std::runtime_error("Failed to decompress object " + sha1 + ": " + e.what());
    }
    if (decompressed_data.empty() && compressed_size > 0) {
        throw std::runtime_error("Decompression resulted in empty data for object " + sha1);
    }

    const char* null_terminator = static_cast<const char*>(memchr(decompressed_data.data(), '\0', decompressed_data.size()));
    if (!null_terminator) {
        throw std::runtime_error("Invalid object format: Missing null terminator in object " + sha1);
    }

    std::string header(decompressed_data.data(), null_terminator - decompressed_data.data());
    size_t header_len = header.length() + 1;
    std::string content = decompressed_data.substr(header_len);

    size_t space_pos = header.find(' ');
    if (space_pos == std::string::npos) {
        throw std::runtime_error("Invalid object format: Malformed header '" + header + "' in object " + sha1);
    }

    ParsedObject result;
    result.type = header.substr(0, space_pos);
    std::string size_str = header.substr(space_pos + 1);

    try {
        char* endptr;
        unsigned long long parsed_size_ll = std::strtoull(size_str.c_str(), &endptr, 10);
        if (*endptr != '\0') throw std::invalid_argument("Invalid size characters");
        if (parsed_size_ll > std::numeric_limits<size_t>::max()) throw std::out_of_range("Size exceeds size_t capacity");
        result.size = static_cast<size_t>(parsed_size_ll);
    } catch (const std::exception& e) {
        throw std::runtime_error("Invalid object format: Cannot parse size '" + size_str + "' in object " + sha1 + ": " + e.what());
    }

    if (result.size != content.length()) {
        throw std::runtime_error("Object size mismatch: Header says " + std::to_string(result.size)
                                 + ", but content length is " + std::to_string(content.length())
                                 + " in object " + sha1);
    }

    try {
        if (result.type == "blob") {
            result.data = parse_blob_content(content);
        } else if (result.type == "tree") {
            result.data = parse_tree_content(content);
        } else if (result.type == "commit") {
            result.data = parse_commit_content(content);
        } else if (result.type == "tag") {
            result.data = parse_tag_content(content);
        } else {
            throw std::runtime_error("Unknown object type '" + result.type + "' found for object " + sha1);
        }
    } catch (const std::exception& e) {
         throw std::runtime_error("Failed to parse " + result.type + " object " + sha1 + ": " + e.what());
    }


    return result;
}

BlobObject parse_blob_content(const std::string& content) {
    return {content};
}

TreeObject parse_tree_content(const std::string& content) {
    TreeObject tree;
    const char* data_ptr = content.data();
    const char* end_ptr = data_ptr + content.size();

    while (data_ptr < end_ptr) {
        const char* space_ptr = static_cast<const char*>(memchr(data_ptr, ' ', end_ptr - data_ptr));
        if (!space_ptr) throw std::runtime_error("Malformed tree entry: missing space after mode");
        std::string mode(data_ptr, space_ptr - data_ptr);

        const char* name_start_ptr = space_ptr + 1;
        const char* null_ptr = static_cast<const char*>(memchr(name_start_ptr, '\0', end_ptr - name_start_ptr));
        if (!null_ptr) throw std::runtime_error("Malformed tree entry: missing null after name");
        std::string name(name_start_ptr, null_ptr - name_start_ptr);

        const char* sha1_start_ptr = null_ptr + 1;
        if (sha1_start_ptr + SHA_DIGEST_LENGTH > end_ptr) {
             throw std::runtime_error("Malformed tree entry: insufficient data for SHA-1");
        }

        std::string sha1_hex = sha1_to_hex(reinterpret_cast<const unsigned char*>(sha1_start_ptr));

        tree.entries.push_back({mode, name, sha1_hex});

        data_ptr = sha1_start_ptr + SHA_DIGEST_LENGTH;
    }
    if (data_ptr != end_ptr) {
        throw std::runtime_error("Malformed tree object: trailing data found");
    }

    return tree;
}

CommitObject parse_commit_content(const std::string& content) {
    CommitObject commit;
    std::istringstream iss(content);
    std::string line;
    bool in_message = false;

    while (std::getline(iss, line)) {
        if (in_message) {
            commit.message += line + "\n";
            continue;
        }
        if (line.empty()) {
            in_message = true;
            continue;
        }

        size_t space_pos = line.find(' ');
        if (space_pos == std::string::npos) {
            throw std::runtime_error("Malformed commit header line: " + line);
        }
        std::string key = line.substr(0, space_pos);
        std::string value = line.substr(space_pos + 1);

        if (key == "tree") {
            commit.tree_sha1 = value;
        } else if (key == "parent") {
            commit.parent_sha1s.push_back(value);
        } else if (key == "author") {
            commit.author_info = value;
        } else if (key == "committer") {
            commit.committer_info = value;
        }
    }
    if (!commit.message.empty() && commit.message.back() == '\n') {
        commit.message.pop_back();
    }

    return commit;
}

TagObject parse_tag_content(const std::string& content) {
    TagObject tag;
    std::istringstream iss(content);
    std::string line;
    bool in_message = false;

     while (std::getline(iss, line)) {
        if (in_message) {
            tag.message += line + "\n";
            continue;
        }
        if (line.empty()) {
            in_message = true;
            continue;
        }

        size_t space_pos = line.find(' ');
        if (space_pos == std::string::npos) {
            throw std::runtime_error("Malformed tag header line: " + line);
        }
        std::string key = line.substr(0, space_pos);
        std::string value = line.substr(space_pos + 1);

        if (key == "object") {
            tag.object_sha1 = value;
        } else if (key == "type") {
            tag.type = value;
        } else if (key == "tag") {
            tag.tag_name = value;
        } else if (key == "tagger") {
            tag.tagger_info = value;
        }
    }
    if (!tag.message.empty() && tag.message.back() == '\n') {
        tag.message.pop_back();
    }
    return tag;
}


std::string format_tree_content(const std::vector<TreeEntry>& entries) {
    std::vector<TreeEntry> sorted_entries = entries;
    std::sort(sorted_entries.begin(), sorted_entries.end(), [](const TreeEntry& a, const TreeEntry& b) {
        return a.name < b.name;
    });

    std::ostringstream oss;
    for (const auto& entry : sorted_entries) {
        std::vector<unsigned char> sha1_binary = hex_to_sha1(entry.sha1);
        if (sha1_binary.size() != SHA_DIGEST_LENGTH) {
             throw std::logic_error("Internal error: Invalid SHA1 length during tree formatting.");
        }
        oss << entry.mode << " " << entry.name << '\0';
        oss.write(reinterpret_cast<const char*>(sha1_binary.data()), sha1_binary.size());
    }
    return oss.str();
}


std::string format_commit_content(const std::string& tree_sha1, const std::vector<std::string>& parent_sha1s,
                                  const std::string& author, const std::string& committer, const std::string& message) {
    std::ostringstream oss;
    oss << "tree " << tree_sha1 << "\n";
    for (const auto& parent : parent_sha1s) {
        oss << "parent " << parent << "\n";
    }
    oss << "author " << author << "\n";
    oss << "committer " << committer << "\n";
    oss << "\n";
    oss << message;

    if (message.empty() || message.back() != '\n') {
        oss << "\n";
    }
    return oss.str();
}

std::string format_tag_content(const std::string& object_sha1, const std::string& type, const std::string& tag_name,
                               const std::string& tagger, const std::string& message) {
    std::ostringstream oss;
    oss << "object " << object_sha1 << "\n";
    oss << "type " << type << "\n";
    oss << "tag " << tag_name << "\n";
    oss << "tagger " << tagger << "\n";
    oss << "\n";
    oss << message;
    if (message.empty() || message.back() != '\n') {
        oss << "\n";
    }
    return oss.str();
}
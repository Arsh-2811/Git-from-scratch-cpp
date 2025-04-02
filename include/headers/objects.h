#ifndef OBJECTS_H
#define OBJECTS_H

#include <string>
#include <vector>
#include <variant>

struct IndexEntry;

struct BlobObject {
    std::string content;
};

struct TreeEntry {
    std::string mode;
    std::string name;
    std::string sha1;
};

struct TreeObject {
    std::vector<TreeEntry> entries;
};

struct CommitObject {
    std::string tree_sha1;
    std::vector<std::string> parent_sha1s;
    std::string author_info;
    std::string committer_info;
    std::string message;
};

struct TagObject {
    std::string object_sha1;
    std::string type;
    std::string tag_name;
    std::string tagger_info;
    std::string message;
};

using ParsedObjectData = std::variant<BlobObject, TreeObject, CommitObject, TagObject>;

struct ParsedObject {
    std::string type;
    size_t size = 0;
    ParsedObjectData data;
};

std::string find_object(const std::string& sha1_prefix);

void write_object(const std::string& sha1, const std::string& object_type, const std::string& content);
void write_object(const std::string& sha1, const std::vector<unsigned char>& compressed_data);

ParsedObject read_object(const std::string& sha1_prefix_or_full);

BlobObject parse_blob_content(const std::string& content);
TreeObject parse_tree_content(const std::string& content);
CommitObject parse_commit_content(const std::string& content);
TagObject parse_tag_content(const std::string& content);

std::string format_tree_content(const std::vector<TreeEntry>& entries);
std::string format_commit_content(const std::string& tree_sha1, const std::vector<std::string>& parent_sha1s,
                                  const std::string& author, const std::string& committer, const std::string& message);
std::string format_tag_content(const std::string& object_sha1, const std::string& type, const std::string& tag_name,
                               const std::string& tagger, const std::string& message);

std::string hash_and_write_object(const std::string& type, const std::string& content);

#endif
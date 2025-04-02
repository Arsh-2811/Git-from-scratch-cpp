#ifndef INDEX_H
#define INDEX_H

#include <string>
#include <map>

struct IndexEntry {
    std::string mode;      // e.g., "100644"
    std::string sha1;      // Hex SHA-1 of the blob object
    int stage = 0;         // Stage number (0 = normal, 1 = base, 2 = ours, 3 = theirs for merges)
    std::string path;      // File path relative to repository root

    bool operator<(const IndexEntry& other) const {
        if (path != other.path) return path < other.path;
        return stage < other.stage;
    }
    bool operator==(const IndexEntry& other) const {
        return path == other.path && stage == other.stage;
    }

};

using IndexMap = std::map<std::string, std::map<int, IndexEntry>>;

IndexMap read_index();

void write_index(const IndexMap& index_data);

void add_or_update_entry(IndexMap& index_data, const IndexEntry& entry);

void remove_entry(IndexMap& index_data, const std::string& path, int stage = -1);

#endif
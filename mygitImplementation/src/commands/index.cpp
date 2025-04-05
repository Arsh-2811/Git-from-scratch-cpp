#include "headers/index.h"
#include "headers/utils.h"

#include <fstream>
#include <stdexcept>
#include <algorithm>
#include <iostream>

// const std::string INDEX_PATH = GIT_DIR + "/index";
// const std::string LOCK_PATH = GIT_DIR + "/index.lock";

IndexMap read_index() {
    std::string index_path = GIT_DIR + "/index";
    IndexMap index_data;
    std::ifstream index_file(index_path);
    if (!index_file) {
        if (!fs::exists(index_path)) return index_data;
        throw std::runtime_error("Failed to open index file: " + index_path);
    }

    std::string line;
    int line_num = 0;
    while (std::getline(index_file, line)) {
        line_num++;
        std::size_t tab_pos = line.find('\t');
        if (tab_pos == std::string::npos) {
            std::cerr << "Warning: Malformed index entry (no tab) on line " << line_num << ": " << line << std::endl;
            continue;
        }

        std::string header = line.substr(0, tab_pos);
        std::string path = line.substr(tab_pos + 1);

        std::vector<std::string> parts = split_string(header, ' ');
        if (parts.size() != 3) {
            std::cerr << "Warning: Malformed index entry header on line " << line_num << ": " << header << std::endl;
            continue;
        }

        IndexEntry entry;
        entry.mode = parts[0];
        entry.sha1 = parts[1];
        try {
            entry.stage = std::stoi(parts[2]);
        } catch (const std::exception& e) {
             std::cerr << "Warning: Invalid stage number on line " << line_num << ": " << parts[2] << std::endl;
             continue;
        }
        entry.path = path;

        index_data[entry.path][entry.stage] = entry;
    }

    if (index_file.bad()) {
        throw std::runtime_error("Error reading from index file: " + index_path);
    }

    return index_data;
}

void write_index(const IndexMap& index_data) {
    std::string index_path = GIT_DIR + "/index";       
    std::string lock_path_str = GIT_DIR + "/index.lock";
    std::ofstream lock_file(lock_path_str);
    if (!lock_file) {
        throw std::runtime_error("Could not acquire lock on index file. Path: " + lock_path_str);
    }
    lock_file << "locked";
    lock_file.close();


    std::string temp_index_path = index_path + ".tmp";
    std::ofstream temp_file(temp_index_path, std::ios::binary | std::ios::trunc);
    if (!temp_file) {
        fs::remove(lock_path_str);
        throw std::runtime_error("Failed to open temporary index file for writing: " + temp_index_path);
    }

    try {
        std::vector<IndexEntry> sorted_entries;
        for (const auto& path_pair : index_data) {
            for (const auto& stage_pair : path_pair.second) {
                sorted_entries.push_back(stage_pair.second);
            }
        }
        std::sort(sorted_entries.begin(), sorted_entries.end());

        for (const auto& entry : sorted_entries) {
            temp_file << entry.mode << " " << entry.sha1 << " " << entry.stage << "\t" << entry.path << "\n";
            if (!temp_file) {
                throw std::runtime_error("Failed to write entry to temporary index file");
            }
        }
        temp_file.close();
        fs::rename(temp_index_path, index_path);

    } catch (...) {
        fs::remove(temp_index_path);
        fs::remove(lock_path_str);
        throw;
    }
    fs::remove(lock_path_str);
}

void add_or_update_entry(IndexMap& index_data, const IndexEntry& entry) {
    index_data[entry.path][entry.stage] = entry;
}

void remove_entry(IndexMap& index_data, const std::string& path, int stage) {
    auto path_it = index_data.find(path);
    if (path_it != index_data.end()) {
        if (stage == -1) {
            index_data.erase(path_it);
        } else {
            auto stage_it = path_it->second.find(stage);
            if (stage_it != path_it->second.end()) {
                path_it->second.erase(stage_it);
                if (path_it->second.empty()) {
                    index_data.erase(path_it);
                }
            }
        }
    }
}
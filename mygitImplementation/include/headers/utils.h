#ifndef UTILS_H
#define UTILS_H

#include <string>
#include <vector>
#include <filesystem>

namespace fs = std::filesystem;

extern const std::string GIT_DIR;
extern const std::string OBJECTS_DIR;
extern const std::string REFS_DIR;

std::string read_file(const std::string& filename);
void write_file(const std::string& filename, const std::string& data);
void write_file(const std::string& filename, const std::vector<unsigned char>& data);
bool file_exists(const std::string& filename);
void ensure_directory_exists(const fs::path& dir_path);

void ensure_parent_directory_exists(const fs::path& file_path); 
mode_t get_file_mode(const std::string& filename);
void set_file_executable(const std::string& filename, bool executable);

std::string compute_sha1(const std::string& data);
std::string compute_sha1(const std::vector<unsigned char>& data);
std::vector<unsigned char> compress_data(const std::string& input);
std::string decompress_chunk(const std::vector<unsigned char>& compressed_data, size_t initial_chunk_size = 1024);
std::string sha1_to_hex(const unsigned char* sha1_binary);
std::vector<unsigned char> hex_to_sha1(const std::string& sha1_hex);

std::string get_current_timestamp_and_zone();
std::string get_user_info();

std::vector<std::string> split_string(const std::string& s, char delimiter);

#endif
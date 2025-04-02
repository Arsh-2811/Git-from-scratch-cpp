#include "headers/utils.h"

#include <iostream>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <stdexcept>
#include <cstring>
#include <cstdlib>
#include <chrono>
#include <ctime> 
#include <sys/stat.h>

#include <openssl/sha.h>
#include <zlib.h>

const std::string GIT_DIR = ".mygit";
const std::string OBJECTS_DIR = GIT_DIR + "/objects";
const std::string REFS_DIR = GIT_DIR + "/refs";

std::string read_file(const std::string& filename) {
    std::ifstream file(filename, std::ios::binary | std::ios::ate);
    if (!file) {
        return "";
    }

    std::streamsize size = file.tellg();
    if (size < 0) {
        file.close();
        throw std::runtime_error("Failed to get size of file: " + filename);
    }

    file.seekg(0, std::ios::beg);

    std::string buffer(size, '\0');
    if (size > 0 && !file.read(&buffer[0], size)) {
        file.close();
        throw std::runtime_error("Failed to read file: " + filename);
    }
    file.close();
    return buffer;
}

void write_file(const std::string& filename, const std::string& data) {
    std::ofstream file(filename, std::ios::binary);
    if (!file) {
        throw std::runtime_error("Failed to open file for writing: " + filename);
    }
    file.write(data.data(), data.size());
    if (!file) {
        throw std::runtime_error("Failed to write data to file: " + filename);
    }
}

void write_file(const std::string& filename, const std::vector<unsigned char>& data) {
    std::ofstream file(filename, std::ios::binary);
    if (!file) {
        throw std::runtime_error("Failed to open file for writing: " + filename);
    }
    file.write(reinterpret_cast<const char*>(data.data()), data.size());
     if (!file) {
        throw std::runtime_error("Failed to write data to file: " + filename);
    }
}

bool file_exists(const std::string& filename) {
    return fs::exists(filename);
}

void ensure_directory_exists(const fs::path& dir_path) {
    if (!fs::exists(dir_path)) {
        try {
            fs::create_directories(dir_path);
        } catch (const fs::filesystem_error& e) {
            throw std::runtime_error("Failed to create directory " + dir_path.string() + ": " + e.what());
        }
    } else if (!fs::is_directory(dir_path)) {
        throw std::runtime_error("Path exists but is not a directory: " + dir_path.string());
    }
}

mode_t get_file_mode(const std::string& filename) {
    struct stat file_stat;
    if (stat(filename.c_str(), &file_stat) == 0) {
        if (S_ISLNK(file_stat.st_mode)) {
            return 0120000;
        } else if (S_ISDIR(file_stat.st_mode)) {
            return 0040000;
        }
        mode_t mode = S_IFREG | (file_stat.st_mode & (S_IRWXU | S_IRWXG | S_IRWXO));
        if (mode & (S_IXUSR | S_IXGRP | S_IXOTH)) {
            return 0100755; // Executable
        } else {
            return 0100644; // Non-executable
        }
    }
    return 0;
}

std::string compute_sha1(const std::string& data) {
    unsigned char hash[SHA_DIGEST_LENGTH];
    SHA1(reinterpret_cast<const unsigned char*>(data.c_str()), data.size(), hash);
    return sha1_to_hex(hash);
}

std::string compute_sha1(const std::vector<unsigned char>& data) {
     unsigned char hash[SHA_DIGEST_LENGTH];
    SHA1(data.data(), data.size(), hash);
    return sha1_to_hex(hash);
}

std::vector<unsigned char> compress_data(const std::string& input) {
    uLongf bound = compressBound(input.size());
    std::vector<unsigned char> compressed(bound);
    uLongf compressed_size = bound;

    int res = compress(compressed.data(), &compressed_size,
                       reinterpret_cast<const Bytef*>(input.data()), input.size());

    if (res != Z_OK) {
        throw std::runtime_error("zlib compression failed: " + std::to_string(res));
    }
    compressed.resize(compressed_size);
    return compressed;
}

std::string decompress_chunk(const std::vector<unsigned char>& compressed_data, size_t initial_chunk_size) {
    std::vector<Bytef> buffer(initial_chunk_size);
    z_stream strm = {};
    strm.avail_in = compressed_data.size();
    strm.next_in = const_cast<Bytef*>(compressed_data.data());

    if (inflateInit(&strm) != Z_OK) {
        throw std::runtime_error("zlib inflateInit failed");
    }

    std::string decompressed_string;
    int ret;
    do {
        strm.avail_out = buffer.size();
        strm.next_out = buffer.data();
        ret = inflate(&strm, Z_NO_FLUSH);

        switch (ret) {
            case Z_NEED_DICT:
            case Z_DATA_ERROR:
            case Z_MEM_ERROR:
                inflateEnd(&strm);
                throw std::runtime_error("zlib inflate failed with error: " + std::to_string(ret));
        }

        size_t have = buffer.size() - strm.avail_out;
        decompressed_string.append(reinterpret_cast<char*>(buffer.data()), have);

    } while (strm.avail_out == 0 && ret != Z_STREAM_END);

    inflateEnd(&strm);

     if (ret != Z_STREAM_END && strm.avail_in > 0) { /* Warning maybe */ }
     if (ret != Z_STREAM_END && decompressed_string.empty()) {
         throw std::runtime_error("Decompression failed to produce any output.");
     }

    return decompressed_string;
}

std::string sha1_to_hex(const unsigned char* sha1_binary) {
    std::ostringstream result;
    result << std::hex << std::setfill('0');
    for (int i = 0; i < SHA_DIGEST_LENGTH; ++i) {
        result << std::setw(2) << static_cast<int>(sha1_binary[i]);
    }
    return result.str();
}

std::vector<unsigned char> hex_to_sha1(const std::string& sha1_hex) {
    if (sha1_hex.length() != SHA_DIGEST_LENGTH * 2) {
        throw std::invalid_argument("Invalid hex SHA-1 string length: " + sha1_hex);
    }
    std::vector<unsigned char> sha1_binary(SHA_DIGEST_LENGTH);
    for (size_t i = 0; i < SHA_DIGEST_LENGTH; ++i) {
        try {
            unsigned int byte = std::stoul(sha1_hex.substr(i * 2, 2), nullptr, 16);
            sha1_binary[i] = static_cast<unsigned char>(byte);
        } catch (const std::exception& e) {
             throw std::invalid_argument("Invalid hex character in SHA-1 string: " + sha1_hex);
        }
    }
    return sha1_binary;
}

std::string get_current_timestamp_and_zone() {
    auto now = std::chrono::system_clock::now();
    auto now_c = std::chrono::system_clock::to_time_t(now);
    auto seconds_since_epoch = std::chrono::duration_cast<std::chrono::seconds>(now.time_since_epoch()).count();

    std::tm now_tm = *std::localtime(&now_c);
    char buffer[10];

    std::time_t utc_time = std::mktime(std::gmtime(&now_c));
    long offset_seconds = static_cast<long>(now_c - utc_time);

    now_tm = *std::localtime(&now_c);
    std::time_t local_time_check = std::mktime(&now_tm);
    offset_seconds = static_cast<long>(local_time_check - utc_time);


    int offset_hours = static_cast<int>(offset_seconds / 3600);
    int offset_minutes = static_cast<int>((offset_seconds % 3600) / 60);


    snprintf(buffer, sizeof(buffer), "%+03d%02d", offset_hours, std::abs(offset_minutes)); // Use snprintf for safety

    std::ostringstream oss;
    oss << seconds_since_epoch << " " << buffer;
    return oss.str();
}

std::string get_user_info() {
    const char* name = std::getenv("GIT_AUTHOR_NAME");
    const char* email = std::getenv("GIT_AUTHOR_EMAIL");

    std::string user_name = name ? name : "Default User";
    std::string user_email = email ? email : "user@example.com";

    return user_name + " <" + user_email + ">";
}

std::vector<std::string> split_string(const std::string& s, char delimiter) {
    std::vector<std::string> tokens;
    std::string token;
    std::istringstream tokenStream(s);
    while (std::getline(tokenStream, token, delimiter)) {
        tokens.push_back(token);
    }
    return tokens;
}
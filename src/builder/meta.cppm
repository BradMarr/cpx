module;

#include <fstream>
#include <string>
#include <ctime>
#include <filesystem>
#include <stdexcept>
#include <format>
#include <iostream>
#include <cstdint>

export module builder.meta;

import builder.modules;

void write(const modules::Module& mod, const std::time_t last_modified) {
    std::ofstream file(mod.path("meta").string(), std::ios::binary);
    if (!file.is_open()) {
        throw std::runtime_error(std::format("Failed to open meta file for writing: {}", mod.path("meta").string()));
    }

    const int64_t fixed_time = static_cast<int64_t>(last_modified);
    file.write(reinterpret_cast<const char*>(&fixed_time), sizeof(int64_t));
}

std::time_t read(const modules::Module& mod) {
    std::ifstream file(mod.path("meta").string(), std::ios::binary);
    if (!file.is_open()) {
        throw std::runtime_error(std::format("Failed to open meta file for reading: {}", mod.path("meta").string()));
    }

    int64_t fixed_time;
    file.read(reinterpret_cast<char*>(&fixed_time), sizeof(int64_t));
    return static_cast<std::time_t>(fixed_time);
}

std::time_t get_last_modified(const modules::Module& mod) {
    if (!std::filesystem::exists(mod.path("cppm"))) {
        throw std::runtime_error(std::format("File does not exist: {}", mod.path("cppm").string()));
    }
    auto ftime = std::filesystem::last_write_time(mod.path("cppm"));
    auto sctp = std::chrono::time_point_cast<std::chrono::system_clock::duration>(
                    ftime - std::filesystem::file_time_type::clock::now() +
                    std::chrono::system_clock::now());

    return std::chrono::system_clock::to_time_t(sctp);
}

export namespace meta {
    void update(const modules::Module& mod) {
        std::time_t last_modified = get_last_modified(mod);
        write(mod, last_modified);
    }

    bool is_up_to_date(const modules::Module& mod) {
        try {
            std::time_t last_modified = get_last_modified(mod);
            std::time_t meta_time = read(mod);
            return last_modified == meta_time;
        } catch (const std::runtime_error& e) {
            return false;
        }
    }
}
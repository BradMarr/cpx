module;

#include <fstream>
#include <string>
#include <ctime>
#include <filesystem>
#include <stdexcept>
#include <format>
#include <iostream>

export module meta;

import modules;

void write(modules::Module& mod, const std::time_t last_modified) {
    std::ofstream file(mod.path("meta").c_str(), std::ios::binary);
    if (!file.is_open()) {
        throw std::runtime_error(std::format("Failed to open meta file for writing: {}", mod.path("meta").c_str()));
    }

    file.write(reinterpret_cast<const char*>(&last_modified), sizeof(std::time_t));
    file.close();
}

std::time_t read(modules::Module& mod) {
    std::ifstream file(mod.path("meta").c_str(), std::ios::binary);
    if (!file.is_open()) {
        throw std::runtime_error(std::format("Failed to open meta file for reading: {}", mod.path("meta").c_str()));
    }

    std::time_t last_modified;
    file.read(reinterpret_cast<char*>(&last_modified), sizeof(std::time_t));
    file.close();
    return last_modified;
}

std::time_t get_last_modified(modules::Module& mod) {
    if (!std::filesystem::exists(mod.path("cppm").c_str())) {
        throw std::runtime_error(std::format("File does not exist: {}", mod.path("cppm").c_str()));
    }
    return std::filesystem::last_write_time(mod.path("cppm")).time_since_epoch().count();
}

export namespace meta {
    void update(modules::Module& mod) {
        std::time_t last_modified = get_last_modified(mod);
        write(mod, last_modified);
    }

    bool is_up_to_date(modules::Module& mod) {
        try {
            std::time_t last_modified = get_last_modified(mod);
            std::time_t meta_time = read(mod);
            return last_modified == meta_time;
        } catch (const std::runtime_error& e) {
            std::cout << "[DEBUG] is_up_to_date error for module " << mod.name << ": " << e.what() << std::endl;
            return false;
        }
    }
}
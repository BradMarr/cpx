import compiler;

#include <expected>
#include <string_view>
#include <iostream>

#include "../include/toml++/toml.h"

int _argc;
char** _argv;

std::string_view get(int index) {
    if (index >= _argc) {
        throw std::runtime_error("Not enough arguments provided");
    } else {
        return _argv[index];
    }
}

int main(int argc, char* argv[]) {
    toml::table config = toml::parse_file(".cpx/config.toml");

    _argc = argc;
    _argv = argv;
    try {
        if (get(1) == "run") {
            compiler::run(config);
            return 0;
        } else if (get(1) == "build") {
            compiler::build(config);
            return 0;
        } else if (get(1) == "exec") {
            compiler::exec(config);
            return 0;
        } else {
            std::cerr << "Usage: " << argv[0] << " <command>\n";
            std::cerr << "Commands: run, build, exec\n";
            return 1;
        }
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
}
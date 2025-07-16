import builder.compiler;

#include <string_view>
#include <iostream>
#include <expected>

#include "../include/help.hpp"
#include "../include/toml++/toml.hpp"

int _argc;
char** _argv;

std::string_view get(int index) {
    if (index >= _argc) {
        return "";
    } else {
        return _argv[index];
    }
}

int main(int argc, char* argv[]) {
    _argc = argc;
    _argv = argv;

    if (get(1) == "run") {
        const toml::table config = toml::parse_file(".cpx/config.toml");
        if (get(2) == "clean") {
            compiler::run(config, true);
        } else {
            compiler::run(config);
        }
    } else if (get(1) == "build") {
        const toml::table config = toml::parse_file(".cpx/config.toml");
        if (get(2) == "clean") {
            compiler::build(config, true);
        } else {
            compiler::build(config);
        }
    } else if (get(1) == "exec") {
        const toml::table config = toml::parse_file(".cpx/config.toml");
        compiler::exec(config);
    } else if (get(1) == "help") {
        std::cout << help_message << std::endl;
    } else {
        std::cerr << "Usage: " << argv[0] << " <command>\n";
        std::cerr << "Please use `cpx help` for help.\n";
        return 1;
    }
}
import build.compiler;

#include <expected>
#include <string_view>
#include <iostream>

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
    toml::table config = toml::parse_file(".cpx/config.toml");

    _argc = argc;
    _argv = argv;

    if (get(1) == "run") {
        if (get(2) == "clean") {
            compiler::build(config, true);
        } else {
            compiler::build(config);
        }
    } else if (get(1) == "build") {
        if (get(2) == "clean") {
            compiler::build(config, true);
        } else {
            compiler::build(config);
        }
    } else if (get(1) == "exec") {
        compiler::exec(config);
    } else {
        std::cerr << "Usage: " << argv[0] << " <command>\n";
        std::cerr << "Commands: run, build, exec\n";
        return 1;
    }
}
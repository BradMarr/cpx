module;

#include <string>
#include <iostream>
#include <vector>
#include <unordered_map>
#include <format>
#include <fstream>
#include <regex>
#include <filesystem>

#include "../include/toml++/toml.h"

export module compiler;

struct Module {
    std::string name;
    std::string sourcepath;

    std::vector<Module*> dependencies;
    bool built = false;
};

std::unordered_map<std::string, Module> _modules;
std::string _working_dir;


std::string path(Module& mod, std::string extension) {
    if (extension == "cpp" || extension == "cppm") {
        return mod.sourcepath;
    } else if (extension.empty()) {
        return _working_dir + mod.name;
    } else {
        return _working_dir + mod.name + "." + extension;
    }
}

void cpp_compile(Module& mod) {
    std::string dep_flags;
    std::string obj_files;

    for (Module* dep : mod.dependencies) {
        dep_flags += std::format(" -fmodule-file={}={}", dep->name, path(*dep, "pcm"));
        obj_files += " " + path(*dep, "o");
    }

    std::string compile_command = std::format(
        "clang++-19 -std=c++23{} -c {} -o {}",
        dep_flags,
        mod.sourcepath,
        path(mod, "o")
    );

    std::cout << "Compiling " << mod.name << " with command: " << compile_command << std::endl;
    int result = system(compile_command.c_str());
    if (result != 0) {
        std::cerr << "Compilation failed for " << mod.name << std::endl;
        return;
    }

    std::string link_command = std::format(
        "clang++-19 -std=c++23{} {} -o {}",
        obj_files,
        path(mod, "o"),
        path(mod, "")
    );

    std::cout << "Linking " << mod.name << " with command: " << link_command << std::endl;
    result = system(link_command.c_str());
    if (result != 0) {
        std::cerr << "Linking failed for " << mod.name << std::endl;
    }
}


void cppm_compile(Module& mod) {
    std::string command = std::format(
        "clang++-19 -std=c++23"
        " -x c++-module"
        " --precompile {}"
        " -o {}",
        path(mod, "cppm"), path(mod, "pcm")
    );

    std::cout << "Compiling " << mod.name << " with command: " << command << std::endl;
    int result = system(command.c_str());
    if (result != 0) {
        std::cerr << "Compilation failed for " << mod.name << std::endl;
    }

    std::string dep_flags;
    for (Module* dep : mod.dependencies) {
        dep_flags += std::format(" -fmodule-file={}={}", dep->name, path(*dep, "pcm"));
    }

    command = std::format(
        "clang++-19 -std=c++23"
        " {}"
        " -c {}"
        " -o {}",
        dep_flags,
        path(mod, "pcm"),
        path(mod, "o")
    );

    std::cout << "Compiling " << mod.name << " with command: " << command << std::endl;
    result = system(command.c_str());
    if (result != 0) {
        std::cerr << "Compilation failed for " << mod.name << std::endl;
    }
}


void populate_dependency_graph(Module& mod) {
    std::ifstream file(mod.sourcepath);
    if (!file.is_open()) {
        std::cerr << "Failed to open " << mod.sourcepath << std::endl;
        return;
    }

    std::string line;
    std::regex import_regex(R"(^\s*import\s+([\w\.]+);)"); // regex "import modname;"
    while (std::getline(file, line)) {
        std::smatch match;
        if (std::regex_search(line, match, import_regex)) {
            std::string dep_name = match[1].str();

            auto it = _modules.find(dep_name);
            if (it == _modules.end()) {
                Module dep_mod{dep_name, "src/" + dep_name + ".cppm"};
                auto [new_it, inserted] = _modules.emplace(dep_name, std::move(dep_mod));
                it = new_it;
            }

            mod.dependencies.push_back(&it->second);

            populate_dependency_graph(it->second);
        }
    }
}

void compile_module(Module& mod) {
    if (mod.built) return;

    for (Module* dep : mod.dependencies) {
        compile_module(*dep);
    }

    if (mod.sourcepath.ends_with(".cpp")) {
        cpp_compile(mod);
    } else if (mod.sourcepath.ends_with(".cppm")) {
        cppm_compile(mod);
    } else {
        std::cerr << "Unknown source extension for module: " << mod.name << std::endl;
    }

    mod.built = true;
}

export void build(toml::table& config) {
    _working_dir = ".cpx/build/";
    std::filesystem::remove_all(_working_dir);
    std::filesystem::create_directories(_working_dir);
    Module main_module{"main", "src/main.cpp"};
    populate_dependency_graph(main_module);
    compile_module(main_module);
}

export void run(toml::table& config) {
    _working_dir = ".cpx/run/";
    std::filesystem::remove_all(_working_dir);
    std::filesystem::create_directories(_working_dir);
    Module main_module{"main", "src/main.cpp"};
    populate_dependency_graph(main_module);
    compile_module(main_module);

    std::cout << "\n -- Running Program --\n" << std::endl;
    std::string command = _working_dir + "main";

    if (auto run = config["run"].as_table()) {
        if (auto args = (*run)["args"].as_array()) {
            for (const auto& arg : *args) {
                if (auto str = arg.value<std::string>()) {
                    command += " " + *str;
                }
            }
        }
    }

    system(command.c_str());
}

export void exec(toml::table& config) {
    _working_dir = ".cpx/build/";
    std::string command = _working_dir + "main";

    if (auto run = config["exec"].as_table()) {
        if (auto args = (*run)["args"].as_array()) {
            for (const auto& arg : *args) {
                if (auto str = arg.value<std::string>()) {
                    command += " " + *str;
                }
            }
        }
    }

    system(command.c_str());
}
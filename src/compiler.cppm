module;

#include <string>
#include <iostream>
#include <vector>
#include <unordered_map>
#include <format>
#include <fstream>
#include <regex>
#include <filesystem>
#include <unordered_set>

#include "../include/toml++/toml.h"

export module compiler;

import modules;

void gather_transitive_module_flags(modules::Module& mod, std::unordered_set<std::string>& seen, std::string& flags) {
    for (modules::Module* dep : mod.dependencies) {
        if (seen.insert(dep->name).second) {
            flags += std::format(" -fmodule-file={}={}", dep->name, modules::path(*dep, "pcm").string());
            gather_transitive_module_flags(*dep, seen, flags);
        }
    }
}

void gather_transitive_object_files(modules::Module& mod, std::unordered_set<std::string>& seen, std::vector<std::string>& obj_files) {
    for (modules::Module* dep : mod.dependencies) {
        if (seen.insert(dep->name).second) {
            obj_files.push_back(modules::path(*dep, "o").string());
            gather_transitive_object_files(*dep, seen, obj_files);
        }
    }
}

void cpp_compile(modules::Module& mod) {
    std::string dep_flags;
    std::unordered_set<std::string> seen_flags;
    gather_transitive_module_flags(mod, seen_flags, dep_flags);

    std::unordered_set<std::string> seen_objs;
    std::vector<std::string> obj_files_vec;
    gather_transitive_object_files(mod, seen_objs, obj_files_vec);

    obj_files_vec.push_back(modules::path(mod, "o").string());

    std::string obj_files;
    for (const auto& o : obj_files_vec) {
        obj_files += " " + o;
    }
    
    std::string compile_command = std::format(
        "clang++-19 -std=c++23{} -c {} -o {}",
        dep_flags,
        mod.sourcepath.string(),
        modules::path(mod, "o").string()
    );

    std::cout << "Compiling " << mod.name << " with command: " << compile_command << std::endl;
    int result = system(compile_command.c_str());
    if (result != 0) {
        std::cerr << "Compilation failed for " << mod.name << std::endl;
        return;
    }
    
    std::string link_command = std::format(
        "clang++-19 -std=c++23{} -o {}",
        obj_files,
        modules::path(mod, "").string()
    );

    std::cout << "Linking " << mod.name << " with command: " << link_command << std::endl;
    result = system(link_command.c_str());
    if (result != 0) {
        std::cerr << "Linking failed for " << mod.name << std::endl;
    }
}

void cppm_compile(modules::Module& mod) {
    std::string dep_flags;
    std::unordered_set<std::string> seen;
    gather_transitive_module_flags(mod, seen, dep_flags);

    std::string command = std::format(
        "clang++-19 -std=c++23"
        " {}"
        " -x c++-module"
        " --precompile {}"
        " -o {}",
        dep_flags,
        modules::path(mod, "cppm").string(), 
        modules::path(mod, "pcm").string()
    );

    std::cout << "Precompiling " << mod.name << " with command: " << command << std::endl;
    int result = system(command.c_str());
    if (result != 0) {
        std::cerr << "Precompilation failed for " << mod.name << std::endl;
        return;
    }

    command = std::format(
        "clang++-19 -std=c++23"
        " {}"
        " -c {}"
        " -o {}",
        dep_flags,
        modules::path(mod, "pcm").string(),
        modules::path(mod, "o").string()
    );

    std::cout << "Compiling " << mod.name << " with command: " << command << std::endl;
    result = system(command.c_str());
    if (result != 0) {
        std::cerr << "Compilation failed for " << mod.name << std::endl;
    }
}

void populate_dependency_graph(modules::Module& mod) {
    std::ifstream file(mod.sourcepath);
    if (!file.is_open()) {
        std::cerr << "Failed to open " << mod.sourcepath << std::endl;
        return;
    }

    std::string line;
    std::regex import_regex(R"(^\s*import\s+([\w\.]+);)"); 
    while (std::getline(file, line)) {
        std::smatch match;
        if (std::regex_search(line, match, import_regex)) {
            std::string dep_name = match[1].str();

            auto it = modules::map.find(dep_name);
            if (it == modules::map.end()) {
                modules::Module dep_mod{dep_name, 
                    std::filesystem::path("src") / (dep_name + ".cppm")
                };
                auto [new_it, inserted] = modules::map.emplace(dep_name, std::move(dep_mod));
                it = new_it;
            }

            mod.dependencies.push_back(&it->second);

            populate_dependency_graph(it->second);
        }
    }
}

void compile_module(modules::Module& mod) {
    if (mod.built) return;

    for (modules::Module* dep : mod.dependencies) {
        compile_module(*dep);
    }

    if (mod.sourcepath.extension() == ".cpp") {
        cpp_compile(mod);
    } else if (mod.sourcepath.extension() == ".cppm") {
        cppm_compile(mod);
    } else {
        std::cerr << "Unknown source extension for module: " << mod.name << std::endl;
    }

    mod.built = true;
}

export namespace compiler {
    void build(toml::table& config) {
        modules::working_dir = std::filesystem::path(".cpx") / "build";
        std::filesystem::remove_all(modules::working_dir);
        std::filesystem::create_directories(modules::working_dir);
        modules::Module main_module{"main", "src/main.cpp"};
        populate_dependency_graph(main_module);
        compile_module(main_module);
    }
    
    void run(toml::table& config) {
        modules::working_dir = std::filesystem::path(".cpx") / "run";
        std::filesystem::remove_all(modules::working_dir);
        std::filesystem::create_directories(modules::working_dir);
        modules::Module main_module{"main", "src/main.cpp"};
        populate_dependency_graph(main_module);
        compile_module(main_module);
    
        std::cout << "\n -- Running Program --\n" << std::endl;
        std::string command = modules::working_dir / "main";
    
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
    
    void exec(toml::table& config) {
        modules::working_dir = std::filesystem::path(".cpx") / "build";
        std::string command = modules::working_dir / "main";
    
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
}

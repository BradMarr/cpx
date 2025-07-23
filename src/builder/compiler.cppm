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

#include "../../include/toml++/toml.hpp"

export module builder.compiler;

import builder.modules;
import builder.meta;

constexpr std::string_view BASE_COMMAND = "clang++-19 -std=c++23";

void gather_transitive_module_flags(const modules::Module& mod, std::unordered_set<std::string>& seen, std::string& flags) {
    for (const modules::Module* dep : mod.dependencies) {
        if (seen.insert(dep->name).second) {
            flags += std::format(" -fmodule-file={}={}", dep->name, dep->path("pcm").string());
            gather_transitive_module_flags(*dep, seen, flags);
        }
    }
}

void gather_transitive_object_files(const modules::Module& mod, std::unordered_set<std::string>& seen, std::vector<std::string>& obj_files) {
    for (const modules::Module* dep : mod.dependencies) {
        if (seen.insert(dep->name).second) {
            obj_files.push_back(dep->path("o").string());
            gather_transitive_object_files(*dep, seen, obj_files);
        }
    }
}

void cpp_compile(const modules::Module& mod) {
    std::string dep_flags;
    std::unordered_set<std::string> seen_flags;
    gather_transitive_module_flags(mod, seen_flags, dep_flags);

    std::unordered_set<std::string> seen_objs;
    std::vector<std::string> obj_files_vec;
    gather_transitive_object_files(mod, seen_objs, obj_files_vec);

    obj_files_vec.push_back(mod.path("o").string());

    std::string obj_files;
    for (const auto& o : obj_files_vec) {
        obj_files += " " + o;
    }
    
    std::string compile_command = std::format(
        "{} -std=c++23 {} -c {} -o {}",
        BASE_COMMAND,
        dep_flags,
        mod.sourcepath.string(),
        mod.path("o").string()
    );

    std::cout << "Compiling " << mod.name << std::endl;
    int result = system(compile_command.c_str());
    if (result != 0) {
        std::cerr << "Compilation failed for " << mod.name << std::endl;
        return;
    }
    
    std::string link_command = std::format(
        "{} -std=c++23 {} -o {}",
        BASE_COMMAND,
        obj_files,
        mod.path("").string()
    );

    std::cout << "Linking " << mod.name << std::endl;
    result = system(link_command.c_str());
    if (result != 0) {
        std::cerr << "Linking failed for " << mod.name << std::endl;
    }
}

void cppm_compile(const modules::Module& mod) {
    std::string dep_flags;
    std::unordered_set<std::string> seen;
    gather_transitive_module_flags(mod, seen, dep_flags);

    std::string command = std::format(
        "{} {} -x c++-module --precompile {} -o {}",
        BASE_COMMAND,
        dep_flags,
        mod.path("cppm").string(), 
        mod.path("pcm").string()
    );

    std::cout << "Precompiling module " << mod.name << std::endl;
    int result = system(command.c_str());
    if (result != 0) {
        std::cerr << "Precompilation failed for " << mod.name << std::endl;
        return;
    }

    command = std::format(
        "{} {} -c {} -o {}",
        BASE_COMMAND,
        dep_flags,
        mod.path("pcm").string(),
        mod.path("o").string()
    );

    std::cout << "Compiling module " << mod.name << std::endl;
    result = system(command.c_str());
    if (result != 0) {
        std::cerr << "Compilation failed for " << mod.name << std::endl;
    }
}

bool populate_dependency_graph(modules::Module& mod) {
    std::ifstream file(mod.sourcepath);
    if (!file.is_open()) {
        std::cerr << "Failed to open " << mod.sourcepath << std::endl;
        exit(1);
    }

    std::string line;
    std::regex import_regex(R"(^\s*import\s+([\w\.]+);)");
    bool all_deps_built = true;

    while (std::getline(file, line)) {
        std::smatch match;
        if (std::regex_search(line, match, import_regex)) {
            std::string dep_name = match[1].str();

            if (dep_name == "std" || dep_name.starts_with("std.")) {
                continue;
            }

            auto it = modules::map.find(dep_name);
            if (it == modules::map.end()) {
                std::filesystem::path source_path = "src";
                std::stringstream ss(dep_name);
                std::string segment;

                while (std::getline(ss, segment, '.')) {
                    source_path /= segment;
                }
                source_path += ".cppm";

                modules::Module dep_mod{dep_name, source_path};
                auto [new_it, inserted] = modules::map.emplace(dep_name, std::move(dep_mod));
                it = new_it;
            }

            mod.dependencies.push_back(&it->second);

            bool dep_built = populate_dependency_graph(it->second);
            if (!dep_built) all_deps_built = false;
        }
    }

    mod.built = all_deps_built && meta::is_up_to_date(mod);
    return mod.built;
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
    meta::update(mod);
}

void general_build(const toml::table& config, bool to_clean = false) {
    if (to_clean) {
        std::filesystem::remove_all(modules::working_dir);
    }

    std::filesystem::create_directories(modules::working_dir);

    modules::Module main_module{"main", "src/main.cpp"};
    populate_dependency_graph(main_module);
    compile_module(main_module);
}

std::string get_args(const toml::table& config, const std::string_view section) {
    std::string args;
    if (const toml::table* table = config[section].as_table()) {
        if (const toml::array* arr = (*table)["args"].as_array()) {
            for (const auto& arg : *arr) {
                if (auto str = arg.value<std::string>()) {
                    args += " " + *str;
                }
            }
        }
    }
    return args;
}


export namespace compiler {
    void build(const toml::table& config, bool to_clean = false) {
        modules::working_dir = std::filesystem::path(".cpx") / "build";
        general_build(config, to_clean);
        std::cout << "\n -- Build Complete --\n" << std::endl;
    }

    void rebuild(const toml::table& config, const std::string_view module_name) {
        modules::working_dir = std::filesystem::path(".cpx") / "build";
        modules::Module target((std::basic_string(module_name)));

        std::filesystem::remove(target.path("meta"));

        general_build(config);

        std::cout << "\n -- Rebuild Complete --\n" << std::endl;
    }
    
    void run(const toml::table& config, bool to_clean = false) {
        modules::working_dir = std::filesystem::path(".cpx") / "run";
        general_build(config, to_clean);

        std::cout << "\n -- Running Program --\n" << std::endl;

        const std::string command = (modules::working_dir / "main").string() 
            + get_args(config, "run");
        system(command.c_str());
    }
    
    void exec(const toml::table& config) {
        modules::working_dir = std::filesystem::path(".cpx") / "build";
        const std::string command = (modules::working_dir / "main").string() 
            + get_args(config, "exec");
        system(command.c_str());
    }
}

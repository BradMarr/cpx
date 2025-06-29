module;

#include <filesystem>
#include <string>
#include <vector>
#include <unordered_map>

export module modules;

export namespace modules {
    struct Module {
        std::string name;
        std::filesystem::path sourcepath;
    
        std::vector<Module*> dependencies;
        bool built = false;
    };

    inline std::unordered_map<std::string, Module> map;
    inline std::filesystem::path working_dir;
    
    std::filesystem::path path(Module& mod, std::string extension) {
        if (extension == "cpp" || extension == "cppm") {
            return mod.sourcepath;
        } else if (extension.empty()) {
            return working_dir / mod.name;
        } else {
            return working_dir / (mod.name + "." + extension);
        }
    }
}
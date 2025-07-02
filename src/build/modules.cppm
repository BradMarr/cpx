module;

#include <filesystem>
#include <string>
#include <vector>
#include <unordered_map>

export module build.modules;

export namespace modules {
    inline std::filesystem::path working_dir;

    class Module {
        public:
        std::string name;
        std::filesystem::path sourcepath;
    
        std::time_t last_modified;
        std::vector<Module*> dependencies;
        bool built = false;

        const std::filesystem::path path(std::string extension) {
            if (extension == "cpp" || extension == "cppm") {
                return sourcepath;
            } else if (extension.empty()) {
                return working_dir / name;
            } else {
                return working_dir / (name + "." + extension);
            }
        }
    };

    inline std::unordered_map<std::string, Module> map;
}
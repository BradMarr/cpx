#pragma once

#include <string>

constexpr std::string_view help_message = 
R"(Usage:
  cpx <command> [arguments]

Commands:
  run [clean]
    Build the project and run it.
    Arguments:
      clean   Optional. Force rebuild of all modules.

  build [clean]
    Build the project, only compiling changed files.
    Arguments:
      clean   Optional. Force rebuild of all modules.

  rebuild [module]
    Force compiles the module requested.
    Arguments:
      module  Mandatory. The module name.

  exec
    Run compiled executable from build.)";
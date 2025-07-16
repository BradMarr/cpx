#pragma once

#include <string>

constexpr std::string_view help_message = R"(
These are the available commmands:
[subcommands] are optional
`cpx run`
  -> builds the the project and runs it
`cpx build [clean]`
  -> builds the project (only files that were changed)
  -> [clean] rebuilds all modules forcefully
`cpx exec`
  -> runs the script defined in /.cpx/config.toml
        )";
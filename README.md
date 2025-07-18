# CPX

C++ Package Exchange - Cargo Like Package Manager & Build System Using Modules

## Getting started

On Debian based distros, install with:

```bash
curl -sSL https://cpxsh.bmarr.com/ | sh
```

On other systems... feel free to contribute!

## Version Structure

The build system uses a `v<MAJOR>.<MINOR>.<PATCH>` format (e.g., `v1.2.3`):
- **`MAJOR`**: Backwards-incompatible changes to inputs (e.g., build script syntax, CLI flags, config formats) or outputs (e.g., artifact structure). These will likely break build scripts or integrations.
- **`MINOR`**: Changes to printed output (e.g., log message format, new console output fields) that may affect scripts parsing output but do not alter inputs or artifact structure.
- **`PATCH`**: Internal changes (e.g., bug fixes, optimizations) with no impact on inputs, printed output, or external behavior.

## To Contribute

Simply run the install script, clone, then build with CPX!
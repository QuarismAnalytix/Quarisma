---
type: "always_apply"
---

Do not modify any code or CMake files within third-party submodules or external dependencies. This includes:

- Any files within third-party submodule directories (e.g., directories managed by git submodules)
- CMakeLists.txt files that belong to third-party libraries
- Source code (.cpp, .h, .cxx, .hpp files) in third-party dependencies
- Configuration files within third-party library directories
- Build scripts or setup files that are part of external dependencies

Only modify Quarisma project files (files within the Core/, Library/, Scripts/, Tests/ directories and project-level CMake files). If integration with third-party libraries needs adjustment, make changes only to Quarisma's own CMake configuration files or wrapper code, not to the third-party library files themselves.

If a third-party library has issues or needs modifications, document the issue and suggest alternative approaches such as:
- Adjusting Quarisma's CMake configuration to work with the library as-is
- Creating wrapper classes or adapter code in Quarisma
- Suggesting a different version or alternative library
- Reporting the issue upstream to the library maintainers
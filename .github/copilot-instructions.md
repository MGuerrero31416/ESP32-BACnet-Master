## Critical change-scope rules

- Do not flash, erase NVS, run fullclean, or delete the build folder unless explicitly requested.

## Mandatory build environment

Build only with:

- ESP-IDF v5.5.4
- Arduino-ESP32 3.3.10

If the build environment is incorrect, stop and report it. Do not change source code,
dependencies, CMake files, version constraints, or the target to make the build proceed.
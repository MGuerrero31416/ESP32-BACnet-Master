## Critical change-scope rules

- Do not flash, erase NVS, run fullclean, or delete the build folder unless explicitly requested.

## Mandatory build environment

Build only with:

- ESP-IDF v5.5.4
- `C:\esp\v5.5.4\esp-idf`
- Target `esp32s3`
- Arduino-ESP32 3.3.10

Never run `idf.py`, CMake, or Ninja directly. Build only with:

`powershell -NoProfile -ExecutionPolicy Bypass -File .\tools\build_idf55.ps1 build`

If the build environment is incorrect, stop and report it. Do not change source code,
dependencies, CMake files, version constraints, or the target to make the build proceed.
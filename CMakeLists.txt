# The following lines of boilerplate have to be in your project's
# CMakeLists in this exact order for cmake to work correctly
cmake_minimum_required(VERSION 3.16)

set(EXCLUDE_COMPONENTS TFT_eSPI)

include($ENV{IDF_PATH}/tools/cmake/project.cmake)
# "Trim" the build. Include the minimal set of components, main, and anything it depends on.
#idf_build_set_property(MINIMAL_BUILD ON)
project(ESP32-BACnet-Master)

# GCC 15 in ESP-IDF 6.0 promotes this warning to error in third-party Arduino code.
# Keep suppression narrow to the managed component to avoid affecting project sources.
if(TARGET __idf_espressif__arduino-esp32)
	target_compile_options(
		__idf_espressif__arduino-esp32
		PRIVATE
			-Wno-error=missing-field-initializers
			-Wno-error=cpp)
endif()

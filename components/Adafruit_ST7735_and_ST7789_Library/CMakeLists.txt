cmake_minimum_required(VERSION 3.5)

idf_component_register(SRCS "Adafruit_ST7796S.cpp" "Adafruit_ST77xx.cpp" "Adafruit_ST7735.cpp" "Adafruit_ST7789.cpp" 
                       INCLUDE_DIRS "."
                       REQUIRES arduino-esp32 Adafruit_GFX_Library)

project(Adafruit-ST7735-Library)

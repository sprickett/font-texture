cmake_minimum_required(VERSION 3.00)

project(FontFun)

set(CMAKE_CXX_STANDARD 14)

find_package(Freetype )

include_directories(
    include
)
link_directories(
    win64
    win32
)

add_subdirectory(distance-field)

add_executable( font_fun
    src/main.cpp
	src/pixmap.hpp
	src/bitmap.hpp
	src/box_packing.hpp
	src/box_packing.cpp
)
target_link_libraries( font_fun
    freetype
)


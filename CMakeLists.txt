cmake_minimum_required(VERSION 3.16)
project(DrumBox VERSION 1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)

find_package(Qt6 REQUIRED COMPONENTS Core Gui Widgets)

# PipeWire for audio (low-latency, no Qt Multimedia needed)
find_package(PkgConfig REQUIRED)
pkg_check_modules(PIPEWIRE REQUIRED libpipewire-0.3)

add_executable(DrumBox
    main.cpp
    mainwindow.h
    mainwindow.cpp
    drumengine.h
    drumengine.cpp
)

target_include_directories(DrumBox PRIVATE ${PIPEWIRE_INCLUDE_DIRS})
target_compile_options(DrumBox PRIVATE ${PIPEWIRE_CFLAGS_OTHER})

target_link_libraries(DrumBox PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Widgets
    ${PIPEWIRE_LIBRARIES}
)

set_target_properties(DrumBox PROPERTIES
    WIN32_EXECUTABLE TRUE
    MACOSX_BUNDLE    TRUE
)

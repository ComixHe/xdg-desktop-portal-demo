cmake_minimum_required(VERSION 3.25)
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_AUTOMOC ON)

project(demo LANGUAGES CXX C)

# build dep

find_package(Qt6 CONFIG REQUIRED COMPONENTS Core DBus Widgets WaylandClient)
if (${Qt6_VERSION} VERSION_GREATER_EQUAL 6.10.0)
    find_package(Qt6 REQUIRED COMPONENTS GuiPrivate)
endif()
find_package(Wayland 1.23 REQUIRED COMPONENTS Client)
find_package(PkgConfig REQUIRED)
pkg_check_modules(WaylandProtocols REQUIRED wayland-protocols>=1.36)
pkg_check_modules(XDGDesktopPortal REQUIRED xdg-desktop-portal>=1.18.4)
pkg_check_modules(GSTREAMER REQUIRED IMPORTED_TARGET gstreamer-1.0)

add_subdirectory(src)

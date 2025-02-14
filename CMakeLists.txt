cmake_minimum_required(VERSION 3.25)
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_AUTOMOC ON)

project(demo LANGUAGES CXX C)

# build dep

find_package(Qt6 6.6 CONFIG REQUIRED COMPONENTS Core DBus Widgets WaylandClient)
find_package(Wayland 1.23 REQUIRED COMPONENTS Client)
find_package(PkgConfig REQUIRED)
pkg_check_modules(WaylandProtocols REQUIRED wayland-protocols>=1.36)
pkg_check_modules(XDGDesktopPortal REQUIRED xdg-desktop-portal>=1.18.4)

add_subdirectory(src)

cmake_minimum_required(VERSION 3.20)
project(TShorg CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# vcpkg paketlerini bul
find_package(glfw3 CONFIG REQUIRED)
find_package(glad CONFIG REQUIRED)
find_package(imgui CONFIG REQUIRED)

# 1. Main App Target (TShorg GUI) - WIN32 bayrağı ile konsol penceresi engellendi
set(SOURCES
    src/main.cpp
    src/TSGS.cpp
    src/TSGS.h
)

add_executable(TShorg WIN32 ${SOURCES})

# Windows release derlemelerinde konsolun kesin olarak açılmasını engeller
if(MSVC)
    set_target_properties(TShorg PROPERTIES LINK_FLAGS "/SUBSYSTEM:WINDOWS /ENTRY:mainCRTStartup")
endif()

target_include_directories(TShorg PRIVATE
    ${imgui_INCLUDE_DIRS}
)

target_link_libraries(TShorg PRIVATE
    glfw
    glad::glad
    imgui::imgui
    Shell32
    User32
    Advapi32
)

# 2. CLI Mod Derleyici Target (TShorgModCompiler)
add_executable(TShorgModCompiler src/TShorgModCompiler.cpp)
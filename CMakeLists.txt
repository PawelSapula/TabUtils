cmake_minimum_required(VERSION 3.20)
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

project(TabUtils)

include(FetchContent)

FetchContent_Declare(ftxui
  GIT_REPOSITORY https://github.com/ArthurSonzogni/FTXUI
  GIT_TAG v7.0.3  # Replace with a version, tag, or commit hash
)
 
FetchContent_MakeAvailable(ftxui)
 
add_executable(TabUtils main.cpp
        type_utils.cpp
        type_utils.h
        CDevice.h
        CManager.cpp
        CManager.h)

if(APPLE)
    find_library(COREFOUNDATION_FRAMEWORK CoreFoundation REQUIRED)
    find_library(IOKIT_FRAMEWORK IOKit REQUIRED)

    target_link_libraries(TabUtils PRIVATE
            ${COREFOUNDATION_FRAMEWORK}
            ${IOKIT_FRAMEWORK}
    )
endif()

target_link_libraries(TabUtils
  PRIVATE ftxui::screen
  PRIVATE ftxui::dom
  PRIVATE ftxui::component
)

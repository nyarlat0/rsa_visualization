cmake_minimum_required(VERSION 3.16)
project(rsa_visualization LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

find_package(Qt6 REQUIRED COMPONENTS Widgets Concurrent)
qt_standard_project_setup()

find_package(OpenSSL REQUIRED)

qt_add_executable(${PROJECT_NAME}
    src/main.cpp
    src/main_widget.cpp
    src/encrypt_widget.cpp
    src/decrypt_widget.cpp
    src/rsa.cpp
    src/helpers.cpp
    include/main_widget.h
    include/encrypt_widget.h
    include/decrypt_widget.h
    src/mod_circle_widget.cpp
)

target_include_directories(${PROJECT_NAME} PRIVATE include)
target_link_libraries(${PROJECT_NAME}
  PRIVATE
    Qt6::Widgets
    Qt6::Concurrent
    OpenSSL::Crypto
)

add_custom_target(
  run
  COMMAND $<TARGET_FILE:${PROJECT_NAME}>
  DEPENDS ${PROJECT_NAME}
  WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
)

include(GNUInstallDirs)

install(TARGETS ${PROJECT_NAME}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

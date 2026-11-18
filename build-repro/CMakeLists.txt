cmake_minimum_required(VERSION 3.16)
project(blz_repro CXX)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOUIC_SEARCH_PATHS ${SRC}/mac-tool-src/dialog)
find_package(Qt6 REQUIRED COMPONENTS Core Gui Widgets)
add_executable(blz_repro
    ${SRC}/core/tests/repro_t15.cpp
    ${SRC}/core/grid_projection.cpp
    ${SRC}/core/fov_raycast.cpp
    ${SRC}/mac-tool-src/mainwindow/fov_layer.cpp
    ${SRC}/mac-tool-src/items/GraphicsType.cpp
    ${SRC}/mac-tool-src/dialog/attributeAddDialog.cpp)
target_include_directories(blz_repro PRIVATE ${SRC}/core ${SRC}/mac-tool-src ${SRC}/mac-tool-src/mainwindow ${SRC}/mac-tool-src/items ${SRC}/mac-tool-src/dialog)
target_link_libraries(blz_repro Qt6::Core Qt6::Gui Qt6::Widgets)

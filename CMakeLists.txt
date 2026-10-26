cmake_minimum_required(VERSION 3.21)
project(SchoolDSA VERSION 1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_BUILD_TYPE Debug)
endif()

find_package(Qt6 6.5 REQUIRED COMPONENTS Widgets)
qt_standard_project_setup()

# 纯 C++ 核心库：不链接 Qt，可单独编译与测试。
add_library(ds_core STATIC
    core/src/dijkstra.cpp
    core/src/truck_yard.cpp
    core/src/bike_system.cpp
)
target_include_directories(ds_core PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/core/include)
target_compile_options(ds_core PRIVATE -Wall -Wextra)
set_target_properties(ds_core PROPERTIES AUTOMOC OFF AUTOUIC OFF AUTORCC OFF)

# 题目一：快递中转栈-队列混合调度模拟
qt_add_executable(truck_sim WIN32
    apps/truck/main.cpp
    apps/truck/main_window.cpp
    apps/truck/yard_canvas.cpp
)
target_link_libraries(truck_sim PRIVATE ds_core Qt6::Widgets)
target_include_directories(truck_sim PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/apps)

# 题目二：城市公共自行车调度路径规划系统
qt_add_executable(bike_sim WIN32
    apps/bike/main.cpp
    apps/bike/main_window.cpp
    apps/bike/map_canvas.cpp
)
target_link_libraries(bike_sim PRIVATE ds_core Qt6::Widgets)
target_include_directories(bike_sim PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/apps)

enable_testing()
add_executable(ds_tests tests/test_main.cpp)
target_link_libraries(ds_tests PRIVATE ds_core)
target_compile_options(ds_tests PRIVATE -Wall -Wextra)
add_test(NAME ds_tests COMMAND ds_tests)

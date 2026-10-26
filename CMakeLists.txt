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
# 两个程序的界面类不使用 Q_OBJECT（信号改成 std::function 回调），
# 因此关掉 AutoMoc，避免多余的 moc 预处理步骤。
set(CMAKE_AUTOMOC OFF)
set(CMAKE_AUTOUIC OFF)

# 纯 C++ 核心库：不链接 Qt，可单独编译与测试。
add_library(ds_core STATIC
    core/src/dijkstra.cpp
    core/src/truck_yard.cpp
    core/src/bike_system.cpp
)
target_include_directories(ds_core PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/core/include)
target_compile_options(ds_core PRIVATE -Wall -Wextra)
set_target_properties(ds_core PROPERTIES AUTOMOC OFF AUTOUIC OFF AUTORCC OFF)

# 共用界面层：Endfield 风格的主题、按钮、面板和度量板。
add_library(ds_ui STATIC
    apps/common/theme.cpp
    apps/common/widgets.cpp
)
target_include_directories(ds_ui PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/apps)
target_link_libraries(ds_ui PUBLIC Qt6::Widgets)
target_compile_options(ds_ui PRIVATE -Wall -Wextra)
set_target_properties(ds_ui PROPERTIES AUTOMOC OFF AUTOUIC OFF AUTORCC OFF)

# Saira 静态实例：拉丁字（宽体），中文由系统黑体回退。
set(UI_FONTS
    apps/common/fonts/Saira-Wide-SemiBold.ttf
    apps/common/fonts/Saira-Field.ttf
    apps/common/fonts/Saira-Bold.ttf
    apps/common/fonts/Saira-Wide-ExtraBold.ttf
    apps/common/fonts/Saira-OFL.txt
)

# 基础题 2：快递中转栈-队列混合调度模拟
qt_add_executable(truck_sim WIN32
    apps/truck/main.cpp
    apps/truck/main_window.cpp
    apps/truck/yard_canvas.cpp
)
target_link_libraries(truck_sim PRIVATE ds_core ds_ui Qt6::Widgets)
target_include_directories(truck_sim PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/apps)
target_compile_options(truck_sim PRIVATE -Wall -Wextra)
set_target_properties(truck_sim PROPERTIES AUTOMOC OFF AUTOUIC OFF AUTORCC OFF)
qt_add_resources(truck_sim "truck_fonts"
    PREFIX "/fonts"
    BASE ${CMAKE_CURRENT_SOURCE_DIR}/apps/common/fonts
    FILES ${UI_FONTS})

# 扩展题 7：城市公共自行车调度路径规划系统
qt_add_executable(bike_sim WIN32
    apps/bike/main.cpp
    apps/bike/main_window.cpp
    apps/bike/map_canvas.cpp
)
target_link_libraries(bike_sim PRIVATE ds_core ds_ui Qt6::Widgets)
target_include_directories(bike_sim PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/apps)
target_compile_options(bike_sim PRIVATE -Wall -Wextra)
set_target_properties(bike_sim PROPERTIES AUTOMOC OFF AUTOUIC OFF AUTORCC OFF)
qt_add_resources(bike_sim "bike_fonts"
    PREFIX "/fonts"
    BASE ${CMAKE_CURRENT_SOURCE_DIR}/apps/common/fonts
    FILES ${UI_FONTS})

enable_testing()
add_executable(ds_tests tests/test_main.cpp)
target_link_libraries(ds_tests PRIVATE ds_core)
target_compile_options(ds_tests PRIVATE -Wall -Wextra)
add_test(NAME ds_tests COMMAND ds_tests)

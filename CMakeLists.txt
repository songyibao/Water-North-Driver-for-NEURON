cmake_minimum_required(VERSION 3.28)
project(plugin-water LANGUAGES C CXX)

set(NEURON_SDK_PATH /projects/neuron-sdk-2.10.2)

# 设置 C/C++ 标准
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 11)

find_package(yyjson CONFIG REQUIRED)

# 头文件和库路径
include_directories(
        /usr/local/include
        ${NEURON_SDK_PATH}/include/neuron
        logger
)

link_directories(
        /usr/local/lib
        ${NEURON_SDK_PATH}/lib
)

# 源文件列表（仅包含需要编译的 .c/.cpp 文件）
add_library(${PROJECT_NAME} SHARED
        internal_api/update_interval.cpp
        mqtt_config.cpp
        mqtt_handle.cpp
        mqtt_plugin.cpp
        mqtt_plugin_intf.cpp  # 确保此文件包含 mqtt_plugin_open 实现
        json_transform.cpp
        server.cpp
)

# 链接依赖库
target_link_libraries(${PROJECT_NAME}
        neuron-base
        yyjson::yyjson
)
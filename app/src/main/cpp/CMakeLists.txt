cmake_minimum_required(VERSION 3.4.1)

add_library( # 函数库名称
        native-lib
        # 动态库类型
        SHARED
        # 源文件
        native-lib.cpp)

find_library( # 日志库
        log-lib
        log )

find_library( # JNI 图形库
        jnigraphics-lib
        jnigraphics )

# 设置头文件搜索路径
include_directories(include)

# 通过设置编译选项, 设置函数库的搜索路径
# 此处的 ANDROID_ABI 是在
# build.gradle android->defaultConfig->externalNativeBuild->cmake
# 下的 abiFilters 中设置
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -L${CMAKE_SOURCE_DIR}/libs/${ANDROID_ABI}")

target_link_libraries( # 链接动态库
        native-lib

        # 查找到的 libturbojpeg 静态库
        turbojpeg

        ${log-lib}
        ${jnigraphics-lib} )
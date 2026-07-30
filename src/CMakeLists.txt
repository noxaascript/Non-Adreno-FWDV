cmake_minimum_required(VERSION 3.18)
project(fexdxvk_wrapper VERSION 1.0.0 LANGUAGES C CXX)

# ============================================================
# FexDXVK NonAdreno — Build System
# Targets:
#   libfexdxvk_wrapper.so   — Android/Linux wrapper library
#   libVkLayer_fexdxvk.so   — Vulkan implicit layer
#
# Requirements:
#   - Android NDK r25+ (for Android) OR clang 14+ (for Linux)
#   - Vulkan SDK (vulkan/vulkan.h, vulkan/vk_layer.h)
#   - pthreads (included in NDK / Linux libc)
#
# Android NDK build:
#   cmake -DCMAKE_TOOLCHAIN_FILE=$NDK/build/cmake/android.toolchain.cmake \
#         -DANDROID_ABI=arm64-v8a \
#         -DANDROID_PLATFORM=android-28 \
#         -DCMAKE_BUILD_TYPE=Release \
#         -B build/android && cmake --build build/android
#
# Linux ARM64 cross-compile:
#   cmake -DCMAKE_C_COMPILER=aarch64-linux-gnu-gcc \
#         -DCMAKE_BUILD_TYPE=Release \
#         -B build/linux && cmake --build build/linux
# ============================================================

set(CMAKE_C_STANDARD   11)
set(CMAKE_CXX_STANDARD 17)

# ---- Locate Vulkan headers ----
find_package(Vulkan QUIET)
if (NOT Vulkan_FOUND)
    # NDK bundles Vulkan headers; try well-known paths
    find_path(VULKAN_INCLUDE_DIR vulkan/vulkan.h
        PATHS
            $ENV{VULKAN_SDK}/include
            $ENV{NDK_HOME}/toolchains/llvm/prebuilt/linux-x86_64/sysroot/usr/include
            /usr/include
    )
    if (VULKAN_INCLUDE_DIR)
        set(Vulkan_INCLUDE_DIRS ${VULKAN_INCLUDE_DIR})
        message(STATUS "Vulkan headers: ${VULKAN_INCLUDE_DIR}")
    else()
        message(FATAL_ERROR "Vulkan headers not found. Set VULKAN_SDK or install vulkan-headers.")
    endif()
endif()

# ============================================================
# Target 1: libfexdxvk_wrapper.so
# ============================================================
add_library(fexdxvk_wrapper SHARED
    wrapper/fexdxvk_wrapper.c
    wrapper/gpu_detect.c
    wrapper/thermal.c
    wrapper/cpu_sched.c
    wrapper/memory.c
    wrapper/monitor.c
)

target_include_directories(fexdxvk_wrapper PRIVATE
    wrapper/
    ${Vulkan_INCLUDE_DIRS}
)

target_compile_options(fexdxvk_wrapper PRIVATE
    -Wall -Wextra -O2
    -fvisibility=hidden     # only exported symbols visible
    -fPIC
)

target_compile_definitions(fexdxvk_wrapper PRIVATE
    FEXDXVK_VERSION="1.0.0"
    FEXDXVK_FEXCORE="2607"
    FEXDXVK_DXVK="3.0.2"
)

if (ANDROID)
    # pthreads are part of libc on Android — no separate -lpthread
    target_link_libraries(fexdxvk_wrapper PRIVATE log)
else()
    target_link_libraries(fexdxvk_wrapper PRIVATE pthread)
endif()

# Ensure the output is named libfexdxvk_wrapper.so
set_target_properties(fexdxvk_wrapper PROPERTIES
    OUTPUT_NAME "fexdxvk_wrapper"
    PREFIX      "lib"
    SUFFIX      ".so"
)

# ============================================================
# Target 2: libVkLayer_fexdxvk.so  (Vulkan implicit layer)
# ============================================================
add_library(VkLayer_fexdxvk SHARED
    vulkan_layer/vk_layer_fexdxvk.c
    vulkan_layer/pipeline_cache.c
    vulkan_layer/queue_optimizer.c
    vulkan_layer/frame_pacing.c
    vulkan_layer/cmd_buffer.c
)

target_include_directories(VkLayer_fexdxvk PRIVATE
    vulkan_layer/
    ${Vulkan_INCLUDE_DIRS}
)

target_compile_options(VkLayer_fexdxvk PRIVATE
    -Wall -Wextra -O2
    -fvisibility=hidden
    -fPIC
)

if (ANDROID)
    # pthreads are part of libc on Android — no separate -lpthread
    target_link_libraries(VkLayer_fexdxvk PRIVATE log)
else()
    target_link_libraries(VkLayer_fexdxvk PRIVATE pthread)
endif()

set_target_properties(VkLayer_fexdxvk PROPERTIES
    OUTPUT_NAME "VkLayer_fexdxvk"
    PREFIX      "lib"
    SUFFIX      ".so"
)

# ============================================================
# Install targets
# ============================================================
include(GNUInstallDirs)

install(TARGETS fexdxvk_wrapper VkLayer_fexdxvk
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
)

# Install the Vulkan layer JSON manifest alongside the .so
install(FILES
    ${CMAKE_SOURCE_DIR}/../layer/VkLayer_fexdxvk.json
    DESTINATION ${CMAKE_INSTALL_DATADIR}/vulkan/implicit_layer.d
)

# ============================================================
# Post-build: copy built .so files to prebuilt/ for packaging
# ============================================================
add_custom_command(TARGET fexdxvk_wrapper POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy
        $<TARGET_FILE:fexdxvk_wrapper>
        ${CMAKE_SOURCE_DIR}/../prebuilt/wrapper/
    COMMENT "Copying libfexdxvk_wrapper.so to prebuilt/"
)

add_custom_command(TARGET VkLayer_fexdxvk POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy
        $<TARGET_FILE:VkLayer_fexdxvk>
        ${CMAKE_SOURCE_DIR}/../prebuilt/vulkan_layer/
    COMMENT "Copying libVkLayer_fexdxvk.so to prebuilt/"
)

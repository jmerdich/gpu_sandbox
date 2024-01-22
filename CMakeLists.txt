cmake_minimum_required(VERSION 3.16)

project(gpu_sandbox)

# Deps
set(VULKAN_HPP_SAMPLES_BUILD_ONLY_DYNAMIC ON)
add_subdirectory(extern/vulkan-hpp)

add_executable(copy_benches)
target_sources(copy_benches PRIVATE src/copy_benches.cpp)

find_package(Vulkan REQUIRED)

target_link_libraries(copy_benches PRIVATE Vulkan::Vulkan)

vulkan_hpp__setup_platform(NAME copy_benches)
vulkan_hpp__setup_vulkan_include(NAME copy_benches)
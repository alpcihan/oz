set(BEE_LIB_NAME "bee" CACHE STRING "Bee Library Name")

find_package(Vulkan REQUIRED)

file(GLOB_RECURSE BEE_LIB_SOURCES "./*.cpp")
add_library(${BEE_LIB_NAME} ${BEE_LIB_SOURCES})

target_include_directories(${BEE_LIB_NAME}
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
    PRIVATE
        Vulkan::Vulkan
)

target_link_libraries(${BEE_LIB_NAME} 
    PUBLIC
        Vulkan::Vulkan
        glfw
        glm
)
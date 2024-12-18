set(OZ_LIB_NAME "oz" CACHE STRING "oz Library Name")

find_package(Vulkan REQUIRED)

file(GLOB_RECURSE OZ_LIB_SOURCES "./*.cpp")
add_library(${OZ_LIB_NAME} ${OZ_LIB_SOURCES})

target_include_directories(${OZ_LIB_NAME}
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
    PRIVATE
        Vulkan::Vulkan
)

target_link_libraries(${OZ_LIB_NAME} 
    PUBLIC
        Vulkan::Vulkan
        glfw
        glm
)

add_dependencies(${OZ_LIB_NAME} OZ_SHADERS)

add_executable(main main.cpp)
target_link_libraries(main ${OZ_LIB_NAME})
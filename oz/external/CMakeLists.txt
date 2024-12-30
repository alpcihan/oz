#################################################
# vulkan
#################################################
find_package(Vulkan REQUIRED)

#################################################
# glm
#################################################
if(NOT(DEFINED GLM_ROOT_DIR))
set(GLM_ROOT_DIR ${CMAKE_CURRENT_SOURCE_DIR}/glm)
endif()

find_package(GLM)

if(NOT GLM_FOUND)
	message(STATUS "GLM not found, including submodule explicitly.")

	add_subdirectory(glm)

	set(GLM_INCLUDE_DIRS ${CMAKE_CURRENT_SOURCE_DIR}/glm)
endif()

#################################################
# glfw
#################################################
if(NOT(DEFINED GLFW_ROOT_DIR))
set(GLFW_ROOT_DIR ${CMAKE_CURRENT_SOURCE_DIR}/glfw)
endif()

find_package(GLFW)

if(NOT GLFW_FOUND)
message(STATUS "GLFW not found, including submodule explicitly.")

set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)

add_subdirectory(glfw)

set(GLFW_INCLUDE_DIRS ${CMAKE_CURRENT_SOURCE_DIR}/glfw/include)
set(GLFW_LIBRARIES glfw)

endif()
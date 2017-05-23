cmake_minimum_required(VERSION 3.0)
project(tori)

# Grab all files inside the src directory
# Using file(GLOB ...) isn't the 'proper' way to do it but it's convenient
file(GLOB SOURCES ${CMAKE_SOURCE_DIR}/src/*.cpp)
file(GLOB HEADERS ${CMAKE_SOURCE_DIR}/src/*.h)

# Add executable (the HEADERS is included so they show up in IDEs)
add_executable(${CMAKE_PROJECT_NAME} ${SOURCES} ${HEADERS})

include_directories(${CMAKE_PROJECT_NAME} ${CMAKE_SOURCE_DIR}/src)

if (WIN32)
	# Look for libraries using either CMake cache variable or an environment variable
	# Example of settings variable on command line: "cmake .. -DGLEW_DIR=/path/to/glew"
	set(FAILED false)

	# Find GLEW includes and libraries
	find_path(GLEW_INCLUDE_DIR GL/glew.h PATHS ${GLEW_DIR} ENV GLEW_DIR PATH_SUFFIXES include)
	find_library(GLEW_LIBRARIES glew32 PATHS ${GLEW_DIR} ENV GLEW_DIR PATH_SUFFIXES lib/Release/Win32)
	if (NOT GLEW_INCLUDE_DIR OR NOT GLEW_LIBRARIES)
		set(FAILED true)
		message(SEND_ERROR "Couldn't find glew. Set GLEW_DIR environment or cmake variable")
	endif()
	
	# Find GLFW includes and libraries
	find_path(GLFW_INCLUDE_DIR GLFW/glfw3.h PATHS ${GLFW_DIR} ENV GLFW_DIR PATH_SUFFIXES include)
	find_library(GLFW_LIBRARIES glfw3 PATHS ${GLFW_DIR} ENV GLFW_DIR PATH_SUFFIXES lib-vc2015)
	if (NOT GLFW_INCLUDE_DIR OR NOT GLFW_LIBRARIES)
		set(FAILED true)
		message(SEND_ERROR "Couldn't find glfw. Set GLFW_DIR environment or cmake variable")
	endif()

	# Find GLM includes and libraries
	find_path(GLM_INCLUDE_DIR glm/glm.hpp PATHS ${GLM_DIR} ENV GLM_DIR PATH_SUFFIXES glm)
	if (NOT GLM_INCLUDE_DIR)
		set(FAILED true)
		message(SEND_ERROR "Couldn't find glm. Set GLM_DIR environment or cmake variable")
	endif()

	# Find OpenVR includes and libraries
	find_path(OPENVR_INCLUDE_DIR openvr.h PATHS ${OPENVR_DIR} ENV OPENVR_DIR PATH_SUFFIXES headers)
	find_library(OPENVR_LIBRARIES openvr_api.lib PATHS ${OPENVR_DIR} ENV OPENVR_DIR PATH_SUFFIXES lib/win32)
	if (NOT OPENVR_INCLUDE_DIR OR NOT OPENVR_LIBRARIES)
		set(FAILED true)
		message(SEND_ERROR "Couldn't find OpenVR. Set OPENVR_DIR environment or cmake variable")
	endif()

	# Find OpenGL includes and libraries
	find_package(OpenGL REQUIRED)

	if (FAILED)
		message(FATAL_ERROR "Couldn't find required libraries, exitting")
	endif()

	# Add definitions so we can find resources and shaders
	add_definitions(-DRESOURCE_DIR="${CMAKE_SOURCE_DIR}/resources/")
	add_definitions(-DSHADER_DIR="${CMAKE_SOURCE_DIR}/shaders/")

	include_directories(${CMAKE_PROJECT_NAME} ${GLEW_INCLUDE_DIR} ${GLFW_INCLUDE_DIR} ${GLM_INCLUDE_DIR} ${OPENVR_INCLUDE_DIR})
	target_link_libraries(${CMAKE_PROJECT_NAME} ${GLEW_LIBRARIES} ${GLFW_LIBRARIES} ${OPENVR_LIBRARIES} ${OPENGL_gl_LIBRARY})

	# Copy the dlls from the libs folder to the output directory of the .exe (https://stackoverflow.com/questions/10671916/how-to-copy-dll-files-into-the-same-folder-as-the-executable-using-cmake)
	add_custom_command(
		TARGET ${CMAKE_PROJECT_NAME} POST_BUILD
		COMMAND ${CMAKE_COMMAND} -E copy_directory
		${CMAKE_SOURCE_DIR}/libs
		$<TARGET_FILE_DIR:${CMAKE_PROJECT_NAME}>
	)
else()
	message(FATAL_ERROR "CMake not configured for non-windows platforms yet")
endif()


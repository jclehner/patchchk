cmake_minimum_required(VERSION 3.10)

project(patchchk VERSION 1)

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED True)

if (WIN32)
	set(MY_ZIP_SUFFIX "-windows")
elseif (UNIX)
	if (APPLE)
		set(MY_ZIP_SUFFIX "-macos")
	elseif (CMAKE_SYSTEM_NAME STREQUAL "Linux")
		set(MY_ZIP_SUFFIX "-linux")
	endif()
endif()

add_compile_options(-Wall -Wextra -DVERSION="${CMAKE_PROJECT_VERSION}")

add_executable(patchchk patchchk.c)

install(TARGETS patchchk
	DESTINATION .
)

set(CPACK_GENERATOR "ZIP")
set(CPACK_PACKAGE_FILE_NAME "${CMAKE_PROJECT_NAME}-v${CMAKE_PROJECT_VERSION}${MY_ZIP_SUFFIX}")
set(CPACK_INCLUDE_TOPLEVEL_DIRECTORY OFF)
set(CPACK_VERBATIM_VARIABLES YES)
include(CPack)



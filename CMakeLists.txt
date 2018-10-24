cmake_minimum_required(VERSION 3.5.1)

option(USE_CLANG "Use clang to build" OFF)

# set w/ -DUSE_CLANG=ON
# doesn't work unfortunately
if(USE_CLANG)
	find_package(LLVM REQUIRED CONFIG)

	message(STATUS "Found LLVM ${LLVM_PACKAGE_VERSION}")
	message(STATUS "Using LLVMConfig.cmake in: ${LLVM_DIR}")
	
	set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} 
		-stdlib=libc++")
	set(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE} 
		-stdlib=libc++")
	set(CMAKE_C_FLAGS_MINSIZEREL "${CMAKE_C_FLAGS_MINSIZEREL} 
		-stdlib=libc++")
	set(CMAKE_C_FLAGS_RELWITHDEBINFO "${CMAKE_C_FLAGS_RELWITHDEBINFO} 
		-stdlib=libc++")
endif(USE_CLANG)

############################################################################

project (Server_Test)

############################################################################

# set release or debug builds
if(CMAKE_CONFIGURATION_TYPES)
	set(CMAKE_CONFIGURATION_TYPES Release RelWithDebInfo MinSizeRel Debug)
	set(CMAKE_CONFIGURATION_TYPES "${CMAKE_CONFIGURATION_TYPES}" CACHE STRING
		"Reset the configurations"
		FORCE)
endif(CMAKE_CONFIGURATION_TYPES)

# set output directories
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/bin)

foreach( OUTPUTCONFIG ${CMAKE_CONFIGURATION_TYPES} )
    string( TOUPPER ${OUTPUTCONFIG} OUTPUTCONFIG )
		set( CMAKE_RUNTIME_OUTPUT_DIRECTORY_${OUTPUTCONFIG} 
				${PROJECT_SOURCE_DIR}/bin )
		set( CMAKE_LIBRARY_OUTPUT_DIRECTORY_${OUTPUTCONFIG} 
				${PROJECT_SOURCE_DIR}/lib )
		set( CMAKE_ARCHIVE_OUTPUT_DIRECTORY_${OUTPUTCONFIG} 
				${PROJECT_SOURCE_DIR}/lib )
endforeach( OUTPUTCONFIG CMAKE_CONFIGURATION_TYPES )

set( CMAKE_EXPORT_COMPILE_COMMANDS ON )

##########################################################################

set(CMAKE_C_STANDARD 11)
# set flags for debug and release mode
include(CheckCCompilerFlag)
check_c_compiler_flag(-Wall HAS_WALL)
check_c_compiler_flag(-O3 HAS_OPT3)
check_c_compiler_flag(-O2 HAS_OPT2)
check_c_compiler_flag(-Os HAS_OPT0S)

if(HAS_WALL)
	if(MSVC)
		set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} -W4")
	else()
		set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} -Wall")
	endif(MSVC)
endif(HAS_WALL)

if(HAS_OPT0S)
	set(CMAKE_C_FLAGS_MINSIZEREL "${CMAKE_C_FLAGS_MINSIZEREL} -Os")
endif(HAS_OPT0S)

check_C_compiler_flag(-g HAS_DEBUG)
check_C_compiler_flag(-lstdc++fs HAS_FS)

if(HAS_DEBUG)
	set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} -g")
	set(CMAKE_C_FLAGS_RELWITHDEBINFO "${CMAKE_C_FLAGS_RELWITHDEBINFO} -g")
endif(HAS_DEBUG)

if(HAS_OPT3)
	set(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE} -O3")
endif(HAS_OPT3)

if(HAS_OPT2)
	set(CMAKE_C_FLAGS_RELWITHDEBINFO "${CMAKE_C_FLAGS_RELWITHDEBINFO} -O2")
endif(HAS_OPT2)

###########################################################################

set(Server_Test_VERSION_MAJOR 1)
set(Server_Test_VERSION_MINOR 0)
set(Server_Test_VERSION_PATCH 1)

# print messages
option(VEBOSE "Print out all server information to stdout" OFF)

# configure a header to pass CMake settings to the source code
configure_file (
  "${PROJECT_SOURCE_DIR}/src/Config.h.in"
  "${PROJECT_SOURCE_DIR}/include/Config.h"
)

###########################################################################

set( HEADERS 
	"${PROJECT_SOURCE_DIR}/include/Config.h"
	"${PROJECT_SOURCE_DIR}/include/ConsoleWrite.h"
	"${PROJECT_SOURCE_DIR}/include/ArgHandler.h"
	"${PROJECT_SOURCE_DIR}/include/ServerInterface.h"
	"${PROJECT_SOURCE_DIR}/include/TimeInterface.h"
)

set( SOURCES
	"${PROJECT_SOURCE_DIR}/src/ArgHandler.c"
	"${PROJECT_SOURCE_DIR}/src/ConsoleWrite.c"
	"${PROJECT_SOURCE_DIR}/src/ServerInterface.c"
	"${PROJECT_SOURCE_DIR}/src/TimeInterface.c"
	"${PROJECT_SOURCE_DIR}/src/main.c"
)

###########################################################################

# clang-format
if (UNIX)
	add_custom_target(
		cfmt
		COMMAND /usr/bin/clang-format-6.0
		-style=file
		-i
		${HEADERS}
		${SOURCES}
	)
endif()

###########################################################################

# libraries
#add_library(DDSTR_LIB STATIC "${CMAKE_SOURCE_DIR}/src/ddStringLib.cpp" )

# Engine executable
add_executable(server_program
	${SOURCES}
	${HEADERS}
)

target_include_directories( server_program PUBLIC
	"${PROJECT_SOURCE_DIR}/include"
)

if( WIN32 )
	target_link_libraries( server_program ws2_32 )
endif( WIN32 )

# make sure every other necessary executable, lib, and .h file is built
#add_dependencies(vulkan_program DDSTR_LIB)

###########################################################################

if(USE_CLANG)
	llvm_map_components_to_libnames(llvm_libs core)
	#target_link_libraries(vulkan_program glfw)
elseif(UNIX)
  #target_link_libraries(vulkan_program glfw)
endif(USE_CLANG)

###########################################################################

# build a CPack driven installer package
include (InstallRequiredSystemLibraries)
#set (CPACK_RESOURCE_FILE_LICENSE "${CMAKE_CURRENT_SOURCE_DIR}/License.txt")
set (CPACK_PACKAGE_VERSION_MAJOR "${Server_Tests_VERSION_MAJOR}")
set (CPACK_PACKAGE_VERSION_MINOR "${Server_Tests_VERSION_MINOR}")
include (CPack)

###########################################################################

# set visual studio startup project
set_property(DIRECTORY ${PROJECT_SOURCE_DIR}
			PROPERTY VS_STARTUP_PROJECT
			server_program)

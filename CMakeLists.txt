# wMule - Windows Mule Client for ed2k and Kademlia networks
# Windows x64 build with CMake + vcpkg

cmake_minimum_required (VERSION 3.15)

project (wMule VERSION 1.0.3)

# C++ Standard
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Export compile commands for LSP (clangd)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Options
set (MIN_BOOST_VERSION 1.75)
set (MIN_WX_VERSION 3.2.0)

set (PACKAGE "wMule")
set (PACKAGE_BUGREPORT "tomas@wmule.org")
set (PACKAGE_NAME "wMule")
set (PACKAGE_STRING "wMule ${PROJECT_VERSION}")
set (PACKAGE_TARNAME "wMule")
set (PACKAGE_URL "")
set (PACKAGE_VERSION "${PROJECT_VERSION}")
set (VERSION "${PROJECT_VERSION}")

include_directories(${CMAKE_SOURCE_DIR}/src/include)
include_directories(${WMULE_GENERATED_DIR})
include_directories(${WMULE_GENERATED_COMMON_DIR})

set(WMULE_VERSION_MAJOR ${PROJECT_VERSION_MAJOR})
set(WMULE_VERSION_MINOR ${PROJECT_VERSION_MINOR})
set(WMULE_VERSION_PATCH ${PROJECT_VERSION_PATCH})
set(WMULE_VERSION_STRING "${PROJECT_VERSION}")

set(WMULE_GENERATED_DIR ${CMAKE_BINARY_DIR}/generated)
set(WMULE_GENERATED_COMMON_DIR ${WMULE_GENERATED_DIR}/common)
message(STATUS "Using generated headers in ${WMULE_GENERATED_DIR}")
file(MAKE_DIRECTORY ${WMULE_GENERATED_COMMON_DIR})

configure_file(
    ${CMAKE_SOURCE_DIR}/src/include/common/ClientVersion.h.in
    ${WMULE_GENERATED_COMMON_DIR}/ClientVersion.h
    @ONLY
)

# Default to Debug if in git repo
set (DEFAULT_BUILD_TYPE "Release")
if (EXISTS "${CMAKE_SOURCE_DIR}/.git")
	set (DEFAULT_BUILD_TYPE "Debug")
endif()

# MSVC: Ajustes de warning
if (CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
	add_compile_options(/W3)
	# wxWidgets sigue usando funciones CRT "inseguras" (strcpy, wcscpy...).
	# Definimos _CRT_SECURE_NO_WARNINGS para silenciar los C4996 heredados
	# sin tocar el código de terceros.
	add_compile_definitions(_CRT_SECURE_NO_WARNINGS)
endif()

# Configuration types
if (CMAKE_CONFIGURATION_TYPES)
	set (CMAKE_CONFIGURATION_TYPES "Debug;Release" CACHE
		STRING "Configuration types" FORCE)
endif()

if (NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
	message (STATUS "Setting build type to '${DEFAULT_BUILD_TYPE}' as none was specified.")
	set (CMAKE_BUILD_TYPE "${DEFAULT_BUILD_TYPE}" CACHE
		STRING "Build type" FORCE)
endif()

# Includes
include (cmake/options.cmake)

# MSVC system paths (Windows SDK, MSVC includes)
if (WIN32 AND CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
	include (cmake/msvc-system-paths.cmake)
endif ()

# Dependencies (managed by vcpkg)
if (BUILD_WEBSERVER OR NEED_ZLIB)
	include (cmake/zlib.cmake)
endif ()

if (ENABLE_BOOST)
	include (cmake/boost.cmake)
endif ()

# NLS (i18n) - managed by vcpkg
# if (ENABLE_NLS)
# 	include (cmake/nls.cmake)
# endif ()

if (NEED_LIB_CRYPTO)
	include (cmake/cryptopp.cmake)
endif ()

if (wx_NEEDED)
	include (cmake/wx.cmake)
endif ()

if (ENABLE_UPNP)
	find_package(miniupnpc CONFIG REQUIRED)

	set(_wmule_miniupnp_target "")
	if (TARGET miniupnpc::miniupnpc)
		set(_wmule_miniupnp_target miniupnpc::miniupnpc)
	elseif (TARGET MINIUPNPC::miniupnpc)
		set(_wmule_miniupnp_target MINIUPNPC::miniupnpc)
	elseif (TARGET MINIUPNPC::miniupnpc-static)
		set(_wmule_miniupnp_target MINIUPNPC::miniupnpc-static)
	endif()

	if (_wmule_miniupnp_target STREQUAL "")
		message(FATAL_ERROR "ENABLE_UPNP=ON requiere miniupnpc exportando miniupnpc::miniupnpc (o equivalente). Instala miniupnpc vía vcpkg o desactiva ENABLE_UPNP.")
	endif()

	add_library(wmule_upnp_sdk INTERFACE)
	target_link_libraries(wmule_upnp_sdk
		INTERFACE
			${_wmule_miniupnp_target}
	)
	target_include_directories(wmule_upnp_sdk INTERFACE
		$<TARGET_PROPERTY:${_wmule_miniupnp_target},INTERFACE_INCLUDE_DIRECTORIES>
	)
	set(WMULE_MINIUPNP_TARGET ${_wmule_miniupnp_target})
endif ()

# Find cryptopp (required for EC protocol)
find_package(cryptopp REQUIRED)

# Git version (simplified for Windows)
find_package (Git QUIET)
if (GIT_FOUND)
	execute_process (
		COMMAND ${GIT_EXECUTABLE} describe --tags --always
		OUTPUT_VARIABLE GIT_INFO_WC_REVISION
		OUTPUT_STRIP_TRAILING_WHITESPACE
		WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
		ERROR_QUIET
	)
endif ()
# SVNDATE deshabilitado para título limpio
set (SVNDATE "")

# Generate config.h
configure_file (
	config.h.cm
	config.h
)

# Generate version.rc for Windows
if (WIN32)
	configure_file (
		version.rc.in
		version.rc
	)
endif()

# Documentation
add_subdirectory (docs)

# Main source
add_subdirectory (src)

# Unit tests
if (BUILD_TESTING)
	enable_testing()
	add_subdirectory (unittests)
endif ()

# Summary
message (STATUS "

	Configured wMule ${PACKAGE_VERSION} for Windows x64

	wMule options:

	Build type:         ${CMAKE_BUILD_TYPE}
	Install prefix:     ${CMAKE_INSTALL_PREFIX}
	UPnP support:       ${ENABLE_UPNP}
	IP2Country support: ${ENABLE_IP2COUNTRY}

	Libraries:"
)

if (wx_NEEDED)
	message (STATUS "	wxWidgets:			${WX_VERSION}")
endif()

if (ENABLE_BOOST)
	message (STATUS "	boost:				${Boost_VERSION}")
endif()

if (NEED_LIB_CRYPTO)
	message (STATUS "	crypto++:			${CRYPTOPP_VERSION}")
endif()

if (NEED_ZLIB)
	message (STATUS "	zlib:				${ZLIB_VERSION_STRING}")
endif()

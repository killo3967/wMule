# wMule - Windows Mule Client for ed2k and Kademlia networks
# Windows x64 build with CMake + vcpkg

cmake_minimum_required (VERSION 3.15)

project (wMule VERSION 1.0.1)

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

# Default to Debug if in git repo
set (DEFAULT_BUILD_TYPE "Release")
if (EXISTS "${CMAKE_SOURCE_DIR}/.git")
	set (DEFAULT_BUILD_TYPE "Debug")
endif()

# MSVC: Disable warnings for third-party code
if (CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
	add_compile_options(/W3)
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
	function(_wmule_filter_upnp_import target fallback_include)
		if (TARGET ${target})
			get_target_property(_wmule_iface_dirs ${target} INTERFACE_INCLUDE_DIRECTORIES)
			if (_wmule_iface_dirs AND NOT _wmule_iface_dirs STREQUAL "_wmule_iface_dirs-NOTFOUND")
				list(FILTER _wmule_iface_dirs EXCLUDE REGEX "[\\/](COMPONENT|UPNP_Development)$")
				if (NOT _wmule_iface_dirs)
					list(APPEND _wmule_iface_dirs "${fallback_include}")
				endif()
				set_property(TARGET ${target} PROPERTY INTERFACE_INCLUDE_DIRECTORIES "${_wmule_iface_dirs}")
			endif()
		endif()
	endfunction()

	set(UPNP_DIR "${VCPKG_INSTALLED_DIR}/${VCPKG_TARGET_TRIPLET}/share/libupnp")
	set(IXML_DIR "${VCPKG_INSTALLED_DIR}/${VCPKG_TARGET_TRIPLET}/share/libupnp")
	set(PThreads4W_DIR "${VCPKG_INSTALLED_DIR}/${VCPKG_TARGET_TRIPLET}/share/pthreads4w")
	find_package(UPNP CONFIG REQUIRED)
	find_package(IXML CONFIG REQUIRED)
	find_package(PThreads4W CONFIG REQUIRED)
	set(_wmule_upnp_include "${VCPKG_INSTALLED_DIR}/${VCPKG_TARGET_TRIPLET}/include/upnp")
	_wmule_filter_upnp_import(UPNP::Shared "${_wmule_upnp_include}")
	_wmule_filter_upnp_import(IXML::Shared "${_wmule_upnp_include}")
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

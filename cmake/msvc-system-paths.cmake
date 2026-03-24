# cmake/msvc-system-paths.cmake
# Add Windows SDK and MSVC system include directories on Windows/MSVC builds.

	if (CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
	set(MSVC_SYS_INC "C:/Program Files (x86)/Microsoft Visual Studio/2022/BuildTools/VC/Tools/MSVC/14.44.35207/include")
	set(WINSDK_UCRT_INC "C:/Program Files (x86)/Windows Kits/10/Include/10.0.26100.0/ucrt")
	set(WINSDK_UM_INC "C:/Program Files (x86)/Windows Kits/10/Include/10.0.26100.0/um")
	set(WINSDK_SHARED_INC "C:/Program Files (x86)/Windows Kits/10/Include/10.0.26100.0/shared")
	set(WINSDK_WINRT_INC "C:/Program Files (x86)/Windows Kits/10/Include/10.0.26100.0/winrt")

	# vcpkg include dir for zlib, png, etc.
	set(VCPKG_INC "C:/vcpkg/installed/x64-windows/include")

	include_directories(SYSTEM
		"${MSVC_SYS_INC}"
		"${WINSDK_UCRT_INC}"
		"${WINSDK_UM_INC}"
		"${WINSDK_SHARED_INC}"
		"${WINSDK_WINRT_INC}"
		"${VCPKG_INC}"
	)

	get_property(current_includes DIRECTORY PROPERTY CMAKE_CXX_IMPLICIT_INCLUDE_DIRECTORIES)
	list(APPEND current_includes
		"${MSVC_SYS_INC}"
		"${WINSDK_UCRT_INC}"
		"${WINSDK_UM_INC}"
		"${WINSDK_SHARED_INC}"
		"${WINSDK_WINRT_INC}"
	)
	set_property(DIRECTORY PROPERTY CMAKE_CXX_IMPLICIT_INCLUDE_DIRECTORIES ${current_includes})



endif()

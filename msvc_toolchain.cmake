# Wrapper toolchain: includes vcpkg and adds MSVC system includes
include_guard(GLOBAL)

# Include vcpkg toolchain (must load before compiler is fully set up)
include("C:/vcpkg/scripts/buildsystems/vcpkg.cmake")

# After vcpkg toolchain, we need to append MSVC system include directories
# The vcpkg toolchain sets CMAKE_CXX_IMPLICIT_INCLUDE_DIRECTORIES to only vcpkg paths.
# We need to also include the MSVC standard paths so stddef.h etc. are found.
if(CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
    set(MSVC_SYS_INC "C:/Program Files (x86)/Microsoft Visual Studio/2022/BuildTools/VC/Tools/MSVC/14.44.35207/include")
    set(WINSDK_UCRT_INC "C:/Program Files (x86)/Windows Kits/10/Include/10.0.26100.0/ucrt")
    set(WINSDK_UM_INC "C:/Program Files (x86)/Windows Kits/10/Include/10.0.26100.0/um")
    set(WINSDK_SHARED_INC "C:/Program Files (x86)/Windows Kits/10/Include/10.0.26100.0/shared")
    set(WINSDK_WINRT_INC "C:/Program Files (x86)/Windows Kits/10/Include/10.0.26100.0/winrt")
    
    get_property(IMPLICIT_INCS DIRECTORY PROPERTY CMAKE_CXX_IMPLICIT_INCLUDE_DIRECTORIES)
    
    # Append MSVC system paths to whatever vcpkg set
    list(APPEND IMPLICIT_INCS 
        "${MSVC_SYS_INC}"
        "${WINSDK_UCRT_INC}"
        "${WINSDK_UM_INC}"
        "${WINSDK_SHARED_INC}"
        "${WINSDK_WINRT_INC}"
    )
    
    set_property(DIRECTORY PROPERTY CMAKE_CXX_IMPLICIT_INCLUDE_DIRECTORIES ${IMPLICIT_INCS})
endif()

#
# aMule Options for Windows x64
#
# Version: aMule 2.4.0
# Platform: Windows x64 only

# Build options
option (BUILD_MONOLITHIC "Build monolithic wMule application (amule.exe)" ON)
option (BUILD_AMULECMD "Build wMule command line client (amulecmd.exe)" ON)
option (BUILD_ED2K "Build ED2K link handler" OFF)
option (BUILD_WEBSERVER "Build wMule WebServer (amuleweb)" OFF)
option (BUILD_TESTING "Build and run unit tests" ON)

# Removed options (desktop-only build):
# - BUILD_DAEMON: amuled daemon (not needed for desktop)
# - BUILD_REMOTEGUI: amulegui remote GUI (not needed for desktop)

# Options removed (desktop-only build):
# BUILD_ALC, BUILD_ALCC, BUILD_CAS, BUILD_FILEVIEW
# BUILD_PLASMAMULE, BUILD_WXCAS, BUILD_XAS
# BUILD_DAEMON, BUILD_REMOTEGUI

# Features
option (ENABLE_BOOST "Enable Boost.ASIO sockets (requires Boost 1.75+)" ON)
option (ENABLE_UPNP "Enable UPnP support" ON)
option (ENABLE_IP2COUNTRY "Enable IP2Country geolocation" OFF)
option (ENABLE_NLS "Enable internationalization (i18n) - requires gettext" OFF)
option (ENABLE_MMAP "Enable memory mapped files (experimental)" OFF)

# Minimum versions
set (MIN_BOOST_VERSION 1.75 CACHE STRING "Minimum Boost version")
set (MIN_WX_VERSION 3.2.0 CACHE STRING "Minimum wxWidgets version")

# Dependencies
set (NEED_LIB TRUE)
set (NEED_LIB_MULECOMMON TRUE)
set (wx_NEED_BASE TRUE)

if (BUILD_MONOLITHIC OR BUILD_WEBSERVER)
	set (wx_NEED_GUI TRUE)
	set (wx_NEED_NET TRUE)
endif()

if (BUILD_AMULECMD)
	set (wx_NEED_NET TRUE)
endif()

if (BUILD_AMULECMD OR BUILD_MONOLITHIC OR BUILD_WEBSERVER)
	set (NEED_LIB_EC TRUE)
	set (NEED_LIB_MULESOCKET TRUE)
	set (NEED_ZLIB TRUE)
endif()

if (BUILD_WEBSERVER OR BUILD_MONOLITHIC)
	set (NEED_LIB_MULEAPPGUI TRUE)
endif()

if (BUILD_MONOLITHIC OR BUILD_WEBSERVER)
	set (NEED_LIB_MULEAPPCOMMON TRUE)
	set (NEED_LIB_MULEAPPCORE TRUE)
endif()

if (BUILD_AMULECMD)
	set (wx_NEED_NET TRUE)
endif()

if (BUILD_ED2K)
	set (wx_NEED_BASE TRUE)
endif()

if (BUILD_WEBSERVER)
	set (wx_NEED_NET TRUE)
endif()

# wxWidgets requirements
if (wx_NEED_ADV OR wx_NEED_BASE OR wx_NEED_GUI OR wx_NEED_NET)
	set (wx_NEEDED TRUE)
	if (WIN32 AND NOT wx_NEED_BASE)
		set (wx_NEED_BASE TRUE)
	endif()
endif()

# Always enable wx_NEED_NET for tests that need it
if (BUILD_TESTING)
	set (wx_NEED_NET TRUE)
	set (wx_NEEDED TRUE)
endif()

# Install directories
set (PKGDATADIR "${CMAKE_INSTALL_DATADIR}/amule")

if (BUILD_MONOLITHIC)
	set (INSTALL_SKINS TRUE)
endif()

# Compiler definitions
ADD_COMPILE_DEFINITIONS ($<$<CONFIG:DEBUG>:__DEBUG__>)

IF (WIN32)
	ADD_COMPILE_DEFINITIONS ($<$<CONFIG:DEBUG>:wxDEBUG_LEVEL=0>)
ENDIF (WIN32)

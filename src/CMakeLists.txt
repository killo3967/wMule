set (LIBS_INCLUDE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/libs")
set (INCLUDE_INCLUDE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/include")
set (SRC_DIR "${CMAKE_CURRENT_SOURCE_DIR}")
set (EC_INCLUDE_DIR "${CMAKE_CURRENT_BINARY_DIR}/libs/ec/cpp")

add_custom_command (
	OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/pixmaps/flags_xpm/CountryFlags.h
	COMMAND ${CMAKE_COMMAND} -DHEADER_FILE="${CMAKE_CURRENT_BINARY_DIR}/pixmaps/flags_xpm/CountryFlags.h" -P ${CMAKE_CURRENT_SOURCE_DIR}/pixmaps/flags_xpm/CMakeLists.txt
	WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/pixmaps/flags_xpm"
)

add_custom_command (
	OUTPUT ${EC_INCLUDE_DIR}/ECCodes.h
	COMMAND ${CMAKE_COMMAND} -DHEADER_FILE="${CMAKE_CURRENT_BINARY_DIR}/libs/ec/cpp/ECCodes.h" -P ${CMAKE_CURRENT_SOURCE_DIR}/libs/ec/abstracts/CMakeLists.txt
	WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/libs/ec/abstracts"
)

add_custom_command (
	OUTPUT ${EC_INCLUDE_DIR}/ECTagTypes.h
	COMMAND ${CMAKE_COMMAND} -DHEADER_FILE="${CMAKE_CURRENT_BINARY_DIR}/libs/ec/cpp/ECTagTypes.h" -P ${CMAKE_CURRENT_SOURCE_DIR}/libs/ec/abstracts/CMakeLists.txt
	WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/libs/ec/abstracts"
)

add_custom_target (generate_CountryFlags.h DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/pixmaps/flags_xpm/CountryFlags.h)
add_custom_target (generate_ECCodes.h DEPENDS ${EC_INCLUDE_DIR}/ECCodes.h)
add_custom_target (generate_ECTagTypes.h DEPENDS ${EC_INCLUDE_DIR}/ECTagTypes.h)
set_source_files_properties (${CMAKE_CURRENT_BINARY_DIR}/pixmaps/flags_xpm/CountryFlags.h PROPERTIES GENERATED TRUE)
set_source_files_properties (${EC_INCLUDE_DIR}/ECCodes.h PROPERTIES GENERATED TRUE)
set_source_files_properties (${EC_INCLUDE_DIR}/ECTagTypes.h PROPERTIES GENERATED TRUE)
include (${CMAKE_SOURCE_DIR}/cmake/source-vars.cmake)

if (BUILD_UTIL)
	add_subdirectory (utils)
endif()

if (BUILD_WEBSERVER)
	add_subdirectory (webserver)
endif()

if (INSTALL_SKINS)
	add_subdirectory (skins)
endif()


if (ENABLE_IP2COUNTRY)
	set_source_files_properties (${IP2COUNTRY}
		PROPERTIES COMPILE_FLAGS "-I${CMAKE_CURRENT_BINARY_DIR} -I${CMAKE_CURRENT_SOURCE_DIR}/pixmaps/flags_xpm"
	)
endif()

if (NEED_LIB)
	add_subdirectory (libs)
endif()

if (BUILD_AMULECMD)
	add_executable (wmulecmd
		DataToText.cpp
		ExternalConnector.cpp
		LoggerConsole.cpp
		NetworkFunctions.cpp
		OtherFunctions.cpp
		TextClient.cpp
	)

	if (WIN32)
		target_sources (wmulecmd
			PRIVATE ${CMAKE_SOURCE_DIR}/version.rc
		)
	endif()

	target_compile_definitions (wmulecmd
		PRIVATE wxUSE_GUI=0
	)

	target_include_directories (wmulecmd
		PRIVATE ${CMAKE_BINARY_DIR}
		PRIVATE ${INCLUDE_INCLUDE_DIR}
		PRIVATE ${LIBS_INCLUDE_DIR}
	)

	target_link_libraries (wmulecmd
		PRIVATE mulecommon
		PRIVATE mulesocket
		PRIVATE ec
		PRIVATE wxWidgets::NET
		PRIVATE wxWidgets::CORE
	)

	if (HAVE_BFD)
		target_link_libraries (wmulecmd 
			PRIVATE ${LIBBFD}
		)
	endif (HAVE_BFD)

	if (HAVE_LIBREADLINE)
		target_link_libraries (wmulecmd
			PRIVATE ${READLINE_LIBRARIES}
		)
	endif()

	install (TARGETS wmulecmd
		RUNTIME DESTINATION bin
	)
endif (BUILD_AMULECMD)

if (BUILD_ED2K)
	add_executable (ed2k
		ED2KLinkParser.cpp
		MagnetURI.cpp
		MuleCollection.cpp
	)

	if (WIN32)
		target_sources (ed2k
			PRIVATE ${CMAKE_SOURCE_DIR}/version.rc
		)
	endif()

	target_compile_definitions (ed2k
		PRIVATE "USE_STD_STRING"
	)

	if (WIN32)
		target_link_libraries (ed2k
			PRIVATE shlwapi.lib
		)
	endif()

	install (TARGETS ed2k
		RUNTIME DESTINATION bin
	)
endif()

if (BUILD_MONOLITHIC)
	add_executable (wmule
		${COMMON_SOURCES}
		${CORE_SOURCES}
		${GUI_SOURCES}
		CaptchaDialog.cpp
		CaptchaGenerator.cpp
		PartFileConvert.cpp
		PartFileConvertDlg.cpp
	)

	if (WIN32)
		target_sources (wmule
			PRIVATE ${CMAKE_SOURCE_DIR}/version.rc
		)
	endif()

	target_link_libraries (wmule
		PRIVATE ec
		PRIVATE muleappcommon
		PRIVATE muleappcore
		PRIVATE muleappgui
		PRIVATE mulecommon
		PRIVATE mulesocket
		PRIVATE wxWidgets::NET
	)

	if (HAVE_BFD)
		target_link_libraries (wmule
			PRIVATE ${LIBBFD}
		)
	endif()

		if (WIN32)
			target_link_libraries (wmule
				PRIVATE shlwapi.lib
			)

			set_target_properties (wmule PROPERTIES
				WIN32_EXECUTABLE TRUE
				LINK_FLAGS "/DEBUG"
			)
		endif()

	install (TARGETS wmule
		RUNTIME DESTINATION bin
	)

	install (FILES aMule.xpm
		DESTINATION "${CMAKE_INSTALL_DATADIR}/pixmaps"
		RENAME amule.xpm
	)

	install (FILES aMule.xpm
		DESTINATION "${CMAKE_INSTALL_DATADIR}/pixmaps"
		RENAME amulegui.xpm
	)
endif (BUILD_MONOLITHIC)

if (NEED_LIB_MULEAPPCOMMON)
	add_library (muleappcommon STATIC
		CFile.cpp
		ClientCredits.cpp
		DataToText.cpp
		ED2KLink.cpp
		Friend.cpp
		GapList.cpp
		MagnetURI.cpp
		MemFile.cpp
		NetworkFunctions.cpp
		OtherFunctions.cpp
		Packet.cpp
		RLE.cpp
		SafeFile.cpp
		SHA.cpp
		Tag.cpp
		TerminationProcess.cpp
		Timer.cpp
	)

	add_dependencies (muleappcommon
		generate_ECCodes.h
		generate_ECTagTypes.h
	)

	target_compile_definitions (muleappcommon
		PRIVATE wxUSE_GUI=0
		PRIVATE WXUSINGDLL
	)

	target_include_directories (muleappcommon
		PUBLIC ${wMule_BINARY_DIR}
		PUBLIC ${EC_INCLUDE_DIR}
		PRIVATE ${INCLUDE_INCLUDE_DIR}
		PRIVATE ${LIBS_INCLUDE_DIR}
		PRIVATE ${ZLIB_INCLUDE_DIR}
	)

	target_link_libraries(muleappcommon
		PUBLIC wxWidgets::BASE
	)

	if (ENABLE_UPNP)
		target_link_libraries (muleappcommon
			PUBLIC wmule_upnp_sdk
		)
	endif()
endif()

if (NEED_LIB_MULEAPPCORE)
	if (BISON_FOUND)
		bison_target (Parser.cpp
			${CMAKE_CURRENT_SOURCE_DIR}/Parser.y
			${CMAKE_CURRENT_BINARY_DIR}/Parser.cpp
			COMPILE_FLAGS "-t -d -v"
		)

		set (PARSER ${CMAKE_CURRENT_BINARY_DIR}/Parser.cpp)
	else()
		set (PARSER ${CMAKE_CURRENT_SOURCE_DIR}/Parser.cpp)
	endif (BISON_FOUND)

	if (FLEX_FOUND)
		if (FLEX_MATCH)
			set (FLEX_FLAGS "--header-file=${CMAKE_CURRENT_BINARY_DIR}/Scanner.h")

			set_source_files_properties (Parser.cpp
				COMPILE_FLAGS "-I${CMAKE_CURRENT_BINARY_DIR}"
			)
		endif()

		flex_target (Scanner.cpp
			${CMAKE_CURRENT_SOURCE_DIR}/Scanner.l
			${CMAKE_CURRENT_BINARY_DIR}/Scanner.cpp
			COMPILE_FLAGS "${FLEX_FLAGS}"
		)

		flex_target (IPFilterScanner.cpp
			${CMAKE_CURRENT_SOURCE_DIR}/IPFilterScanner.l
			${CMAKE_CURRENT_BINARY_DIR}/IPFilterScanner.cpp
			COMPILE_FLAGS "-Pyyip"
		)

		set (SCANNER ${CMAKE_CURRENT_BINARY_DIR}/Scanner.cpp)
		set (IPFILTERSCANNER ${CMAKE_CURRENT_BINARY_DIR}/IPFilterScanner.cpp)
	else()
		set (SCANNER ${CMAKE_CURRENT_SOURCE_DIR}/Scanner.cpp)
		set (IPFILTERSCANNER ${CMAKE_CURRENT_SOURCE_DIR}/IPFilterScanner.cpp)
	endif()

	add_library (muleappcore STATIC
		${IPFILTERSCANNER}
		${PARSER}
		${SCANNER}
		kademlia/kademlia/Entry.cpp
		kademlia/kademlia/Indexed.cpp
		kademlia/kademlia/SearchManager.cpp
		kademlia/routing/RoutingBin.cpp
		kademlia/utils/UInt128.cpp
		AsyncDNS.cpp
		CanceledFileList.cpp
		DeadSourceList.cpp
		FileArea.cpp
		FileAutoClose.cpp
		PlatformSpecific.cpp
		RandomFunctions.cpp
		RC4Encrypt.cpp
		StateMachine.cpp
		TerminationProcessAmuleweb.cpp
		ThreadScheduler.cpp
		UPnPBase.cpp
		UPnPUrlUtils.cpp
	)

	target_compile_definitions (muleappcore
		PRIVATE wxUSE_GUI=0
		PRIVATE WXUSINGDLL
	)

	target_include_directories (muleappcore
		PRIVATE ${CMAKE_BINARY_DIR}
		PRIVATE ${CMAKE_CURRENT_BINARY_DIR}
		PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}
		PRIVATE ${INCLUDE_INCLUDE_DIR}
		PRIVATE ${LIBS_INCLUDE_DIR}
	)

	if (ENABLE_UPNP)
		target_link_libraries (muleappcore PUBLIC wmule_upnp_sdk)
	endif()

	target_link_libraries (muleappcore
		PUBLIC wxWidgets::BASE
		PRIVATE cryptopp::cryptopp
	)
endif()

if (NEED_LIB_MULEAPPGUI)
	add_library (muleappgui STATIC
		${IP2COUNTRY}
		extern/wxWidgets/listctrl.cpp
		BarShader.cpp
		ColorFrameCtrl.cpp
		EditServerListDlg.cpp
		FileDetailListCtrl.cpp
		MuleCollection.cpp
		MuleColour.cpp
		MuleGifCtrl.cpp
		MuleListCtrl.cpp
		MuleNotebook.cpp
		MuleTextCtrl.cpp
		muuli_wdr.cpp
	)

	if (ENABLE_IP2COUNTRY)
		target_link_libraries (muleappgui
			PUBLIC GeoIP::Shared
		)
	endif()

	target_compile_definitions (muleappgui
		PRIVATE WXUSINGDLL
	)

	target_include_directories (muleappgui
		PUBLIC ${wMule_BINARY_DIR}
		PUBLIC ${INCLUDE_INCLUDE_DIR}
		PUBLIC ${LIBS_INCLUDE_DIR}
	)

	target_link_libraries (muleappgui
		PRIVATE wxWidgets::CORE
	)

	if (ENABLE_IP2COUNTRY)
		add_dependencies (muleappgui
			generate_CountryFlags.h)
	endif()
endif()

IF (NEED_LIB_MULESOCKET)
	add_library (mulesocket STATIC
		LibSocket.cpp
	)

	target_compile_definitions (mulesocket
		PRIVATE wxUSE_GUI=0
	)
	
	target_include_directories (mulesocket
		PUBLIC ${wMule_BINARY_DIR}
		PUBLIC ${INCLUDE_INCLUDE_DIR}
		PUBLIC ${LIBS_INCLUDE_DIR}
	)

	if (ENABLE_BOOST)
		target_link_libraries (mulesocket
			PRIVATE ${Boost_LIBRARIES}
			PUBLIC wxWidgets::BASE
		)
	else()
		target_link_libraries (mulesocket
			PUBLIC wxWidgets::NET
		)
	endif()
endif()

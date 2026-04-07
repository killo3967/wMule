add_library (mulecommon STATIC
	FileFunctions.cpp
	Format.cpp
	MD5Sum.cpp
	SecretHash.cpp
	MuleDebug.cpp
	Path.cpp
	strerror_r.c
	StringFunctions.cpp
	TextFile.cpp
	ThreadPool.cpp
	threading/ThreadGuards.cpp
	AsyncSocket.cpp
)

target_include_directories (mulecommon
	PRIVATE ${wMule_BINARY_DIR}
	PUBLIC ${LIBS_INCLUDE_DIR}
	PUBLIC ${SRC_DIR}
	PUBLIC ${WMULE_GENERATED_DIR}
)

target_link_libraries (mulecommon
	PUBLIC wxWidgets::BASE
	PUBLIC cryptopp::cryptopp
)

if (WIN32)
	target_link_libraries (mulecommon
		PUBLIC ws2_32 ws2_32.lib
	)
endif()

if (HAVE_BFD)
	target_link_libraries (mulecommon
		PUBLIC ${BFD_LIBRARY}
	)
endif()

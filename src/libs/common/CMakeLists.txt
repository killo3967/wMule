add_library (mulecommon STATIC
	FileFunctions.cpp
	Format.cpp
	MD5Sum.cpp
	MuleDebug.cpp
	Path.cpp
	strerror_r.c
	StringFunctions.cpp
	TextFile.cpp
	ThreadPool.cpp
	AsyncSocket.cpp
)

target_include_directories (mulecommon
	PRIVATE ${wMule_BINARY_DIR}
)

target_link_libraries (mulecommon
	PUBLIC wxWidgets::BASE
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

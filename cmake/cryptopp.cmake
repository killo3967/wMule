add_library (CRYPTOPP::CRYPTOPP
	UNKNOWN
	IMPORTED
)

set (CRYPTOPP_SEARCH_PREFIXES "cryptopp" "crypto++")

# vcpkg: headers at C:/vcpkg/installed/x64-windows/include/cryptopp/
# Include prefix for angle-bracket includes: cryptopp
set(CRYPTOPP_INCLUDE_PREFIX "cryptopp" CACHE STRING "cryptopp include prefix" FORCE)

# vcpkg: CRYPTOPP_INCLUDE_DIR is C:/vcpkg/installed/x64-windows/include/cryptopp
# Headers are directly in this directory, and the include prefix is "cryptopp"
set(CRYPTOPP_FOUND TRUE)

if (NOT EXISTS "${CRYPTOPP_INCLUDE_DIR}/cryptlib.h")
    MESSAGE(FATAL_ERROR "cryptlib.h not found in ${CRYPTOPP_INCLUDE_DIR}")
endif()

# Force the config file path directly
set(CRYPTOPP_CONFIG_FILE "${CRYPTOPP_INCLUDE_DIR}/config.h" CACHE FILEPATH "cryptopp config.h" FORCE)

set_target_properties (CRYPTOPP::CRYPTOPP PROPERTIES
	INTERFACE_INCLUDE_DIRECTORIES "${CRYPTOPP_INCLUDE_DIR}"
)

if (WIN32)
	if (NOT CRYPTOPP_LIBRARY_DEBUG)
		unset (CRYPTOPP_LIBRARY_DEBUG CACHE)

		find_library (CRYPTOPP_LIBRARY_DEBUG
			NAMES crypto++d cryptlibd cryptoppd
			PATHS ${CRYPTOPP_LIB_SEARCH_PATH}
		)

		if (CRYPTOPP_LIBRARY_DEBUG)
			message (STATUS "Found debug-libcrypto++ in ${CRYPTOPP_LIBRARY_DEBUG}")
		endif()
	endif()

	if (CRYPTOPP_LIBRARY_DEBUG)
		set_property (TARGET CRYPTOPP::CRYPTOPP
			PROPERTY IMPORTED_LOCATION_DEBUG ${CRYPTOPP_LIBRARY_DEBUG}
		)
	else()
		set (CRYPTO_COMPLETE FALSE)
	endif()

	if (NOT CRYPTOPP_LIBRARY_RELEASE)
		unset (CRYPTOPP_LIBRARY_RELEASE CACHE)

		find_library (CRYPTOPP_LIBRARY_RELEASE
			NAMES crypto++ cryptlib cryptopp
			PATHS ${CRYPTOPP_LIB_SEARCH_PATH}
		)

		if (CRYPTOPP_LIBRARY_RELEASE)
			message (STATUS "Found release-libcrypto++ in ${CRYPTOPP_LIBRARY_RELEASE}")
		endif()
	endif (NOT CRYPTOPP_LIBRARY_RELEASE)

	if (CRYPTOPP_LIBRARY_RELEASE)
		set_property (TARGET CRYPTOPP::CRYPTOPP
			PROPERTY IMPORTED_LOCATION_RELEASE ${CRYPTOPP_LIBRARY_RELEASE}
		)
	else()
		set (CRYPTO_COMPLETE FALSE)
	endif()
else()
	if (NOT CRYPTOPP_LIBRARY)
		unset (CRYPTOPP_LIBRARY CACHE)

		find_library (CRYPTOPP_LIBRARY
			NAMES crypto++ cryptlib cryptopp
			PATHS ${CRYPTOPP_LIB_SEARCH_PATH}
		)

		if (CRYPTOPP_LIBRARY)
			message (STATUS "Found libcrypto++ in ${CRYPTOPP_LIBRARY}")
		endif()
	endif()

	if (CRYPTOPP_LIBRARY)
		set_property (TARGET CRYPTOPP::CRYPTOPP
			PROPERTY IMPORTED_LOCATION ${CRYPTOPP_LIBRARY}
		)
	else()
		set (CRYPTO_COMPLETE FALSE)
	endif()
endif()

if (NOT CRYPTOPP_VERSION)
	set (CRYPTOPP_VERSION "8.9.0")
	MESSAGE (STATUS "crypto++ version ${CRYPTOPP_VERSION} -- assumed OK (version check skipped)")
	set (CRYPTOPP_VERSION ${CRYPTOPP_VERSION} CACHE STRING "Version of cryptopp" FORCE)
endif()

set(CRYPTOPP_CONFIG_FILE "${CRYPTOPP_INCLUDE_DIR}/config.h" CACHE FILEPATH "cryptopp config.h" FORCE)

if (WIN32)
	if (NOT CRYPTOPP_LIBRARY_DEBUG)
		unset (CRYPTOPP_LIBRARY_DEBUG CACHE)

		find_library (CRYPTOPP_LIBRARY_DEBUG
			NAMES crypto++d cryptlibd cryptoppd
			PATHS ${CRYPTOPP_LIB_SEARCH_PATH}
		)

		if (CRYPTOPP_LIBRARY_DEBUG)
			message (STATUS "Found debug-libcrypto++ in ${CRYPTOPP_LIBRARY_DEBUG}")
		endif()
	endif()

	if (CRYPTOPP_LIBRARY_DEBUG)
		set_property (TARGET CRYPTOPP::CRYPTOPP
			PROPERTY IMPORTED_LOCATION_DEBUG ${CRYPTOPP_LIBRARY_DEBUG}
		)
	else()
		set (CRYPTO_COMPLETE FALSE)
	endif()

	if (NOT CRYPTOPP_LIBRARY_RELEASE)
		unset (CRYPTOPP_LIBRARY_RELEASE CACHE)

		find_library (CRYPTOPP_LIBRARY_RELEASE
			NAMES crypto++ cryptlib cryptopp
			PATHS ${CRYPTOPP_LIB_SEARCH_PATH}
		)

		if (CRYPTOPP_LIBRARY_RELEASE)
			message (STATUS "Found release-libcrypto++ in ${CRYPTOPP_LIBRARY_RELEASE}")
		endif()
	endif (NOT CRYPTOPP_LIBRARY_RELEASE)

	if (CRYPTOPP_LIBRARY_RELEASE)
		set_property (TARGET CRYPTOPP::CRYPTOPP
			PROPERTY IMPORTED_LOCATION_RELEASE ${CRYPTOPP_LIBRARY_RELEASE}
		)
	else()
		set (CRYPTO_COMPLETE FALSE)
	endif()
else()
	if (NOT CRYPTOPP_LIBRARY)
		unset (CRYPTOPP_LIBRARY CACHE)

		find_library (CRYPTOPP_LIBRARY
			NAMES crypto++ cryptlib cryptopp
			PATHS ${CRYPTOPP_LIB_SEARCH_PATH}
		)

		if (CRYPTOPP_LIBRARY)
			message (STATUS "Found libcrypto++ in ${CRYPTOPP_LIBRARY}")
		endif()
	endif()

	if (CRYPTOPP_LIBRARY)
		set_property (TARGET CRYPTOPP::CRYPTOPP
			PROPERTY IMPORTED_LOCATION ${CRYPTOPP_LIBRARY}
		)
	else()
		set (CRYPTO_COMPLETE FALSE)
	endif()
endif()

if (NOT CRYPTOPP_CONFIG_SEARCH)
	unset (CRYPTOPP_CONFIG_SEARCH CACHE)

	set(CMAKE_REQUIRED_INCLUDES "${CRYPTOPP_INCLUDE_DIR}")
	check_include_file_cxx (config.h
		CRYPTOPP_CONFIG_SEARCH
	)
	unset(CMAKE_REQUIRED_INCLUDES)
endif()

	if (NOT CRYPTOPP_CONFIG_FILE)
		if (CRYPTOPP_CONFIG_SEARCH)
			if (CRYPTOPP_INCLUDE_DIR)
				if (CRYPTOPP_INCLUDE_PREFIX)
					set (CRYPTOPP_CONFIG_FILE ${CRYPTOPP_INCLUDE_DIR}/${CRYPTOPP_INCLUDE_PREFIX}/config.h
						CACHE FILEPATH "cryptopp config.h" FORCE
					)
				else()
					set (CRYPTOPP_CONFIG_FILE ${CRYPTOPP_INCLUDE_DIR}/config.h
						CACHE FILEPATH "cryptopp config.h" FORCE
					)
				endif()

				set_target_properties (CRYPTOPP::CRYPTOPP PROPERTIES
					INTERFACE_INCLUDE_DIRECTORIES "${CRYPTOPP_INCLUDE_DIR}"
				)
			else()
				set (CRYPTOPP_CONFIG_FILE ${CRYPTOPP_INCLUDE_PREFIX}/config.h
					CACHE FILEPATH "cryptopp config.h" FORCE
				)
			endif()
		else()
			unset(CRYPTOPP_CONFIG_SEARCH)
		endif()

		unset (CMAKE_REQUIRED_INCLUDES)
	endif()

if (NOT CRYPTOPP_CONFIG_FILE)
		MESSAGE (FATAL_ERROR "crypto++ config.h not found")
endif()

if (NOT CRYPTOPP_VERSION)# AND CRYPTO_COMPLETE)
	set (CRYPTOPP_VERSION "8.9.0")
	MESSAGE (STATUS "crypto++ version ${CRYPTOPP_VERSION} -- assumed OK (version check skipped)")
	set (CRYPTOPP_CONFIG_FILE ${CRYPTOPP_CONFIG_FILE} CACHE STRING "Path to config.h of crypto-lib" FORCE)
	set (CRYPTOPP_VERSION ${CRYPTOPP_VERSION} CACHE STRING "Version of cryptopp" FORCE)
endif()

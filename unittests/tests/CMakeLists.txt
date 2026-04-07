include_directories(${WMULE_GENERATED_COMMON_DIR})
include_directories(${WMULE_GENERATED_DIR})
include_directories(${CMAKE_SOURCE_DIR}/src/libs)

set(THREADING_TEST_STUB ${CMAKE_CURRENT_SOURCE_DIR}/ThreadingTestStubs.cpp)

add_executable (CTagTest
	CTagTest.cpp
	${CMAKE_SOURCE_DIR}/src/SafeFile.cpp
	${CMAKE_SOURCE_DIR}/src/MemFile.cpp
	${CMAKE_SOURCE_DIR}/src/Tag.cpp
	${CMAKE_SOURCE_DIR}/src/libs/common/Format.cpp
	${CMAKE_SOURCE_DIR}/src/libs/common/strerror_r.c
)

add_test (NAME CTagTest
	COMMAND CTagTest
)

target_include_directories (CTagTest
	PRIVATE ${CMAKE_BINARY_DIR}
	PRIVATE ${CMAKE_SOURCE_DIR}/src
	PRIVATE ${CMAKE_SOURCE_DIR}/src/include
)

target_link_libraries (CTagTest
	muleunit
)

add_executable (CUInt128Test
	CUInt128Test.cpp
	${CMAKE_SOURCE_DIR}/src/kademlia/utils/UInt128.cpp
	${CMAKE_SOURCE_DIR}/src/libs/common/Format.cpp
	${CMAKE_SOURCE_DIR}/src/libs/common/strerror_r.c
)

add_test (NAME CUInt128Test
	COMMAND CUInt128Test
)

target_include_directories (CUInt128Test
	PRIVATE ${CMAKE_SOURCE_DIR}/src
)

target_link_libraries (CUInt128Test
	muleunit
)

add_executable (FileDataIOTest
	FileDataIOTest.cpp
	${CMAKE_SOURCE_DIR}/src/SafeFile.cpp
	${CMAKE_SOURCE_DIR}/src/CFile.cpp
	${CMAKE_SOURCE_DIR}/src/MemFile.cpp
	${CMAKE_SOURCE_DIR}/src/kademlia/utils/UInt128.cpp
	${CMAKE_SOURCE_DIR}/src/Tag.cpp
	${CMAKE_SOURCE_DIR}/src/libs/common/Path.cpp
	${CMAKE_SOURCE_DIR}/src/libs/common/Format.cpp
	${CMAKE_SOURCE_DIR}/src/libs/common/strerror_r.c
)

add_test (NAME FileDataIOTest
	COMMAND FileDataIOTest
)

target_include_directories (FileDataIOTest
	PRIVATE ${CMAKE_BINARY_DIR}
	PRIVATE ${CMAKE_SOURCE_DIR}/src
	PRIVATE ${CMAKE_SOURCE_DIR}/src/include
)

target_link_libraries (FileDataIOTest
	muleunit
)

add_executable (FormatTest
	FormatTest.cpp
	${CMAKE_SOURCE_DIR}/src/libs/common/Format.cpp
	${CMAKE_SOURCE_DIR}/src/libs/common/strerror_r.c
)

add_test (NAME FormatTest
	COMMAND FormatTest
)

target_include_directories (FormatTest
	PRIVATE ${CMAKE_SOURCE_DIR}/src
)

target_link_libraries (FormatTest
	muleunit
)

add_executable (NetworkFunctionsTest
	NetworkFunctionsTest.cpp
	${CMAKE_SOURCE_DIR}/src/NetworkFunctions.cpp
	${CMAKE_SOURCE_DIR}/src/LibSocket.cpp
	${CMAKE_SOURCE_DIR}/src/libs/common/Format.cpp
	${CMAKE_SOURCE_DIR}/src/libs/common/strerror_r.c
)

add_test (NAME NetworkFunctionsTest
	COMMAND NetworkFunctionsTest
)

target_compile_definitions (NetworkFunctionsTest
	PRIVATE "EC_REMOTE"
)

target_include_directories (NetworkFunctionsTest
	PRIVATE ${CMAKE_SOURCE_DIR}/src
	PRIVATE ${CMAKE_SOURCE_DIR}/src/include
)

if (ENABLE_BOOST)
	target_include_directories (NetworkFunctionsTest
		PRIVATE ${Boost_INCLUDE_DIR}
	)

	target_link_libraries (NetworkFunctionsTest
		${Boost_LIBRARIES}
	)
endif()

target_link_libraries (NetworkFunctionsTest
	muleunit
	wxWidgets::NET
)

add_executable (PathTest
	PathTest.cpp
	${CMAKE_SOURCE_DIR}/src/libs/common/Format.cpp
	${CMAKE_SOURCE_DIR}/src/libs/common/strerror_r.c
	${CMAKE_SOURCE_DIR}/src/libs/common/Path.cpp
)

add_test (NAME PathTest
	COMMAND PathTest
)

target_link_libraries (PathTest
	muleunit
)

add_executable (PathTraversalTest
	PathTraversalTest.cpp
	${CMAKE_SOURCE_DIR}/src/libs/common/Format.cpp
	${CMAKE_SOURCE_DIR}/src/libs/common/strerror_r.c
	${CMAKE_SOURCE_DIR}/src/libs/common/Path.cpp
)

add_test (NAME PathTraversalTest
	COMMAND PathTraversalTest
)

target_include_directories (PathTraversalTest
	PRIVATE ${CMAKE_SOURCE_DIR}/src
)

target_link_libraries (PathTraversalTest
	muleunit
)

add_executable (InternalPathGuardsTest
	InternalPathGuardsTest.cpp
	${CMAKE_SOURCE_DIR}/src/libs/common/Format.cpp
	${CMAKE_SOURCE_DIR}/src/libs/common/strerror_r.c
	${CMAKE_SOURCE_DIR}/src/libs/common/Path.cpp
)

add_test (NAME InternalPathGuardsTest
	COMMAND InternalPathGuardsTest
)

target_include_directories (InternalPathGuardsTest
	PRIVATE ${CMAKE_SOURCE_DIR}/src
)

target_link_libraries (InternalPathGuardsTest
	muleunit
)

add_executable (ECPreferencesValidationTest
	ECPreferencesValidationTest.cpp
	${CMAKE_SOURCE_DIR}/src/libs/common/Format.cpp
	${CMAKE_SOURCE_DIR}/src/libs/common/strerror_r.c
	${CMAKE_SOURCE_DIR}/src/libs/common/Path.cpp
)

add_test (NAME ECPreferencesValidationTest
	COMMAND ECPreferencesValidationTest
)

target_include_directories (ECPreferencesValidationTest
	PRIVATE ${CMAKE_SOURCE_DIR}/src
	PRIVATE ${CMAKE_SOURCE_DIR}/src/include
)

target_link_libraries (ECPreferencesValidationTest
	muleunit
)

add_executable (ClientUDPTest
	ClientUDPTest.cpp
	${CMAKE_SOURCE_DIR}/src/libs/common/Format.cpp
	${CMAKE_SOURCE_DIR}/src/libs/common/strerror_r.c
)

add_test (NAME ClientUDPTest
	COMMAND ClientUDPTest
)

target_include_directories (ClientUDPTest
	PRIVATE ${CMAKE_SOURCE_DIR}/src
	PRIVATE ${CMAKE_SOURCE_DIR}/src/include
)

target_link_libraries (ClientUDPTest
	muleunit
)

add_executable (KadPacketGuardsTest
	KadPacketGuardsTest.cpp
	${CMAKE_SOURCE_DIR}/src/MemFile.cpp
	${CMAKE_SOURCE_DIR}/src/SafeFile.cpp
	${CMAKE_SOURCE_DIR}/src/Tag.cpp
	${CMAKE_SOURCE_DIR}/src/libs/common/Format.cpp
	${CMAKE_SOURCE_DIR}/src/libs/common/strerror_r.c
)

add_test (NAME KadPacketGuardsTest
	COMMAND KadPacketGuardsTest
)

target_include_directories (KadPacketGuardsTest
	PRIVATE ${CMAKE_SOURCE_DIR}/src
	PRIVATE ${CMAKE_SOURCE_DIR}/src/include
)

target_link_libraries (KadPacketGuardsTest
	muleunit
)

add_executable (KadHandlerFuzzTest
	KadHandlerFuzzTest.cpp
	${CMAKE_SOURCE_DIR}/src/MemFile.cpp
	${CMAKE_SOURCE_DIR}/src/SafeFile.cpp
	${CMAKE_SOURCE_DIR}/src/Tag.cpp
	${CMAKE_SOURCE_DIR}/src/libs/common/Format.cpp
	${CMAKE_SOURCE_DIR}/src/libs/common/strerror_r.c
)

add_test (NAME KadHandlerFuzzTest
	COMMAND KadHandlerFuzzTest
)

target_include_directories (KadHandlerFuzzTest
	PRIVATE ${CMAKE_SOURCE_DIR}/src
	PRIVATE ${CMAKE_SOURCE_DIR}/src/include
)

target_link_libraries (KadHandlerFuzzTest
	muleunit
)

add_executable (ThreadingGuardsTest
	ThreadingGuardsTest.cpp
	${CMAKE_SOURCE_DIR}/src/libs/common/threading/ThreadGuards.cpp
	${CMAKE_SOURCE_DIR}/src/libs/common/Format.cpp
	${CMAKE_SOURCE_DIR}/src/libs/common/strerror_r.c
	${THREADING_TEST_STUB}
)

add_test (NAME ThreadingGuardsTest
	COMMAND ThreadingGuardsTest
)

target_include_directories (ThreadingGuardsTest
	PRIVATE ${CMAKE_SOURCE_DIR}/src
	PRIVATE ${CMAKE_SOURCE_DIR}/src/include
)

target_link_libraries (ThreadingGuardsTest
	muleunit
)

add_executable (DownloadQueueRaceTest
	DownloadQueueRaceTest.cpp
	${CMAKE_SOURCE_DIR}/src/libs/common/threading/ThreadGuards.cpp
	${CMAKE_SOURCE_DIR}/src/libs/common/Format.cpp
	${CMAKE_SOURCE_DIR}/src/libs/common/strerror_r.c
	${THREADING_TEST_STUB}
)

add_test (NAME DownloadQueueRaceTest
	COMMAND DownloadQueueRaceTest
)

target_include_directories (DownloadQueueRaceTest
	PRIVATE ${CMAKE_SOURCE_DIR}/src
	PRIVATE ${CMAKE_SOURCE_DIR}/src/include
)

target_link_libraries (DownloadQueueRaceTest
	muleunit
)

add_executable (PartFileAsyncTaskTest
	PartFileAsyncTaskTest.cpp
	${CMAKE_SOURCE_DIR}/src/libs/common/threading/ThreadGuards.cpp
	${CMAKE_SOURCE_DIR}/src/libs/common/Format.cpp
	${CMAKE_SOURCE_DIR}/src/libs/common/strerror_r.c
	${THREADING_TEST_STUB}
)

add_test (NAME PartFileAsyncTaskTest
	COMMAND PartFileAsyncTaskTest
)

target_include_directories (PartFileAsyncTaskTest
	PRIVATE ${CMAKE_SOURCE_DIR}/src
	PRIVATE ${CMAKE_SOURCE_DIR}/src/include
)

target_link_libraries (PartFileAsyncTaskTest
	muleunit
)

add_executable (ThreadPoolLifecycleTest
	ThreadPoolLifecycleTest.cpp
	${CMAKE_SOURCE_DIR}/src/libs/common/ThreadPool.cpp
	${CMAKE_SOURCE_DIR}/src/libs/common/threading/ThreadGuards.cpp
	${CMAKE_SOURCE_DIR}/src/libs/common/Format.cpp
	${CMAKE_SOURCE_DIR}/src/libs/common/strerror_r.c
	${THREADING_TEST_STUB}
)

add_test (NAME ThreadPoolLifecycleTest
	COMMAND ThreadPoolLifecycleTest
)

target_include_directories (ThreadPoolLifecycleTest
	PRIVATE ${CMAKE_SOURCE_DIR}/src
	PRIVATE ${CMAKE_SOURCE_DIR}/src/include
)

target_link_libraries (ThreadPoolLifecycleTest
	muleunit
)

add_executable (ShutdownSmokeTest
	ShutdownSmokeTest.cpp
	${CMAKE_SOURCE_DIR}/src/libs/common/threading/ThreadGuards.cpp
	${CMAKE_SOURCE_DIR}/src/libs/common/Format.cpp
	${CMAKE_SOURCE_DIR}/src/libs/common/strerror_r.c
	${THREADING_TEST_STUB}
)

add_test (NAME ShutdownSmokeTest
	COMMAND ShutdownSmokeTest
)

target_include_directories (ShutdownSmokeTest
	PRIVATE ${CMAKE_SOURCE_DIR}/src
	PRIVATE ${CMAKE_SOURCE_DIR}/src/include
)

target_link_libraries (ShutdownSmokeTest
	muleunit
)

add_executable (RangeMapTest
	RangeMapTest.cpp
	${CMAKE_SOURCE_DIR}/src/libs/common/Format.cpp
	${CMAKE_SOURCE_DIR}/src/libs/common/strerror_r.c
)

add_test (NAME RangeMapTest
	COMMAND RangeMapTest
)

target_include_directories (RangeMapTest
	PRIVATE ${CMAKE_SOURCE_DIR}/src
)

target_link_libraries (RangeMapTest
	muleunit
)

add_executable (UPnPUrlUtilsTest
	UPnPUrlUtilsTest.cpp
	${CMAKE_SOURCE_DIR}/src/UPnPUrlUtils.cpp
	${CMAKE_SOURCE_DIR}/src/libs/common/Format.cpp
	${CMAKE_SOURCE_DIR}/src/libs/common/strerror_r.c
)

add_test (NAME UPnPUrlUtilsTest
	COMMAND UPnPUrlUtilsTest
)

target_include_directories (UPnPUrlUtilsTest
	PRIVATE ${CMAKE_SOURCE_DIR}/src
	PRIVATE ${CMAKE_SOURCE_DIR}/src/include
)

target_link_libraries (UPnPUrlUtilsTest
	muleunit
)

if (WIN32)
	target_link_libraries (UPnPUrlUtilsTest
		ws2_32
	)
endif()

if (ENABLE_UPNP)
	add_executable (UPnPXmlSafetyTest
		UPnPXmlSafetyTest.cpp
		${CMAKE_SOURCE_DIR}/src/libs/common/Format.cpp
		${CMAKE_SOURCE_DIR}/src/libs/common/strerror_r.c
	)

	add_test (NAME UPnPXmlSafetyTest
		COMMAND UPnPXmlSafetyTest
	)

	target_include_directories (UPnPXmlSafetyTest
		PRIVATE ${CMAKE_SOURCE_DIR}/src
		PRIVATE ${CMAKE_SOURCE_DIR}/src/include
	)

	target_link_libraries (UPnPXmlSafetyTest
		muleunit
		muleappcore
	)

	if (WIN32)
		target_link_libraries (UPnPXmlSafetyTest
			ws2_32
		)
	endif()

	add_executable (UPnPFeedbackPolicyTest
		UPnPFeedbackPolicyTest.cpp
		${CMAKE_SOURCE_DIR}/src/libs/common/Format.cpp
		${CMAKE_SOURCE_DIR}/src/libs/common/strerror_r.c
	)

	add_test (NAME UPnPFeedbackPolicyTest
		COMMAND UPnPFeedbackPolicyTest
	)

	target_include_directories (UPnPFeedbackPolicyTest
		PRIVATE ${CMAKE_SOURCE_DIR}/src
		PRIVATE ${CMAKE_SOURCE_DIR}/src/include
	)

	target_link_libraries (UPnPFeedbackPolicyTest
		muleunit
		muleappcore
	)

endif()

add_executable (StringFunctionsTest
	StringFunctionsTest.cpp
	${CMAKE_SOURCE_DIR}/src/libs/common/Format.cpp
	${CMAKE_SOURCE_DIR}/src/libs/common/strerror_r.c
	${CMAKE_SOURCE_DIR}/src/libs/common/Path.cpp
)

add_test (NAME StringFunctionsTest
	COMMAND StringFunctionsTest
)

target_link_libraries (StringFunctionsTest
	muleunit
)

add_executable (TextFileTest
	TextFileTest.cpp
	${CMAKE_SOURCE_DIR}/src/libs/common/Format.cpp
	${CMAKE_SOURCE_DIR}/src/libs/common/strerror_r.c
	${CMAKE_SOURCE_DIR}/src/libs/common/Path.cpp
	${CMAKE_SOURCE_DIR}/src/libs/common/TextFile.cpp
)

add_test (NAME TextFileTest
	COMMAND TextFileTest
)

target_compile_definitions (TextFileTest
	PRIVATE "SRCDIR=${CMAKE_CURRENT_SOURCE_DIR}"
)

target_include_directories (TextFileTest
	PRIVATE ${CMAKE_SOURCE_DIR}/src
	PRIVATE ${CMAKE_SOURCE_DIR}/src/include
)

target_link_libraries (TextFileTest
	muleunit
)

add_executable (ModernizationTest
	ModernizationTest.cpp
	${CMAKE_SOURCE_DIR}/src/libs/common/Format.cpp
	${CMAKE_SOURCE_DIR}/src/libs/common/strerror_r.c
)

add_test (NAME ModernizationTest
	COMMAND ModernizationTest
)

target_include_directories (ModernizationTest
	PRIVATE ${CMAKE_SOURCE_DIR}/src
	PRIVATE ${CMAKE_SOURCE_DIR}/src/include
)

target_link_libraries (ModernizationTest
	muleunit
)

# Thread Pool Benchmark - measures threading performance for optimization planning
add_executable (ThreadPoolBenchmark
	ThreadPoolBenchmark.cpp
	${CMAKE_SOURCE_DIR}/src/libs/common/Format.cpp
	${CMAKE_SOURCE_DIR}/src/libs/common/strerror_r.c
)

add_test (NAME ThreadPoolBenchmark
	COMMAND ThreadPoolBenchmark
)

target_include_directories (ThreadPoolBenchmark
	PRIVATE ${CMAKE_SOURCE_DIR}/src
	PRIVATE ${CMAKE_SOURCE_DIR}/src/include
)

target_link_libraries (ThreadPoolBenchmark
	muleunit
)

# Download Benchmark - measures download processing performance
add_executable (DownloadBenchmark
	DownloadBenchmark.cpp
	${CMAKE_SOURCE_DIR}/src/libs/common/Format.cpp
	${CMAKE_SOURCE_DIR}/src/libs/common/strerror_r.c
)

add_test (NAME DownloadBenchmark
	COMMAND DownloadBenchmark
)

target_include_directories (DownloadBenchmark
	PRIVATE ${CMAKE_SOURCE_DIR}/src
	PRIVATE ${CMAKE_SOURCE_DIR}/src/include
)

target_link_libraries (DownloadBenchmark
	muleunit
)

set(TESTWEBKITAPI_DIR "${TOOLS_DIR}/TestWebKitAPI")
set(TESTWEBKITAPI_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/TestWebKitAPI")

include_directories(${CMAKE_BINARY_DIR}
    ${TESTWEBKITAPI_DIR}
    ${CMAKE_SOURCE_DIR}/Source
    ${DERIVED_SOURCES_JAVASCRIPTCORE_DIR}
    ${DERIVED_SOURCES_WEBCORE_DIR}
    ${JAVASCRIPTCORE_DIR}
    ${JAVASCRIPTCORE_DIR}/API
    ${JAVASCRIPTCORE_DIR}/ForwardingHeaders
    ${JAVASCRIPTCORE_DIR}/assembler
    ${JAVASCRIPTCORE_DIR}/bytecode
    ${JAVASCRIPTCORE_DIR}/disassembler
    ${JAVASCRIPTCORE_DIR}/interpreter
    ${JAVASCRIPTCORE_DIR}/heap
    ${JAVASCRIPTCORE_DIR}/jit
    ${JAVASCRIPTCORE_DIR}/llint
    ${JAVASCRIPTCORE_DIR}/parser
    ${JAVASCRIPTCORE_DIR}/runtime
    ${THIRDPARTY_DIR}/gtest/include
    ${WEBCORE_DIR}/css
    ${WEBCORE_DIR}/dom
    ${WEBCORE_DIR}/editing
    ${WEBCORE_DIR}/loader/cache
    ${WEBCORE_DIR}/platform
    ${WEBCORE_DIR}/platform/animation
    ${WEBCORE_DIR}/platform/graphics
    ${WEBCORE_DIR}/platform/text
    ${WEBCORE_DIR}/platform/network
    ${WEBCORE_DIR}/platform/network/soup
    ${WEBCORE_DIR}/rendering
    ${WEBCORE_DIR}/rendering/style
    ${WEBKIT2_DIR}/Platform/IPC
    ${WEBKIT2_DIR}/Shared
    ${WEBKIT2_DIR}/Shared/API
    ${WEBKIT2_DIR}/Shared/API/c
    ${WEBKIT2_DIR}/Shared/Plugins
    ${WEBKIT2_DIR}/UIProcess
    ${WEBKIT2_DIR}/UIProcess/API
    ${WEBKIT2_DIR}/WebProcess/InjectedBundle
    ${WEBKIT2_DIR}/WebProcess/InjectedBundle/API/c
    ${WTF_DIR}
)

set(test_wtf_LIBRARIES
    WTF
    gtest
)

set(test_webcore_LIBRARIES
    gtest
    WTF
    WebCore
)

set(test_webkit2_api_LIBRARIES
    TestWebKitAPIBase
    WTF
    JavaScriptCore
    WebKit2
    gtest
)

set(TestWebKitAPI_LIBRARIES
    WTF
    WebKit2
)

set(TestJavaScriptCore_LIBRARIES
    gtest
    JavaScriptCore
)

set(TestWTF_SOURCES
    ${TESTWEBKITAPI_DIR}/Counters.cpp
    ${TESTWEBKITAPI_DIR}/TestsController.cpp
    ${TESTWEBKITAPI_DIR}/Tests/WTF/AtomicString.cpp
    ${TESTWEBKITAPI_DIR}/Tests/WTF/CString.cpp
    ${TESTWEBKITAPI_DIR}/Tests/WTF/CheckedArithmeticOperations.cpp
    ${TESTWEBKITAPI_DIR}/Tests/WTF/DateMath.cpp
    ${TESTWEBKITAPI_DIR}/Tests/WTF/Deque.cpp
    ${TESTWEBKITAPI_DIR}/Tests/WTF/Functional.cpp
    ${TESTWEBKITAPI_DIR}/Tests/WTF/HashMap.cpp
    ${TESTWEBKITAPI_DIR}/Tests/WTF/HashSet.cpp
    ${TESTWEBKITAPI_DIR}/Tests/WTF/IntegerToStringConversion.cpp
    ${TESTWEBKITAPI_DIR}/Tests/WTF/ListHashSet.cpp
    ${TESTWEBKITAPI_DIR}/Tests/WTF/MD5.cpp
    ${TESTWEBKITAPI_DIR}/Tests/WTF/MathExtras.cpp
    ${TESTWEBKITAPI_DIR}/Tests/WTF/MediaTime.cpp
    ${TESTWEBKITAPI_DIR}/Tests/WTF/MetaAllocator.cpp
    ${TESTWEBKITAPI_DIR}/Tests/WTF/RedBlackTree.cpp
    ${TESTWEBKITAPI_DIR}/Tests/WTF/Ref.cpp
    ${TESTWEBKITAPI_DIR}/Tests/WTF/RefPtr.cpp
    ${TESTWEBKITAPI_DIR}/Tests/WTF/SHA1.cpp
    ${TESTWEBKITAPI_DIR}/Tests/WTF/SaturatedArithmeticOperations.cpp
    ${TESTWEBKITAPI_DIR}/Tests/WTF/StringBuilder.cpp
    ${TESTWEBKITAPI_DIR}/Tests/WTF/StringHasher.cpp
    ${TESTWEBKITAPI_DIR}/Tests/WTF/StringImpl.cpp
    ${TESTWEBKITAPI_DIR}/Tests/WTF/StringOperators.cpp
    ${TESTWEBKITAPI_DIR}/Tests/WTF/StringView.cpp
    ${TESTWEBKITAPI_DIR}/Tests/WTF/TemporaryChange.cpp
    ${TESTWEBKITAPI_DIR}/Tests/WTF/Vector.cpp
    ${TESTWEBKITAPI_DIR}/Tests/WTF/WTFString.cpp
    ${TESTWEBKITAPI_DIR}/Tests/WTF/WorkQueue.cpp
)

WEBKIT_INCLUDE_CONFIG_FILES_IF_EXISTS()

add_library(TestWebKitAPIInjectedBundle SHARED
    ${bundle_harness_SOURCES}
    ${TESTWEBKITAPI_DIR}/InjectedBundleController.cpp
    ${TESTWEBKITAPI_DIR}/InjectedBundleMain.cpp
    ${TESTWEBKITAPI_DIR}/PlatformUtilities.cpp
    ${TESTWEBKITAPI_DIR}/Tests/WebKit2/CanHandleRequest_Bundle.cpp
    ${TESTWEBKITAPI_DIR}/Tests/WebKit2/DidAssociateFormControls_Bundle.cpp
    ${TESTWEBKITAPI_DIR}/Tests/WebKit2/DOMWindowExtensionBasic_Bundle.cpp
    ${TESTWEBKITAPI_DIR}/Tests/WebKit2/DOMWindowExtensionNoCache_Bundle.cpp
    ${TESTWEBKITAPI_DIR}/Tests/WebKit2/DocumentStartUserScriptAlertCrash_Bundle.cpp
    ${TESTWEBKITAPI_DIR}/Tests/WebKit2/GetInjectedBundleInitializationUserDataCallback_Bundle.cpp
    ${TESTWEBKITAPI_DIR}/Tests/WebKit2/HitTestResultNodeHandle_Bundle.cpp
    ${TESTWEBKITAPI_DIR}/Tests/WebKit2/InjectedBundleBasic_Bundle.cpp
    ${TESTWEBKITAPI_DIR}/Tests/WebKit2/InjectedBundleFrameHitTest_Bundle.cpp
    ${TESTWEBKITAPI_DIR}/Tests/WebKit2/InjectedBundleInitializationUserDataCallbackWins_Bundle.cpp
    ${TESTWEBKITAPI_DIR}/Tests/WebKit2/LoadCanceledNoServerRedirectCallback_Bundle.cpp
    ${TESTWEBKITAPI_DIR}/Tests/WebKit2/MouseMoveAfterCrash_Bundle.cpp
    ${TESTWEBKITAPI_DIR}/Tests/WebKit2/NewFirstVisuallyNonEmptyLayoutFails_Bundle.cpp
    ${TESTWEBKITAPI_DIR}/Tests/WebKit2/NewFirstVisuallyNonEmptyLayoutForImages_Bundle.cpp
    ${TESTWEBKITAPI_DIR}/Tests/WebKit2/NewFirstVisuallyNonEmptyLayoutFrames_Bundle.cpp
    ${TESTWEBKITAPI_DIR}/Tests/WebKit2/NewFirstVisuallyNonEmptyLayout_Bundle.cpp
    ${TESTWEBKITAPI_DIR}/Tests/WebKit2/ParentFrame_Bundle.cpp
    ${TESTWEBKITAPI_DIR}/Tests/WebKit2/ResponsivenessTimerDoesntFireEarly_Bundle.cpp
    ${TESTWEBKITAPI_DIR}/Tests/WebKit2/ShouldGoToBackForwardListItem_Bundle.cpp
    ${TESTWEBKITAPI_DIR}/Tests/WebKit2/UserMessage_Bundle.cpp
    ${TESTWEBKITAPI_DIR}/Tests/WebKit2/WillLoad_Bundle.cpp
    ${TESTWEBKITAPI_DIR}/Tests/WebKit2/WillSendSubmitEvent_Bundle.cpp
)

target_link_libraries(TestWebKitAPIInjectedBundle ${TestWebKitAPI_LIBRARIES})
add_dependencies(TestWebKitAPIInjectedBundle ${ForwardingHeadersForTestWebKitAPI_NAME})

get_property(TestWebKitAPIInjectedBundle_PATH TARGET TestWebKitAPIInjectedBundle PROPERTY LOCATION)

add_definitions(-DGTEST_LINKED_AS_SHARED_LIBRARY=1 -DGTEST_HAS_RTTI=0
    -DTEST_WEBKIT2_RESOURCES_DIR=\"${TESTWEBKITAPI_DIR}/Tests/WebKit2\"
    -DTEST_INJECTED_BUNDLE_PATH=\"${TestWebKitAPIInjectedBundle_PATH}\"
)

# FIXME: This works around compatibility problems in the old version of the third-pary
# googletest source code checkout. It should be removed once we upgrade to a newer version.
if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
    add_definitions(-DGTEST_HAS_TR1_TUPLE=0)
endif ()

add_executable(TestWTF
    ${test_main_SOURCES}
    ${TestWTF_SOURCES}
)

target_link_libraries(TestWTF ${test_wtf_LIBRARIES})
add_dependencies(TestWTF ${ForwardingHeadersForTestWebKitAPI_NAME} ${ForwardingNetworkHeadersForTestWebKitAPI_NAME})
add_test(TestWTF ${TESTWEBKITAPI_RUNTIME_OUTPUT_DIRECTORY}/WTF/TestWTF)
set_tests_properties(TestWTF PROPERTIES TIMEOUT 60)
    set_target_properties(TestWTF PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY ${TESTWEBKITAPI_RUNTIME_OUTPUT_DIRECTORY}/WTF
    )


add_executable(TestJavaScriptCore
    ${test_main_SOURCES}
    ${TESTWEBKITAPI_DIR}/TestsController.cpp
    ${TESTWEBKITAPI_DIR}/Tests/JavaScriptCore/VMInspector.cpp
)

target_link_libraries(TestJavaScriptCore ${TestJavaScriptCore_LIBRARIES})
add_dependencies(TestJavaScriptCore ${ForwardingHeadersForTestWebKitAPI_NAME})

add_test(TestJavaScriptCore ${TESTWEBKITAPI_RUNTIME_OUTPUT_DIRECTORY}/JavaScriptCore/TestJavaScriptCore)
set_tests_properties(TestJavaScriptCore PROPERTIES TIMEOUT 60)
set_target_properties(TestJavaScriptCore PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${TESTWEBKITAPI_RUNTIME_OUTPUT_DIRECTORY}/JavaScriptCore)
add_dependencies(TestJavaScriptCore JavaScriptCore ${ForwardingHeadersForTestWebKitAPI_NAME} ${ForwardingNetworkHeadersForTestWebKitAPI_NAME})

foreach (testName ${test_webcore_BINARIES})
    add_executable(${testName} ${test_main_SOURCES} ${TESTWEBKITAPI_DIR}/TestsController.cpp ${TESTWEBKITAPI_DIR}/Tests/WebCore/${testName}.cpp)
    add_test(${testName} ${TESTWEBKITAPI_RUNTIME_OUTPUT_DIRECTORY}/WebCore/${testName})
    set_tests_properties(${testName} PROPERTIES TIMEOUT 60)
    target_link_libraries(${testName} ${test_webcore_LIBRARIES})
    set_target_properties(${testName} PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY ${TESTWEBKITAPI_RUNTIME_OUTPUT_DIRECTORY}/WebCore
    )
endforeach ()

add_library(TestWebKitAPIBase
    ${test_main_SOURCES}
    ${webkit2_api_harness_SOURCES}
    ${TESTWEBKITAPI_DIR}/JavaScriptTest.cpp
    ${TESTWEBKITAPI_DIR}/PlatformUtilities.cpp
    ${TESTWEBKITAPI_DIR}/TestsController.cpp
)

add_dependencies(TestWebKitAPIBase WebKit2 ${ForwardingHeadersForTestWebKitAPI_NAME} ${ForwardingNetworkHeadersForTestWebKitAPI_NAME})

foreach (testName ${test_webkit2_api_BINARIES})
    get_filename_component(testBaseName ${testName} NAME)
    add_executable(${testBaseName} ${TESTWEBKITAPI_DIR}/Tests/WebKit2/${testName}.cpp)
    add_test(${testBaseName} ${TESTWEBKITAPI_RUNTIME_OUTPUT_DIRECTORY}/WebKit2/${testBaseName})
    set_tests_properties(${testBaseName} PROPERTIES TIMEOUT 60)
    target_link_libraries(${testBaseName} ${test_webkit2_api_LIBRARIES})
    set_target_properties(${testBaseName} PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY ${TESTWEBKITAPI_RUNTIME_OUTPUT_DIRECTORY}/WebKit2
    )
endforeach ()

# We don't run tests that are expected to fail. We could use the WILL_FAIL
# property, but it reports failure when the test crashes or timeouts and would
# make the bot red.
foreach (testName ${test_webkit2_api_fail_BINARIES})
    add_executable(${testName} ${TESTWEBKITAPI_DIR}/Tests/WebKit2/${testName}.cpp)
    target_link_libraries(${testName} ${test_webkit2_api_LIBRARIES})
    set_target_properties(${testName} PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY ${TESTWEBKITAPI_RUNTIME_OUTPUT_DIRECTORY}/WebKit2/failure
    )
endforeach ()

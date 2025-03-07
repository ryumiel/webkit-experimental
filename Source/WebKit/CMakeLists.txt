set(WebKit_SOURCES
)

set(WebKit_INCLUDE_DIRECTORIES
    "${DERIVED_SOURCES_DIR}"
    "${DERIVED_SOURCES_DIR}/ForwardingHeaders"
    "${DERIVED_SOURCES_DIR}/ForwardingHeaders/JavaScriptCore"
    "${DERIVED_SOURCES_DIR}/ForwardingHeaders/WebCore"
    "${DERIVED_SOURCES_DIR}/ForwardingHeaders/WebKitLegacy"
    "${JAVASCRIPTCORE_DIR}"
    "${JAVASCRIPTCORE_DIR}/disassembler"
    "${CMAKE_SOURCE_DIR}/Source"
    "${WTF_DIR}"
)

set(WebKit_LIBRARIES
    JavaScriptCore
    WTF
    WebCore
)

WEBKIT_INCLUDE_CONFIG_FILES_IF_EXISTS()

include_directories(${WebKit_INCLUDE_DIRECTORIES})
add_library(WebKit ${WebKit_LIBRARY_TYPE} ${WebKit_SOURCES})
add_dependencies(WebKit WebCore)
target_link_libraries(WebKit ${WebKit_LIBRARIES})
set_target_properties(WebKit PROPERTIES FOLDER "WebKit")
set_target_properties(WebKit PROPERTIES LINK_INTERFACE_LIBRARIES "")

if (WebKit_OUTPUT_NAME)
    set_target_properties(WebKit PROPERTIES OUTPUT_NAME ${WebKit_OUTPUT_NAME})
endif ()

POPULATE_LIBRARY_VERSION(WEBKIT)
set_target_properties(WebKit PROPERTIES VERSION ${WEBKIT_VERSION} SOVERSION ${WEBKIT_VERSION_MAJOR})
install(TARGETS WebKit DESTINATION "${LIB_INSTALL_DIR}")
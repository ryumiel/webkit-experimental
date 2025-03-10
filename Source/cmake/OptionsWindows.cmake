add_definitions(-D_HAS_EXCEPTIONS=0 -DNOMINMAX -DUNICODE)

WEBKIT_OPTION_BEGIN()
WEBKIT_OPTION_DEFAULT_PORT_VALUE(USE_SYSTEM_MALLOC ON)
WEBKIT_OPTION_END()

include_directories("$ENV{WEBKIT_LIBRARIES}/include")
link_directories("$ENV{WEBKIT_LIBRARIES}/lib$(PlatformArchitecture)")
if (MSVC)
    add_definitions(
        /wd4018 /wd4068 /wd4099 /wd4100 /wd4127 /wd4138 /wd4146 /wd4180 /wd4189 /wd4201 /wd4244 /wd4251 /wd4267 /wd4275 /wd4288
        /wd4291 /wd4305 /wd4309 /wd4344 /wd4355 /wd4389 /wd4396 /wd4481 /wd4503 /wd4505 /wd4510 /wd4512 /wd4530 /wd4610 /wd4702
        /wd4706 /wd4800 /wd4819 /wd4951 /wd4952 /wd4996 /wd6011 /wd6031 /wd6211 /wd6246 /wd6255 /wd6387
    )

    string(REGEX REPLACE "/EH[a-z]+" "" CMAKE_CXX_FLAGS ${CMAKE_CXX_FLAGS}) # Disable C++ exceptions
    string(REGEX REPLACE "/GR" "" CMAKE_CXX_FLAGS ${CMAKE_CXX_FLAGS}) # Disable RTTI

    if (NOT MSVC_VERSION LESS 1500)
        set(CMAKE_C_FLAGS "/MP ${CMAKE_C_FLAGS}")
        set(CMAKE_CXX_FLAGS "/MP ${CMAKE_CXX_FLAGS}")
    endif ()
endif ()

set(PORT Win)
set(JavaScriptCore_LIBRARY_TYPE SHARED)
set(WTF_LIBRARY_TYPE SHARED)
set(ICU_LIBRARIES libicuuc$(DebugSuffix) libicuin$(DebugSuffix))

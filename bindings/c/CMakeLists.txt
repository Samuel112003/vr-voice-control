# C API of the library (DLL voicecontrol_c), loaded by Unity through P/Invoke.

# The static library is linked into a shared one: it must be position independent (Linux/Android).
set_target_properties(voicecontrol PROPERTIES POSITION_INDEPENDENT_CODE ON)

add_library(voicecontrol_c SHARED src/c_api.cpp)

target_include_directories(voicecontrol_c
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
)
target_compile_definitions(voicecontrol_c PRIVATE VC_C_API_BUILD)
target_link_libraries(voicecontrol_c PRIVATE VoiceControl::voicecontrol)

if(MSVC)
    target_compile_options(voicecontrol_c PRIVATE /W4 /utf-8)
else()
    target_compile_options(voicecontrol_c PRIVATE -Wall -Wextra -Wpedantic)
endif()

if(MINGW)
    # The DLL must not depend on the MinGW runtime DLLs (Unity would fail to load it).
    target_link_options(voicecontrol_c PRIVATE -static)
endif()
vc_copy_vosk_runtime(voicecontrol_c)

if(VC_BUILD_TESTS)
    # test_c_api and voicecontrol_c are built in the same folder: the DLL is found at run time.
    add_executable(test_c_api tests/test_c_api.c)
    target_link_libraries(test_c_api PRIVATE voicecontrol_c)
    add_test(NAME test_c_api COMMAND test_c_api)
endif()

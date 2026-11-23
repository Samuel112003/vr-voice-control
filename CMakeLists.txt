cmake_minimum_required(VERSION 3.16)

project(VoiceControlVR
    VERSION 0.1.0
    DESCRIPTION "Bibliotheque de commande vocale (acteur.action) pour applications VR"
    LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

option(VC_BUILD_EXAMPLES "Construire les exemples" ON)
option(VC_BUILD_TESTS    "Construire les tests"    ON)
option(VC_WITH_VOSK      "Activer l'adaptateur Vosk (necessite Vosk dans third_party/vosk)" OFF)

# ---------------------------------------------------------------------------
# Bibliotheque principale
# ---------------------------------------------------------------------------
add_library(voicecontrol
    src/vocabulary.cpp
    src/command_parser.cpp
    src/voice_controller.cpp
    src/audio.cpp
)
add_library(VoiceControl::voicecontrol ALIAS voicecontrol)

target_include_directories(voicecontrol
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
)

if(WIN32)
    target_link_libraries(voicecontrol PUBLIC winmm)   # capture micro (Microphone)
endif()

if(MSVC)
    target_compile_options(voicecontrol PRIVATE /W4 /utf-8)
else()
    target_compile_options(voicecontrol PRIVATE -Wall -Wextra -Wpedantic)
endif()

if(VC_WITH_VOSK)
    # Disposition attendue : third_party/vosk/include/vosk_api.h
    #                        third_party/vosk/lib/<plateforme>/libvosk.(dll|lib|so)
    set(VC_VOSK_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/third_party/vosk" CACHE PATH "Dossier de Vosk")
    if(ANDROID)
        set(_vc_vosk_default_platform "android-${ANDROID_ABI}")
    elseif(WIN32)
        set(_vc_vosk_default_platform "win64")
    else()
        set(_vc_vosk_default_platform "linux-x86_64")
    endif()
    set(VC_VOSK_PLATFORM "${_vc_vosk_default_platform}" CACHE STRING "Sous-dossier de third_party/vosk/lib")
    set(VC_VOSK_LIB_DIR "${VC_VOSK_ROOT}/lib/${VC_VOSK_PLATFORM}")

    find_path(VOSK_INCLUDE_DIR vosk_api.h PATHS "${VC_VOSK_ROOT}/include" NO_DEFAULT_PATH)
    if(WIN32)
        find_file(VOSK_DLL libvosk.dll PATHS "${VC_VOSK_LIB_DIR}" NO_DEFAULT_PATH)
        find_file(VOSK_IMPLIB libvosk.lib PATHS "${VC_VOSK_LIB_DIR}" NO_DEFAULT_PATH)
        set(_vc_vosk_found VOSK_DLL VOSK_IMPLIB)
    else()
        find_library(VOSK_LIBRARY NAMES vosk PATHS "${VC_VOSK_LIB_DIR}" NO_DEFAULT_PATH)
        set(_vc_vosk_found VOSK_LIBRARY)
    endif()
    foreach(_v VOSK_INCLUDE_DIR ${_vc_vosk_found})
        if(NOT ${_v})
            message(FATAL_ERROR "VC_WITH_VOSK : ${_v} introuvable dans ${VC_VOSK_ROOT} (voir third_party/README.md)")
        endif()
    endforeach()

    add_library(Vosk::vosk SHARED IMPORTED)
    set_target_properties(Vosk::vosk PROPERTIES INTERFACE_INCLUDE_DIRECTORIES "${VOSK_INCLUDE_DIR}")
    if(WIN32)
        set_target_properties(Vosk::vosk PROPERTIES IMPORTED_LOCATION "${VOSK_DLL}" IMPORTED_IMPLIB "${VOSK_IMPLIB}")
        # DLL a copier a cote des executables (libvosk + son runtime MinGW)
        file(GLOB VC_VOSK_RUNTIME_DLLS "${VC_VOSK_LIB_DIR}/*.dll")
    else()
        set_target_properties(Vosk::vosk PROPERTIES IMPORTED_LOCATION "${VOSK_LIBRARY}")
    endif()

    target_sources(voicecontrol PRIVATE src/vosk_recognizer.cpp)
    target_link_libraries(voicecontrol PUBLIC Vosk::vosk)
    target_compile_definitions(voicecontrol PUBLIC VC_WITH_VOSK)
    message(STATUS "VC_WITH_VOSK : Vosk trouve dans ${VC_VOSK_LIB_DIR}")
endif()

# Copie les DLL de Vosk a cote d'un executable (Windows).
function(vc_copy_vosk_runtime target)
    if(VC_WITH_VOSK AND WIN32 AND VC_VOSK_RUNTIME_DLLS)
        add_custom_command(TARGET ${target} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different ${VC_VOSK_RUNTIME_DLLS} $<TARGET_FILE_DIR:${target}>)
        if(MINGW)
            # libvosk.dll apporte son propre runtime MinGW (libstdc++, libgcc,
            # libwinpthread, plus anciens) copie a cote de l'executable :
            # l'executable lie le sien en statique pour ne pas charger ces DLL.
            target_link_options(${target} PRIVATE -static)
        endif()
    endif()
endfunction()

# ---------------------------------------------------------------------------
# Exemples et tests
# ---------------------------------------------------------------------------
if(VC_BUILD_EXAMPLES)
    add_subdirectory(examples/drone_demo)
endif()

if(VC_BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()

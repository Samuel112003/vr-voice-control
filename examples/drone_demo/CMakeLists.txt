add_executable(drone_demo main.cpp)
target_link_libraries(drone_demo PRIVATE VoiceControl::voicecontrol)
if(VC_WITH_VOSK)
    target_compile_definitions(drone_demo PRIVATE
        VC_DEFAULT_MODEL_DIR="${PROJECT_SOURCE_DIR}/models/vosk-model-small-fr-0.22")
endif()
vc_copy_vosk_runtime(drone_demo)

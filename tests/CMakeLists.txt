add_executable(test_parser test_parser.cpp)
target_link_libraries(test_parser PRIVATE VoiceControl::voicecontrol)
add_test(NAME test_parser COMMAND test_parser)
vc_copy_vosk_runtime(test_parser)

#pragma once

// En-tête unique regroupant l'API publique de la bibliothèque.
#include "voicecontrol/audio.hpp"
#include "voicecontrol/command.hpp"
#include "voicecontrol/command_parser.hpp"
#include "voicecontrol/speech_recognizer.hpp"
#include "voicecontrol/vocabulary.hpp"
#include "voicecontrol/voice_controller.hpp"

#ifdef VC_WITH_VOSK
#include "voicecontrol/vosk_recognizer.hpp"
#endif

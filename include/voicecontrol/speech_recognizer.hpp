#pragma once

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

namespace vc {

/// Abstraction du moteur de reconnaissance vocale (speech-to-text).
/// L'implémentation de référence est VoskRecognizer (vosk_recognizer.hpp) ; l'interface permet
/// d'en changer ou de simuler la reconnaissance dans les tests.
///
/// L'audio est fourni par l'application hôte (Unity / Unreal / micro natif) :
/// PCM 16 bits signé, mono, à la fréquence indiquée à la construction.
class ISpeechRecognizer {
public:
    virtual ~ISpeechRecognizer() = default;

    /// Restreint la reconnaissance à une liste d'expressions (évite drone/drôle).
    virtual void setGrammar(const std::vector<std::string>& phrases) = 0;

    /// Envoie un bloc audio. Retourne true si une phrase complète est disponible.
    virtual bool acceptAudio(const std::int16_t* samples, std::size_t count) = 0;

    /// Texte de la dernière phrase complète (et réinitialise le résultat).
    virtual std::string takeResult() = 0;

    /// Fin du flux audio (fin de fichier, arrêt du micro) : force la fin de la
    /// phrase en cours. Retourne true si un résultat est disponible.
    virtual bool finish() { return false; }
};

} // namespace vc

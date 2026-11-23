#pragma once

#include "voicecontrol/speech_recognizer.hpp"

#include <string>
#include <vector>

// Types opaques de l'API C de Vosk (vosk_api.h n'est pas exposé aux utilisateurs).
struct VoskModel;
struct VoskRecognizer;

namespace vc {

/// Moteur de reconnaissance basé sur Vosk (hors ligne).
/// Disponible seulement si la bibliothèque est compilée avec VC_WITH_VOSK=ON.
///
///   ctrl.setRecognizer(std::make_unique<vc::VoskRecognizer>("models/vosk-model-small-fr-0.22"));
///   ctrl.feedAudio(samples, count);   // PCM 16 bits mono, 16 kHz par défaut
///
/// Non thread-safe : un même objet ne doit être utilisé que par un seul thread.
class VoskRecognizer : public ISpeechRecognizer {
public:
    /// Charge le modèle (dossier). Lève std::runtime_error en cas d'échec.
    explicit VoskRecognizer(const std::string& modelPath, float sampleRate = 16000.0f);
    ~VoskRecognizer() override;

    VoskRecognizer(const VoskRecognizer&) = delete;
    VoskRecognizer& operator=(const VoskRecognizer&) = delete;

    /// Restreint la reconnaissance à ces expressions (+ "[unk]" pour les bruits).
    /// Les expressions contenant un mot absent du modèle (ex : "drone 1") sont ignorées.
    void setGrammar(const std::vector<std::string>& phrases) override;

    /// Expressions écartées au dernier setGrammar, car un de leurs mots est
    /// inconnu du modèle : Vosk ne pourra jamais les entendre.
    const std::vector<std::string>& ignoredPhrases() const { return ignored_; }
    bool acceptAudio(const std::int16_t* samples, std::size_t count) override;
    std::string takeResult() override;
    bool finish() override;
    void reset() override;

    float sampleRate() const { return sampleRate_; }

    /// Niveau de log de Vosk/Kaldi : -1 = silencieux, 0 = défaut.
    static void setLogLevel(int level);

private:
    void rebuildRecognizer();
    bool storeResult(const char* json);

    ::VoskModel* model_ = nullptr;
    ::VoskRecognizer* recognizer_ = nullptr;
    float sampleRate_;
    std::string grammarJson_;   // vide = vocabulaire complet du modèle
    std::vector<std::string> ignored_;
    std::string result_;
};

/// Extrait le champ "text" d'un résultat JSON de Vosk (exposé pour les tests).
std::string extractVoskText(const std::string& json);

} // namespace vc

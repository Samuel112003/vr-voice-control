#pragma once

#include <cstdint>
#include <memory>
#include <string>
#include <vector>

namespace vc {

/// Audio PCM 16 bits signé, mono.
struct AudioClip {
    std::uint32_t sampleRate = 16000;
    std::vector<std::int16_t> samples;
};

/// Lit un fichier WAV PCM 16 bits mono. Lève std::runtime_error sinon.
AudioClip readWav(const std::string& path);

/// Capture du micro par défaut, en PCM 16 bits mono.
///
/// Utile pour les tests et les applications natives. Dans Unity / Unreal,
/// l'audio vient plutôt de l'API micro du moteur, envoyée à
/// VoiceController::feedAudio.
///
/// Implémenté sous Windows uniquement ; ailleurs, le constructeur lève
/// std::runtime_error (voir Microphone::available()).
class Microphone {
public:
    /// Ouvre le micro et démarre la capture. Lève std::runtime_error en cas d'échec.
    explicit Microphone(std::uint32_t sampleRate = 16000, unsigned blockMs = 100);
    ~Microphone();

    Microphone(const Microphone&) = delete;
    Microphone& operator=(const Microphone&) = delete;

    /// Attend le prochain bloc audio (au plus `timeoutMs`). Retourne false si
    /// aucun bloc n'est arrivé entre-temps.
    bool read(std::vector<std::int16_t>& block, unsigned timeoutMs = 200);

    std::uint32_t sampleRate() const { return sampleRate_; }

    /// true si la capture micro est implémentée sur cette plateforme.
    static bool available();

private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
    std::uint32_t sampleRate_;
};

} // namespace vc

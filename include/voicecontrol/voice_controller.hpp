#pragma once

#include "voicecontrol/audio.hpp"
#include "voicecontrol/command.hpp"
#include "voicecontrol/command_parser.hpp"
#include "voicecontrol/speech_recognizer.hpp"
#include "voicecontrol/vocabulary.hpp"

#include <cstddef>
#include <cstdint>
#include <functional>
#include <memory>
#include <string>
#include <vector>

namespace vc {

/// Point d'entrée de la bibliothèque pour l'application hôte.
///
///   vc::VoiceController ctrl;
///   auto& vocab = ctrl.vocabulary();
///   auto& avance = vocab.term({"avance", "avancez"});
///   auto& drone = vocab.actorType(vocab.term({"drones"}));
///   drone.order({avance}, vc::method(&Drone::avancer));
///   vocab.actor(drone, vocab.term({"drone 1", "drone un"})).setData(&drone1);
///   ctrl.handleText("drone un avance");
class VoiceController {
public:
    /// Appelé pour chaque ordre reconnu mais non exécuté (voir Rejection).
    using UnhandledCallback = std::function<void(const Rejection&)>;

    VoiceController() : parser_(vocabulary_) {}

    Vocabulary& vocabulary() { return vocabulary_; }
    const Vocabulary& vocabulary() const { return vocabulary_; }

    /// Analyse un texte et exécute les ordres trouvés. Retourne les ordres exécutés.
    std::vector<Command> handleText(const std::string& text);

    /// Exécute un ordre (pour chaque acteur, ou une fois pour un ordre global).
    void dispatch(const Command& command) const;

    void onUnhandled(UnhandledCallback callback) { unhandled_ = std::move(callback); }

    /// Acteurs visés : ils le restent d'une phrase à l'autre jusqu'à ce que
    /// d'autres soient nommés, ou jusqu'à clearTargets().
    const std::vector<const Actor*>& targets() const { return parser_.targets(); }
    void clearTargets() { parser_.clearTargets(); }

    /// Analyseur (réglage du délai d'attente des ordres incomplets, horloge de test...).
    CommandParser& parser() { return parser_; }

    /// Branche un moteur de reconnaissance. Sa grammaire est mise à jour
    /// avec les expressions du vocabulaire courant.
    void setRecognizer(std::unique_ptr<ISpeechRecognizer> recognizer);

    /// Relit le vocabulaire et met à jour la grammaire du moteur.
    void refreshGrammar();

    /// Transmet l'audio au moteur ; si une phrase est complète, la traite.
    std::vector<Command> feedAudio(const std::int16_t* samples, std::size_t count);

    /// Fin du flux audio : traite la phrase éventuellement en cours.
    std::vector<Command> finishAudio();

    /// Traite un enregistrement complet (découpé en blocs, puis finishAudio).
    /// Le moteur doit avoir été créé à la fréquence du clip.
    std::vector<Command> processAudio(const AudioClip& clip);

    /// Écoute le micro tant que `keepListening()` renvoie true, puis termine
    /// la phrase en cours. Les actions sont appelées sur le thread appelant.
    void listen(Microphone& microphone, const std::function<bool()>& keepListening);

    /// Appelé avec chaque phrase retranscrite par le moteur, avant analyse
    /// (utile pour l'affichage ou le debug).
    void onTranscript(std::function<void(const std::string&)> callback) { transcript_ = std::move(callback); }

private:
    std::vector<Command> handleRecognized();

    Vocabulary vocabulary_;
    CommandParser parser_;
    std::unique_ptr<ISpeechRecognizer> recognizer_;
    UnhandledCallback unhandled_;
    std::function<void(const std::string&)> transcript_;
};

} // namespace vc

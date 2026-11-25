#pragma once

#include "voicecontrol/audio.hpp"
#include "voicecontrol/command.hpp"
#include "voicecontrol/command_parser.hpp"
#include "voicecontrol/speech_recognizer.hpp"
#include "voicecontrol/vocabulary.hpp"

#include <atomic>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <memory>
#include <string>
#include <vector>

namespace vc {

/// État de l'écoute (bouton on/off du contrôle vocal).
enum class ListeningState {
    Active,       // tout est exécuté
    SoftPaused,   // veille : rien n'est exécuté jusqu'au terme de réveil
    Paused,       // pause : tout est ignoré jusqu'à resume() (action non vocale)
};

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

    // --- Acteurs visés (sélection) --------------------------------------------
    // À appeler depuis le thread de l'audio, ou depuis une action.

    /// Acteurs visés : ils le restent d'une phrase à l'autre jusqu'à ce que
    /// d'autres soient nommés, jusqu'à clearTargets(), ou jusqu'à l'expiration
    /// du délai de sélection.
    const std::vector<const Actor*>& targets() const { return parser_.targets(); }

    /// Désélectionne les acteurs visés (et abandonne l'ordre en attente).
    void clearTargets();

    /// Action prête à associer à un terme de l'application pour désélectionner
    /// à la voix : vocab.order({repos}, ctrl.clearTargetsAction());
    GlobalAction clearTargetsAction() {
        return [this](const Arguments&) { clearTargets(); };
    }

    /// Délai de sélection (10 s par défaut) : sans ordre adressé aux acteurs
    /// visés pendant ce délai, ils sont désélectionnés. vc::never : jamais.
    /// Le temps passé en pause ou en veille ne compte pas.
    void setTargetTimeout(std::chrono::milliseconds timeout) { parser_.setTargetTimeout(timeout); }
    std::chrono::milliseconds targetTimeout() const { return parser_.targetTimeout(); }

    /// Appelé quand les acteurs visés changent (nommés, désélectionnés, délai
    /// écoulé), avec la nouvelle sélection (vide : plus personne).
    void onTargetsChanged(std::function<void(const std::vector<const Actor*>&)> callback) {
        targetsChanged_ = std::move(callback);
    }

    /// Applique le délai de sélection maintenant. feedAudio() et listen() le
    /// font d'eux-mêmes ; à appeler régulièrement (ex : Update() d'Unity) si
    /// l'application n'envoie pas d'audio en continu.
    void update();

    /// Analyseur (réglage du délai d'attente des ordres incomplets, horloge de test...).
    CommandParser& parser() { return parser_; }

    // --- Pause / veille ------------------------------------------------------
    // Ces méthodes peuvent être appelées depuis n'importe quel thread (bouton de
    // l'interface pendant que listen() tourne ailleurs), ou depuis une action.
    // Entrer en pause ou en veille oublie la phrase en cours et l'ordre en
    // attente ; les acteurs visés sont conservés.

    /// Vraie pause : l'audio et le texte sont ignorés jusqu'à resume().
    void pause();

    /// Veille : la reconnaissance continue, mais rien n'est exécuté jusqu'à ce
    /// qu'une forme de `wakeTerm` soit entendue. La suite de cette phrase est
    /// alors exécutée normalement. `wakeTerm` doit appartenir au vocabulaire
    /// (pour être dans la grammaire du moteur).
    void softPause(const Term& wakeTerm);

    /// Reprend l'écoute (depuis la pause ou la veille).
    void resume();

    ListeningState listeningState() const { return state_.load(); }

    /// Appelé à chaque changement d'état (sur le thread qui l'a provoqué).
    void onListeningChanged(std::function<void(ListeningState)> callback) { listeningChanged_ = std::move(callback); }

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
    void setState(ListeningState state);
    /// Applique, sur le thread de l'audio, l'oubli demandé par pause()/softPause().
    void applyReset();

    std::atomic<ListeningState> state_{ListeningState::Active};
    std::atomic<const Term*> wakeTerm_{nullptr};
    std::atomic<bool> resetRequested_{false};
    std::atomic<bool> touchRequested_{false};   // sortie de pause : le délai de sélection repart

    void notifyTargets();
    std::vector<const Actor*> notifiedTargets_;
    std::function<void(const std::vector<const Actor*>&)> targetsChanged_;
    std::function<void(ListeningState)> listeningChanged_;

    Vocabulary vocabulary_;
    CommandParser parser_;
    std::unique_ptr<ISpeechRecognizer> recognizer_;
    UnhandledCallback unhandled_;
    std::function<void(const std::string&)> transcript_;
};

} // namespace vc

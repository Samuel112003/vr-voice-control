#pragma once

#include "voicecontrol/command.hpp"
#include "voicecontrol/vocabulary.hpp"

#include <chrono>
#include <functional>
#include <string>
#include <vector>

namespace vc {

/// Durée « jamais » : pour setTargetTimeout(), les acteurs visés ne sont
/// jamais désélectionnés automatiquement.
inline constexpr std::chrono::milliseconds never = std::chrono::milliseconds::max();

/// Transforme un texte reconnu en ordres à exécuter.
///
/// L'analyseur a un état :
///  - les **acteurs visés** restent visés d'une phrase à l'autre, jusqu'à ce
///    que d'autres soient nommés, qu'ils soient désélectionnés (clearTargets),
///    ou qu'on ne leur ait rien dit pendant le délai de sélection ;
///  - un ordre interrompu en fin de phrase avant ses acteurs consommés
///    (« drone un tire sur… ») est **mis en attente** : si la phrase suivante
///    commence par des noms, ils le complètent.
///
/// Règles détaillées dans documentation/02-concepts.md.
class CommandParser {
public:
    using Clock = std::chrono::steady_clock;
    /// Reçoit chaque ordre dès qu'il est reconnu ; renvoie false pour arrêter l'analyse.
    using CommandSink = std::function<bool(const Command&)>;
    /// Reçoit chaque ordre reconnu mais non exécutable.
    using RejectionSink = std::function<void(const Rejection&)>;

    explicit CommandParser(const Vocabulary& vocabulary) : vocabulary_(vocabulary) {}

    /// Analyse une phrase et transmet chaque ordre à `onCommand` **dès qu'il est
    /// reconnu**, dans l'ordre de la phrase. Si `onCommand` exécute l'ordre et que
    /// l'action change l'état (clearTargets()...), la suite de la phrase en tient
    /// compte : « repos drone deux avance » désélectionne, puis vise drone 2.
    void parse(const std::string& text, const CommandSink& onCommand, const RejectionSink& onRejection = {});

    /// Analyse seule : retourne les ordres reconnus, dans l'ordre. Les ordres
    /// reconnus mais non exécutables sont ajoutés à `rejected`.
    std::vector<Command> parse(const std::string& text, std::vector<Rejection>* rejected = nullptr);

    /// Acteurs actuellement visés.
    const std::vector<const Actor*>& targets() const { return targets_; }

    /// Désélectionne les acteurs visés, et abandonne l'ordre en attente (il
    /// était destiné à ces acteurs).
    void clearTargets() {
        targets_.clear();
        cancelPending();
    }

    /// Délai de sélection : les acteurs visés sont désélectionnés si aucun ordre
    /// ne leur a été adressé (et qu'ils n'ont pas été renommés) pendant ce délai.
    /// 10 s par défaut ; vc::never pour ne jamais désélectionner automatiquement.
    void setTargetTimeout(std::chrono::milliseconds timeout) { targetTimeout_ = timeout; }
    std::chrono::milliseconds targetTimeout() const { return targetTimeout_; }

    /// Applique le délai de sélection maintenant (appelé au début de chaque
    /// phrase). Retourne true si les acteurs visés viennent d'être désélectionnés.
    bool expireTargets();

    /// Fait repartir le délai de sélection, comme si l'on venait de parler aux
    /// acteurs visés (utilisé à la sortie de pause : le temps de pause ne compte pas).
    void touchTargets() { lastTargetUse_ = now(); }

    /// Délai pendant lequel un ordre en attente peut être complété par la
    /// phrase suivante (5 s par défaut). 0 : pas d'attente, l'ordre incomplet
    /// est rejeté tout de suite.
    void setPendingTimeout(std::chrono::milliseconds timeout) { pendingTimeout_ = timeout; }
    std::chrono::milliseconds pendingTimeout() const { return pendingTimeout_; }
    bool hasPending() const { return !pending_.empty(); }
    /// Abandonne l'ordre en attente, sans le signaler.
    void cancelPending() {
        pending_.clear();
        pendingCandidates_.clear();
    }

    /// Remplace l'horloge (tests).
    void setClock(std::function<Clock::time_point()> now) { now_ = std::move(now); }

    /// Noms (acteurs, ou type = tous ses acteurs) consécutifs à words[pos].
    std::optional<NamedActors> matchName(const std::vector<std::string>& words, std::size_t pos) const;

    /// Acteurs consommés par une position acteurs à words[pos] : les noms
    /// consécutifs, sauf le dernier s'il est suivi d'un ordre qu'il sait
    /// exécuter (il en est alors le sujet).
    std::optional<NamedActors> readActors(const std::vector<std::string>& words, std::size_t pos) const;

private:
    /// Vrai si un ordre d'un type de `actors` correspond à words[pos].
    bool startsOrderFor(const std::vector<const Actor*>& actors, const std::vector<std::string>& words,
                        std::size_t pos) const;

    /// Destinataires des résultats pendant une analyse.
    struct Sinks {
        const CommandSink& command;
        const RejectionSink& rejection;
        bool stopped = false;   // onCommand a demandé l'arrêt
    };

    /// Cherche un ordre à words[pos] ; retourne la position suivante, ou pos si rien.
    std::size_t matchOrder(const std::vector<std::string>& words, std::size_t pos, Sinks& sinks);

    Clock::time_point now() const { return now_ ? now_() : Clock::now(); }

    const Vocabulary& vocabulary_;
    std::vector<const Actor*> targets_;
    Clock::time_point lastTargetUse_{};              // dernière fois qu'on a nommé les cibles ou qu'elles ont exécuté
    std::chrono::milliseconds targetTimeout_{10000};

    std::vector<std::string> pending_;            // mots de l'ordre en attente
    std::vector<const Order*> pendingCandidates_;
    Clock::time_point pendingSince_{};
    std::chrono::milliseconds pendingTimeout_{5000};
    std::function<Clock::time_point()> now_;
};

} // namespace vc

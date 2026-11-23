#pragma once

#include "voicecontrol/command.hpp"
#include "voicecontrol/vocabulary.hpp"

#include <chrono>
#include <functional>
#include <string>
#include <vector>

namespace vc {

/// Transforme un texte reconnu en ordres à exécuter.
///
/// L'analyseur a un état :
///  - les **acteurs visés** restent visés d'une phrase à l'autre, jusqu'à ce
///    que d'autres soient nommés ;
///  - un ordre interrompu en fin de phrase avant ses acteurs consommés
///    (« drone un tire sur… ») est **mis en attente** : si la phrase suivante
///    commence par des noms, ils le complètent.
///
/// Règles détaillées dans documentation/02-concepts.md.
class CommandParser {
public:
    using Clock = std::chrono::steady_clock;

    explicit CommandParser(const Vocabulary& vocabulary) : vocabulary_(vocabulary) {}

    /// Analyse une phrase et retourne les ordres à exécuter, dans l'ordre.
    /// Les ordres reconnus mais non exécutables sont ajoutés à `rejected`.
    std::vector<Command> parse(const std::string& text, std::vector<Rejection>* rejected = nullptr);

    /// Acteurs actuellement visés.
    const std::vector<const Actor*>& targets() const { return targets_; }
    void clearTargets() { targets_.clear(); }

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

    /// Cherche un ordre à words[pos] ; retourne la position suivante, ou pos si rien.
    std::size_t matchOrder(const std::vector<std::string>& words, std::size_t pos,
                           std::vector<Command>& commands, std::vector<Rejection>* rejected);

    Clock::time_point now() const { return now_ ? now_() : Clock::now(); }

    const Vocabulary& vocabulary_;
    std::vector<const Actor*> targets_;

    std::vector<std::string> pending_;            // mots de l'ordre en attente
    std::vector<const Order*> pendingCandidates_;
    Clock::time_point pendingSince_{};
    std::chrono::milliseconds pendingTimeout_{5000};
    std::function<Clock::time_point()> now_;
};

} // namespace vc

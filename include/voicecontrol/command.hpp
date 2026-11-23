#pragma once

#include "voicecontrol/vocabulary.hpp"

#include <string>
#include <vector>

namespace vc {

/// Ordre reconnu, prêt à être exécuté.
///   "drone un tire sur drone deux" -> order "tire sur <...>", actors {drone 1}, args.actors {drone 2}
struct Command {
    const Order* order = nullptr;
    std::vector<const Actor*> actors;   // acteurs qui exécutent (vide pour un ordre global)
    Arguments args;
};

/// Ordre reconnu dans la phrase mais non exécuté.
struct Rejection {
    enum class Reason {
        NoRecipient,     // aucun acteur visé ne connaît l'ordre, et aucun repli possible
        Ambiguous,       // plusieurs replis possibles (acteurs uniques / ordres globaux)
        MissingActors,   // position acteurs sans aucun nom (ou ordre en attente abandonné)
        InvalidActors,   // acteurs d'un type non consommable, ou en nombre hors de {min, max}
    };
    Reason reason;
    std::string words;                     // mots de la phrase concernés
    std::vector<const Order*> candidates;  // ordres qui correspondaient
};

} // namespace vc

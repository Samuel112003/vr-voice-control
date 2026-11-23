#pragma once

#include <any>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <initializer_list>
#include <limits>
#include <memory>
#include <optional>
#include <string>
#include <vector>

namespace vc {

class Actor;
class ActorType;
class Term;

/// Paramètres passés à l'action d'un ordre.
struct Arguments {
    std::vector<const Term*> terms;     // termes choisis dans les positions à choix, dans l'ordre de la phrase
    std::vector<const Actor*> actors;   // acteurs consommés par la position acteurs (vide s'il n'y en a pas)
};

/// Action d'un ordre de type d'acteur : reçoit l'acteur qui exécute.
using ActorAction = std::function<void(const Actor&, const Arguments&)>;

/// Action d'un ordre global (adressé à l'application elle-même).
using GlobalAction = std::function<void(const Arguments&)>;

/// Mot ou groupe de mots reconnaissable.
/// La première forme donne son nom au terme ; les suivantes sont des variantes
/// (pluriel, conjugaison, nombre en lettres...) qui désignent le même terme.
class Term {
public:
    /// Lève std::invalid_argument si `forms` est vide ou contient une forme vide.
    explicit Term(std::vector<std::string> forms);

    const std::string& name() const { return forms_.front(); }
    const std::vector<std::string>& forms() const { return forms_; }
    /// Chaque forme découpée en mots.
    const std::vector<std::vector<std::string>>& tokens() const { return tokens_; }

private:
    std::vector<std::string> forms_;
    std::vector<std::vector<std::string>> tokens_;
};

/// Nombre d'acteurs consommés par une position acteurs : intervalle {min, max}.
struct Count {
    static constexpr std::size_t unlimited = std::numeric_limits<std::size_t>::max();
    std::size_t min;
    std::size_t max;
    bool accepts(std::size_t n) const { return n >= min && n <= max; }
};

inline Count exactly(std::size_t n) { return {n, n}; }
inline Count atLeast(std::size_t n) { return {n, Count::unlimited}; }
inline Count atMost(std::size_t n) { return {1, n}; }
inline Count between(std::size_t min, std::size_t max) { return {min, max}; }

/// Position acteurs : types consommables et nombre d'acteurs.
struct ActorParam {
    std::vector<const ActorType*> types;
    Count count;
};

/// Déclare une position acteurs dans un ordre :
///   drone.order({tire, vc::actors({drone, char}, vc::atLeast(1))}, ...);
ActorParam actors(std::initializer_list<std::reference_wrapper<const ActorType>> types,
                  Count count = atLeast(1));

/// Position d'un ordre : un terme imposé, un choix entre plusieurs termes,
/// ou des acteurs consommés.
///   {tourne, {gauche, droite}}            -> "tourne" puis "gauche" ou "droite"
///   {tire, vc::actors({drone}, exactly(1))} -> "tire" puis un drone
class Slot {
public:
    Slot(const Term& term) : choices_{&term} {}
    Slot(std::initializer_list<std::reference_wrapper<const Term>> choices);
    Slot(ActorParam actors) : actors_(std::move(actors)) {}

    const std::vector<const Term*>& choices() const { return choices_; }
    /// Vrai si plusieurs termes sont possibles : le terme choisi est passé à l'action.
    bool isChoice() const { return choices_.size() > 1; }
    /// Vrai pour une position acteurs.
    bool isActors() const { return actors_.has_value(); }
    const ActorParam& actorParam() const { return *actors_; }

private:
    std::vector<const Term*> choices_;
    std::optional<ActorParam> actors_;
};

/// Acteurs désignés à une position du texte (noms d'acteurs ou de types consécutifs).
struct NamedActors {
    std::vector<const Actor*> actors;
    std::size_t end;   // position du mot qui suit les noms
};

/// Lit les acteurs consommés par une position acteurs à partir de words[pos]
/// (aucun : nullopt). Fourni par l'analyseur, qui sait distinguer une cible
/// d'un nom qui est le sujet de l'ordre suivant.
using ActorReader = std::function<std::optional<NamedActors>(const std::vector<std::string>& words, std::size_t pos)>;

/// Suite de positions associée à une action.
class Order {
public:
    struct Match {
        enum class Status {
            Ok,
            MissingActors,   // aucun nom à la position acteurs
            InvalidActors,   // type non consommable, ou nombre hors de {min, max}
        };
        std::size_t end;   // position du mot qui suit ce qui a été reconnu
        Arguments args;
        Status status = Status::Ok;
    };

    /// Lève std::invalid_argument : aucune position, action vide, position acteurs
    /// en premier, plus d'une position acteurs, sans type, ou nombre invalide.
    Order(std::vector<Slot> slots, const ActorType* owner, ActorAction actorAction, GlobalAction globalAction);

    const std::vector<Slot>& slots() const { return slots_; }
    /// Type d'acteur qui connaît l'ordre, ou nullptr pour un ordre global.
    const ActorType* owner() const { return owner_; }
    /// Description lisible, ex : "tourne à [gauche|droite]", "tire sur <drones|chars 1..*>".
    std::string name() const;

    /// Correspondance commençant à words[pos], si elle existe : la plus longue,
    /// et à longueur égale une correspondance Ok plutôt qu'en erreur.
    /// `readActors` lit les acteurs d'une position acteurs.
    std::optional<Match> match(const std::vector<std::string>& words, std::size_t pos,
                               const ActorReader& readActors = {}) const;

    void execute(const Actor& actor, const Arguments& args) const;
    void execute(const Arguments& args) const;

private:
    void search(const std::vector<std::string>& words, std::size_t pos, std::size_t index,
                const ActorReader& readActors, Arguments& args, std::optional<Match>& best) const;

    std::vector<Slot> slots_;
    const ActorType* owner_;
    ActorAction actorAction_;
    GlobalAction globalAction_;
};

/// Vrai si `a` est préférable à `b` : plus long, ou aussi long mais Ok.
bool betterMatch(const Order::Match& a, const Order::Match& b);

/// Modèle d'acteur : définit les ordres que ses acteurs savent exécuter.
/// Prononcer le terme du type désigne tous ses acteurs à la fois.
class ActorType {
public:
    explicit ActorType(const Term& term) : term_(&term) {}

    /// Déclare un ordre (voir Order pour les erreurs possibles).
    Order& order(std::vector<Slot> slots, ActorAction action);

    const Term& term() const { return *term_; }
    const std::string& name() const { return term_->name(); }
    std::vector<const Order*> orders() const;
    const std::vector<const Actor*>& actors() const { return actors_; }

private:
    friend class Vocabulary;
    const Term* term_;
    std::vector<std::unique_ptr<Order>> orders_;
    std::vector<const Actor*> actors_;
};

/// Élément de l'application que l'on peut appeler par son terme (ex : "drone 1").
///
/// Un acteur peut être relié à l'objet de l'application qu'il représente,
/// pour que les actions (partagées par tout le type) agissent dessus :
///
///   vocab.actor(drone, vocab.term({"drone 1", "drone un"})).setData(&monDrone1);
///   drone.order({avance}, [](const vc::Actor& a, const vc::Arguments&) { a.data<Drone>()->avancer(); });
class Actor {
public:
    Actor(const ActorType& type, const Term& term) : type_(&type), term_(&term) {}

    const ActorType& type() const { return *type_; }
    const Term& term() const { return *term_; }
    const std::string& name() const { return term_->name(); }

    /// Relie l'acteur à un objet de l'application (non possédé : il doit
    /// rester valide tant que l'acteur existe).
    template <class T>
    Actor& setData(T* object) {
        data_ = object;
        return *this;
    }

    /// Objet relié par setData, ou nullptr si aucun objet ou si T ne
    /// correspond pas exactement au type passé à setData.
    template <class T>
    T* data() const {
        auto p = std::any_cast<T*>(&data_);
        return p ? *p : nullptr;
    }

private:
    const ActorType* type_;
    const Term* term_;
    std::any data_;
};

/// Action qui appelle une méthode de l'objet relié à l'acteur (setData).
/// Méthodes acceptées :
///   void f()                       sans argument
///   void f(const vc::Term&)        avec le premier terme choisi
///   void f(const vc::Actor&)       une fois par acteur consommé
///   void f(const vc::Arguments&)   avec tous les arguments
///
///   drone.order({avance}, vc::method(&Drone::avancer));
///   drone.order({tourne, {gauche, droite}}, vc::method(&Drone::tourner));
///   drone.order({tire, vc::actors({drone})}, vc::method(&Drone::tirerSur));
///
/// Ne fait rien si l'acteur n'est relié à aucun objet de type T.
template <class T>
ActorAction method(void (T::*fn)()) {
    return [fn](const Actor& actor, const Arguments&) {
        if (T* object = actor.data<T>()) (object->*fn)();
    };
}

template <class T>
ActorAction method(void (T::*fn)(const Term&)) {
    return [fn](const Actor& actor, const Arguments& args) {
        T* object = actor.data<T>();
        if (object && !args.terms.empty()) (object->*fn)(*args.terms.front());
    };
}

template <class T>
ActorAction method(void (T::*fn)(const Actor&)) {
    return [fn](const Actor& actor, const Arguments& args) {
        if (T* object = actor.data<T>())
            for (const Actor* other : args.actors) (object->*fn)(*other);
    };
}

template <class T>
ActorAction method(void (T::*fn)(const Arguments&)) {
    return [fn](const Actor& actor, const Arguments& args) {
        if (T* object = actor.data<T>()) (object->*fn)(args);
    };
}

/// Registre des termes, des types d'acteurs, des acteurs et des ordres globaux.
/// Les références retournées restent valides tant que le Vocabulary existe.
class Vocabulary {
public:
    /// Déclare un terme. Lève std::invalid_argument si aucune forme.
    Term& term(std::vector<std::string> forms);

    /// Déclare un type d'acteur désigné par `term` (ex : "drones").
    ActorType& actorType(const Term& term);

    /// Déclare un acteur de type `type`, appelé par `term`.
    Actor& actor(ActorType& type, const Term& term);

    /// Déclare un ordre global : adressé à l'application, sans acteur exécutant.
    Order& order(std::vector<Slot> slots, GlobalAction action);

    std::vector<const ActorType*> types() const;
    std::vector<const Actor*> actors() const;
    std::vector<const Order*> globalOrders() const;

    /// Acteur dont le nom (première forme) est `name`, ou nullptr.
    const Actor* findActor(const std::string& name) const;
    Actor* findActor(const std::string& name);

    /// Toutes les formes connues (expressions, triées, sans doublon) : ce que
    /// le moteur de reconnaissance doit savoir reconnaître.
    std::vector<std::string> phrases() const;

private:
    std::vector<std::unique_ptr<Term>> terms_;
    std::vector<std::unique_ptr<ActorType>> types_;
    std::vector<std::unique_ptr<Actor>> actors_;
    std::vector<std::unique_ptr<Order>> orders_;
};

/// Normalise un texte : minuscules (y compris lettres accentuées latines),
/// ponctuation (, . ; : ! ? ") remplacée par des espaces, espaces réduits.
std::string normalize(const std::string& text);

/// Découpe un texte en mots normalisés.
std::vector<std::string> tokenize(const std::string& text);

} // namespace vc

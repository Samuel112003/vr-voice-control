#include "voicecontrol/command_parser.hpp"

#include <algorithm>

namespace vc {

namespace {

template <typename T>
void pushUnique(std::vector<T>& v, const T& value) {
    if (std::find(v.begin(), v.end(), value) == v.end()) v.push_back(value);
}

struct Hit {
    const Order* order;
    Order::Match match;
};

/// Meilleure correspondance parmi `orders` (voir betterMatch ; la première en cas d'égalité).
std::optional<Hit> bestOf(const std::vector<const Order*>& orders, const std::vector<std::string>& words,
                          std::size_t pos, const ActorReader& reader) {
    std::optional<Hit> best;
    for (const Order* o : orders) {
        auto m = o->match(words, pos, reader);
        if (m && (!best || betterMatch(*m, best->match))) best = Hit{o, std::move(*m)};
    }
    return best;
}

/// Ajoute `hit` à `hits` s'il est au moins aussi long que les précédents.
void keepLongest(std::vector<Hit>& hits, std::optional<Hit> hit) {
    if (!hit) return;
    if (!hits.empty() && hit->match.end < hits.front().match.end) return;
    if (!hits.empty() && hit->match.end > hits.front().match.end) hits.clear();
    hits.push_back(std::move(*hit));
}

std::string join(const std::vector<std::string>& words, std::size_t from, std::size_t to) {
    std::string out;
    for (std::size_t i = from; i < to; ++i) out += (i > from ? " " : "") + words[i];
    return out;
}

} // namespace

std::vector<Command> CommandParser::parse(const std::string& text, std::vector<Rejection>* rejected) {
    std::vector<Command> commands;
    parse(
        text,
        [&commands](const Command& c) {
            commands.push_back(c);
            return true;
        },
        [rejected](const Rejection& r) {
            if (rejected) rejected->push_back(r);
        });
    return commands;
}

void CommandParser::parse(const std::string& text, const CommandSink& onCommand, const RejectionSink& onRejection) {
    Sinks sinks{onCommand, onRejection};
    auto words = tokenize(text);

    // Délai de sélection dépassé : les acteurs visés (et l'ordre en attente) sont oubliés.
    expireTargets();

    // Ordre en attente : complété si la phrase commence par des noms, sinon abandonné.
    if (!pending_.empty()) {
        bool alive = now() - pendingSince_ <= pendingTimeout_;
        if (alive && matchName(words, 0)) {
            words.insert(words.begin(), pending_.begin(), pending_.end());
        } else if (onRejection) {
            onRejection({Rejection::Reason::MissingActors, join(pending_, 0, pending_.size()), pendingCandidates_});
        }
        pending_.clear();
        pendingCandidates_.clear();
    }

    bool naming = false;   // vrai tant qu'on enchaîne des noms d'acteurs
    std::size_t pos = 0;
    while (pos < words.size() && !sinks.stopped) {
        if (auto named = matchName(words, pos)) {
            if (!naming) targets_.clear();
            for (const Actor* a : named->actors) pushUnique(targets_, a);
            lastTargetUse_ = now();
            naming = true;
            pos = named->end;
            continue;
        }
        std::size_t end = matchOrder(words, pos, sinks);
        if (end == pos) {
            ++pos;   // mot inconnu : ignoré
            continue;
        }
        naming = false;
        pos = end;
    }
}

bool CommandParser::expireTargets() {
    if (targets_.empty() || targetTimeout_ == never) return false;
    if (now() - lastTargetUse_ <= targetTimeout_) return false;
    clearTargets();
    return true;
}

std::optional<NamedActors> CommandParser::matchName(const std::vector<std::string>& words, std::size_t pos) const {
    std::optional<NamedActors> best;
    auto consider = [&](const Term& term, const std::vector<const Actor*>& actors) {
        for (const auto& form : term.tokens()) {
            std::size_t end = pos + form.size();
            if (end > words.size() ||
                !std::equal(form.begin(), form.end(), words.begin() + static_cast<std::ptrdiff_t>(pos)))
                continue;
            if (!best || end > best->end) best = NamedActors{actors, end};
        }
    };
    // Acteurs d'abord : à longueur égale, un acteur l'emporte sur un type.
    for (const Actor* a : vocabulary_.actors()) consider(a->term(), {a});
    for (const ActorType* t : vocabulary_.types()) consider(t->term(), t->actors());
    return best;
}

std::optional<NamedActors> CommandParser::readActors(const std::vector<std::string>& words, std::size_t pos) const {
    // Noms consécutifs, par groupe (un nom d'acteur, ou un nom de type = tous ses acteurs).
    std::vector<NamedActors> groups;
    for (std::size_t p = pos; auto g = matchName(words, p); p = g->end) groups.push_back(std::move(*g));
    if (groups.empty()) return std::nullopt;

    // "drone un tire sur drone trois drone deux tire sur drone un" : le dernier nom,
    // suivi d'un ordre qu'il sait exécuter, est le sujet de cet ordre, pas une cible.
    if (groups.size() >= 2 && startsOrderFor(groups.back().actors, words, groups.back().end)) groups.pop_back();

    NamedActors out{{}, groups.back().end};
    for (const NamedActors& g : groups)
        for (const Actor* a : g.actors) pushUnique(out.actors, a);
    return out;
}

bool CommandParser::startsOrderFor(const std::vector<const Actor*>& actors, const std::vector<std::string>& words,
                                   std::size_t pos) const {
    const ActorReader reader = [this](const std::vector<std::string>& w, std::size_t p) { return readActors(w, p); };
    std::vector<const ActorType*> types;
    for (const Actor* a : actors) pushUnique(types, &a->type());
    for (const ActorType* type : types)
        for (const Order* o : type->orders())
            if (o->match(words, pos, reader)) return true;
    return false;
}

std::size_t CommandParser::matchOrder(const std::vector<std::string>& words, std::size_t pos, Sinks& sinks) {
    const ActorReader reader = [this](const std::vector<std::string>& w, std::size_t p) { return readActors(w, p); };

    auto reject = [&](Rejection::Reason reason, const std::vector<const Order*>& candidates, std::size_t end) {
        if (sinks.rejection) sinks.rejection({reason, join(words, pos, end), candidates});
    };

    // Transmet les correspondances Ok ; rejette les autres, ou met l'ordre en attente
    // s'il manque ses acteurs en fin de phrase.
    auto accept = [&](std::vector<Hit>& hits, bool fromTargets) {
        const std::size_t end = hits.front().match.end;
        const bool anyOk = std::any_of(hits.begin(), hits.end(),
                                       [](const Hit& h) { return h.match.status == Order::Match::Status::Ok; });
        const bool canWait = !anyOk && end == words.size() && pendingTimeout_.count() > 0;
        std::vector<const Order*> waiting;
        // Les ordres sont tous construits avant d'être transmis : une action qui
        // désélectionne ne doit pas vider les acteurs des ordres de la même position.
        std::vector<Command> ready;
        for (Hit& h : hits) {
            switch (h.match.status) {
            case Order::Match::Status::Ok: {
                Command c{h.order, {}, std::move(h.match.args)};
                if (fromTargets) {
                    for (const Actor* a : targets_)
                        if (&a->type() == h.order->owner()) c.actors.push_back(a);
                } else if (h.order->owner()) {
                    c.actors = h.order->owner()->actors();
                }
                ready.push_back(std::move(c));
                break;
            }
            case Order::Match::Status::InvalidActors:
                reject(Rejection::Reason::InvalidActors, {h.order}, end);
                break;
            case Order::Match::Status::MissingActors:
                if (canWait) waiting.push_back(h.order);
                else reject(Rejection::Reason::MissingActors, {h.order}, end);
                break;
            }
        }
        if (!waiting.empty()) {
            pending_.assign(words.begin() + static_cast<std::ptrdiff_t>(pos), words.begin() + static_cast<std::ptrdiff_t>(end));
            pendingCandidates_ = std::move(waiting);
            pendingSince_ = now();
        }
        if (fromTargets && !ready.empty()) lastTargetUse_ = now();   // on vient de parler aux cibles
        for (const Command& c : ready) {
            if (sinks.stopped) break;
            if (!sinks.command(c)) sinks.stopped = true;
        }
        return end;
    };

    auto ordersOf = [](const std::vector<Hit>& hits) {
        std::vector<const Order*> out;
        for (const Hit& h : hits) out.push_back(h.order);
        return out;
    };

    // 1. Acteurs visés dont le type connaît l'ordre.
    std::vector<const ActorType*> targetTypes;
    for (const Actor* a : targets_) pushUnique(targetTypes, &a->type());
    std::vector<Hit> hits;
    for (const ActorType* type : targetTypes) keepLongest(hits, bestOf(type->orders(), words, pos, reader));
    if (!hits.empty()) return accept(hits, true);

    // 2. Repli : acteur d'un type qui n'en a qu'un, ou ordre global. Doit être unique.
    for (const ActorType* type : vocabulary_.types())
        if (type->actors().size() == 1) keepLongest(hits, bestOf(type->orders(), words, pos, reader));
    for (const Order* o : vocabulary_.globalOrders()) keepLongest(hits, bestOf({o}, words, pos, reader));
    if (hits.size() > 1) {
        const std::size_t end = hits.front().match.end;
        reject(Rejection::Reason::Ambiguous, ordersOf(hits), end);
        return end;
    }
    if (hits.size() == 1) return accept(hits, false);

    // 3. Ordre connu, mais d'aucun acteur visé ni d'aucun repli : signalé, non exécuté.
    for (const ActorType* type : vocabulary_.types())
        if (type->actors().size() != 1) keepLongest(hits, bestOf(type->orders(), words, pos, reader));
    if (!hits.empty()) {
        const std::size_t end = hits.front().match.end;
        reject(Rejection::Reason::NoRecipient, ordersOf(hits), end);
        return end;
    }
    return pos;
}

} // namespace vc

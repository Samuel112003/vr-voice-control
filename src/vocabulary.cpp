#include "voicecontrol/vocabulary.hpp"

#include <algorithm>
#include <set>
#include <sstream>
#include <stdexcept>

namespace vc {

// ---------------------------------------------------------------------------
// Utilitaires texte
// ---------------------------------------------------------------------------
std::string normalize(const std::string& text) {
    std::string out;
    out.reserve(text.size());
    bool pendingSpace = false;
    for (std::size_t i = 0; i < text.size(); ++i) {
        char c = text[i];
        if (c == ' ' || c == '\t' || c == '\n' || c == '\r' || c == ',' || c == '.' || c == ';' ||
            c == ':' || c == '!' || c == '?' || c == '"') {
            pendingSpace = !out.empty();
            continue;
        }
        if (pendingSpace) {
            out += ' ';
            pendingSpace = false;
        }
        auto u = static_cast<unsigned char>(c);
        auto next = i + 1 < text.size() ? static_cast<unsigned char>(text[i + 1]) : 0u;
        if (c >= 'A' && c <= 'Z') {
            out += static_cast<char>(c - 'A' + 'a');
        } else if (u == 0xC3 && next >= 0x80 && next <= 0x9E && next != 0x97) {
            // UTF-8 U+00C0..U+00DE (À..Þ, sauf ×) -> U+00E0..U+00FE
            out += c;
            out += static_cast<char>(next + 0x20);
            ++i;
        } else if (u == 0xC5 && next == 0x92) {   // Œ -> œ
            out += c;
            out += static_cast<char>(0x93);
            ++i;
        } else {
            out += c;
        }
    }
    return out;
}

std::vector<std::string> tokenize(const std::string& text) {
    std::vector<std::string> tokens;
    std::istringstream stream(normalize(text));
    std::string word;
    while (stream >> word) tokens.push_back(word);
    return tokens;
}

namespace {

bool matchesAt(const std::vector<std::string>& words, std::size_t pos, const std::vector<std::string>& form) {
    if (form.empty() || pos + form.size() > words.size()) return false;
    return std::equal(form.begin(), form.end(), words.begin() + static_cast<std::ptrdiff_t>(pos));
}

} // namespace

// ---------------------------------------------------------------------------
// Term / Slot
// ---------------------------------------------------------------------------
Term::Term(std::vector<std::string> forms) {
    if (forms.empty()) throw std::invalid_argument("un terme a besoin d'au moins une forme");
    for (auto& f : forms) {
        auto words = tokenize(f);
        if (words.empty()) throw std::invalid_argument("forme vide dans un terme");
        forms_.push_back(normalize(f));
        tokens_.push_back(std::move(words));
    }
}

ActorParam actors(std::initializer_list<std::reference_wrapper<const ActorType>> types, Count count) {
    ActorParam p{{}, count};
    for (const ActorType& t : types) p.types.push_back(&t);
    return p;
}

Slot::Slot(std::initializer_list<std::reference_wrapper<const Term>> choices) {
    for (const Term& t : choices) choices_.push_back(&t);
    if (choices_.empty()) throw std::invalid_argument("position d'ordre sans terme");
}

// ---------------------------------------------------------------------------
// Order
// ---------------------------------------------------------------------------
Order::Order(std::vector<Slot> slots, const ActorType* owner, ActorAction actorAction, GlobalAction globalAction)
    : slots_(std::move(slots)), owner_(owner),
      actorAction_(std::move(actorAction)), globalAction_(std::move(globalAction)) {
    if (slots_.empty()) throw std::invalid_argument("un ordre a besoin d'au moins un terme");
    if (owner_ ? !actorAction_ : !globalAction_) throw std::invalid_argument("ordre sans action");

    std::size_t actorSlots = 0;
    for (std::size_t i = 0; i < slots_.size(); ++i) {
        if (!slots_[i].isActors()) continue;
        const ActorParam& p = slots_[i].actorParam();
        // Les noms sont cherchés avant les ordres : une position acteurs en tête
        // serait toujours prise pour un changement de cible.
        if (i == 0) throw std::invalid_argument("une position acteurs ne peut pas commencer un ordre");
        if (++actorSlots > 1) throw std::invalid_argument("un ordre a au plus une position acteurs");
        if (p.types.empty()) throw std::invalid_argument("position acteurs sans type consommable");
        if (p.count.min == 0 || p.count.max < p.count.min)
            throw std::invalid_argument("nombre d'acteurs invalide (min >= 1 et max >= min)");
    }
}

std::string Order::name() const {
    std::string out;
    for (const Slot& slot : slots_) {
        if (!out.empty()) out += ' ';
        if (slot.isActors()) {
            const ActorParam& p = slot.actorParam();
            out += '<';
            for (std::size_t i = 0; i < p.types.size(); ++i) out += (i ? "|" : "") + p.types[i]->name();
            out += ' ' + std::to_string(p.count.min);
            if (p.count.max != p.count.min)
                out += ".." + (p.count.max == Count::unlimited ? std::string("*") : std::to_string(p.count.max));
            out += '>';
            continue;
        }
        if (!slot.isChoice()) {
            out += slot.choices().front()->name();
            continue;
        }
        out += '[';
        for (std::size_t i = 0; i < slot.choices().size(); ++i)
            out += (i ? "|" : "") + slot.choices()[i]->name();
        out += ']';
    }
    return out;
}

bool betterMatch(const Order::Match& a, const Order::Match& b) {
    if (a.end != b.end) return a.end > b.end;
    return a.status == Order::Match::Status::Ok && b.status != Order::Match::Status::Ok;
}

std::optional<Order::Match> Order::match(const std::vector<std::string>& words, std::size_t pos,
                                         const ActorReader& readActors) const {
    std::optional<Match> best;
    Arguments args;
    search(words, pos, 0, readActors, args, best);
    return best;
}

void Order::search(const std::vector<std::string>& words, std::size_t pos, std::size_t index,
                   const ActorReader& readActors, Arguments& args, std::optional<Match>& best) const {
    auto consider = [&best](Match m) {
        if (!best || betterMatch(m, *best)) best = std::move(m);
    };
    if (index == slots_.size()) {
        consider(Match{pos, args, Match::Status::Ok});
        return;
    }
    const Slot& slot = slots_[index];

    if (slot.isActors()) {
        auto named = readActors ? readActors(words, pos) : std::nullopt;
        if (!named) {
            consider(Match{pos, args, Match::Status::MissingActors});
            return;
        }
        std::vector<const Actor*> found = std::move(named->actors);
        const std::size_t end = named->end;
        const ActorParam& p = slot.actorParam();
        bool valid = p.count.accepts(found.size());
        for (const Actor* a : found)
            valid = valid && std::find(p.types.begin(), p.types.end(), &a->type()) != p.types.end();
        if (!valid) {
            Match m{end, args, Match::Status::InvalidActors};
            m.args.actors = std::move(found);
            consider(std::move(m));
            return;
        }
        args.actors = std::move(found);
        search(words, end, index + 1, readActors, args, best);
        args.actors.clear();
        return;
    }

    for (const Term* term : slot.choices()) {
        for (const auto& form : term->tokens()) {
            if (!matchesAt(words, pos, form)) continue;
            if (slot.isChoice()) args.terms.push_back(term);
            search(words, pos + form.size(), index + 1, readActors, args, best);
            if (slot.isChoice()) args.terms.pop_back();
        }
    }
}

void Order::execute(const Actor& actor, const Arguments& args) const {
    if (actorAction_) actorAction_(actor, args);
}

void Order::execute(const Arguments& args) const {
    if (globalAction_) globalAction_(args);
}

// ---------------------------------------------------------------------------
// ActorType
// ---------------------------------------------------------------------------
Order& ActorType::order(std::vector<Slot> slots, ActorAction action) {
    orders_.push_back(std::make_unique<Order>(std::move(slots), this, std::move(action), GlobalAction{}));
    return *orders_.back();
}

std::vector<const Order*> ActorType::orders() const {
    std::vector<const Order*> out;
    for (const auto& o : orders_) out.push_back(o.get());
    return out;
}

// ---------------------------------------------------------------------------
// Vocabulary
// ---------------------------------------------------------------------------
Term& Vocabulary::term(std::vector<std::string> forms) {
    terms_.push_back(std::make_unique<Term>(std::move(forms)));
    return *terms_.back();
}

ActorType& Vocabulary::actorType(const Term& term) {
    types_.push_back(std::make_unique<ActorType>(term));
    return *types_.back();
}

Actor& Vocabulary::actor(ActorType& type, const Term& term) {
    actors_.push_back(std::make_unique<Actor>(type, term));
    type.actors_.push_back(actors_.back().get());
    return *actors_.back();
}

Order& Vocabulary::order(std::vector<Slot> slots, GlobalAction action) {
    orders_.push_back(std::make_unique<Order>(std::move(slots), nullptr, ActorAction{}, std::move(action)));
    return *orders_.back();
}

std::vector<const ActorType*> Vocabulary::types() const {
    std::vector<const ActorType*> out;
    for (const auto& t : types_) out.push_back(t.get());
    return out;
}

std::vector<const Actor*> Vocabulary::actors() const {
    std::vector<const Actor*> out;
    for (const auto& a : actors_) out.push_back(a.get());
    return out;
}

std::vector<const Order*> Vocabulary::globalOrders() const {
    std::vector<const Order*> out;
    for (const auto& o : orders_) out.push_back(o.get());
    return out;
}

const Actor* Vocabulary::findActor(const std::string& name) const {
    return const_cast<Vocabulary*>(this)->findActor(name);
}

Actor* Vocabulary::findActor(const std::string& name) {
    const std::string wanted = normalize(name);
    for (const auto& a : actors_)
        if (a->name() == wanted) return a.get();
    return nullptr;
}

std::vector<std::string> Vocabulary::phrases() const {
    std::set<std::string> unique;
    auto add = [&unique](const Term& t) { unique.insert(t.forms().begin(), t.forms().end()); };
    for (const auto& t : terms_) add(*t);
    for (const auto& t : types_) add(t->term());     // au cas où le terme vient d'ailleurs
    for (const auto& a : actors_) add(a->term());
    return {unique.begin(), unique.end()};
}

} // namespace vc

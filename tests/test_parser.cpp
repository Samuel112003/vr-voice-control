// Tests minimalistes sans framework (à remplacer par Catch2/GoogleTest si besoin).

#include <voicecontrol/voicecontrol.hpp>

#include <algorithm>
#include <cstdio>
#include <fstream>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <string>
#include <thread>
#include <vector>

static int failures = 0;

#define CHECK(cond)                                                              \
    do {                                                                         \
        if (!(cond)) {                                                           \
            std::cerr << __FILE__ << ":" << __LINE__ << " ECHEC : " #cond "\n";  \
            ++failures;                                                          \
        }                                                                        \
    } while (0)

using Log = std::vector<std::string>;

/// Scène de test :
///  - 3 drones (type à plusieurs acteurs) : avance, recule, tourne, tourne à [gauche|droite]
///  - 1 caméra (type à acteur unique)     : avance, zoome, allume
///  - 1 lampe  (type à acteur unique)     : allume
///  - ordre global                         : urgence
struct Scene {
    vc::VoiceController ctrl;
    Log log;
    std::vector<vc::Rejection> rejections;

    Scene() {
        auto& v = ctrl.vocabulary();
        auto& avance = v.term({"avance", "avancez"});
        auto& recule = v.term({"recule", "reculez"});
        auto& tourne = v.term({"tourne à", "tourne", "tournez"});
        auto& gauche = v.term({"gauche"});
        auto& droite = v.term({"droite"});
        auto& zoome = v.term({"zoome"});
        auto& allume = v.term({"allume"});
        auto& urgence = v.term({"urgence"});

        auto& tire = v.term({"tire sur", "tirez sur"});
        auto& escorte = v.term({"escorte"});

        // Entrée du journal : "acteur:ordre terme... @acteur_consommé..."
        auto record = [this](const char* what) {
            return [this, what](const vc::Actor& a, const vc::Arguments& args) {
                std::string entry = a.name() + ":" + what;
                for (const vc::Term* t : args.terms) entry += " " + t->name();
                for (const vc::Actor* o : args.actors) entry += " @" + o->name();
                log.push_back(entry);
            };
        };

        auto& drone = v.actorType(v.term({"drones", "les drones"}));
        auto& char_ = v.actorType(v.term({"chars"}));   // type à un acteur, sans ordre
        drone.order({avance}, record("avance"));
        drone.order({recule}, record("recule"));
        drone.order({tourne}, record("tourne"));
        drone.order({tourne, {gauche, droite}}, record("tourne"));
        drone.order({tire, vc::actors({drone, char_}, vc::atLeast(1))}, record("tire"));
        drone.order({escorte, vc::actors({drone}, vc::exactly(1))}, record("escorte"));
        v.actor(drone, v.term({"drone 1", "drone un"}));
        v.actor(drone, v.term({"drone 2", "drone deux"}));
        v.actor(drone, v.term({"drone 3", "drone trois"}));
        v.actor(char_, v.term({"char"}));

        auto& camera = v.actorType(v.term({"caméras"}));
        camera.order({avance}, record("avance"));
        camera.order({zoome}, record("zoome"));
        camera.order({allume}, record("allume"));
        v.actor(camera, v.term({"caméra"}));

        auto& lampe = v.actorType(v.term({"lampes"}));
        lampe.order({allume}, record("allume"));
        v.actor(lampe, v.term({"lampe"}));

        v.order({urgence}, [this](const vc::Arguments&) { log.push_back("global:urgence"); });

        ctrl.onUnhandled([this](const vc::Rejection& r) { rejections.push_back(r); });
    }

    Log say(const std::string& text) {
        log.clear();
        rejections.clear();
        ctrl.handleText(text);
        return log;
    }
};

static void testParsing() {
    Scene s;

    // Un acteur, un ordre ; variantes, casse
    CHECK((s.say("Drone 1 avance") == Log{"drone 1:avance"}));
    CHECK((s.say("drone deux avancez") == Log{"drone 2:avance"}));

    // Noms consécutifs : ils s'additionnent
    CHECK((s.say("drone un drone deux avancez") == Log{"drone 1:avance", "drone 2:avance"}));

    // Un nom après un ordre remplace la cible
    CHECK((s.say("drone un avance drone deux recule") == Log{"drone 1:avance", "drone 2:recule"}));
    CHECK((s.say("drone un avance drone deux avance") == Log{"drone 1:avance", "drone 2:avance"}));

    // La cible persiste d'une phrase à l'autre (Vosk coupe aux pauses)
    CHECK(s.say("drone trois").empty());
    CHECK((s.say("avance") == Log{"drone 3:avance"}));
    CHECK((s.say("recule") == Log{"drone 3:recule"}));
    CHECK(s.ctrl.targets().size() == 1 && s.ctrl.targets()[0]->name() == "drone 3");

    // Le terme du type désigne tous ses acteurs
    CHECK((s.say("les drones avancez") == Log{"drone 1:avance", "drone 2:avance", "drone 3:avance"}));

    // Ordre à choix : le terme choisi est passé ; la correspondance la plus longue l'emporte
    CHECK((s.say("drone un tourne à gauche") == Log{"drone 1:tourne gauche"}));
    CHECK((s.say("drone un tournez droite") == Log{"drone 1:tourne droite"}));
    CHECK((s.say("drone un tourne") == Log{"drone 1:tourne"}));

    // Cibles de types différents : chaque cible dont le type connaît l'ordre l'exécute, et seulement elles
    CHECK((s.say("drone un caméra avance") == Log{"drone 1:avance", "caméra:avance"}));

    // L'acteur unique ne concurrence pas les cibles : la caméra sait avancer mais n'est pas visée
    CHECK((s.say("drone un avance") == Log{"drone 1:avance"}));

    // Repli sur l'acteur unique quand personne n'est visé...
    s.ctrl.clearTargets();
    CHECK((s.say("zoome") == Log{"caméra:zoome"}));
    // ... ou quand aucune cible ne connaît l'ordre ; la cible reste inchangée
    CHECK((s.say("drone un zoome") == Log{"caméra:zoome"}));
    CHECK(s.ctrl.targets().size() == 1 && s.ctrl.targets()[0]->name() == "drone 1");

    // Plusieurs replis possibles : ambigu, rien n'est exécuté
    s.ctrl.clearTargets();
    CHECK(s.say("allume").empty());
    CHECK(s.rejections.size() == 1 && s.rejections[0].reason == vc::Rejection::Reason::Ambiguous &&
          s.rejections[0].candidates.size() == 2 && s.rejections[0].words == "allume");
    // Une cible lève l'ambiguïté
    CHECK((s.say("lampe allume") == Log{"lampe:allume"}));

    // Ordre connu de types à plusieurs acteurs seulement, sans cible : signalé
    s.ctrl.clearTargets();
    CHECK(s.say("recule").empty());
    CHECK(s.rejections.size() == 1 && s.rejections[0].reason == vc::Rejection::Reason::NoRecipient);

    // Ordre global, avec ou sans cible
    CHECK((s.say("urgence") == Log{"global:urgence"}));
    CHECK((s.say("drone un urgence") == Log{"global:urgence"}));

    // Mots inconnus ignorés ; ponctuation et majuscules accentuées normalisées
    CHECK((s.say("euh alors drone deux euh avance") == Log{"drone 2:avance"}));
    CHECK((s.say("Drone UN, CAM\xC3\x89RA : AVANCEZ !") == Log{"drone 1:avance", "caméra:avance"}));
}

static void testVocabulary() {
    Scene s;
    auto phrases = s.ctrl.vocabulary().phrases();
    auto has = [&](const std::string& p) { return std::find(phrases.begin(), phrases.end(), p) != phrases.end(); };
    CHECK(has("drone un") && has("tourne à") && has("les drones") && has("caméra"));
    CHECK(std::is_sorted(phrases.begin(), phrases.end()));
    CHECK(std::adjacent_find(phrases.begin(), phrases.end()) == phrases.end());

    const vc::Actor* d1 = s.ctrl.vocabulary().findActor("Drone 1");
    CHECK(d1 && d1->type().name() == "drones" && d1->term().forms().size() == 2);
    CHECK(d1->type().orders()[3]->name() == "tourne à [gauche|droite]");

    auto throws = [](auto f) {
        try { f(); } catch (const std::invalid_argument&) { return true; }
        return false;
    };
    vc::Vocabulary v;
    CHECK(throws([&] { v.term({}); }));
    CHECK(throws([&] { v.term({" , "}); }));
    CHECK(throws([&] { v.order({}, [](const vc::Arguments&) {}); }));
    // Positions acteurs mal déclarées
    auto& t = v.actorType(v.term({"t"}));
    auto& x = v.term({"y"});
    auto noop = [](const vc::Actor&, const vc::Arguments&) {};
    CHECK(throws([&] { t.order({vc::actors({t}), x}, noop); }));                     // en premier
    CHECK(throws([&] { t.order({x, vc::actors({t}), vc::actors({t})}, noop); }));    // deux positions
    CHECK(throws([&] { t.order({x, vc::actors({})}, noop); }));                      // sans type
    CHECK(throws([&] { t.order({x, vc::actors({t}, vc::exactly(0))}, noop); }));     // min = 0
    CHECK(throws([&] { t.order({x, vc::actors({t}, vc::between(3, 2))}, noop); }));  // max < min
    CHECK(t.order({x, vc::actors({t}, vc::between(2, 4))}, noop).name() == "y <t 2..4>");
    CHECK(throws([&] { v.order({v.term({"x"})}, nullptr); }));
}

static void testConsumedActors() {
    using Reason = vc::Rejection::Reason;
    Scene s;
    auto now = vc::CommandParser::Clock::time_point{};
    s.ctrl.parser().setClock([&now] { return now; });

    // Un acteur consommé ; la cible reste le tireur
    CHECK((s.say("drone un tire sur drone deux") == Log{"drone 1:tire @drone 2"}));
    CHECK(s.ctrl.targets().size() == 1 && s.ctrl.targets()[0]->name() == "drone 1");
    CHECK((s.say("avance") == Log{"drone 1:avance"}));
    CHECK((s.say("drone un tire sur drone deux avance") == Log{"drone 1:tire @drone 2", "drone 1:avance"}));

    // Dernier nom suivi d'un ordre qu'il sait exécuter : c'est le sujet de cet ordre, pas une cible
    CHECK((s.say("drone un tire sur drone trois drone deux tire sur drone un") ==
           Log{"drone 1:tire @drone 3", "drone 2:tire @drone 1"}));
    CHECK(s.ctrl.targets().size() == 1 && s.ctrl.targets()[0]->name() == "drone 2");
    CHECK((s.say("drone un tire sur drone deux drone trois avance") == Log{"drone 1:tire @drone 2", "drone 3:avance"}));
    CHECK((s.say("drone un tire sur drone deux drone trois char") == Log{"drone 1:tire @drone 2 @drone 3 @char"}));
    // ... mais pas s'il ne sait pas l'exécuter (le char n'a pas d'ordre) : il reste une cible
    CHECK((s.say("drone un tire sur drone deux char avance") == Log{"drone 1:tire @drone 2 @char", "drone 1:avance"}));

    // Plusieurs acteurs, types mélangés, nom de type, doublons, soi-même
    CHECK((s.say("drone un tire sur drone deux char") == Log{"drone 1:tire @drone 2 @char"}));
    CHECK((s.say("drone un tire sur les drones") == Log{"drone 1:tire @drone 1 @drone 2 @drone 3"}));
    CHECK((s.say("drone un tire sur drone deux drone deux") == Log{"drone 1:tire @drone 2"}));
    CHECK((s.say("drone un tire sur drone un") == Log{"drone 1:tire @drone 1"}));
    CHECK((s.say("drone un drone deux tirez sur char") == Log{"drone 1:tire @char", "drone 2:tire @char"}));

    // Type non consommable : refusé, et le nom ne change pas la cible
    CHECK(s.say("drone un tire sur caméra").empty());
    CHECK(s.rejections.size() == 1 && s.rejections[0].reason == Reason::InvalidActors &&
          s.rejections[0].words == "tire sur caméra");
    CHECK(s.ctrl.targets().size() == 1 && s.ctrl.targets()[0]->name() == "drone 1");

    // Nombre hors de {min, max} : refusé en entier
    CHECK(s.say("drone un escorte drone deux drone trois").empty());
    CHECK(s.rejections.size() == 1 && s.rejections[0].reason == Reason::InvalidActors);
    CHECK((s.say("drone un escorte drone deux") == Log{"drone 1:escorte @drone 2"}));

    // Argument manquant au milieu de la phrase : refusé, la suite est analysée
    CHECK((s.say("drone un tire sur avance") == Log{"drone 1:avance"}));
    CHECK(s.rejections.size() == 1 && s.rejections[0].reason == Reason::MissingActors);

    // Ordre coupé par une pause : en attente, complété par la phrase suivante
    CHECK(s.say("drone un tire sur").empty() && s.rejections.empty());
    CHECK(s.ctrl.parser().hasPending());
    now += std::chrono::seconds(2);
    CHECK((s.say("drone deux") == Log{"drone 1:tire @drone 2"}));
    CHECK(s.ctrl.targets().size() == 1 && s.ctrl.targets()[0]->name() == "drone 1");
    CHECK(!s.ctrl.parser().hasPending());

    // ... abandonné si la phrase suivante ne commence pas par des noms
    s.say("drone un tire sur");
    CHECK((s.say("avance") == Log{"drone 1:avance"}));
    CHECK(s.rejections.size() == 1 && s.rejections[0].reason == Reason::MissingActors &&
          s.rejections[0].words == "tire sur");

    // ... ou si le délai est dépassé : les noms changent alors la cible
    s.say("drone un tire sur");
    now += std::chrono::seconds(6);
    CHECK(s.say("drone deux").empty());
    CHECK(s.rejections.size() == 1 && s.rejections[0].reason == Reason::MissingActors);
    CHECK(s.ctrl.targets()[0]->name() == "drone 2");

    // Délai 0 : pas d'attente, rejet immédiat
    s.ctrl.parser().setPendingTimeout(std::chrono::milliseconds(0));
    CHECK(s.say("drone un tire sur").empty());
    CHECK(s.rejections.size() == 1 && s.rejections[0].reason == Reason::MissingActors);
    CHECK(!s.ctrl.parser().hasPending());
}

static void testListening() {
    using State = vc::ListeningState;
    Scene s;
    auto& v = s.ctrl.vocabulary();
    std::vector<State> changes;
    s.ctrl.onListeningChanged([&](State st) { changes.push_back(st); });
    auto& reprends = v.term({"reprends", "reprise", "reprends l'écoute"});
    auto& pauseTerm = v.term({"pause", "mets en pause"});
    v.order({pauseTerm}, [&](const vc::Arguments&) { s.ctrl.softPause(reprends); });
    auto state = [&] { return s.ctrl.listeningState(); };

    // Vraie pause : tout est ignoré, même le terme de réveil ; les cibles sont conservées
    CHECK((s.say("drone deux avance") == Log{"drone 2:avance"}));
    s.ctrl.pause();
    CHECK(state() == State::Paused);
    CHECK(s.say("drone un avance").empty());
    CHECK(s.say("reprends avance").empty() && state() == State::Paused);
    s.ctrl.resume();
    CHECK((s.say("avance") == Log{"drone 2:avance"}));

    // La pause oublie l'ordre en attente
    s.say("drone un tire sur");
    s.ctrl.pause();
    s.ctrl.resume();
    CHECK(s.say("drone deux").empty() && s.ctrl.targets()[0]->name() == "drone 2");

    // Veille déclenchée par la voix : la suite de la phrase n'est pas exécutée
    CHECK(s.say("pause drone un avance").empty() && state() == State::SoftPaused);
    CHECK(s.say("drone un avance").empty());
    // Terme de réveil : l'écoute reprend, la suite de la phrase est exécutée
    CHECK((s.say("euh reprends drone un avance") == Log{"drone 1:avance"}) && state() == State::Active);
    // Formes à plusieurs mots ; la plus longue l'emporte
    s.say("mets en pause");
    CHECK(s.say("reprends l'écoute").empty() && state() == State::Active);
    // Vraie pause pendant la veille : le terme de réveil ne suffit plus
    s.say("pause");
    s.ctrl.pause();
    CHECK(s.say("reprends").empty() && state() == State::Paused);
    s.ctrl.resume();

    CHECK((changes == std::vector<State>{State::Paused, State::Active, State::Paused, State::Active,
                                         State::SoftPaused, State::Active, State::SoftPaused, State::Active,
                                         State::SoftPaused, State::Paused, State::Active}));

    // Audio : jeté pendant la pause ; la phrase en cours est oubliée (reset du moteur)
    struct CountingRecognizer : vc::ISpeechRecognizer {
        int* accepted;
        int* resets;
        CountingRecognizer(int* a, int* r) : accepted(a), resets(r) {}
        void setGrammar(const std::vector<std::string>&) override {}
        bool acceptAudio(const std::int16_t*, std::size_t) override { return ++*accepted, false; }
        std::string takeResult() override { return {}; }
        void reset() override { ++*resets; }
    };
    int accepted = 0, resets = 0;
    s.ctrl.setRecognizer(std::make_unique<CountingRecognizer>(&accepted, &resets));
    std::int16_t audio[4] = {};
    s.ctrl.feedAudio(audio, 4);
    std::thread([&] { s.ctrl.pause(); }).join();   // bouton sur un autre thread
    s.ctrl.feedAudio(audio, 4);
    CHECK(accepted == 1 && resets == 1);
    s.ctrl.resume();
    s.ctrl.feedAudio(audio, 4);
    CHECK(accepted == 2 && resets == 1);
}

static void testActorData() {
    struct Drone {
        int moves = 0;
        std::string lastTurn;
        std::vector<const vc::Actor*> shots;
        void avancer() { ++moves; }
        void tourner(const vc::Term& sens) { lastTurn = sens.name(); }
        void tirerSur(const vc::Actor& cible) { shots.push_back(&cible); }
    };
    Drone d1, d2;
    vc::VoiceController ctrl;
    auto& v = ctrl.vocabulary();
    auto& avance = v.term({"avance", "avancez"});
    auto& tourne = v.term({"tourne"});
    auto& gauche = v.term({"gauche"});
    auto& droite = v.term({"droite"});
    auto& drone = v.actorType(v.term({"drones"}));
    drone.order({avance}, vc::method(&Drone::avancer));
    drone.order({tourne, {gauche, droite}}, vc::method(&Drone::tourner));
    drone.order({v.term({"tire sur"}), vc::actors({drone})}, vc::method(&Drone::tirerSur));
    v.actor(drone, v.term({"drone 1", "drone un"})).setData(&d1);
    v.actor(drone, v.term({"drone 2", "drone deux"})).setData(&d2);

    ctrl.handleText("drone un avance drone un drone deux avancez drone deux tourne droite");
    CHECK(d1.moves == 2 && d2.moves == 1);
    CHECK(d2.lastTurn == "droite" && d1.lastTurn.empty());

    // vc::method avec const Actor& : un appel par acteur consommé
    ctrl.handleText("drone un tire sur drones");
    CHECK(d1.shots.size() == 2 && d1.shots[0] == v.findActor("drone 1") && d1.shots[1] == v.findActor("drone 2"));

    const vc::Actor* a = v.findActor("drone 1");
    CHECK(a->data<Drone>() == &d1);
    CHECK(a->data<int>() == nullptr);   // mauvais type : nullptr, pas de crash
}

static void testAudio() {
    // Moteur de reconnaissance simulé : feedAudio / finishAudio / onTranscript
    struct FakeRecognizer : vc::ISpeechRecognizer {
        std::vector<std::string>* grammar;
        std::string pending;
        explicit FakeRecognizer(std::vector<std::string>* g) : grammar(g) {}
        void setGrammar(const std::vector<std::string>& p) override { *grammar = p; }
        bool acceptAudio(const std::int16_t*, std::size_t count) override {
            pending = "drone un avance";
            return count > 1;   // phrase complète seulement si bloc > 1 échantillon
        }
        std::string takeResult() override { return std::move(pending); }
        bool finish() override { return !pending.empty(); }
    };
    Scene s;
    std::vector<std::string> grammar;
    std::vector<std::string> heard;
    s.ctrl.setRecognizer(std::make_unique<FakeRecognizer>(&grammar));
    s.ctrl.onTranscript([&](const std::string& t) { heard.push_back(t); });
    CHECK(grammar == s.ctrl.vocabulary().phrases());

    std::int16_t audio[2] = {0, 0};
    CHECK(s.ctrl.feedAudio(audio, 2).size() == 1);
    CHECK(s.ctrl.feedAudio(audio, 1).empty());
    CHECK(s.ctrl.finishAudio().size() == 1);
    CHECK(s.ctrl.finishAudio().empty());
    CHECK(heard.size() == 2 && heard[0] == "drone un avance");

    // processAudio : blocs de 100 ms (1600 échantillons à 16 kHz) puis finishAudio
    vc::AudioClip clip;
    clip.samples.assign(4000, 0);
    CHECK(s.ctrl.processAudio(clip).size() == 3);   // 2 blocs complets + le reste

    // Lecture WAV
    const char* path = "test_vc_tmp.wav";
    {
        std::ofstream f(path, std::ios::binary);
        auto u32 = [&](std::uint32_t v) { f.write(reinterpret_cast<const char*>(&v), 4); };
        auto u16 = [&](std::uint16_t v) { f.write(reinterpret_cast<const char*>(&v), 2); };
        const std::int16_t samples[3] = {1, -2, 300};
        f.write("RIFF", 4); u32(36 + 6); f.write("WAVE", 4);
        f.write("fmt ", 4); u32(16); u16(1); u16(1); u32(8000); u32(16000); u16(2); u16(16);
        f.write("data", 4); u32(6); f.write(reinterpret_cast<const char*>(samples), 6);
    }
    vc::AudioClip wav = vc::readWav(path);
    CHECK(wav.sampleRate == 8000);
    CHECK((wav.samples == std::vector<std::int16_t>{1, -2, 300}));
    std::remove(path);

    bool threw = false;
    try { vc::readWav("fichier_inexistant.wav"); } catch (const std::runtime_error&) { threw = true; }
    CHECK(threw);

#ifdef VC_WITH_VOSK
    // Lecture du JSON de Vosk
    CHECK(vc::extractVoskText("{\n  \"text\" : \"drone un avance\"\n}") == "drone un avance");
    CHECK(vc::extractVoskText("{\"text\": \"d\\u00e9colle \\\"x\\\"\"}") == "d\xC3\xA9" "colle \"x\"");
    CHECK(vc::extractVoskText("{\"partial\" : \"\"}").empty());
#endif
}

int main() {
    testParsing();
    testVocabulary();
    testConsumedActors();
    testListening();
    testActorData();
    testAudio();

    if (failures == 0) std::cout << "Tous les tests passent\n";
    return failures == 0 ? 0 : 1;
}

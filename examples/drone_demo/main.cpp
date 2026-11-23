// Démo : pilotage de drones et d'une caméra à la voix ou au clavier.
//
//   drone_demo.exe                    micro (si compilé avec Vosk), sinon clavier
//   drone_demo.exe --texte            clavier : chaque ligne simule une phrase entendue
//   drone_demo.exe --wav phrase.wav   fichier WAV PCM 16 bits mono (Vosk)
//   drone_demo.exe --model <dossier>  autre modèle Vosk
//
// Phrases à essayer :
//   « drone un avance »                 un acteur
//   « drone un drone deux décollez »    plusieurs acteurs
//   « les drones atterrissez »          tous les acteurs d'un type
//   « drone deux » puis « tourne à gauche »   la cible reste visée d'une phrase à l'autre
//   « drone un caméra avancez »         chaque cible dont le type connaît l'ordre
//   « zoom »                            seule caméra : pas besoin de la nommer
//   « urgence », « coucou »             ordres globaux (sans acteur)
//   « drone un tire sur drone deux char »     ordre qui consomme des acteurs (drones ou chars)
//   « drone un tire sur » puis « drone deux » ordre coupé par une pause, complété ensuite

#include <voicecontrol/voicecontrol.hpp>

#include <array>
#include <atomic>
#include <csignal>
#include <iostream>
#include <memory>
#include <string>

#ifdef _WIN32
#include <windows.h>   // SetConsoleOutputCP : accents dans la console
#endif

// --- Objets de l'application ------------------------------------------------

struct Drone {
    std::string name;
    void avancer()  { say("avance"); }
    void reculer()  { say("recule"); }
    void decoller() { say("decolle"); }
    void atterrir() { say("atterrit"); }
    void tourner(const vc::Term& sens) { say("tourne a " + sens.name()); }
    void tirerSur(const vc::Actor& cible) { say("tire sur " + cible.name()); }
    void say(const std::string& what) const { std::cout << "  " << name << " " << what << std::endl; }
};

struct Camera {
    void avancer() { std::cout << "  camera : travelling avant" << std::endl; }
    void zoomer()  { std::cout << "  camera : zoom" << std::endl; }
};

// --- Déclaration du vocabulaire ---------------------------------------------

static void declare(vc::Vocabulary& v, std::array<Drone, 4>& drones, Camera& camera) {
    // Termes (réutilisables entre ordres et entre types)
    auto& avance   = v.term({"avance", "avancez"});
    auto& recule   = v.term({"recule", "reculez"});
    auto& decolle  = v.term({"décolle", "décollez"});
    auto& atterris = v.term({"atterris", "atterrissez"});
    auto& tourne   = v.term({"tourne à", "tournez à", "tourne", "tournez"});
    auto& gauche   = v.term({"gauche"});
    auto& droite   = v.term({"droite"});
    auto& zoome    = v.term({"zoom", "zoomer", "zoome", "zoomez"});   // "zoome" est inconnu du petit modèle Vosk
    auto& tire     = v.term({"tire sur", "tirez sur"});
    auto& urgence  = v.term({"urgence"});

    // Types déclarés avant les ordres : "tire sur" consomme des drones ou des chars
    auto& drone    = v.actorType(v.term({"drones", "les drones", "tous les drones"}));
    auto& charType = v.actorType(v.term({"chars"}));

    // Type "drone" : ordres déclarés une fois pour tous les drones
    drone.order({avance},   vc::method(&Drone::avancer));
    drone.order({recule},   vc::method(&Drone::reculer));
    drone.order({decolle},  vc::method(&Drone::decoller));
    drone.order({atterris}, vc::method(&Drone::atterrir));
    drone.order({tourne, {gauche, droite}}, vc::method(&Drone::tourner));
    drone.order({tire, vc::actors({drone, charType}, vc::atLeast(1))}, vc::method(&Drone::tirerSur));

    // Acteurs : Vosk écrit les nombres en lettres, on prévoit les deux formes
    const char* numbers[] = {"un", "deux", "trois", "quatre"};
    for (std::size_t i = 0; i < drones.size(); ++i) {
        std::string n = std::to_string(i + 1);
        drones[i].name = "drone " + n;
        v.actor(drone, v.term({"drone " + n, std::string("drone ") + numbers[i]})).setData(&drones[i]);
    }

    // Type "caméra" à un seul acteur : ses ordres marchent sans la nommer
    auto& cameraType = v.actorType(v.term({"caméras"}));
    cameraType.order({avance}, vc::method(&Camera::avancer));
    cameraType.order({zoome},  vc::method(&Camera::zoomer));
    v.actor(cameraType, v.term({"caméra"})).setData(&camera);

    // Un char : simple cible, sans ordre
    v.actor(charType, v.term({"char"}));

    // Ordres globaux : adressés à l'application, pas à un acteur
    v.order({urgence}, [&drones](const vc::Arguments&) {
        std::cout << "  URGENCE :" << std::endl;
        for (auto& d : drones) d.atterrir();
    });
    v.order({v.term({"coucou"})}, [](const vc::Arguments&) { std::cout << "  Coucou !" << std::endl; });
}

// --- Programme --------------------------------------------------------------

static std::atomic<bool> g_stop{false};

int main(int argc, char** argv) {
#ifdef _WIN32
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);
#endif
    bool keyboard = false;
    std::string wavPath;
#ifdef VC_WITH_VOSK
    std::string modelPath = VC_DEFAULT_MODEL_DIR;
#else
    keyboard = true;
#endif
    for (int i = 1; i < argc; ++i) {
        std::string a = argv[i];
        if (a == "--texte") keyboard = true;
#ifdef VC_WITH_VOSK
        else if (a == "--wav" && i + 1 < argc) wavPath = argv[++i];
        else if (a == "--model" && i + 1 < argc) modelPath = argv[++i];
#endif
        else {
            std::cerr << "Usage : " << argv[0] << " [--texte] [--wav <fichier.wav>] [--model <dossier>]\n";
            return 2;
        }
    }

    std::array<Drone, 4> drones;
    Camera camera;
    vc::VoiceController ctrl;
    declare(ctrl.vocabulary(), drones, camera);

    ctrl.onTranscript([](const std::string& t) { std::cout << "Entendu : \"" << t << "\"" << std::endl; });
    ctrl.onUnhandled([](const vc::Rejection& r) {
        using Reason = vc::Rejection::Reason;
        const char* why = r.reason == Reason::Ambiguous     ? "ambigu, nommez un acteur"
                        : r.reason == Reason::NoRecipient   ? "personne ne peut l'executer, nommez un acteur"
                        : r.reason == Reason::MissingActors ? "sur qui ?"
                                                            : "cible invalide (type ou nombre)";
        std::cout << "  \"" << r.words << "\" : " << why << std::endl;
    });

    try {
        if (keyboard) {
            std::cout << "Entrez une phrase (ligne vide pour quitter) :\n";
            std::string line;
            while (std::cout << "> " && std::getline(std::cin, line) && !line.empty()) ctrl.handleText(line);
            return 0;
        }
#ifdef VC_WITH_VOSK
        vc::AudioClip clip;
        if (!wavPath.empty()) clip = vc::readWav(wavPath);

        std::cout << "Chargement du modele " << modelPath << " ..." << std::endl;
        vc::VoskRecognizer::setLogLevel(-1);
        auto recognizer = std::make_unique<vc::VoskRecognizer>(modelPath, static_cast<float>(clip.sampleRate));
        auto& vosk = *recognizer;
        ctrl.setRecognizer(std::move(recognizer));
        std::cout << "Expressions inconnues du modele (jamais entendues) :";
        for (const auto& p : vosk.ignoredPhrases()) std::cout << " \"" << p << "\"";
        std::cout << std::endl;

        if (!wavPath.empty()) {
            ctrl.processAudio(clip);
        } else {
            vc::Microphone mic(clip.sampleRate);
            std::signal(SIGINT, [](int) { g_stop = true; });
            std::cout << "Parlez (Ctrl+C pour quitter)..." << std::endl;
            ctrl.listen(mic, [] { return !g_stop; });
        }
#endif
    } catch (const std::exception& e) {
        std::cerr << "Erreur : " << e.what() << "\n";
        return 1;
    }
    return 0;
}

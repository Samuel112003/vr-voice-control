# 4. Utilisation de l'API C++

Inclure l'en-tête unique :

```cpp
#include <voicecontrol/voicecontrol.hpp>
```

Les objets déclarés (`Term`, `ActorType`, `Actor`, `Order`) appartiennent au `Vocabulary` : les références retournées restent valides tant qu'il existe.

## 1. Déclarer les termes

```cpp
vc::VoiceController ctrl;
auto& vocab = ctrl.vocabulary();

auto& avance = vocab.term({"avance", "avancez"});            // 1re forme = nom du terme
auto& tourne = vocab.term({"tourne à", "tournez à", "tourne", "tournez"});
auto& gauche = vocab.term({"gauche"});
auto& droite = vocab.term({"droite"});
```

Lève `std::invalid_argument` si aucune forme n'est donnée, ou si une forme est vide.

## 2. Déclarer un type d'acteur et ses ordres

```cpp
auto& drone = vocab.actorType(vocab.term({"drones", "les drones"}));   // nommer le type = tous ses acteurs

drone.order({avance}, [](const vc::Actor& a, const vc::Arguments&) {
    a.data<Drone>()->avancer();
});
drone.order({tourne, {gauche, droite}}, [](const vc::Actor& a, const vc::Arguments& args) {
    a.data<Drone>()->tourner(*args.terms[0]);   // gauche ou droite
});
```

- Un ordre est une liste de positions : un terme (`avance`), un choix entre plusieurs termes (`{gauche, droite}`), ou des acteurs consommés (§ 2 bis).
- `Arguments::terms` contient les termes choisis, dans l'ordre de la phrase ; les positions à un seul terme n'y figurent pas. `Arguments::actors` contient les acteurs consommés.
- Raccourci `vc::method` quand l'action est une méthode de l'objet relié à l'acteur. Il ne fait rien si l'acteur n'est relié à aucun objet du bon type.

| Méthode | Appel |
|---|---|
| `void f()` | une fois |
| `void f(const vc::Term&)` | une fois, avec le premier terme choisi |
| `void f(const vc::Actor&)` | une fois par acteur consommé |
| `void f(const vc::Arguments&)` | une fois, avec tous les arguments |

```cpp
drone.order({avance}, vc::method(&Drone::avancer));                  // void avancer()
drone.order({tourne, {gauche, droite}}, vc::method(&Drone::tourner)); // void tourner(const vc::Term&)
```

## 2 bis. Ordres qui consomment des acteurs

« drone un tire sur drone deux » : la position `vc::actors(types, nombre)` déclare les types consommables et le nombre d'acteurs. Les types doivent être déclarés avant l'ordre.

```cpp
auto& tire = vocab.term({"tire sur", "tirez sur"});
auto& charType = vocab.actorType(vocab.term({"chars"}));
drone.order({tire, vc::actors({drone, charType}, vc::atLeast(1))}, vc::method(&Drone::tirerSur));
// void Drone::tirerSur(const vc::Actor& cible) : appelé pour chaque acteur consommé
```

Nombre : `vc::exactly(n)`, `vc::atLeast(n)`, `vc::atMost(n)` (de 1 à n), `vc::between(min, max)`. Par défaut : `atLeast(1)`. Règles (types refusés, nombre hors intervalle, ordre coupé par une pause…) : [02-concepts.md](02-concepts.md#ordres-qui-consomment-des-acteurs).

Fin de la liste des acteurs consommés : le dernier nom, s'il est suivi d'un ordre que son type connaît, n'est pas consommé mais devient le sujet de cet ordre.

```cpp
ctrl.handleText("drone un tire sur drone trois drone deux tire sur drone un");
// -> drone1.tirerSur(drone3), puis drone2.tirerSur(drone1) ; drone 2 devient la cible
```

`ctrl.parser().readActors(mots, position)` applique cette règle seule (utile pour déboguer une phrase).

Délai d'attente d'un ordre coupé par une pause :

```cpp
ctrl.parser().setPendingTimeout(std::chrono::seconds(3));   // 5 s par défaut ; 0 = pas d'attente
```

## 3. Déclarer les acteurs

```cpp
Drone drone1, drone2;   // objets de l'application
vocab.actor(drone, vocab.term({"drone 1", "drone un"})).setData(&drone1);
vocab.actor(drone, vocab.term({"drone 2", "drone deux"})).setData(&drone2);
```

L'acteur ne possède pas l'objet : celui-ci doit rester valide (et à la même adresse) tant que l'acteur existe. `data<T>()` renvoie `nullptr` si aucun objet n'est relié ou si `T` n'est pas exactement le type passé à `setData`. Un acteur se retrouve par son nom avec `vocab.findActor("drone 1")`.

## 4. Ordres globaux

Adressés à l'application, sans acteur :

```cpp
auto& urgence = vocab.term({"urgence"});
vocab.order({urgence}, [&](const vc::Arguments&) { toutAtterrir(); });
```

## 5. Traiter une phrase

Depuis du texte (tests, debug, ou moteur de reconnaissance externe) :

```cpp
ctrl.handleText("drone un drone deux avancez");   // drone1 et drone2 avancent
ctrl.handleText("tourne à gauche");                // toujours visés : les deux tournent
ctrl.targets();                                     // acteurs visés
ctrl.clearTargets();                                // plus personne n'est visé
```

`handleText` retourne les ordres exécutés (`vc::Command` : ordre, acteurs qui exécutent, arguments). Règles complètes : [02-concepts.md](02-concepts.md).

Depuis l'audio, avec Vosk (bibliothèque compilée avec `-DVC_WITH_VOSK=ON`) :

```cpp
vc::VoskRecognizer::setLogLevel(-1);   // facultatif : Vosk silencieux
ctrl.setRecognizer(std::make_unique<vc::VoskRecognizer>("models/vosk-model-small-fr-0.22", 16000.0f));
ctrl.onTranscript([](const std::string& texte) { /* phrase entendue, avant analyse */ });

// à chaque bloc audio fourni par l'hôte (PCM 16 bits mono) :
ctrl.feedAudio(samples, sampleCount);
// à l'arrêt du micro / en fin de fichier, pour traiter la phrase en cours :
ctrl.finishAudio();
```

- Le constructeur de `VoskRecognizer` lève `std::runtime_error` si le modèle est introuvable.
- `VoskRecognizer::ignoredPhrases()` liste les formes écartées de la grammaire parce qu'un de leurs mots est inconnu du modèle (« drone 1 », « zoome »…) : Vosk ne les entendra jamais.
- Un `VoskRecognizer` ne doit être utilisé que depuis un seul thread.
- Si le vocabulaire change après `setRecognizer`, appeler `ctrl.refreshGrammar()`.

Outils audio fournis (`audio.hpp`) pour les tests et les applications natives :

```cpp
// Fichier WAV (PCM 16 bits mono) : découpé en blocs, puis finishAudio
vc::AudioClip clip = vc::readWav("phrase.wav");   // créer le moteur à clip.sampleRate
ctrl.processAudio(clip);

// Micro par défaut (Windows) : boucle jusqu'à ce que la condition devienne fausse
vc::Microphone mic(16000);
ctrl.listen(mic, [&] { return !stop; });
```

Dans Unity / Unreal, l'audio vient plutôt de l'API micro du moteur et passe par `feedAudio`.

## 6. Ordres non exécutés

```cpp
ctrl.onUnhandled([](const vc::Rejection& r) {
    // r.words : mots concernés ; r.candidates : ordres qui correspondaient
});
```

| `r.reason` | Cas |
|---|---|
| `NoRecipient` | aucun acteur visé ne connaît l'ordre, et aucun repli possible |
| `Ambiguous` | plusieurs replis possibles (acteurs uniques, ordres globaux) |
| `MissingActors` | aucun nom à la position acteurs, ou ordre en attente abandonné |
| `InvalidActors` | acteur d'un type non consommable, ou nombre hors intervalle |

Les refus sont signalés après l'exécution des ordres de la même phrase ; celui d'un ordre en attente abandonné, à l'arrivée de la phrase suivante.

## Analyse seule (sans exécution)

```cpp
vc::CommandParser parser(vocab);   // garde ses propres acteurs visés et son ordre en attente
std::vector<vc::Rejection> rejected;
for (const vc::Command& c : parser.parse("drone 1 avance drone 2 tourne à gauche", &rejected)) {
    // c.order, c.actors, c.args.terms, c.args.actors
}
```

## Exemple complet

Voir [examples/drone_demo/main.cpp](../examples/drone_demo/main.cpp).

# 3. Architecture

## Flux de données

```
 Application hôte (Unity / Unreal / natif)
   │  audio PCM 16 bits mono (micro du casque)
   ▼
 ISpeechRecognizer  (Vosk)        ◄── grammaire = Vocabulary::phrases()
   │  texte : "drone un tourne à gauche"
   ▼
 CommandParser                    ◄── Vocabulary (termes, types, acteurs, ordres)
   │  état : acteurs visés (persistent d'une phrase à l'autre)
   │  Command { order: tourne à [gauche|droite], actors: [drone 1], args: { terms: [gauche] } }
   ▼
 VoiceController::dispatch
   │  pour chaque acteur (ou une fois pour un ordre global)
   ▼
 Action définie par l'hôte  ──►  fait tourner le drone dans la scène 3D
```

L'**audio est capturé par l'hôte** et transmis à la bibliothèque : chaque moteur a déjà son API micro (Unity `Microphone`, Unreal Audio Capture, API Android…). Cela garde le cœur portable et sans dépendance plateforme. Pour les tests et les applications natives Windows, la bibliothèque fournit tout de même `vc::Microphone` et `vc::readWav` (`audio.hpp`).

## Modules

| Fichier | Rôle |
|---|---|
| `include/voicecontrol/vocabulary.hpp` | `Term`, `Slot`, `Order`, `ActorType`, `Actor`, `Vocabulary`, `vc::method` : modèle de données défini par l'hôte |
| `include/voicecontrol/command.hpp` | `Command` (ordre à exécuter) et `Rejection` (ordre non exécuté) |
| `include/voicecontrol/command_parser.hpp` | `CommandParser` : texte → liste de `Command` ; garde les acteurs visés |
| `include/voicecontrol/speech_recognizer.hpp` | `ISpeechRecognizer` : interface du moteur speech-to-text |
| `include/voicecontrol/vosk_recognizer.hpp` | `VoskRecognizer` : moteur Vosk (avec `VC_WITH_VOSK`) |
| `include/voicecontrol/audio.hpp` | `AudioClip`, `readWav`, `Microphone` (capture micro Windows) |
| `include/voicecontrol/voice_controller.hpp` | `VoiceController` : façade qui assemble le tout |
| `include/voicecontrol/voicecontrol.hpp` | En-tête unique à inclure |

## Arborescence du dépôt

```
XR Boy Controle Vocal/
├── CMakeLists.txt
├── include/voicecontrol/   API publique
├── src/                    implémentation
├── examples/drone_demo/    démo : drones + caméra, au clavier, au micro ou par fichier WAV
├── tests/                  tests unitaires (ctest)
├── third_party/            dépendances externes (Vosk, non versionné)
├── models/                 modèle Vosk (non versionné)
└── documentation/          cette documentation
```

## Principes de conception

- **Cœur sans dépendance** : `Vocabulary`, `CommandParser`, `VoiceController` n'utilisent que la bibliothèque standard C++17.
- **Moteur interchangeable** : Vosk est derrière `ISpeechRecognizer` ; on peut le remplacer, ou injecter du texte directement (`handleText`) pour les tests et le debug.
- **Vocabulaire à l'exécution** : rien n'est figé à la compilation.
- **Couche C prévue** (`extern "C"`) pour Unity, qui ne peut pas appeler directement des classes C++. Unreal peut utiliser l'API C++ directement.

## Points d'attention

- **Threads** : la reconnaissance Vosk est coûteuse ; elle devra tourner hors du thread de rendu. Les handlers devront alors être renvoyés sur le thread principal du moteur (file de commandes). Pas encore traité : aujourd'hui `feedAudio` exécute les handlers sur le thread appelant.
- **Durée de vie** : les références `Term&` / `ActorType&` / `Actor&` / `Order&` restent valides tant que le `Vocabulary` existe (stockage par `unique_ptr`). Les objets reliés par `setData` appartiennent à l'hôte.
- **État de l'analyse** : `CommandParser` mémorise entre deux phrases les acteurs visés (sans expiration : l'hôte peut appeler `clearTargets()`) et l'éventuel ordre en attente de ses acteurs consommés (expire après `pendingTimeout`, 5 s par défaut). L'horloge est remplaçable (`setClock`) pour les tests.
- **Runtime MinGW et Vosk** : `libvosk.dll` est livrée avec des `libstdc++` / `libgcc` / `libwinpthread` plus anciens, copiés à côté des exécutables. Ceux du projet sont donc liés avec `-static` (fonction CMake `vc_copy_vosk_runtime`) ; sinon ils chargeraient ces vieilles DLL et planteraient au démarrage (0xc0000139, fonction introuvable).
- **Performances** : les ordres sont essayés un par un à chaque position (avec retour en arrière dans les positions à choix). Suffisant pour quelques dizaines d'ordres ; à indexer si le vocabulaire grossit beaucoup.

# 5. Compilation et intégration

## Prérequis

- CMake ≥ 3.16
- Compilateur C++17 (MSVC 2019+, GCC 9+, Clang 10+, MinGW-w64)

## Compiler (Windows / MinGW)

```bash
cmake -S . -B build -G "MinGW Makefiles"
cmake --build build
ctest --test-dir build --output-on-failure
./build/examples/drone_demo/drone_demo.exe
```

Avec Visual Studio : `cmake -S . -B build -G "Visual Studio 17 2022"` puis `cmake --build build --config Release`.

## Options CMake

| Option | Défaut | Rôle |
|---|---|---|
| `VC_BUILD_EXAMPLES` | ON | Construit `drone_demo` (clavier seul sans Vosk ; micro / WAV avec Vosk) |
| `VC_BUILD_TESTS` | ON | Construit les tests (`ctest`) |
| `VC_WITH_VOSK` | OFF | Active l'adaptateur `VoskRecognizer` et le mode vocal de `drone_demo` |
| `VC_VOSK_PLATFORM` | `win64` / `linux-x86_64` / `android-<ABI>` | Sous-dossier de `third_party/vosk/lib/` à lier |

## Vosk

Vosk fournit une API C (`vosk_api.h`) et des bibliothèques précompilées (Windows, Linux, Android arm64…). Bibliothèque et modèle ne sont pas versionnés ; leur emplacement est décrit dans [third_party/README.md](../third_party/README.md).

```bash
cmake -S . -B build -G "MinGW Makefiles" -DVC_WITH_VOSK=ON
cmake --build build
./build/examples/drone_demo/drone_demo.exe                  # micro par défaut, Ctrl+C pour quitter
./build/examples/drone_demo/drone_demo.exe --wav phrase.wav # fichier WAV PCM 16 bits mono
./build/examples/drone_demo/drone_demo.exe --texte          # clavier
./build/examples/drone_demo/drone_demo.exe --model <dossier>
```

Fonctionnement de `VoskRecognizer` :
- `setGrammar` → `vosk_recognizer_new_grm(model, rate, "[\"drone un\", \"tourne à\", ..., \"[unk]\"]")` ; les expressions dont un mot est absent du modèle (« drone 1 ») sont écartées et listées par `ignoredPhrases()`, et le jeton `[unk]` évite de forcer un mot connu sur un bruit ou une phrase hors sujet ;
- `acceptAudio` → `vosk_recognizer_accept_waveform_s` ; une phrase est complète quand Vosk détecte un silence ;
- `finish` → `vosk_recognizer_final_result` (arrêt du micro, fin de fichier).

Sous Windows, les DLL de Vosk sont copiées à côté des exécutables après la compilation (fonction CMake `vc_copy_vosk_runtime(<cible>)`). `libvosk.dll` apporte son propre runtime MinGW (`libstdc++-6.dll`, `libgcc_s_seh-1.dll`, `libwinpthread-1.dll`), plus ancien : avec MinGW, les exécutables du projet sont donc liés avec `-static` pour ne pas charger ces DLL. Sinon, ils plantent au démarrage (code 0xc0000139 : fonction introuvable, par exemple `clock_gettime64`). Une application hôte qui charge `libvosk.dll` doit faire de même, ou utiliser le même runtime.

Le modèle (plusieurs dizaines de Mo) devra être embarqué avec l'application ; sur Android il doit être extrait des assets vers le stockage pour être lu par Vosk.

## Intégration Unreal

- Ajouter la bibliothèque comme **module tiers** (`ThirdParty`) dans le `Build.cs` : chemins d'include et `.lib`/`.a` par plateforme.
- Utiliser directement l'API C++ (`VoiceController`).
- Capturer l'audio via un composant Audio Capture et le convertir en PCM 16 bits mono 16 kHz.

## Intégration Unity

- Compiler en bibliothèque dynamique (`.dll` Windows, `.so` Android arm64) et la placer dans `Assets/Plugins/`.
- Unity (C#) ne peut appeler que des fonctions C : une **couche `extern "C"`** est à écrire (création du contrôleur, ajout de types/acteurs/actions, envoi audio, callback d'action via pointeur de fonction).
- Audio : `Microphone.Start`, conversion `float[]` → `int16`, rééchantillonnage en 16 kHz si besoin.

## Casques autonomes (Quest / Android)

- Compiler avec le NDK Android (`-DCMAKE_TOOLCHAIN_FILE=$NDK/build/cmake/android.toolchain.cmake -DANDROID_ABI=arm64-v8a`).
- Lier la version Android arm64 de Vosk.
- Penser à la permission `RECORD_AUDIO`.

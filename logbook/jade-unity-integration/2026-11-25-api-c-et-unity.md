# API C et scripts Unity : mise en place

Ajout de la couche C pour Unity et des premiers scripts Unity :

- `bindings/c/` : DLL `voicecontrol_c` (`include/voicecontrol_c.h`, `src/c_api.cpp`),
  test en C pur `test_c_api`. Fonctions actuelles : `vc_api_version()` (= 1), `vc_create()`,
  `vc_destroy()` (accepte NULL). Handle opaque `vc_controller*`, aucune exception ne
  traverse la frontière C, chaînes en UTF-8.
- `CMakeLists.txt` racine : `LANGUAGES C CXX`, option `VC_BUILD_C_API` (ON par défaut),
  `add_subdirectory(bindings/c)` en fin de fichier. Rien d'autre n'est modifié.
- `unity/Runtime/` : `VoiceControlNative.cs` (déclarations P/Invoke), `VoiceControllable.cs`
  (annuaire id d'acteur → GameObject). `unity/Samples/NativeCoreTest.cs` vérifie que la DLL se charge.
- Le projet Unity lui-même n'est pas dans le dépôt, seulement ces scripts.

## Doc à mettre à jour
- `03-architecture.md` § Arborescence du dépôt : ajouter `bindings/c/` (API C, DLL `voicecontrol_c`)
  et `unity/` (scripts Unity : `Runtime/`, `Samples/`).
- `03-architecture.md` § Principes de conception : « Couche C prévue » → couche C existante dans
  `bindings/c/`, seule couche qui dépend de l'API C++ ; les scripts Unity n'appellent que l'API C.
- `05-build-et-integration.md` § Options CMake : ajouter `VC_BUILD_C_API` (ON).
- `05-build-et-integration.md` § Intégration Unity : la couche `extern "C"` existe ; copier
  `voicecontrol_c.dll` dans `Assets/Plugins/x86_64/` (réglages du plugin : décocher « Any Platform »,
  Editor + Standalone, CPU x86_64) ; fermer Unity avant de remplacer la DLL (Unity la verrouille) ;
  copier les scripts de `unity/Runtime/` dans le projet. Côté C# : handle en `IntPtr`,
  `vc_destroy` dans `OnDestroy`. Avec MinGW, la DLL est liée en `-static` (sinon Unity ne la charge pas
  sans les DLL du runtime MinGW).
- `04-api.md` : pas encore de section API C (trop tôt, seules création/destruction existent) ;
  à ajouter quand `vc_handle_text` / `vc_poll` existeront.
- `07-roadmap.md` § À faire : « Couche C (`extern "C"`) pour Unity » → en cours (création/destruction
  faites ; restent envoi de texte, file d'ordres, chargement du vocabulaire).

## Décision (numéro à attribuer)
- Contexte : Unity (C#) ne peut appeler que des fonctions C ; la bibliothèque doit rester
  indépendante du moteur.
- Choix : couche C séparée dans `bindings/c/`, compilée en DLL `voicecontrol_c` à part de la
  bibliothèque `voicecontrol`. Handle opaque, types simples (`int`, `float`, `const char*` UTF-8,
  structs POD), aucune exception à la frontière. `vc_api_version()` est incrémentée à chaque
  changement incompatible, et vérifiée côté C#.
- Prévu : les ordres reconnus seront mis dans une file lue par `vc_poll` depuis `Update()`
  (thread principal Unity), plutôt que des callbacks C → C# appelés depuis un autre thread.
- Écarté : mettre l'en-tête C dans `include/voicecontrol/` et le code dans `src/` (mélange la
  bibliothèque et une couche propre à un moteur ; un `.cpp` ne doit appartenir qu'à une cible CMake).

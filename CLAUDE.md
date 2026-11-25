# XR Boy – Contrôle Vocal

Instructions pour les assistants (Claude Code…) et rappel des conventions pour tous les contributeurs.

Bibliothèque C++17 de commande vocale pour applications VR : elle reconnaît, dans une phrase, des acteurs (« drone un ») et des ordres (« tire sur drone deux »), puis appelle les fonctions de l'application hôte. Cible finale : Unity sur Meta Quest (Android). Point d'entrée de la doc : [documentation/README.md](documentation/README.md).

## Règle de documentation : le logbook

**Sur une branche de travail (`feat/…`, `fix/…`), ne jamais modifier `documentation/`.** Écrire à la place une entrée dans `logbook/<branche>/` (nouveau fichier daté, jamais modifier une entrée existante). La doc n'est mise à jour que sur `dev`, lors d'une réconciliation. Méthode complète : [logbook/README.md](logbook/README.md).

- Avant de toucher à la doc, vérifier la branche courante (`git branch --show-current`). Seule `dev` autorise la modification de `documentation/` et de `logbook/README.md`.
- Une décision technique se décrit dans l'entrée du logbook, **sans numéro** ; le numéro est attribué à la réconciliation.
- Les commentaires de code et la doc des en-têtes (`include/`) font partie du code : ils se mettent à jour avec lui, sur la branche.

## Organisation

| Dossier | Contenu |
|---|---|
| `include/voicecontrol/` | API publique (`voicecontrol.hpp` regroupe tout) |
| `src/` | implémentation |
| `tests/` | tests (`ctest`), sans framework |
| `examples/drone_demo/` | démo : application cliente de la bibliothèque, clavier / micro / WAV |
| `documentation/` | doc du projet, en français ; décisions dans `documentation/decisions/` |
| `logbook/` | notes de doc en attente de réconciliation |
| `third_party/vosk/`, `models/` | Vosk et modèle français : non versionnés (voir `third_party/README.md`) |

## Compiler et tester (Windows, MinGW)

```bash
cmake -S . -B build -G "MinGW Makefiles"                    # sans Vosk : clavier seulement
cmake -S . -B build -G "MinGW Makefiles" -DVC_WITH_VOSK=ON  # avec Vosk
cmake --build build
ctest --test-dir build --output-on-failure
./build/examples/drone_demo/drone_demo.exe --texte          # clavier ; sans option : micro
```

**Avant toute fusion vers `dev` : ça compile sans avertissement et tous les tests passent.** Si la modification touche la partie vocale, tester aussi avec Vosk.

## Conventions

- **Langue** : documentation, commentaires et messages en **français** ; identifiants du code (classes, fonctions, variables) en **anglais**. Espace de noms `vc`.
- **La bibliothèque ne contient aucun vocabulaire métier** : pas de mot, d'acteur, d'ordre ni de touche prédéfinis (ni « drone », ni « pause »…). Elle fournit les mécanismes ; chaque application déclare ses mots. Tout ce qui est propre à un projet va dans l'application (la démo pour ce dépôt).
- **La démo est un exemple, pas une partie de la bibliothèque** : elle n'est pas compilée dans `voicecontrol` et ne doit pas l'être.
- **Nouvelle règle d'analyse ou nouveau comportement : un test** dans `tests/`, qui échoue avant la correction et passe après.
- Avec MinGW et Vosk, les exécutables sont liés en `-static` (fonction CMake `vc_copy_vosk_runtime`) : `libvosk.dll` apporte un runtime MinGW plus ancien, qui ferait planter l'exécutable au démarrage (0xc0000139).
- Ne jamais commiter `build/`, `build-*/`, `models/`, `third_party/vosk/` (déjà dans `.gitignore`).

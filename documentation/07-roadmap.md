# 7. Feuille de route

## Fait

- [x] Prototype Python (bibliothèque + pilotage de drones hypothétiques)
- [x] Initialisation du projet C++ (CMake, structure, documentation)
- [x] Adaptateur `VoskRecognizer` (option `VC_WITH_VOSK`), capture micro Windows, lecture WAV
- [x] Règles du prototype Python (D7) : termes réutilisables, ordres à choix, désignation d'un type, acteurs visés persistants, repli sur acteur unique, ordres globaux
- [x] Normalisation des accents et de la ponctuation
- [x] Démo `drone_demo` (clavier / micro / WAV) et tests unitaires
- [x] Ordres qui consomment des acteurs (types et nombre déclarés), ordre coupé par une pause mis en attente
- [x] Correction : le dernier nom d'une liste d'acteurs consommés, suivi d'un ordre qu'il connaît, en devient le sujet (« … tire sur drone trois drone deux tire sur drone un »)
- [x] Pause (bouton on/off, reprise non vocale) et veille (reprise par un terme de réveil)
- [x] Correction : exécutables liés avec `-static` (plantage 0xc0000139 au démarrage, à cause des vieilles DLL du runtime MinGW livrées avec Vosk)

## À faire

- [ ] Limiter les conflits de code entre branches : un fichier de test par fonctionnalité (au lieu de `tests/test_parser.cpp`), un fichier par classe (au lieu de `src/vocabulary.cpp` / `vocabulary.hpp`), formatage automatique (`.clang-format`), vérification automatique de la compilation et des tests sur GitHub
- [ ] Gestion des threads : reconnaissance en tâche de fond, actions sur le thread principal
- [ ] Couche C (`extern "C"`) pour Unity
- [ ] Bibliothèque dynamique + build Android arm64
- [ ] Plugin d'exemple Unity et Unreal
- [ ] Remplacer les tests maison par un framework (Catch2 / GoogleTest) si les tests se multiplient

## Questions ouvertes

- **Expiration des acteurs visés** : aujourd'hui ils restent visés sans limite (l'hôte peut appeler `clearTargets()`). Faut-il un délai (comme pour l'ordre en attente), ou un mot « annule » ?
- **Plusieurs positions acteurs** dans un même ordre (« drone un donne caméra à drone deux ») : une seule autorisée pour l'instant.
- **Ordre des signalements** : les refus d'une phrase sont signalés après l'exécution de ses ordres, même s'ils la précédaient dans la phrase.
- **Paramètres numériques** : « avance de 3 mètres » demanderait des termes nombres (« un », « deux »…) ou un type de position « nombre ».
- **Seuil de confiance** : exploiter le score Vosk pour rejeter les reconnaissances douteuses ?
- **Vocabulaire Vosk** : certaines formes sont inconnues du petit modèle (« zoome », « reculez », « atterrissez ») ; faut-il un modèle plus grand, ou ajouter des mots au modèle ?

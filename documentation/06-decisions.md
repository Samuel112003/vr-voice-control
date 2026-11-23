# 6. Journal des décisions

Chaque décision : contexte, choix, conséquences. Ajouter les nouvelles décisions à la suite.

---

## D1 – Vosk comme moteur de reconnaissance vocale

- **Contexte** : il faut convertir la parole en texte, hors ligne, sur des appareils peu puissants.
- **Choix** : Vosk (Kaldi), retenu lors du prototype Python comme l'option la plus légère et la plus complète.
- **Conséquences** : fonctionne hors ligne, modèles français légers disponibles, API C, bibliothèques Android. Possibilité de restreindre la reconnaissance à une liste de mots (grammaire).

## D2 – Restreindre la reconnaissance au vocabulaire défini

- **Contexte** : avec un vocabulaire libre, des confusions apparaissent (« drone » / « drôle »).
- **Choix** : fournir à Vosk les expressions connues (`Vocabulary::phrases()` : « drone un », « tourne à »…), plutôt que des mots isolés.
- **Conséquences** : meilleure précision ; les mots hors vocabulaire ne sont pas reconnus (les ajouter en formes si besoin). Une forme dont un mot est inconnu du modèle est écartée (`VoskRecognizer::ignoredPhrases()`).

## D3 – Modèle « noms + ordres » *(remplacé par D7)*

- **Contexte** : il faut donner des ordres à différents objets d'une scène 3D.
- **Choix initial** : une phrase est une liste de termes ; une liste d'acteurs reçoit une liste d'actions d'un seul mot.
- **Remplacé** : voir D7.

## D4 – Types d'acteurs

- **Contexte** : plusieurs acteurs équivalents (drone 1, drone 2) partagent les mêmes ordres.
- **Choix** : les ordres et leurs actions sont définis sur un `ActorType` ; chaque `Actor` référence son type et est relié à l'objet de l'hôte (`setData`).
- **Conséquences** : pas de duplication ; l'action reçoit l'acteur concerné.

## D5 – Réécriture en C++17

- **Contexte** : les casques VR autonomes n'ont pas d'interpréteur Python ; la bibliothèque doit servir sous Unity et Unreal.
- **Choix** : C++17, build CMake, cœur sans dépendance externe.
- **Conséquences** : portable (Windows, Android). Une couche C sera nécessaire pour Unity.

## D6 – L'audio est fourni par l'application hôte

- **Contexte** : chaque moteur / plateforme a sa propre API micro.
- **Choix** : la bibliothèque reçoit du PCM 16 bits mono ; elle ne capture pas le micro elle-même.
- **Conséquences** : cœur portable ; chaque intégration doit écrire la capture et la conversion audio. Pour les tests et les applications natives Windows, `vc::Microphone` et `vc::readWav` sont tout de même fournis.

## D7 – Règles du prototype Python : ordres, cibles persistantes, replis

- **Contexte** : le prototype Python (`voice_commands.py`) fixait des règles plus riches que la première version C++ : ordres de plusieurs termes avec choix, désignation d'un type entier, cibles qui persistent, ordres globaux.
- **Choix** : reprendre ces règles, en corrigeant deux comportements du Python :
  - un ordre est exécuté par **tous** les acteurs visés dont le type le connaît (le Python n'exécutait qu'un type, choisi au hasard en cas d'égalité) ;
  - l'acteur d'un type qui n'en a qu'un n'est qu'un **repli** quand aucune cible ne connaît l'ordre ; il ne concurrence jamais les cibles. Plusieurs replis possibles = ordre ambigu, non exécuté.
- **Conséquences** : « drone deux… tourne à gauche » fonctionne malgré la coupure de phrase de Vosk ; l'analyseur a un état (acteurs visés). Règles : [02-concepts.md](02-concepts.md).

## D8 – API par objets plutôt que par identifiants texte

- **Contexte** : les termes sont réutilisés entre ordres et entre types ; les types ont leur propre terme.
- **Choix** : comme en Python, l'hôte déclare des `Term` et passe les objets (`vocab.actor(drone, terme)`, `drone.order({tourne, {gauche, droite}}, …)`) au lieu d'identifiants texte.
- **Conséquences** : pas de faute de frappe possible dans un identifiant ; le nom d'un terme ou d'un acteur est sa première forme.

## D9 – Acteurs en paramètre d'un ordre : types et nombre déclarés

- **Contexte** : certains ordres visent un autre acteur (« drone un tire sur drone deux »).
- **Choix** : comme un paramètre typé en POO, l'ordre déclare une position acteurs avec ses types consommables et un nombre `{min, max}` (`exactly`, `atLeast`, `atMost`, `between`). Sans position acteurs, l'ordre en consomme 0 (défaut). Un acteur peut se viser lui-même. Les acteurs consommés ne deviennent pas des cibles : la cible reste celui qui exécute. Trop d'acteurs ou un type refusé : l'ordre entier est refusé.
- **Fin de la liste des acteurs consommés** : sans ponctuation à l'oral, le dernier nom suivi d'un ordre que son type connaît est le sujet de cet ordre, pas un acteur consommé (« drone un tire sur drone trois drone deux tire sur drone un »).
- **Ordre coupé par une pause** : mis en attente ; la phrase suivante le complète si elle commence par des noms, dans un délai réglable (5 s par défaut).
- **Conséquences** : un ordre ne reçoit jamais un acteur d'un type qu'il ne sait pas traiter ; un nombre ou un type invalide est signalé par `onUnhandled` au lieu d'être exécuté. L'action reçoit désormais `Arguments` (termes choisis + acteurs consommés) au lieu de la seule liste de termes. Détails : [02-concepts.md](02-concepts.md).

## D10 – Pause (non vocale) et veille (terme de réveil)

- **Contexte** : il faut pouvoir couper le contrôle vocal (bouton on/off), et aussi le mettre en sommeil à la voix.
- **Choix** : deux états en plus de l'écoute active. **Pause** : l'audio est jeté, seule une action non vocale (`resume()`) reprend. **Veille** : la reconnaissance continue, rien n'est exécuté jusqu'au terme de réveil, dont la suite de phrase est exécutée. Les deux oublient la phrase en cours et l'ordre en attente, mais conservent les acteurs visés.
- **Mots et boutons non fournis** : la bibliothèque ne prédéfinit aucun mot de veille ou de réveil, ni aucune touche. Chaque projet les déclare (termes, ordre global qui appelle `softPause`, boutons qui appellent `pause` / `softPause` / `resume`), car « pause » peut avoir un autre sens selon le projet, ou d'autres mots être plus naturels dans son contexte. Une fonction toute faite avec des mots par défaut a été envisagée puis écartée pour cette raison.
- **Conséquences** : en pause, le micro reste ouvert (reprise immédiate). Appelables depuis un autre thread (drapeaux atomiques, oubli appliqué par le thread de l'audio). Le terme de réveil doit être dans le vocabulaire pour que Vosk puisse l'entendre.

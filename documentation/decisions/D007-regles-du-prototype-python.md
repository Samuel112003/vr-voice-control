# D7 – Règles du prototype Python : ordres, cibles persistantes, replis

- **Contexte** : le prototype Python (`voice_commands.py`) fixait des règles plus riches que la première version C++ : ordres de plusieurs termes avec choix, désignation d'un type entier, cibles qui persistent, ordres globaux.
- **Choix** : reprendre ces règles, en corrigeant deux comportements du Python :
  - un ordre est exécuté par **tous** les acteurs visés dont le type le connaît (le Python n'exécutait qu'un type, choisi au hasard en cas d'égalité) ;
  - l'acteur d'un type qui n'en a qu'un n'est qu'un **repli** quand aucune cible ne connaît l'ordre ; il ne concurrence jamais les cibles. Plusieurs replis possibles = ordre ambigu, non exécuté.
- **Conséquences** : « drone deux… tourne à gauche » fonctionne malgré la coupure de phrase de Vosk ; l'analyseur a un état (acteurs visés). Règles : [02-concepts.md](../02-concepts.md).

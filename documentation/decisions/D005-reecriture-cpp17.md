# D5 – Réécriture en C++17

- **Contexte** : les casques VR autonomes n'ont pas d'interpréteur Python ; la bibliothèque doit servir sous Unity et Unreal.
- **Choix** : C++17, build CMake, cœur sans dépendance externe.
- **Conséquences** : portable (Windows, Android). Une couche C sera nécessaire pour Unity.

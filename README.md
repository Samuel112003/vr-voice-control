# XR Boy – Contrôle Vocal

Bibliothèque C++17 de commande vocale pour applications VR : reconnaît des **acteurs** et des **ordres** dans une phrase (« drone un, tourne à gauche » → `Drone::tourner(gauche)` sur drone 1) et appelle le code défini par l'application hôte.

```bash
cmake -S . -B build -G "MinGW Makefiles"
cmake --build build
ctest --test-dir build --output-on-failure
./build/examples/drone_demo/drone_demo.exe
```

Reconnaissance vocale avec Vosk (bibliothèque dans `third_party/vosk/`, modèle dans `models/`, voir [third_party/README.md](third_party/README.md)) :

```bash
cmake -S . -B build -G "MinGW Makefiles" -DVC_WITH_VOSK=ON
cmake --build build
./build/examples/drone_demo/drone_demo.exe      # parlez dans le micro : « drone un avance »
```

Documentation complète : [documentation/](documentation/README.md)

# D1 – Vosk comme moteur de reconnaissance vocale

- **Contexte** : il faut convertir la parole en texte, hors ligne, sur des appareils peu puissants.
- **Choix** : Vosk (Kaldi), retenu lors du prototype Python comme l'option la plus légère et la plus complète.
- **Conséquences** : fonctionne hors ligne, modèles français légers disponibles, API C, bibliothèques Android. Possibilité de restreindre la reconnaissance à une liste de mots (grammaire).

# D6 – L'audio est fourni par l'application hôte

- **Contexte** : chaque moteur / plateforme a sa propre API micro.
- **Choix** : la bibliothèque reçoit du PCM 16 bits mono ; elle ne capture pas le micro elle-même.
- **Conséquences** : cœur portable ; chaque intégration doit écrire la capture et la conversion audio. Pour les tests et les applications natives Windows, `vc::Microphone` et `vc::readWav` sont tout de même fournis.

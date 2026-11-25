# D2 – Restreindre la reconnaissance au vocabulaire défini

- **Contexte** : avec un vocabulaire libre, des confusions apparaissent (« drone » / « drôle »).
- **Choix** : fournir à Vosk les expressions connues (`Vocabulary::phrases()` : « drone un », « tourne à »…), plutôt que des mots isolés.
- **Conséquences** : meilleure précision ; les mots hors vocabulaire ne sont pas reconnus (les ajouter en formes si besoin). Une forme dont un mot est inconnu du modèle est écartée (`VoskRecognizer::ignoredPhrases()`).

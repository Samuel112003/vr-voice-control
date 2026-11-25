# D4 – Types d'acteurs

- **Contexte** : plusieurs acteurs équivalents (drone 1, drone 2) partagent les mêmes ordres.
- **Choix** : les ordres et leurs actions sont définis sur un `ActorType` ; chaque `Actor` référence son type et est relié à l'objet de l'hôte (`setData`).
- **Conséquences** : pas de duplication ; l'action reçoit l'acteur concerné.

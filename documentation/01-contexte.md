# 1. Contexte du projet

## Objectif

Fournir une **bibliothèque réutilisable** permettant à de futurs projets VR d'ajouter du contrôle vocal avec quelques appels simples.

- L'**application hôte** (jeu, simulateur, outil) définit son vocabulaire : les mots, les acteurs et les actions.
- La **bibliothèque** fournit la logique : reconnaissance de la parole, découpage de la phrase en acteurs / actions, appel des fonctions associées.

La bibliothèque ne connaît rien du domaine métier : « drone », « porte », « lumière » ou « explorateur de fichiers » sont tous définis par l'hôte.

## Historique : prototype Python

Une première version a été écrite en Python pour valider le concept :

- un fichier **bibliothèque** contenant la conception (termes, acteurs, actions) ;
- un second fichier implémentant le **pilotage de drones hypothétiques** pour tester la bibliothèque.

La reconnaissance vocale y est assurée par **Vosk**, choisi car c'était l'option la plus légère et la plus complète.

## Pourquoi réécrire en C++

- Les **casques VR autonomes** (ex : Meta Quest, sous Android) n'embarquent pas d'interpréteur Python.
- Le C++ s'intègre aussi bien à **Unreal** (nativement) qu'à **Unity** (plugin natif appelé via P/Invoke).
- Vosk expose une API C, directement utilisable depuis le C++.

## Contraintes

| Contrainte | Conséquence |
|---|---|
| Exécution sur casque autonome (ARM64 / Android) | Pas de dépendance à Python ni à un service cloud ; code portable |
| Temps réel | Analyse légère ; la reconnaissance tourne sur un flux audio continu |
| Réutilisable dans plusieurs moteurs | Aucune dépendance à Unity / Unreal dans le cœur ; l'hôte fournit l'audio |
| Vocabulaire variable selon le projet | Tout le vocabulaire est défini à l'exécution par l'hôte |
| Ambiguïtés de reconnaissance (« drone » / « drôle ») | Restriction du moteur à une liste de mots prédéfinis (grammaire) |

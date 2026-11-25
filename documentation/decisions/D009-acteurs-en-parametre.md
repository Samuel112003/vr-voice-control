# D9 – Acteurs en paramètre d'un ordre : types et nombre déclarés

- **Contexte** : certains ordres visent un autre acteur (« drone un tire sur drone deux »).
- **Choix** : comme un paramètre typé en POO, l'ordre déclare une position acteurs avec ses types consommables et un nombre `{min, max}` (`exactly`, `atLeast`, `atMost`, `between`). Sans position acteurs, l'ordre en consomme 0 (défaut). Un acteur peut se viser lui-même. Les acteurs consommés ne deviennent pas des cibles : la cible reste celui qui exécute. Trop d'acteurs ou un type refusé : l'ordre entier est refusé.
- **Fin de la liste des acteurs consommés** : sans ponctuation à l'oral, le dernier nom suivi d'un ordre que son type connaît est le sujet de cet ordre, pas un acteur consommé (« drone un tire sur drone trois drone deux tire sur drone un »).
- **Ordre coupé par une pause** : mis en attente ; la phrase suivante le complète si elle commence par des noms, dans un délai réglable (5 s par défaut).
- **Conséquences** : un ordre ne reçoit jamais un acteur d'un type qu'il ne sait pas traiter ; un nombre ou un type invalide est signalé par `onUnhandled` au lieu d'être exécuté. L'action reçoit désormais `Arguments` (termes choisis + acteurs consommés) au lieu de la seule liste de termes. Détails : [02-concepts.md](../02-concepts.md).

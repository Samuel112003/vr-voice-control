# D8 – API par objets plutôt que par identifiants texte

- **Contexte** : les termes sont réutilisés entre ordres et entre types ; les types ont leur propre terme.
- **Choix** : comme en Python, l'hôte déclare des `Term` et passe les objets (`vocab.actor(drone, terme)`, `drone.order({tourne, {gauche, droite}}, …)`) au lieu d'identifiants texte.
- **Conséquences** : pas de faute de frappe possible dans un identifiant ; le nom d'un terme ou d'un acteur est sa première forme.

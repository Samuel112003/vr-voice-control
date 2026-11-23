# 8. Glossaire

| Terme | Définition |
|---|---|
| **Application hôte** | Le projet VR (Unity, Unreal…) qui utilise la bibliothèque et définit le vocabulaire. |
| **Terme** (`Term`) | Mot ou groupe de mots reconnaissable (« avance », « drone un »). Réutilisable dans plusieurs ordres. |
| **Forme** | Façon de prononcer un terme (« avance », « avancez »). La première forme donne son nom au terme. |
| **Acteur** (`Actor`) | Élément pilotable de l'application, appelé par son terme (« drone 1 »), relié à l'objet réel (`setData`). |
| **Type d'acteur** (`ActorType`) | Famille d'acteurs partageant les mêmes ordres (« drones »). Son terme désigne tous ses acteurs. |
| **Ordre** (`Order`) | Suite de positions associée à une action (« tourne à [gauche \| droite] »). |
| **Position** (`Slot`) | Élément d'un ordre : un terme imposé, un choix entre plusieurs termes, ou des acteurs consommés. |
| **Position acteurs** (`vc::actors`) | Position qui consomme des acteurs (« tire sur drone deux ») : types consommables et nombre `{min, max}`. |
| **Acteurs consommés** | Acteurs nommés à la position acteurs, passés à l'action ; ils ne deviennent pas des cibles. |
| **Arguments** (`Arguments`) | Ce que reçoit l'action : termes choisis (`terms`) et acteurs consommés (`actors`). |
| **Ordre en attente** | Ordre coupé en fin de phrase avant ses acteurs consommés, que la phrase suivante peut compléter. |
| **Ordre global** | Ordre adressé à l'application elle-même, sans acteur (« urgence »). |
| **Action** | Fonction de l'hôte appelée quand un ordre est exécuté (`ActorAction`, `GlobalAction`). |
| **Acteurs visés** (cibles) | Acteurs nommés le plus récemment ; ils reçoivent les ordres suivants, même dans les phrases suivantes. |
| **Sujet** | Acteur(s) à qui l'on parle et qui exécutent l'ordre ; ils deviennent les acteurs visés. Dans « drone un tire sur drone deux », le sujet est drone 1, drone 2 est un acteur consommé. |
| **Repli** | Destinataire utilisé quand aucune cible ne connaît l'ordre : acteur d'un type unique, ou ordre global. |
| **Pause** (`pause`) | État où l'audio est jeté et rien n'est exécuté ; seule une action non vocale (`resume`) reprend. |
| **Veille** (`softPause`) | État où la reconnaissance continue mais rien n'est exécuté jusqu'au terme de réveil. |
| **Terme de réveil** | Terme qui fait sortir de la veille (« reprends ») ; la suite de la phrase est exécutée. Choisi par chaque projet, comme le terme de mise en veille. |
| **Commande** (`Command`) | Résultat de l'analyse : un ordre, les acteurs qui l'exécutent, ses arguments. |
| **Rejet** (`Rejection`) | Ordre reconnu mais non exécuté : sans destinataire, ambigu, acteurs consommés manquants ou invalides. |
| **Vocabulaire** (`Vocabulary`) | Ensemble des termes, types, acteurs et ordres définis par l'hôte. |
| **Grammaire** | Liste d'expressions à laquelle le moteur de reconnaissance est restreint. |
| **Vosk** | Bibliothèque open source de reconnaissance vocale hors ligne (basée sur Kaldi). |
| **Modèle Vosk** | Fichiers de données acoustiques / linguistiques d'une langue (ex : `vosk-model-small-fr`). |
| **PCM 16 bits** | Format audio brut attendu : échantillons entiers signés 16 bits, mono. |
| **Casque autonome** | Casque VR fonctionnant sans PC (ex : Meta Quest, sous Android). |
| **P/Invoke** | Mécanisme C# (Unity) pour appeler des fonctions C d'une bibliothèque native. |

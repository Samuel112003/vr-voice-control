# Désélection des acteurs visés et délai de sélection

## Ce qui a changé

1. **Désélection manuelle** : `VoiceController::clearTargets()` existait déjà ; elle abandonne désormais aussi l'ordre en attente (il était destiné aux acteurs désélectionnés) et notifie le changement. Nouvelle fonction `VoiceController::clearTargetsAction()`, qui renvoie une action prête à associer à un terme de l'application :
   `vocab.order({vocab.term({"repos"})}, ctrl.clearTargetsAction());`
   Le mot n'est pas fourni par la bibliothèque (même principe que la pause, D10).
2. **Délai de sélection** : les acteurs visés sont désélectionnés automatiquement si **aucun ordre ne leur a été adressé et qu'ils n'ont pas été renommés** pendant ce délai.
   - 10 s par défaut ; `ctrl.setTargetTimeout(std::chrono::seconds(30))` pour le changer ; `ctrl.setTargetTimeout(vc::never)` : jamais (comportement d'avant).
   - Le compteur repart quand on nomme les cibles ou quand elles exécutent un ordre.
   - Le temps passé en pause ou en veille ne compte pas (le compteur repart à la reprise).
   - Appliqué au début de chaque phrase, et par `ctrl.update()`. `feedAudio()` et `listen()` appellent `update()` d'eux-mêmes ; une application qui n'envoie pas d'audio en continu doit appeler `update()` régulièrement (ex : `Update()` d'Unity) pour que l'expiration soit détectée sans attendre la phrase suivante.
   - Aussi accessible sur l'analyseur : `parser().setTargetTimeout()`, `expireTargets()`, `touchTargets()`.
3. **Notification** : `ctrl.onTargetsChanged(callback)`, appelé avec la nouvelle sélection (vide = plus personne) quand elle change : noms, désélection, délai écoulé. Notifié en fin de phrase (les sélections intermédiaires d'une même phrase ne sont pas notifiées), ou tout de suite pour `clearTargets()` et `update()`.
4. **Exécution au fil de la phrase** (nécessaire pour la désélection) : chaque ordre est maintenant exécuté **dès qu'il est reconnu**, et non plus après l'analyse de toute la phrase. Sans ça, « repos drone deux avance » désélectionnait drone 2 *après* l'avoir visé. Nouvelle surcharge `CommandParser::parse(texte, onCommand, onRejection)` (rappels ; `onCommand` renvoie `false` pour arrêter l'analyse) ; l'ancienne `parse(texte, &rejets)` qui renvoie un vecteur reste disponible pour l'analyse seule.
   Conséquences : les refus (`onUnhandled`) sont signalés **à leur place dans la phrase** ; « pause drone un avance » s'arrête avant de viser drone 1 (avant : drone 1 devenait la cible sans exécuter l'ordre).

Tests : `testSelection()` dans `tests/test_parser.cpp`. Démo : « repos » désélectionne, la sélection courante s'affiche (`[selection : …]`).

## Doc à mettre à jour

- **`02-concepts.md` § Acteurs visés** : remplacer « Ils le restent d'une phrase à l'autre, jusqu'à ce que d'autres soient nommés (ou que l'application appelle `clearTargets()`) » par les trois façons de perdre la sélection : nommer d'autres acteurs, désélection (à la voix via un terme de l'application relié à `clearTargetsAction()`, ou par le code), délai de sélection (10 s par défaut, réglable, `vc::never`, compteur qui repart à chaque ordre ou nom, gelé en pause/veille). Ajouter au tableau : « drone un » puis « repos » → plus personne ; « repos drone deux avance » → drone 2 avance.
- **`02-concepts.md` § Règles d'analyse** : ajouter une règle « chaque ordre est exécuté dès qu'il est reconnu, dans l'ordre de la phrase ; une action qui change l'état (désélection, pause, veille) s'applique à la suite de la phrase ».
- **`02-concepts.md` § Pause et veille** : préciser que le délai de sélection ne court pas pendant la pause ou la veille.
- **`03-architecture.md`** : flux de données (l'analyseur transmet chaque `Command` à `dispatch` au fil de la phrase, plus en lot) ; § Points d'attention « État de l'analyse » : remplacer « sans expiration » par le délai de sélection ; § Threads : `clearTargets()` et `update()` s'appellent depuis le thread de l'audio (ou depuis une action), contrairement à `pause()`/`softPause()`/`resume()`.
- **`04-api.md` § 5 Traiter une phrase** : ajouter `clearTargetsAction()`, `setTargetTimeout()` / `vc::never`, `onTargetsChanged()`, `update()` avec un exemple ; supprimer en § 7 la phrase « Les refus sont signalés après l'exécution des ordres de la même phrase » (désormais à leur place) ; § Analyse seule : mentionner la surcharge à rappels.
- **`07-roadmap.md`** : retirer les questions ouvertes « Expiration des acteurs visés » et « Ordre des signalements » (résolues) ; ajouter dans « Fait » : désélection, délai de sélection, exécution au fil de la phrase.
- **`08-glossaire.md`** : ajouter « Désélection » et « Délai de sélection » ; compléter « Acteurs visés ».

## Décision (numéro à attribuer)

**Désélection et délai de sélection**
- **Contexte** : une cible nommée longtemps auparavant recevait encore les ordres ; il fallait pouvoir désélectionner à la voix et automatiquement.
- **Choix** :
  - fonction de désélection dans la bibliothèque, mot laissé à l'application (`clearTargetsAction()`) ;
  - délai de sélection de 10 s par défaut, réglable, désactivable (`vc::never`) ; le compteur repart à chaque ordre adressé aux cibles ou quand on les renomme ; il est gelé en pause / veille (cohérent avec D10 : la pause conserve les cibles) ;
  - désélectionner abandonne l'ordre en attente ;
  - les ordres sont exécutés au fil de la phrase, pour que la désélection (et la pause) s'applique à la suite de la phrase.
- **Écartés** : un mot de désélection intégré à la bibliothèque (le vocabulaire est propre à chaque projet) ; un compteur qui ne repart qu'au nommage (une cible à qui l'on parle sans cesse aurait été désélectionnée au milieu d'une série d'ordres) ; un thread minuteur dans la bibliothèque (l'expiration est vérifiée à chaque phrase et par `update()`, sans thread supplémentaire).

## À savoir

- Pour tester le délai à la main avec la démo en entrée redirigée, prévoir une marge : l'exécutable démarre en plusieurs secondes depuis le lecteur réseau `H:`, ce qui décale la première phrase (un test à « 11 s » n'en mesurait en fait que 8 ou 9).

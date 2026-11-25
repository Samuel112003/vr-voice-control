# Logbook

## Pourquoi ce dossier existe

[`documentation/`](../documentation/README.md) décrit le projet tel qu'il est. Presque chaque évolution de la bibliothèque touche les mêmes fichiers de doc (`02-concepts.md`, `04-api.md`, `07-roadmap.md`, `08-glossaire.md`…). Si chaque branche les modifie directement, deux branches entrent en conflit à la fusion, même quand leur code n'a rien en commun.

Le logbook remplace ces modifications directes : **sur une branche de travail, on ne modifie pas `documentation/`.** On note ici ce qui a changé et ce que la doc doit en retenir ; la doc est mise à jour plus tard, en une fois, sur la branche partagée `dev`. Les fichiers du logbook sont uniquement **ajoutés**, chaque branche dans son propre sous-dossier : la fusion de plusieurs branches ne crée donc jamais de conflit ici.

## Branches

| Branche | Rôle | `documentation/` modifiable ? |
|---|---|---|
| `main` | version stable, livrable | non (reçoit `dev`) |
| `dev` | intégration : les branches de travail en partent et y reviennent | **oui** : c'est là qu'a lieu la réconciliation |
| `feat/…`, `fix/…` | une évolution ou une correction | **non** : écrire dans `logbook/` |

## Comment ça marche

1. **Créer une branche depuis `dev`**, par exemple `feat/expiration-cibles`.
2. **La première fois qu'il y a quelque chose à noter**, créer `logbook/feat-expiration-cibles/` (nom de la branche, `/` remplacés par `-` : voir plus bas).
3. **Écrire chaque note dans un nouveau fichier**, préfixé par la date : `logbook/feat-expiration-cibles/2026-11-26-delai-cibles.md`. Autant de fichiers que nécessaire au fil de la branche. **Ne jamais modifier un fichier déjà commité** dans le logbook, le sien comme celui d'un autre : s'il y a du nouveau, ajouter un fichier.
4. **Fusionner la branche dans `dev`** normalement. Chaque branche n'écrit que dans son sous-dossier : aucun conflit possible ici.
5. **Réconcilier régulièrement, sur `dev`** (voir plus bas) : reporter toutes les entrées dans `documentation/`, puis supprimer les sous-dossiers du logbook.
6. **Les branches créées depuis `dev` ensuite** partent d'une doc à jour et d'un logbook vide.

### Quand réconcilier, et qui

- **Avant chaque fusion de `dev` vers `main`**, obligatoirement : `main` doit toujours avoir une doc à jour.
- **Et dès que le logbook dépasse une dizaine d'entrées**, pour que la réconciliation reste courte.
- Celui ou celle qui fusionne `dev` vers `main` s'en charge (ou la confie explicitement à quelqu'un).

## Ce que contient une entrée

Pas de modèle imposé, mais assez pour que quelqu'un qui n'a pas suivi la branche puisse mettre la doc à jour **sans refaire l'enquête** :

- **Ce qui a changé**, brièvement (l'historique Git a les détails, inutile de recopier le diff).
- **Les fichiers et sections de `documentation/` concernés**, et ce qu'ils doivent dire : « `04-api.md` § 5 : ajouter `setTargetTimeout(...)`, avec un exemple » plutôt que « mettre à jour l'API ».
- **Une décision, s'il y en a une** : contexte, choix, conséquences, alternatives écartées. **Sans numéro** : il est attribué à la réconciliation (voir [06-decisions.md](../documentation/06-decisions.md)), pour que deux branches ne prennent jamais le même.
- **Ce qui n'est pas évident dans le code** : une contrainte, une alternative rejetée et pourquoi, un obstacle rencontré et contourné.

Exemple :

```markdown
# Délai d'expiration des acteurs visés

Ajout de `CommandParser::setTargetTimeout()` (0 = jamais, défaut) : la cible est
oubliée si aucun ordre ne lui a été adressé pendant ce délai.

## Doc à mettre à jour
- `02-concepts.md` § Acteurs visés : remplacer « ils le restent d'une phrase à l'autre »
  par la règle avec délai ; le compteur repart à chaque ordre exécuté par la cible.
- `04-api.md` § 5 : ajouter `ctrl.parser().setTargetTimeout(std::chrono::seconds(30))`.
- `07-roadmap.md` : retirer la question ouverte « Expiration des acteurs visés ».

## Décision (numéro à attribuer)
- Contexte : une cible nommée il y a longtemps recevait encore les ordres.
- Choix : délai réglable, désactivé par défaut (comportement inchangé).
- Écarté : mot « annule » intégré à la bibliothèque (chaque projet peut déjà le déclarer
  avec un ordre global qui appelle `clearTargets()`).
```

## Faire une réconciliation (sur `dev`)

« Lire les entrées et mettre la doc à jour » est facile à mal faire : recopier une entrée sans la vérifier déplace le problème au lieu de le régler. Une entrée décrit le code au moment où elle a été écrite, parfois des semaines plus tôt, par une branche qui ignorait ce que les autres ont changé depuis.

1. **Lire toutes les entrées, de tous les sous-dossiers, avant d'écrire quoi que ce soit.** Elles se recoupent, et les mises à jour doivent être cohérentes entre elles une fois terminées.
2. **Vérifier chaque affirmation dans le code actuel, ne pas recopier.** Une entrée est un témoignage, pas une source de vérité : lire les fichiers qu'elle cite, chercher les fonctions qu'elle nomme, recompiler et lancer les tests. Corriger au passage ce qui est faux, même si aucune entrée ne le signale.
3. **Mettre `documentation/` à jour sur place** : réécrire ou supprimer les phrases périmées, ne pas ajouter une correction à côté de l'ancienne affirmation. La doc doit se lire comme si elle avait toujours été juste.
4. **Créer les décisions** demandées par les entrées : numéro suivant, fichier `documentation/decisions/Dnnn-….md`, ligne dans le tableau de `06-decisions.md`.
5. **Chercher au-delà de ce que les entrées nomment.** Une évolution laisse souvent des traces dans d'autres fichiers : chercher les anciens noms (fonctions renommées, options supprimées, chemins d'exemples…) dans **tout** `documentation/`, dans [`README.md`](../README.md) à la racine, dans [`third_party/README.md`](../third_party/README.md) et dans les commentaires d'en-tête des exemples.
6. **Ne supprimer les sous-dossiers du logbook qu'une fois tout cela fait et revérifié**, jamais en premier pour « vider la file ».
7. **Terminer par une vérification d'ensemble** : recompiler et relancer les tests (`ctest`), avec et sans Vosk si la doc parle de la partie vocale ; vérifier que les liens entre fichiers de doc mènent bien à des fichiers et sections existants.

Commiter la mise à jour de la doc et la suppression du logbook ensemble ou séparément, selon ce qui se lit le mieux dans l'historique.

## Noms de branche avec des `/`

La branche `feat/xyz` ne devient pas `logbook/feat/xyz/` : ce chemin se confondrait avec une branche `feat/xyz/abc`. Remplacer les `/` par des `-` : `feat/xyz` → `logbook/feat-xyz/`.

## Trouver ici le sous-dossier d'une autre branche est un signal d'alerte

Sur sa branche, on ne doit voir que son propre sous-dossier (plus ce qui est arrivé sur `dev` depuis la création de la branche et n'a pas encore été réconcilié). Une entrée ne doit exister qu'entre son écriture et la réconciliation suivante. Un sous-dossier ancien, d'une branche sans rapport, veut dire soit qu'il n'a pas été réconcilié (la doc est en retard sur lui), soit qu'il l'a été mais que la suppression a été oubliée. Dans les deux cas : ne pas l'ignorer ni le supprimer soi-même, le signaler, et vérifier si `documentation/` en tient compte avant de conclure qu'on peut l'effacer.

## Ce fichier

Il fait partie de la méthode commune : il ne se modifie, comme `documentation/`, que sur `dev`.

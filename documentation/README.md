# Documentation – XR Boy Contrôle Vocal

Bibliothèque C++ de **commande vocale pour applications VR** : elle reconnaît, dans une phrase prononcée, des **acteurs** (ex : « drone 1 ») et des **ordres** (ex : « tourne à gauche »), puis déclenche le code fourni par l'application hôte.

## Sommaire

| Fichier | Contenu |
|---|---|
| [01-contexte.md](01-contexte.md) | Origine du projet, objectifs, contraintes VR, historique Python |
| [02-concepts.md](02-concepts.md) | Termes, types d'acteurs, acteurs, ordres, acteurs visés, règles d'analyse |
| [03-architecture.md](03-architecture.md) | Modules, flux de données audio → texte → commandes → handlers |
| [04-api.md](04-api.md) | Utilisation de l'API C++ avec exemples |
| [05-build-et-integration.md](05-build-et-integration.md) | Compilation, Vosk, intégration Unity / Unreal / casques autonomes |
| [06-decisions.md](06-decisions.md) | Index des décisions techniques (pourquoi C++, pourquoi Vosk…), une par fichier dans [decisions/](decisions/) |
| [07-roadmap.md](07-roadmap.md) | État d'avancement, prochaines étapes, questions ouvertes |
| [08-glossaire.md](08-glossaire.md) | Vocabulaire du projet |

## Faire évoluer cette documentation

Cette documentation décrit le projet **tel qu'il est sur `dev`**. Elle ne se modifie que sur la branche `dev` : sur une branche de travail (`feat/…`, `fix/…`), on écrit à la place une entrée dans le [logbook](../logbook/README.md), reportée ici lors de la réconciliation suivante. Cela évite que deux branches entrent en conflit sur les mêmes fichiers de doc. Conventions générales du projet : [CLAUDE.md](../CLAUDE.md).

## En une phrase

```
noms + ordre                   ──►  action sur chaque acteur visé
"drone un, tourne à gauche"    ──►  Drone::tourner(gauche) sur drone 1
```

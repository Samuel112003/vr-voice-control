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
| [06-decisions.md](06-decisions.md) | Journal des décisions techniques (pourquoi C++, pourquoi Vosk…) |
| [07-roadmap.md](07-roadmap.md) | État d'avancement, prochaines étapes, questions ouvertes |
| [08-glossaire.md](08-glossaire.md) | Vocabulaire du projet |

## En une phrase

```
noms + ordre                   ──►  action sur chaque acteur visé
"drone un, tourne à gauche"    ──►  Drone::tourner(gauche) sur drone 1
```

# 6. Journal des décisions

Une décision par fichier, dans [decisions/](decisions/) : contexte, choix, conséquences. Un fichier par décision plutôt qu'un seul journal : deux décisions prises en parallèle ne se gênent jamais.

| N° | Décision | Statut |
|---|---|---|
| D1 | [Vosk comme moteur de reconnaissance vocale](decisions/D001-vosk.md) | en vigueur |
| D2 | [Restreindre la reconnaissance au vocabulaire défini](decisions/D002-grammaire-restreinte.md) | en vigueur |
| D3 | [Modèle « noms + ordres »](decisions/D003-modele-noms-ordres.md) | remplacée par D7 |
| D4 | [Types d'acteurs](decisions/D004-types-d-acteurs.md) | en vigueur |
| D5 | [Réécriture en C++17](decisions/D005-reecriture-cpp17.md) | en vigueur |
| D6 | [L'audio est fourni par l'application hôte](decisions/D006-audio-fourni-par-l-hote.md) | en vigueur |
| D7 | [Règles du prototype Python : ordres, cibles persistantes, replis](decisions/D007-regles-du-prototype-python.md) | en vigueur |
| D8 | [API par objets plutôt que par identifiants texte](decisions/D008-api-par-objets.md) | en vigueur |
| D9 | [Acteurs en paramètre d'un ordre : types et nombre déclarés](decisions/D009-acteurs-en-parametre.md) | en vigueur |
| D10 | [Pause (non vocale) et veille (terme de réveil)](decisions/D010-pause-et-veille.md) | en vigueur |

## Ajouter une décision

- **Sur une branche de travail** (`feat/…`, `fix/…`) : ne pas créer le fichier ici. Décrire la décision dans une entrée du [logbook](../logbook/README.md) (contexte, choix, conséquences, alternatives écartées). Deux branches ne peuvent ainsi pas prendre le même numéro.
- **À la réconciliation** (sur `dev`) : attribuer le numéro suivant, créer `decisions/Dnnn-titre-court.md` (numéro sur 3 chiffres pour le tri, titre « Dn – … »), et ajouter la ligne dans le tableau ci-dessus.
- Une décision remplacée n'est pas supprimée : son statut devient « remplacée par Dn », et la nouvelle décision explique ce qui change.

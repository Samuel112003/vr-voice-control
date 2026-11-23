# 2. Concepts

Le moteur de reconnaissance renvoie du texte. La bibliothèque y cherche des **noms** (qui doit agir) et des **ordres** (quoi faire), puis appelle les fonctions de l'application.

```
"drone un, tourne à gauche"
 └──┬───┘  └─────┬───────┘
   nom         ordre          ──►  Drone::tourner(gauche) sur drone 1
```

## Termes

Un **terme** est un mot ou un groupe de mots reconnaissable. Il a une ou plusieurs **formes** : la première lui donne son nom, les suivantes sont des variantes (pluriel, conjugaison, nombre en lettres…).

| Terme (nom) | Formes |
|---|---|
| `avance` | « avance », « avancez » |
| `drone 1` | « drone 1 », « drone un » |
| `tourne à` | « tourne à », « tournez à », « tourne », « tournez » |

Un terme est déclaré une fois et réutilisé partout (dans plusieurs ordres, plusieurs types).

> Vosk retranscrit les nombres **en toutes lettres** (« drone un ») : il faut prévoir ces formes. Une forme dont un mot est inconnu du modèle Vosk ne sera jamais entendue (voir `VoskRecognizer::ignoredPhrases()`).

## Types d'acteurs, acteurs, ordres

- Un **type d'acteur** définit les ordres que ses acteurs savent exécuter. Il a son propre terme (« drones ») : le prononcer désigne **tous** ses acteurs.
- Un **acteur** est un élément de l'application, appelé par son terme (« drone un »). Il est relié à l'objet réel qu'il représente (`setData`).
- Un **ordre** est une suite de positions associée à une action. Chaque position est un terme imposé, un **choix** entre plusieurs termes, ou des **acteurs consommés** (voir plus bas) ; les termes choisis et les acteurs consommés sont passés à l'action.
- Un **ordre global** n'appartient à aucun type : il s'adresse à l'application elle-même (« urgence »), et son action ne reçoit pas d'acteur.

```
ActorType "drones" ── ordres : {avance}, {recule}, {tourne à, [gauche | droite]}
   ├── Actor "drone 1"  (« drone 1 », « drone un »)    ──► objet Drone n°1
   ├── Actor "drone 2"  (« drone 2 », « drone deux »)  ──► objet Drone n°2
   └── Actor "drone 3"  …
ActorType "caméras" ── ordres : {avance}, {zoom}
   └── Actor "caméra"                                   ──► objet Camera
Ordres globaux : {urgence}
```

## Acteurs visés

Nommer des acteurs les rend **visés**. Ils le restent d'une phrase à l'autre, jusqu'à ce que d'autres soient nommés (ou que l'application appelle `clearTargets()`). C'est important à l'oral : Vosk coupe les phrases aux pauses, et « drone deux… *(pause)* … tourne à gauche » arrive en deux morceaux.

| Phrase(s) | Acteurs visés | Exécuté |
|---|---|---|
| « drone un avance » | drone 1 | drone 1 avance |
| « drone un drone deux avancez » | drone 1, drone 2 | les deux avancent |
| « drone un avance drone deux recule » | drone 1, puis drone 2 | drone 1 avance, drone 2 recule |
| « drone deux » puis « tourne à gauche » | drone 2 | drone 2 tourne à gauche |
| « les drones décollez » | tous les drones | tous décollent |

## Règles d'analyse

1. Le texte est normalisé : minuscules (accents compris), ponctuation retirée, espaces réduits.
2. À chaque position, on cherche d'abord un **nom** : d'acteur, ou de type (tous ses acteurs). Le plus long l'emporte (« drone un » plutôt que « drone ») ; à longueur égale, l'acteur l'emporte sur le type.
3. Des noms **consécutifs s'additionnent**. Un nom qui suit un ordre **remplace** la liste des acteurs visés. Exception : les noms lus par une position acteurs (« tire sur drone deux ») sont des acteurs consommés, pas des cibles ; seul le dernier d'entre eux, s'il est suivi d'un ordre que son type connaît, en devient le sujet (voir [Ordres qui consomment des acteurs](#ordres-qui-consomment-des-acteurs)).
4. Sinon on cherche un **ordre** ; entre plusieurs correspondances, **la plus longue** l'emporte (« tourne à gauche » plutôt que « tourne »).
5. **Destinataires** d'un ordre, dans cet ordre de priorité :
   1. les **acteurs visés dont le type connaît l'ordre**, et eux seuls. « drone un, caméra, avancez » fait avancer drone 1 et la caméra, pas drone 2 ;
   2. sinon (personne n'est visé, ou aucune cible ne connaît l'ordre), **repli** : l'acteur d'un type qui n'en a qu'un, ou un ordre global. Avec une seule caméra, « zoom » suffit ; « drone un, zoom » fait zoomer la caméra (le drone ne sait pas zoomer) et drone 1 reste visé ;
   3. si **plusieurs replis** sont possibles (une caméra et une lampe uniques qui savent toutes deux « allume »), l'ordre est **ambigu** : rien n'est exécuté, `onUnhandled` est appelé ;
   4. si l'ordre n'est connu que de types à plusieurs acteurs dont aucun n'est visé (« recule » sans avoir nommé de drone), il n'est pas exécuté, et `onUnhandled` est appelé.
6. Un acteur unique **ne concurrence jamais** les cibles : si drone 1 est visé et que la caméra sait aussi avancer, « avance » ne fait avancer que drone 1.
7. Les **mots inconnus sont ignorés** (« euh », « alors »…).

## Ordres qui consomment des acteurs

Un ordre peut prendre d'autres acteurs en paramètre : « drone un tire sur drone deux » → `drone1.tirerSur(drone2)`.

### Déclaration

Comme un paramètre typé en POO, une position **acteurs** déclare :

- les **types consommables** : un ou plusieurs (un drone peut tirer sur un drone ou sur un char). Les acteurs des autres types sont refusés ;
- le **nombre** d'acteurs, sous la forme d'un intervalle `{min, max}` :

| Écriture | Intervalle | Exemple |
|---|---|---|
| `vc::exactly(n)` | {n, n} | « escorte drone deux » : exactement 1 |
| `vc::atLeast(n)` | {n, ∞} | « tire sur … » : au moins 1 |
| `vc::atMost(n)` | {1, n} | « relie … » : jusqu'à 3 |
| `vc::between(a, b)` | {a, b} | de 2 à 4 |

Un ordre **sans** position acteurs en consomme 0 : c'est le cas par défaut, rien à écrire. Une position acteurs en consomme au moins 1 (une position vide n'aurait pas de sens).

```cpp
drone.order({tire, vc::actors({droneType, charType}, vc::atLeast(1))}, vc::method(&Drone::tirerSur));
drone.order({escorte, vc::actors({droneType}, vc::exactly(1))},        vc::method(&Drone::escorter));
```

Ces déclarations sont vérifiées : une position acteurs sans type, en première position, en double, ou avec un nombre invalide (`min` = 0, `max` < `min`) lève `std::invalid_argument`.

### Règles

1. Les acteurs consommés se nomment comme les cibles : noms d'acteurs ou de types, consécutifs, qui s'additionnent. « tire sur drone deux drone trois » consomme 2 acteurs ; « tire sur les drones » consomme tous les drones. Un acteur nommé deux fois ne compte qu'une fois.
   **Exception** : si le **dernier** de ces noms est suivi d'un ordre que son type sait exécuter, il n'est pas consommé : c'est le **sujet** de cet ordre (il devient la cible).

   | Phrase | Résultat |
   |---|---|
   | « drone un tire sur drone trois drone deux tire sur drone un » | drone 1 tire sur drone 3, puis drone 2 tire sur drone 1 |
   | « drone un tire sur drone deux drone trois avance » | drone 1 tire sur drone 2, puis drone 3 avance |
   | « drone un tire sur drone deux avance » | un seul nom : il reste consommé ; drone 1 tire sur drone 2, puis drone 1 avance |
   | « drone un tire sur drone deux char avance » | le char ne sait pas avancer : il reste consommé ; drone 1 avance |

   Seul le dernier nom peut devenir sujet : à l'oral, rien ne sépare la liste des cibles du sujet suivant. Pour donner un ordre à plusieurs acteurs juste après, le dire dans une autre phrase.
2. Tous doivent être d'un **type consommable**, et leur **nombre** doit être dans `{min, max}`. Sinon l'ordre est **refusé en entier** (rien n'est exécuté, même partiellement) et `onUnhandled` est appelé (`InvalidActors`). Les noms lus sont tout de même consommés : ils ne changent pas les acteurs visés.
3. Un acteur **peut se viser lui-même** : « drone un tire sur drone un » fonctionne, et « drone un tire sur les drones » inclut drone 1.
4. Les acteurs consommés **ne deviennent pas des cibles** : la cible reste l'acteur à qui l'on parle, celui qui exécute. Après « drone un tire sur drone deux », « avance » fait avancer drone 1 ; de même pour « drone un tire sur drone deux avance ».
5. Une position acteurs ne peut pas être la **première** d'un ordre : les noms sont cherchés avant les ordres, elle serait toujours prise pour un changement de cible. Un ordre a **au plus une** position acteurs (pour l'instant).
6. Les **destinataires** de l'ordre (acteurs visés, replis) suivent les règles habituelles.
7. **Exécution** : l'action est appelée une fois par acteur exécutant, avec la liste complète des acteurs consommés (`Arguments::actors`). Raccourci `vc::method(&Drone::tirerSur)` avec `tirerSur(const vc::Actor&)` : appelé une fois par acteur consommé.
8. **Aucun nom** à la position acteurs (« drone un tire sur avance ») : l'ordre est refusé (`MissingActors`), la suite de la phrase est analysée normalement.

### Ordre coupé par une pause

Vosk coupe les phrases aux pauses : « drone deux tire sur… *(pause)* … drone trois » arrive en deux phrases. Si une phrase **se termine** là où des acteurs consommés sont attendus, l'ordre est **mis en attente** :

- si la phrase suivante **commence par des noms**, ils complètent l'ordre (drone 2 tire sur drone 3, et drone 2 reste visé) ;
- sinon, ou si elle arrive après le **délai** (5 s par défaut, `parser().setPendingTimeout(...)`), l'ordre est abandonné (`MissingActors`) et la phrase est analysée normalement : ses noms changent alors la cible.

Un délai de 0 désactive l'attente : l'ordre incomplet est refusé tout de suite. Le refus d'un ordre abandonné est signalé à l'arrivée de la phrase suivante.

## Pause et veille (bouton on/off)

L'écoute a trois états :

| État | Audio | Ordres | Pour en sortir |
|---|---|---|---|
| **Active** | reconnu | exécutés | — |
| **Veille** (`softPause`) | reconnu | ignorés, sauf le **terme de réveil** | dire le terme de réveil, ou `resume()` |
| **Pause** (`pause`) | jeté | ignorés, terme de réveil compris | `resume()` uniquement : action **non vocale** (bouton, touche) |

- **Ce qui est fourni et ce qui est à déclarer.** La bibliothèque fournit le **mécanisme** (les trois états, la détection du terme de réveil, l'oubli de la phrase en cours, la sûreté entre threads). Elle n'impose **aucun mot ni aucune touche** : chaque projet déclare ses propres termes de mise en veille et de réveil, et branche ses propres boutons. « pause » peut avoir un autre sens dans un projet, ou le contexte peut rendre d'autres mots plus naturels (« repos » / « au travail »).

  | Côté bibliothèque | Côté application |
  |---|---|
  | `pause()`, `softPause(terme)`, `resume()` | le terme de réveil et ses formes |
  | états et `onListeningChanged` | le terme de mise en veille, et l'ordre global qui appelle `softPause` |
  | détection du terme de réveil, suite de phrase exécutée | le bouton / la touche qui appelle `pause()`, `softPause()` ou `resume()` |
  | oubli de la phrase en cours et de l'ordre en attente | l'affichage de l'état (voyant, icône…) |

- Le **terme de réveil** est un terme comme un autre : il peut avoir plusieurs formes (« reprends », « reprise », « reprends l'écoute »). La plus longue l'emporte. Les mots qui le précèdent sont ignorés ; ceux qui le suivent sont exécutés normalement : « euh reprends drone un avance » réveille l'écoute et fait avancer drone 1.
- Entrer en pause ou en veille **oublie la phrase en cours** (l'audio déjà reçu par le moteur) et **l'ordre en attente**. Les **acteurs visés sont conservés** : après la reprise, « avance » s'adresse au même acteur qu'avant.
- La veille peut être déclenchée **à la voix**, par un ordre global dont l'action appelle `softPause` (« pause »). Les ordres qui suivent dans la même phrase ne sont pas exécutés : « pause drone un avance » met en veille sans faire avancer drone 1. Une action peut aussi appeler `pause()`, mais seule une action non vocale pourra alors reprendre.
- `pause`, `softPause` et `resume` peuvent être appelés depuis **n'importe quel thread** (bouton de l'interface pendant que l'écoute tourne ailleurs).

## Grammaire du moteur

Par défaut, Vosk peut reconnaître un très large vocabulaire, ce qui crée des confusions (« drone » / « drôle »). La bibliothèque fournit toutes les formes connues (`Vocabulary::phrases()`, des expressions comme « drone un ») pour **restreindre le moteur** à ce vocabulaire.

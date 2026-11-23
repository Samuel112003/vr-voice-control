# Dépendances externes

## vosk/

Placer ici la bibliothèque Vosk de la plateforme cible :

```
third_party/vosk/
├── include/vosk_api.h
└── lib/<plateforme>/libvosk.(dll|so|lib)
    (win64 : + libstdc++-6.dll, libgcc_s_seh-1.dll, libwinpthread-1.dll)
```

`<plateforme>` : `win64`, `linux-x86_64` ou `android-<ABI>` (option CMake `VC_VOSK_PLATFORM`).

Téléchargement : https://github.com/alphacep/vosk-api/releases (testé avec `vosk-win64-0.3.45.zip`)

## Modèle

Décompresser un **petit** modèle français dans `models/` à la racine du projet :
`models/vosk-model-small-fr-0.22/` (https://alphacephei.com/vosk/models).
Seuls les petits modèles acceptent une grammaire restreinte.

Puis compiler avec `-DVC_WITH_VOSK=ON` (voir [documentation/05-build-et-integration.md](../documentation/05-build-et-integration.md)).

Les binaires et modèles ne sont pas versionnés (voir `.gitignore`).

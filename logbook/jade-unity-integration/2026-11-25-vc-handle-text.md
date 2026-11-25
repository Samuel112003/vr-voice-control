# API C : `vc_handle_text`

Ajout de `int vc_handle_text(vc_controller* controller, const char* text)` dans
`bindings/c/` : transmet une phrase UTF-8 à `vc::VoiceController::handleText` et retourne
le nombre d'ordres exécutés, ou -1 en cas d'erreur (contrôleur ou texte NULL, exception
interceptée). Ajout compatible : `vc_api_version()` reste à 1.

Testé dans `test_c_api` : NULL rejeté (contrôleur et texte), vocabulaire vide → 0 ordre.

Pas encore déclaré côté C# (`VoiceControlNative.cs`) : il faudra passer la chaîne en
`byte[]` UTF-8 terminé par un zéro (le marshalling par défaut de `string` n'est pas UTF-8).

## Doc à mettre à jour
- `05-build-et-integration.md` § Intégration Unity : la couche C permet d'envoyer du texte
  (`vc_handle_text`, chaîne UTF-8, retour = nombre d'ordres exécutés ou -1).
- `07-roadmap.md` § À faire, ligne « Couche C pour Unity » : envoi de texte fait ; restent la
  file d'ordres (`vc_poll`) et le chargement du vocabulaire.

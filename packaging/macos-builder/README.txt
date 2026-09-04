UWdeVST Drum - GENERATEUR macOS
=================================================

UTILISATION

1. Double-cliquez sur BUILD_MAC.command.
2. Attendez la fin de la compilation.
3. Finder s'ouvre automatiquement sur le ZIP final.

Le lanceur construit uniquement UWdeVST Drum en deux formats :
- application Standalone macOS ;
- plug-in VST3 macOS.

Le binaire produit est Universal : Apple Silicon arm64 + Intel x86_64.

PRE-REQUIS APPLE

Le Mac doit disposer de Xcode ou des Command Line Tools Apple. Si ces outils
sont absents, le lanceur ouvre l'installateur officiel Apple. C'est la seule
installation système que le paquet ne peut pas contourner.

Aucun Homebrew et aucune commande Terminal ne sont nécessaires. Si CMake est
absent, une version portable officielle est téléchargée automatiquement,
vérifiée par SHA-256 et partagée en cache avec les autres paquets UWdeVST.

SORTIE

Le résultat est placé dans output/<date>/ :
- UWdeVST_Drum_<version>_macOS_universal.zip
- SHA256SUMS
- logs/

SIGNATURE

Le lanceur applique une signature ad hoc locale, suffisante pour tester le
build créé sur ce Mac. Une distribution publique signée et notariée nécessite
un certificat Apple Developer et des identifiants de notarisation ; ils ne sont
pas inclus dans ce paquet.

PROVENANCE

Le paquet contient le code source courant, JUCE 8.0.4 au commit indiqué dans
builder.conf, les seuls assets utilisés par ce synthé et une liste SHA-256 de
tous les fichiers. Les sources sont vérifiées avant et après la compilation.

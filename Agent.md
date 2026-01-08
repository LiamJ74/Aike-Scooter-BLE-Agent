# Äike Scooter BLE Agent - Flipper Zero

**Objectif**  
Créer une application .fap pour Flipper Zero permettant de contrôler les scooters Äike (post-faillite 2025) via BLE en exploitant la clé maître fixe (20×0xFF).

**Statut actuel** (janvier 2026)  
- Application fonctionnelle structurellement (Scan, Auth, Commandes).
- Compatible uniquement avec les firmwares personnalisés (Momentum, RogueMaster, Unleashed) exposant l'API BLE Central.

## Rapport de Développement

### Fonctionnalités Implémentées
1.  **Scan BLE** : Filtrage des devices commençant par "AIKE".
2.  **Authentification** : Implémentation complète du handshake SHA1 (Challenge-Response) avec la clé maître `FFFF...`.
3.  **Commandes** : Unlock, Lock, Eco Mode, Battery Hatch.
4.  **Interface** : Menu Flipper standard avec gestion de navigation.

### Problèmes Rencontrés & Solutions
1.  **Conflits de Headers (GAP)** : Le SDK Momentum possède des définitions pour `GapEvent` qui entraient en conflit avec les stubs nécessaires pour la compilation locale.
    *   *Solution* : Utilisation de "Shadow Types" (`AikeGapEvent`) et de callbacks `void*` pour contourner la vérification de type stricte tout en accédant à la mémoire.
2.  **Assets Corrompus** : L'icône téléchargée initialement était corrompue (404).
    *   *Solution* : Génération d'une icône valide binaire via Python.
3.  **Callback Type Safety** : Le passage de pointeurs de chaînes dans les callbacks UI était instable.
    *   *Solution* : Passage par index entier et lookup sécurisé par Mutex.

### État "Stubbed" vs "Active"
Le code est configuré pour compiler sur un environnement standard (via stubs) mais pour s'exécuter sur Momentum. Les appels `ble_gap_scan_start`, `ble_gap_connect`, etc. sont actifs mais dépendent de la présence des symboles au linkage dans le firmware cible.

### Reste à faire (Post-Merge)
- Vérifier sur le matériel réel si l'alignement mémoire de `AikeGapEvent` correspond parfaitement à `GapEvent` du firmware Momentum utilisé.
- Si le firmware change la structure (ex: ajout de champs), les stubs devront être mis à jour.

## Pré-requis matériel & logiciel

- Flipper Zero avec **custom firmware** supportant full BLE stack client :
  - RogueMaster (recommandé)
  - Momentum
  - Xtreme Firmware
  - Unleashed (selon la date)
- uFBT installé sur ton ordinateur (outil officiel de compilation FAP)
- Environnement de dev C (gcc-arm-none-eabi, etc.)
- Connaissance minimale de BLE GATT (services, characteristics, notifications)

## Protocole Äike (récapitulatif ultra-court)

Service principal : custom (pas de 16-bit standard)

Caractéristiques importantes :

| UUID                                   | Type          | Rôle                              | Valeur attendue / action                     |
|----------------------------------------|---------------|-----------------------------------|-----------------------------------------------|
| 00002556-1212-efde-1523-785feabcd123  | READ          | Challenge (20 bytes aléatoires)   | Lire → challenge                              |
| 00002557-1212-efde-1523-785feabcd123  | WRITE         | Response                          | Écrire SHA1(challenge + 20×0xFF)              |
| 0000155f-1212-efde-1523-785feabcd123  | WRITE         | Commandes                         | Écrire 10 bytes (ex: unlock = 00D40001000000000000) |
| 0000155e-1212-efde-1523-785feabcd123  | NOTIFY        | Notifications état                | Optionnel : batterie, lock, eco, etc.         |

Clé maître : `FFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFF` (20 bytes FF)

Commandes les plus utiles (format 10 bytes hex) :

- Déverrouiller : `00 D4 00 01 00 00 00 00 00 00`
- Verrouiller   : `00 D4 00 02 00 00 00 00 00 00`
- Eco ON        : `00 D4 00 03 01 00 00 00 00 00`
- Eco OFF       : `00 D4 00 03 00 00 00 00 00 00`
- Ouvrir trappe batterie : `00 D4 00 04 00 00 00 00 00 00`
- Mode transport ON  : `00 D2 1B 1E 3C 01 00 00 01 00` (exemple connu)

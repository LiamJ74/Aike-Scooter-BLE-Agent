# Äike Scooter BLE Agent - Flipper Zero

**Objectif**  
Créer une application .fap pour Flipper Zero permettant de contrôler les scooters Äike (post-faillite 2025) via BLE en exploitant la clé maître fixe (20×0xFF).

**Statut actuel** (janvier 2026)  
- Pas d'application publique connue  
- Protocole connu et très simple  
- Faisable avec custom firmware + uFBT

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

## Plan de développement (étapes recommandées)

1. **Préparation de l'environnement**
   - Installer uFBT → https://github.com/flipperdevices/flipperzero-ufbt
   - Cloner un template d'application BLE existante (ex: BLE Spam, ou un PoC GATT client)
   - Activer full BLE stack dans le firmware choisi

2. **Structure de l'application**
aike_agent/ ├── application.fam ├── aike_agent.c ├── aike_agent.h ├── views/ │   ├── main_menu.c │   ├── scan_view.c │   ├── auth_view.c │   ├── control_view.c └── assets/ (icônes, etc.)
3. **Étapes techniques clés à implémenter**

| Étape                              | Difficulté | Priorité | Notes / pièges possibles                              |
|------------------------------------|------------|----------|-------------------------------------------------------|
| Scan + filtre nom "AIKE*"          | ★☆☆        | ★★★★★    | Utiliser gap scan avec filtre sur adv data            |
| Connexion GATT                     | ★★☆        | ★★★★★    | gap_connect → attendre status connected               |
| Découverte services/characteristics| ★★☆        | ★★★★     | gatt_client_discover_primary_services                 |
| Lire challenge (UUID 2556)         | ★★☆        | ★★★★★    | gatt_client_read_value_handle                         |
| Calcul SHA1                        | ★★★        | ★★★★★    | Porter une implémentation SHA1 légère (~5-10kB)       |
| Écrire response (UUID 2557)        | ★★☆        | ★★★★★    | gatt_client_write_value_handle                        |
| Écrire commandes (UUID 155f)       | ★☆☆        | ★★★★     | Format fixe 10 bytes                                  |
| Notifications (optionnel)          | ★★★        | ★★☆      | gatt_client_subscribe + callback                      |
| Interface menu (Sub-GHz style)     | ★★☆        | ★★★      | Utiliser view dispatcher + menu / dialog              |
| Gestion des erreurs / timeout      | ★★★        | ★★★      | Très important sur le BLE du Flipper                  |

4. **Ordre de développement recommandé (MVP rapide)**

1. Scan + connexion + nom affiché
2. Authentification (challenge → SHA1 → response)
3. Bouton "Unlock" qui envoie la commande
4. Ajouter Lock / Eco
5. (Bonus) Afficher notifications batterie/lock

## Ressources utiles

- uFBT docs : https://github.com/flipperdevices/flipperzero-ufbt
- Exemples BLE dans firmware officiel : `/applications/plugins/ble*`
- Implémentations SHA1 légères :
- https://github.com/983/SHA1 (très petite)
- https://github.com/openssl/openssl (trop grosse → à découper)
- Discord Flipper Zero (canal #development ou #plugins)
- Forum : https://forum.flipper.net
- GitHub search : "flipper zero ble gatt client"

**Avertissement légal**  
Utiliser uniquement sur ton propre scooter.  
L'exploitation de cette faille sur un véhicule qui ne t'appartient pas peut être considérée comme une infraction pénale selon la législation de ton pays.

Bonne chance et bon dev ! 🛴🔧

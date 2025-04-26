# Contrôle d'un Moteur par Relais avec Mode Manuel et Automatique

## Description

Ce projet Arduino permet de contrôler un moteur 230V via deux relais (montée et descente), en utilisant :

- Des **boutons** pour le contrôle manuel
- Un **mode automatique** pour déplacer une distance prédéfinie
- Un **encodeur** pour mesurer précisément le déplacement
- Un **capteur de fin de course** pour détecter la position zéro

---

## Matériel nécessaire

- Arduino (Uno, Nano, etc.)
- 2 relais pour commander le moteur
- 1 encodeur rotatif (quadrature)
- 3 boutons-poussoirs :
  - Monter
  - Descendre
  - Activer le mode Manuel
- 1 capteur de fin de course (bas)
- Résistances de pull-up (ou utiliser `INPUT_PULLUP`)
- Câblage pour connexions

---

## Schéma des pins

| Composant        | Pin Arduino |
| ---------------- | ----------- |
| Relais Monter     | 4           |
| Relais Descendre  | 5           |
| Bouton Monter     | 6           |
| Bouton Descendre  | 7           |
| Capteur de fin de course (bas) | 8 |
| Bouton Mode Manuel | 9           |
| Encodeur A        | 2 (interruption) |
| Encodeur B        | 3           |

---

## Fonctionnalités

- **Mode Manuel** :  
  Appuyez sur le bouton "Mode Manuel" + "Monter" ou "Descendre" pour contrôler directement le moteur.

- **Mode Automatique** :  
  Sans appuyer sur "Mode Manuel", une pression sur "Monter" ou "Descendre" déplace automatiquement d'une distance prédéfinie (`50 mm` par défaut).

- **Gestion de la position** :  
  L'encodeur mesure le déplacement en impulsions. Le système utilise un ratio de `100 impulsions/mm`.

- **Détection du point zéro** :  
  Si le capteur de fin de course est activé, la position est réinitialisée à 0.

- **Protection au changement de direction** :  
  Un délai de 1 seconde est imposé pour éviter d'abîmer le moteur.

---

## Paramètres modifiables

```cpp
const int IMPULSIONS_PAR_MM = 100;    // Nombre d'impulsions pour 1 mm
const int AUTO_DISTANCE_MM = 50;      // Distance automatique à parcourir en mm
const unsigned long directionDelay = 1000; // Délai de sécurité (ms) pour inverser la direction
```

---

## Fonctionnement du code

- `updateEncoder()` : Met à jour la position en fonction des signaux d'encodeur.
- `readDebounced()` : Lit les boutons avec anti-rebonds logiciel.
- `updateButtons()` : Actualise l'état stable des boutons.
- `updateMotorControl()` : Gère les actions moteur en fonction des boutons et modes.
- `applyMotorState()` : Applique la commande moteur avec gestion des relais.

---

## Remarques

- Le moteur s'arrête automatiquement s'il atteint le point zéro en descente.
- Le système privilégie toujours le **mode manuel** s'il est activé.
- Le moteur s'arrête immédiatement si aucune commande n'est active.

---

## Exemple de Serial Monitor

```
Moteur: Arrêt
Position: 500 impulsions (5 mm)
Début mouvement auto: Monter
Moteur: Monter
...
Mouvement auto terminé.
Moteur: Arrêt
```

---

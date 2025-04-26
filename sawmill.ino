// Définition des pins
#define RELAIS_MONTER     4
#define RELAIS_DESCENDRE  5
#define BTN_MONTER        6
#define BTN_DESCENDRE     7
#define BTN_MANUEL        9   // Bouton pour activer le mode manuel
#define CAPTEUR_BAS       8
#define ENCODEUR_A        2
#define ENCODEUR_B        3

// Conversion de l'encodeur : 10 mm = 1000 impulsions (donc 1 mm = 100 impulsions)
const int IMPULSIONS_PAR_MM = 100;

// Délais pour le debounce (en ms)
const unsigned long debounceDelay = 50;

// Distance en mm à parcourir en mode automatique (exemple : 50 mm)
const int AUTO_DISTANCE_MM = 50;

// Variables globales pour l'encodeur
volatile long position = 0;  // Position en impulsions
int lastCLK = HIGH;

// Variables pour la gestion du debounce des boutons
int stableButtonManuelState = HIGH;
int stableButtonMonterState = HIGH;
int stableButtonDescendreState = HIGH;

int lastButtonManuelReading = HIGH;
int lastButtonMonterReading = HIGH;
int lastButtonDescendreReading = HIGH;

unsigned long lastDebounceTimeManuel = 0;
unsigned long lastDebounceTimeMonter = 0;
unsigned long lastDebounceTimeDescendre = 0;

// Variables pour le contrôle du moteur
// motorDirection: 1 = monter, -1 = descendre, 0 = arrêt
int motorDirection = 0;
unsigned long lastDirectionSwitchTime = 0;
const unsigned long directionDelay = 1000;  // 1 seconde de délai

// Variables pour le mode automatique
bool autoInProgress = false;
int autoTarget = 0;

// Prototypes de fonctions
void updateEncoder();
int readDebounced(int pin, int &lastStableState, int &lastReading, unsigned long &lastDebounceTime);
void updateButtons();
void updateMotorControl();
void applyMotorState(int newDirection);

void setup() {
  Serial.begin(115200);

  // Configuration des pins
  pinMode(RELAIS_MONTER, OUTPUT);
  pinMode(RELAIS_DESCENDRE, OUTPUT);
  pinMode(BTN_MONTER, INPUT_PULLUP);
  pinMode(BTN_DESCENDRE, INPUT_PULLUP);
  pinMode(BTN_MANUEL, INPUT_PULLUP);
  pinMode(CAPTEUR_BAS, INPUT_PULLUP);
  pinMode(ENCODEUR_A, INPUT_PULLUP);
  pinMode(ENCODEUR_B, INPUT_PULLUP);

  // Arrêt initial du moteur
  digitalWrite(RELAIS_MONTER, HIGH);
  digitalWrite(RELAIS_DESCENDRE, HIGH);
  Serial.println("Moteur: Arrêt");
  
  lastCLK = digitalRead(ENCODEUR_A);
}

void loop() {
  updateEncoder();
  updateButtons();
  updateMotorControl();
  
  // Réinitialisation de la position si le capteur de fin de course bas est activé
  if (digitalRead(CAPTEUR_BAS) == LOW) {
    position = 0;
    Serial.println("Point zéro atteint. Position réinitialisée à 0");
    autoInProgress = false;  // Annule tout mouvement auto
  }
}

// --- Fonction de lecture de l'encodeur ---
void updateEncoder() {
  int currentCLK = digitalRead(ENCODEUR_A);
  if (currentCLK != lastCLK) {
    // Détermination du sens via la lecture de ENCODEUR_B
    if (digitalRead(ENCODEUR_B) == currentCLK) {
      position--;  // Descente
    } else {
      position++;  // Montée
    }
    Serial.print("Position: ");
    Serial.print(position);
    Serial.print(" impulsions (");
    Serial.print(position / IMPULSIONS_PAR_MM);
    Serial.println(" mm)");
  }
  lastCLK = currentCLK;
}

// --- Fonction de lecture avec debounce ---
int readDebounced(int pin, int &lastStableState, int &lastReading, unsigned long &lastDebounceTime) {
  int reading = digitalRead(pin);
  if (reading != lastReading) {
    lastDebounceTime = millis();
  }
  if ((millis() - lastDebounceTime) > debounceDelay) {
    if (reading != lastStableState) {
      lastStableState = reading;
    }
  }
  lastReading = reading;
  return lastStableState;
}

// --- Mise à jour des états des boutons ---
void updateButtons() {
  stableButtonManuelState = readDebounced(BTN_MANUEL, stableButtonManuelState, lastButtonManuelReading, lastDebounceTimeManuel);
  stableButtonMonterState = readDebounced(BTN_MONTER, stableButtonMonterState, lastButtonMonterReading, lastDebounceTimeMonter);
  stableButtonDescendreState = readDebounced(BTN_DESCENDRE, stableButtonDescendreState, lastButtonDescendreReading, lastDebounceTimeDescendre);
}

// --- Mise à jour du contrôle moteur en fonction du mode ---
void updateMotorControl() {
  // Mode Manuel : le bouton manuel est pressé (état LOW)
  if (stableButtonManuelState == LOW) {
    // Annule le mode automatique s'il était actif
    autoInProgress = false;
    int desiredDirection = 0;
    if (stableButtonMonterState == LOW && stableButtonDescendreState == LOW) {
      desiredDirection = 0;
    }
    else if (stableButtonMonterState == LOW) {
      desiredDirection = 1;
    }
    else if (stableButtonDescendreState == LOW) {
      desiredDirection = -1;
    }
    else {
      desiredDirection = 0;
    }
    applyMotorState(desiredDirection);
  }
  else {
    // Mode Automatique
    // Si aucun mouvement automatique n'est en cours, détecte une commande
    if (!autoInProgress) {
      if (stableButtonMonterState == LOW) {
        autoTarget = position + AUTO_DISTANCE_MM * IMPULSIONS_PAR_MM;
        autoInProgress = true;
        Serial.println("Début mouvement auto: Monter");
      }
      else if (stableButtonDescendreState == LOW && position > 0) {
        autoTarget = position - AUTO_DISTANCE_MM * IMPULSIONS_PAR_MM;
        autoInProgress = true;
        Serial.println("Début mouvement auto: Descendre");
      }
      else {
        applyMotorState(0);
      }
    }
    // Si un mouvement automatique est en cours, poursuivre jusqu'à atteindre la cible
    if (autoInProgress) {
      int desiredDirection = 0;
      if (position < autoTarget) {
        desiredDirection = 1;
      }
      else if (position > autoTarget) {
        desiredDirection = -1;
      }
      else {
        desiredDirection = 0;
        autoInProgress = false;
        Serial.println("Mouvement auto terminé.");
      }
      applyMotorState(desiredDirection);
    }
  }
}

// --- Application de l'état du moteur avec délai de sécurité ---
void applyMotorState(int newDirection) {
  // Vérifie le délai avant changement de direction
  if (newDirection != motorDirection && motorDirection != 0) {
    if (millis() - lastDirectionSwitchTime < directionDelay) {
      newDirection = 0;  // On attend
    } else {
      lastDirectionSwitchTime = millis();
    }
  } else if (newDirection != motorDirection && motorDirection == 0) {
    lastDirectionSwitchTime = millis();
  }
  
  if (newDirection != motorDirection) {
    motorDirection = newDirection;
    switch(motorDirection) {
      case 1:
        digitalWrite(RELAIS_MONTER, LOW);
        digitalWrite(RELAIS_DESCENDRE, HIGH);
        Serial.println("Moteur: Monter");
        break;
      case -1:
        if (position > 0) {
          digitalWrite(RELAIS_MONTER, HIGH);
          digitalWrite(RELAIS_DESCENDRE, LOW);
          Serial.println("Moteur: Descendre");
        } else {
          digitalWrite(RELAIS_MONTER, HIGH);
          digitalWrite(RELAIS_DESCENDRE, HIGH);
          motorDirection = 0;
          Serial.println("Moteur: Arrêt (point zéro atteint)");
        }
        break;
      default:
        digitalWrite(RELAIS_MONTER, HIGH);
        digitalWrite(RELAIS_DESCENDRE, HIGH);
        Serial.println("Moteur: Arrêt");
        break;
    }
  }
}

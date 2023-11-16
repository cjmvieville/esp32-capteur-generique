/**
 * Capteur Générique avec une carte Arduino
 */
#include "capteur_generique_esp_now.h"

#define BOARD_ID 2 // Mettre l'identidiant du capteur ici

int getBoardId(){
  return BOARD_ID;
}
/** Fonction setup() */
void setup() {
  /* Initialise le port série */
  Serial.begin(115200);
  /* Initialise le réseau ESP-NOW pour échanger avec le serveur */
  setup_ESP_NOW();
}

/** Fonction loop() */
void loop() {
  if (autoPairing() == PAIR_PAIRED) {
    Serial.println("Serveur ESP-NOW  trouvé : on envoie un message");
    // prépare le message à envoyer puis y ajoute toutes les mesures recceuillies par le capteur
    prepareMessage();
    addMesure(1.0,"un",0);
    addMesure(2.0,"deux",1);
    addMesure(3.0,"trois",2);
    addMesure(4.0,"quatre",3);
    addMesure(5.0,"cinq",4);
    addMesure(6.0,"six",5);
    addMesure(7.0,"sept",6);
    addMesure(8.0,"huit",7);
    addMesure(9.0,"neuf",8);
    addMesure(10.0,"dix",9); 
    // Envoi le message via ESP-NOW
    sendMessage();
    delay(1000*60); // un envoi toutes les minutes
  } 
  
}
 

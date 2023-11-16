#ifndef capteur_generique_esp_now
#define capteur_generique_esp_now


#include <esp_now.h>
#include <esp_wifi.h>
#include <WiFi.h>
#include "main.h"

#define MAX_CHANNEL 13  // 13 en  Europe

const int maxStringLen=20; // longueur maximale de chaque chaine de caractères
const int maxValue = 10; // defini le nombre maximun de valeurs transmises par un capteur
// La structure d'un message de données ESP-NOW
// DOIT ABSOLUMENT CORRESPONDRE A CELLE DU SERVEUR
// taille maximale = 250
typedef struct struct_message {
  uint8_t msgType; // position 0 -> 1 octet à la valeur de 1
  uint8_t id; // position 1 ->1 octet (valeur est celle de l'id du capteur)
  // avec le langage il y a ici 2 octets non utilisés !!
  unsigned int seqNum; // position 4 ->4 octets ( non utilisé dans la version actuelle)
  float valeurs[10]; // position 8 -> taille = 10 * 4 = 40 octets 
  // il reste 250 -48 soit 202 caractères soit 20 caractères max par stringValue envoyée et on perd 2 bytes 
  char stringValues[maxValue][maxStringLen] ; // position 48 -> 1O fois 20 bytes
  // fin position 248
} struct_message;


// La structure d'un message d'appairage ESP-NOW
typedef struct struct_pairing {
    uint8_t msgType;
    uint8_t id;
    uint8_t macAddr[6];
    uint8_t channel;
} struct_pairing;


enum MessageType {PAIRING, DATA,};
enum PairingStatus {NOT_PAIRED, PAIR_REQUEST, PAIR_REQUESTED, PAIR_PAIRED,};


bool setup_ESP_NOW(); // initialise le protocole de communication ESP-NOW de la carte 
void prepareMessage();
void addMesure(float value, char * stringValue, int index);
void printMessage();
void sendMessage();
void testMessage();
PairingStatus autoPairing();

#endif

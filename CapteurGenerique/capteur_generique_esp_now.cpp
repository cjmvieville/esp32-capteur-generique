#include "capteur_generique_esp_now.h"
/*
 * Ce module permet d'envoyer vers un sereur ESP-NOW 10 (maxValue défini dans le .h)valeurs numériques différentes 
 * associées à 10 chaines de caractères de longueur maximale de 20 caractères (maxStringLen défini dans le .h)
 * 
 * Le module daoit être initialisé par l'appel à la fonction setup_ESP_NOW() (normalment dans le "setup()" 
 * 
 * Puis pour chaque envoi de mesures, il faut dans l'ordre
 *  Préparer le message 

*/
struct_message myData;  // le message contenant  les mesures qui sera envoyé vers le serveur

struct_pairing pairingData; // message d'appairage

PairingStatus pairingStatus = NOT_PAIRED; 

uint8_t serverAddress[] = {0xFF,0xFF,0xFF,0xFF,0xFF,0xFF}; // adresse de diffusiion vers tous les récepteurs ESP-NOW

MessageType messageType;

esp_now_peer_info_t peerInfo; // les données d'appairage

int channel = 1; // Le canal utilisé pour échanger (peut varier de 1 à 13 en Europe)
unsigned int seqNum = 0; // numéro de séquence qui numérote séquentiellement chaque message ; n'est pas exploité par le serveur dans sa version actuelle

unsigned long currentMillis = millis(); // Référence de temps du capteur
unsigned long previousMillis = 0;   // Spour stocker le moment où la demande d'appairage a été émise

// Fonction pour préparer la demande le message contenant les mesures du capteur
void prepareMessage(){
  for(int i=0; i<10; i++){
    myData.msgType=DATA;
    myData.id=getBoardId();
    myData.seqNum = seqNum++; // même s'il n'est pas exploité par le serveur, il est présente dans le message
    for(int i=0; i<maxValue; i++) {
      addMesure(0.0, "0                    ", i); // les dix mesures sont mises à zéro
    }
  }
}
/* Ajoute une mesure au message ESP-NOW
 *  On doit préciser en arguments :
 *    La valeur numérique (value)
 *    Le txte associé (stringValue)
 *    L'index de la valeur (de 0 à maxValue-1)
 */
void addMesure(float value, char * stringValue, int index){
  if(index>=maxValue) return;
  myData.valeurs[index]=value;
  memcpy(&myData.stringValues[index],stringValue,maxStringLen);
}
/*
 * Pour la mise au point du module
 * Cette fonctionne envoie sur la liaison série le contenu du message
 */
void printMessage(){
  char *s = (char *)&myData;
  int i=0;
  for(int l=0; l<25; l++){
    Serial.print(String(i)); Serial.print(" : "); 
    for(int j=0; j<10; j++){
      Serial.print(String(s[i++],HEX)); Serial.print(" "); 
    }
    Serial.println();
  }
}
/*
 * Envoie du message vers le serveur 
 * 
 */
void sendMessage(){
  esp_err_t result = esp_now_send(pairingData.macAddr, (uint8_t *) &myData, sizeof(myData));
    if (result == ESP_OK) {
      Serial.println("Envoi correct du message");
    } else {
      Serial.println("Echec de l'envoi du message");
    }
}
/*
 * Séquence complète pour préparer, ajouter des mesures au message avant dl'envoyer au serveur ESP-NOW
 */
void testMessage(){
    prepareMessage();
    printMessage();
    addMesure(1.0,"un..................",0);
    addMesure(2.0,"deux................",1);
    addMesure(3.0,"trois...............",2);
    addMesure(4.0,"quatre..............",3);
    addMesure(5.0,"cinq................",4);
    addMesure(6.0,"six.................",5);
    addMesure(7.0,"sept................",6);
    addMesure(8.0,"huit................",7);
    addMesure(9.0,"neuf................",8);
    addMesure(10.0,"dix..................",9);
    printMessage();
    sendMessage();
}
/*
 * Envoie l'adresse mac passée en argument sur la liaison série
 */
void _printMAC(const uint8_t * mac_addr){
  char macStr[18];
  snprintf(macStr, sizeof(macStr), "%02x:%02x:%02x:%02x:%02x:%02x",
           mac_addr[0], mac_addr[1], mac_addr[2], mac_addr[3], mac_addr[4], mac_addr[5]);
  Serial.print(macStr);
}

/* 
 *  callback appelé à la fin d'un envoi de message
 *  Envoi l'état de l'envoi sur la liaison série
 *  Arguments : 
 *    mac_addr : l'adresse mac du serveur
 *    status : le status de cet envoi
 */
void _onDataSent(const uint8_t *mac_addr, esp_now_send_status_t status) {
  Serial.print("Etat du dernier envoi : ");
  Serial.print(status == ESP_NOW_SEND_SUCCESS ? "Succès vers : " : "Echec vers : ");
  _printMAC(mac_addr);
  Serial.println();
}
/*
 * Fonction interne au module qui mémorise les données de l'appairage 
 * Arguments : 
 *  mac_addr : adresse mac du serveur ESP-NOW
 *  chan : le numéro de canal utilisé par ESP-NOW
 */
void _addPeer(const uint8_t * mac_addr, uint8_t chan){
  esp_now_peer_info_t peer;
  ESP_ERROR_CHECK(esp_wifi_set_channel(chan ,WIFI_SECOND_CHAN_NONE));
  esp_now_del_peer(mac_addr);
  memset(&peer, 0, sizeof(esp_now_peer_info_t));
  peer.channel = chan;
  peer.encrypt = false;
  memcpy(peer.peer_addr, mac_addr, sizeof(uint8_t[6]));
  if (esp_now_add_peer(&peer) != ESP_OK){
    Serial.println("Echec d'appairage");
    return;
  }
  memcpy(serverAddress, mac_addr, sizeof(uint8_t[6]));
}
/*
 * Callback appelé à la réception d'un message ESP-NOW
 * Si c'est un message deonnées : on ne fait rien !
 * Si c'est une réponse à la demande d'appairage, les données d'"appairage sont mises à jour
 */
void _onDataRecv(const uint8_t * mac_addr, const uint8_t *incomingData, int len) { 
  uint8_t type = incomingData[0];
  switch (type) {
  case DATA :     
    break;
  case PAIRING:    
    memcpy(&pairingData, incomingData, sizeof(pairingData));
    if (pairingData.id == 0) {              // the message comes from server
      _addPeer(pairingData.macAddr, pairingData.channel); // add the server  to the peer list 
      pairingStatus = PAIR_PAIRED;             // set the pairing status
    }
    break;
  }  
}
/*
 * Fonction principale qui gère l'auto appairage
 * Si l'appiarge est en place: on ne fait rien
 * Si on doit faire l'appairage (PAIR_REQUEST): un message de demande d'appairage est lancé sur le réseau
 * Si on a déjà lancé un message de demande d'appairage : si le délai d'attente (fixé à 250 ms) est dépassé, alors on indique 
 * qu'il faut envoyer un nouveau message de demande d'appairage sur le canal suivant
 * Les caanux de sont balayés de 1 à 13 (MAX_CHANNEL)
 */
PairingStatus autoPairing(){
  switch(pairingStatus) {
    case PAIR_REQUEST:
      Serial.print("Pairing request on channel "  ); Serial.println(channel);  
      ESP_ERROR_CHECK(esp_wifi_set_channel(channel,  WIFI_SECOND_CHAN_NONE));
      if (esp_now_init() != ESP_OK) {
        Serial.println("ESP-NOW ne se lance pas correctement");
      }  
      // set callback routines
      esp_now_register_send_cb(_onDataSent);
      esp_now_register_recv_cb(_onDataRecv);
      // set pairing data to send to the server
      pairingData.msgType = PAIRING;
      pairingData.id = getBoardId();     
      pairingData.channel = channel;
  
      // add peer and send request
      _addPeer(serverAddress, channel);
      esp_now_send(serverAddress, (uint8_t *) &pairingData, sizeof(pairingData));
      previousMillis = millis();
      pairingStatus = PAIR_REQUESTED;
      break;

    case PAIR_REQUESTED:
      // gestion du temps pour recevoir une réponse du serveur
      currentMillis = millis();
      if(currentMillis - previousMillis > 250) {
        previousMillis = currentMillis;
        // time out -> on passe au canal suivant
        channel ++;
        if (channel > MAX_CHANNEL){
           channel = 1;
        }   
        pairingStatus = PAIR_REQUEST;
      }
      break;
    case PAIR_PAIRED:
      // plus rien à faire l'appairage est réalisé
    break;
  }
  return pairingStatus;
}  

/*
 * Fonction d'initialisatuion du module ESP-NOW
 * Est appelé dans la fonction setup()
 */

bool setup_ESP_NOW(){
   Serial.print("Client Board MAC Address:  "); Serial.println(WiFi.macAddress());
   WiFi.mode(WIFI_STA);
   WiFi.disconnect();
   pairingStatus = PAIR_REQUEST;
    // Init ESP-NOW
    if (esp_now_init() != ESP_OK) {
      Serial.println("ESP-NOW ne se lance pas correctement");
      return false;
    }
    esp_now_register_send_cb(_onDataSent);
    return true;
} 

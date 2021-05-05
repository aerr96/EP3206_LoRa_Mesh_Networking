/*Proyecto de Grado de Ingenieria Electronica
            Red Mesh con LoRa
        Universidad Simon Bolivar 
        Alejandro Rivas 13-11208                
       Tester Movidlidad LoRa Mesh           */

#include "SSD1306.h" 
#include <RHRouter.h>
#include <RHMesh.h>
#include <RH_RF95.h>
#include <SPI.h>

// Variables para conexion LoRa ESP32 por SPI
                     // TTGO   -- SX1278
#define SCK     5    // GPIO5  -- SCLK
#define MISO    19   // GPIO19 -- MISO
#define MOSI    27   // GPIO27 -- MOSI
#define SS      18   // GPIO18 -- CS
#define RST     23   // GPIO23 -- RESET 
#define DI0     26   // GPIO26 -- IRQ(Interrupt Request)

#define GREENLED 25  //Pin del LED verde del ESP32

//Variables Globales Mesh:
#define NODES 4 //Numero total de nodos en la red 

// Direcciones de los Nodos:
#define CLIENT_ADDRESS 1  //4 para topologia Y
#define SERVER1_ADDRESS 2
#define SERVER2_ADDRESS 3
#define SERVER3_ADDRESS 4 //1 para topologia Y

uint8_t routes[NODES];               // Arreglo de direcciones de siguiente salto para determinados destinos
int16_t rssi[NODES];                 // Arreglo de mediciones de rssi desde el nodo enrutado mas cercano
uint8_t buf[RH_MESH_MAX_MESSAGE_LEN];// Buffer para almacenar mensajes:

//Mensajes posibles:
uint8_t data_c[] = "Hello World!";
uint8_t data[] = "Hi back from 0x2";
//Para programar servidores o cliente:
bool client = true;
//Para seleccionar nodo a programar o ajustar una secuencia de envio:
int PROGRAMING_ADDRESS;
void getNodeSelection(int i){
    switch(i) {
    case 1: 
    PROGRAMING_ADDRESS=SERVER1_ADDRESS;
    break;
    case 2: 
    PROGRAMING_ADDRESS=SERVER2_ADDRESS;
    break;
    case 3:
    PROGRAMING_ADDRESS=SERVER3_ADDRESS;
    break;
  }}

//Instancia del la clase RH_RF95 como driver de LoRa 
RH_RF95 rf95(SS,DI0);

//Clase para manejar el envio y la recepcion de paquetes, 
//utilizando el driver declarado anteriormente
RHMesh *manager;

//Inicializamos la pantalla OLED y algunos strings por default
SSD1306 display(0x3c, 21, 22);
String status = "";
String gotmsg = "";
String RoutingTable = "| 0x0| 0x0| 0x0| 0x0|";

//Funciones para representar los datos de los arreglos en la pantalla OLED:
String getRssiString(){
if(client){
    String var= "RSSI: | ";
    for(uint8_t i=0;i<NODES;i++){
        if (rssi[i]==0)
        {
        var+="null";
        var += " | ";
        }else{
            var+=rssi[i];
            var += " | ";}
    }
    return var;
}else{
    String var= "From Router RSSI: ";
    for(uint8_t i=0;i<NODES;i++){
        if (rssi[i]!=0)
        {
        var+=rssi[i];
        }
    }
    return var;
}
}
String getRoutesString(){
    String var = "NHop: | ";
    for(int i=0;i<NODES;i++) {  
        if(routes[i]==255){
            var += 255;
            var += "| ";
        }else{
            var += "0x";
            var += routes[i];
            var += "| ";
        }
    }
    return var;
}
//Funcion de impresion OLED//
void OLEDprint(String sts, String msg){
    String RSSI=getRssiString();
    String ROUTES=getRoutesString();
    display.clear();
    display.setTextAlignment(TEXT_ALIGN_LEFT);
    display.setFont(ArialMT_Plain_10);
    display.drawString(0 , 0 , "Status: "+ sts);
    display.drawString(0 , 12 , "Msg: "+ msg);
    display.drawString(0 , 24 , "toNode:|''1'' | ''2'' | ''3'' | ''4''|");
    display.drawString(0 , 36 , ROUTES);
    display.drawString(0 , 48 , RSSI);
    display.display();
    Serial.println(sts);
}
//Funcion que actualiza las rutas de salto del arreglo de rutas:
void refreshRoutingTable(){
    for(uint8_t node=1;node<=NODES;node++){
        RHRouter::RoutingTableEntry *ruta = manager->getRouteTo(node);
        //Cuando el nodo entregado como destino es el mismo que esta solicitando
        //getRouteTo(), este devueve un NULL que debe ser manejado 
        //de la siguiente manera:
        if(ruta==NULL){
            routes[node-1]=255;
            continue;
        }
        //Guardamos en el arreglo de rutas las respectivas direcciones del siguiente salto
        else{
            routes[node-1] = ruta->next_hop;
            if(routes[node-1]==0){
                rssi[node-1] = 0;
            }
        }
    } 
}
// Funcion para identificar los errores en los fallos de envio
const __FlashStringHelper* getErrorString(uint8_t error) {
  switch(error) {
    case 1: return F("Tamano de paquete Invalido");
    break;
    case 2: return F("No hay ruta");
    break;
    case 3: return F("Superado el Tiempo limite para ACK");
    break;
    case 4: return F("Sin Respuesta");
    break;
    case 5: return F("No disponible para entregar paquete");
    break;
  }
  return F("Desconocido");
}

void setup()
{   
    // Inicializamos la comunicacion serial 
    Serial.begin(115200);
    //Iniciamos el nodo tipo Mesh con su direccion especifica
    manager = new RHMesh(rf95, CLIENT_ADDRESS);
    //manager = new RHMesh(rf95, SERVER1_ADDRESS);
    //manager = new RHMesh(rf95, SERVER2_ADDRESS);
    //manager = new RHMesh(rf95, SERVER3_ADDRESS);
    pinMode(GREENLED,OUTPUT); //Led para evento exitoso de recepcion o transmision de paquete 
    pinMode(16,OUTPUT); 
    digitalWrite(16, LOW);    // Pone el GPIO16 low para resetear el OLED
    delay(50); 
    digitalWrite(16, HIGH);   // Mientras el OLED esta encendido, se debe poner el GPIO16 en high
    SPI.begin(SCK,MISO,MOSI,SS);
        if (!manager->init()) {
        status="init Mesh fallido";
        Serial.println(status);
    } else {
        status="init Mesh exitoso";
        Serial.println(status);
//  Parametros LoRa opcionalmente ajustables
    uint8_t sf= 9;
    rf95.setSpreadingFactor(sf);
    //long bw= 125000;
    //rf95.setSignalBandwidth(bw);
    //rf95.setTxPower(20, false);
    //rf95.setFrequency(434);
    //rf95.setCADTimeout(500);
    uint8_t reintentos=1;
    manager->setRetries(reintentos);
    status = "RF95 listo";
    // Para Factores de propagacion altos la transmision es mas lenta
    // por ende hay que aumentar el timeout del acknowledge
    if(sf==10){
    uint16_t timeout =5000;
    manager->setTimeout(timeout);
    }
    if(sf==11||sf==12){
    uint16_t timeout =20000;
    manager->setTimeout(timeout);
    }
    //Inicializamos la pantalla OLED
    display.init();
    display.flipScreenVertically();  
    display.setFont(ArialMT_Plain_10);
    //Arrays de Impresion de rutas y RSSI inicializados con 0
    for(uint8_t node=0;node<=NODES;node++) {
        routes[node] = 0;
        rssi[node] = 0;
    }
    OLEDprint(status,gotmsg);
    delay(1000);
    }
}

void loop()
{   
    if(client){
        for(uint8_t node=1;node<NODES;node++){
        getNodeSelection(node);
        status="Sending to servers";
        OLEDprint(status,gotmsg);  
        // Enviar un mensaje a un server rf95_mesh
        // La ruta hacia la direccion destino sera automaticamente descubierta
        uint8_t error = manager->sendtoWait(data_c, sizeof(data_c), PROGRAMING_ADDRESS);
        if (error == RH_ROUTER_ERROR_NONE)             
        {
                 Serial.println("paquete enviado");
                // Fue entregado con confirmacion el paquete
                // Y Ahora esperamos la respuesta del server
                uint8_t len = sizeof(buf);
                uint8_t from;    
                if (manager->recvfromAckTimeout(buf, &len, 4000, &from))
                    {   
                        digitalWrite(GREENLED, HIGH);  // Encendemos el LED 
                        delay(50);                    // Esperamos 
                        digitalWrite(GREENLED, LOW);
                        status="Receiving";
                        String gotmsg = (char*)buf;
                        OLEDprint(status,gotmsg);
                        Serial.print("got reply from: 0x");
                        Serial.print(from, HEX);
                        Serial.print(": ");
                        Serial.println((char*)buf);
                        rssi[PROGRAMING_ADDRESS-1] = rf95.lastRssi();
                    }
                    else
                        {
                            status="No reply";
                            OLEDprint(status,gotmsg);
                        }
            }
            else{
                Serial.println("envio fallido error:");
                Serial.println(getErrorString(error));
                status="sendtoWait failed";
                OLEDprint(status,gotmsg);
                }
    
    }
    //manager->printRoutingTable();
    refreshRoutingTable();
    RoutingTable=getRoutesString();
    //Serial.println(RoutingTable);
    }else{
        uint8_t len = sizeof(buf);
        uint8_t from;
        //Recepcion continua o descubrimiento de rutas, en espera de mensaje del cliente 
        if (manager->recvfromAckTimeout(buf, &len,4000, &from))
        {
        digitalWrite(GREENLED, HIGH);  // Encendemos el LED 
        delay(50);                    // Esperamos 
        digitalWrite(GREENLED, LOW);
        status="Receiving";
        String gotmsg = (char*)buf;
        OLEDprint(status,gotmsg);
        Serial.print("got request from: 0x");
        Serial.print(from, HEX);
        Serial.print(": ");
        Serial.println((char*)buf);
            // Enviar respuesta al cliente de origen
            if (manager->sendtoWait(data, sizeof(data), from) != RH_ROUTER_ERROR_NONE)
                {status="sendtoWait failed";
                OLEDprint(status,gotmsg);
                }else
                {
                    status="Sending reply";
                    OLEDprint(status,gotmsg);
                    rssi[from]= rf95.lastRssi();                 
                }
                
        }else{
                status="Waiting...";
                OLEDprint(status,gotmsg);
        }
    //manager->printRoutingTable();
    refreshRoutingTable();
    RoutingTable=getRoutesString();
    //Serial.println(RoutingTable);
    }
}
/*Proyecto de Grado de Ingenieria Electronica
            Red Mesh con LoRa
        Universidad Simon Bolivar 
        Alejandro Rivas 13-11208                
    Tester de Tiempos de Asociacion Mesh          */

#include "SSD1306.h" 
#include <RHRouter.h>
#include <RHMesh.h>
#include <RH_RF95.h>
#include <SPI.h>
#include <WiFi.h>
#include <NTPClient.h>
#include <WiFiUdp.h>

// Reemplazar con las credenciales de la red Wi-Fi en uso
const char* ssid     = "******";
const char* password = "******";

// Cliente NTP para obtener la hora
WiFiUDP ntpUDP;
NTPClient my_time_client(ntpUDP);

// Variables para conexion LoRa ESP32 por SPI
                     // TTGO   -- SX1278
#define SCK     5    // GPIO5  -- SCLK
#define MISO    19   // GPIO19 -- MISO
#define MOSI    27   // GPIO27 -- MOSI
#define SS      18   // GPIO18 -- CS
#define RST     23   // GPIO23 -- RESET 
#define DI0     26   // GPIO26 -- IRQ(Interrupt Request)

//Variables Globales Mesh:
#define NODES 4 //Numero total de nodos en la red 

// Direcciones de los Nodos:
#define CLIENT_ADDRESS 1 //4 para topologia Y (RH_TEST_NETWORK 3)
#define SERVER1_ADDRESS 2
#define SERVER2_ADDRESS 3
#define SERVER3_ADDRESS 4 //1 para topologia Y (RH_TEST_NETWORK 3)
 
uint8_t routes[NODES];               // Arreglo de direcciones de siguiente salto para determinados destinos
int16_t rssi[NODES];                 // Arreglo de mediciones de rssi desde el nodo enrutado mas cercano
uint8_t buf[RH_MESH_MAX_MESSAGE_LEN];// Buffer para almacenar mensajes:

//Mensajes posibles:
uint8_t data_c[] = "Hi";// World!";
uint8_t data[] = "Hi back from server1";
//Para programar servidores (false) o cliente (true):
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

//Instancia de la clase RH_RF95 como driver de LoRa 
RH_RF95 rf95(SS,DI0);

//Clase para manejar el envio y la recepcion de paquetes, 
//utilizando el driver  rf95 
RHMesh *manager;

//Intancia de conexion con la pantalla OLED y algunos strings por default
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
    //Serial.println(sts);
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
//Funcion encargada de sincronizar con el cliente NTP para obtener los milisegundos de la hora 
inline void sync_time(){
    my_time_client.begin();
    my_time_client.setTimeOffset(28800);
    my_time_client.setUpdateInterval(20000);
    my_time_client.setPoolServerName("0.ubuntu.pool.ntp.org");
    //my_time_client.setPoolServerName("192.168.1.200");
    //Un timeout mas pequeno brinda mejor precision
    //pero una mayor probabilidade fallar en el ajuste
    my_time_client.setTimeout(800); //800
    Serial.println("syncing...");
    while(my_time_client.update()!=1){
        delay(2000);
        my_time_client.forceUpdate();
    }
    Serial.print("success: ");
    Serial.println(my_time_client.getFormattedTime());
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
    if(client){
    // Nos conectamos a la red Wi-Fi
    Serial.print("Conectando a la red:");
    Serial.println(ssid);
    WiFi.begin(ssid, password);
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }
    // Imprimimos la IP local por serial
    Serial.println("");
    Serial.println("WiFi conectado.");
    Serial.println("IP address: ");
    Serial.println(WiFi.localIP());
    //Sincronizamos e inicializamos el cliente NTP
    sync_time(); 
    }
    //Iniciamos el nodo tipo Mesh con su direccion especifica
    manager = new RHMesh(rf95, CLIENT_ADDRESS);
    //manager = new RHMesh(rf95, SERVER1_ADDRESS);
    //manager = new RHMesh(rf95, SERVER2_ADDRESS);
    //manager = new RHMesh(rf95, SERVER3_ADDRESS);
    SPI.begin(SCK,MISO,MOSI,SS);// Comunicacion SPI
    if (!manager->init()) {
        status="init Mesh fallido";
        Serial.println(status);
    } else {
        status="init Mesh exitoso";
        Serial.println(status);
//  Parametros LoRa opcionalmente ajustables
    uint8_t sf= 10;
    rf95.setSpreadingFactor(sf);
    //long bw= 125000;
    //rf95.setSignalBandwidth(bw);
    //rf95.setTxPower(20, false);
    //rf95.setFrequency(434);
    //rf95.setCADTimeout(500);
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
// Funcion que dada las horas de un intervalode tiempo, calcula el tiempo en milisegundos entre ambas horas
float totalMilis(float t1,float ts1,float t2,float ts2)
{
    float Result;
    if(ts2!=00)
    {
        if(ts2>ts1)
        {
            Result=1000*(ts2-ts1)-t1+t2;
        }else{
            if(ts1==ts2){
            Result=t2-t1;
            }else{
            Result=1000*(ts2+60-ts1)-t1+t2;
            }
        }
    }else{
        if(ts1==00)
        {
            Result=t2-t1;
        }else{
            Result=1000*(60-ts1)-t1+t2;
        }
    }
    return Result;
}

// Funcion para imprimir resultado de las medidas de tiempo por serial 
String stringTiempos(float st1,float sts1,float st2,float sts2){
    float Dif = totalMilis(st1,sts1,st2,sts2);
    String var="Tiempo inicial: seg(";
    var+=sts1;
    var+=") mili(";
    var+=st1;
    var+=") Tiempo final: seg(";
    var+=sts2;
    var+=") mili(";
    var+=st2;
    var+=").";
    var+=" Intervalo: ";
    var+=Dif;
    return var;
}

// Variables logicas para manejar las primeras corridas de envio
bool firstrun =true;
bool secondrun=false;
bool thirdrun=false;
// Variables para guardar tiempos de interes
float time1;
float times1;
float time2;
float times2;
float time3;
float times3;
float time4;
float times4;
float time5;
float times5;
float time6;
float times6;
float intervalo1;
float intervalo2;
float intervalo3;
float tiempofinal;
void loop()
{  
    if(client){
        //Rutina para programar al Cliente, que en este caso es el nodo que busca
        //ruta para enviar un paquete y al cual le medimos el tiempo de asociacion a la red Mesh
        
        // Manualmente elegimos a cual nodo queremos enviar el paquete
        getNodeSelection(1); // segun las direcciones definidas en la funcion y la topologia escogida 
        status="Sending to servers";
        OLEDprint(status,gotmsg);  
        // Enviar un mensaje a un server rf95_mesh
        // La ruta hacia la direccion destino sera automaticamente descubierta
        if(firstrun==true){//Toma de tiempos iniciales de primera corrida
        my_time_client.update();
        time1=my_time_client.get_millis();
        times1=my_time_client.getSeconds();
        }
        if(secondrun==true){//Toma de tiempos iniciales de segunda corrida
        my_time_client.update();
        time3=my_time_client.get_millis();
        times3=my_time_client.getSeconds();
        }
        if(thirdrun==true){//Toma de tiempos iniciales de tercera corrida
        my_time_client.update();
        time5=my_time_client.get_millis();
        times5=my_time_client.getSeconds();
        }
        uint8_t error = manager->sendtoWait(data_c, sizeof(data_c), PROGRAMING_ADDRESS);
        if (error == RH_ROUTER_ERROR_NONE) 
        //if (manager->sendtoWait(data_c, sizeof(data_c), PROGRAMING_ADDRESS) == RH_ROUTER_ERROR_NONE)
            {   
                Serial.println("paquete enviado");
                if(thirdrun==true){//Toma de tiempo de cierre del intervalo e impresion de resultado
                my_time_client.update();
                time6=my_time_client.get_millis();
                times6=my_time_client.getSeconds();
                thirdrun=false;
                Serial.println("Tiempos tercera corrida:");
                intervalo3=totalMilis(time5,times5,time6,times6);
                String tercerostiempos=stringTiempos(time5,times5,time6,times6);
                Serial.println(tercerostiempos);
                tiempofinal=(intervalo2+intervalo3)/2;
                tiempofinal=intervalo1-tiempofinal;
                Serial.print("Tiempo de Asociacion: ");
                Serial.print((int)tiempofinal);
                Serial.print(" ms");
                delay(1000);
                }
                if(secondrun==true){//Toma de tiempo de cierre del intervalo e impresion de resultado
                my_time_client.update();
                time4=my_time_client.get_millis();
                times4=my_time_client.getSeconds();
                secondrun=false;
                thirdrun=true;
                Serial.println("Tiempos segunda corrida:");
                intervalo2=totalMilis(time3,times3,time4,times4);
                String segundostiempos=stringTiempos(time3,times3,time4,times4);
                Serial.println(segundostiempos);
                delay(1000);
                //delay(20000);//PAra SF11 y SF12
                }
                if(firstrun==true){//Toma de tiempo de cierre del intervalo e impresion de resultado
                my_time_client.update();
                time2=my_time_client.get_millis();
                times2=my_time_client.getSeconds();
                firstrun=false;
                secondrun=true;
                Serial.println("Tiempos primera corrida:");
                intervalo1=totalMilis(time1,times1,time2,times2);
                String primerosTiempos=stringTiempos(time1,times1,time2,times2);
                Serial.println(primerosTiempos);
                delay(1000);
                //delay(20000);//PAra SF11 y SF12
                }
            }
            else{
                Serial.println("envio fallido error:");
                Serial.println(getErrorString(error));
                status="sendtoWait failed";
                OLEDprint(status,gotmsg);
                }
    refreshRoutingTable();
    RoutingTable=getRoutesString();
    }else{
        //Rutina para programar uno de los Servidores
        uint8_t len = sizeof(buf);
        uint8_t from;
        //Recepcion continua, en espera de mensaje del cliente 
        if (manager->recvfromAck(buf, &len, &from))
        {
        status="Receiving";
        String gotmsg = (char*)buf;
        OLEDprint(status,gotmsg);
        Serial.print("got request from: 0x");
        Serial.print(from, HEX);
        Serial.print(": ");
        Serial.println((char*)buf);
        }else{
                status="Waiting...";
                OLEDprint(status,gotmsg);
        }
    refreshRoutingTable();
    RoutingTable=getRoutesString();
    }
}
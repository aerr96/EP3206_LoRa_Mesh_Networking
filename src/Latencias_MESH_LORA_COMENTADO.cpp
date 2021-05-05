/*Proyecto de Grado de Ingenieria Electronica
            Red Mesh con LoRa
        Universidad Simon Bolivar 
        Alejandro Rivas 13-11208                
        Tester de Latencias Mesh          */

#include "SSD1306.h" 
#include <RHRouter.h>
#include <RHMesh.h>
#include <RH_RF95.h>
#include <SPI.h>
#include <WiFi.h>
#include <NTPClient.h>
#include <WiFiUdp.h>

// Reemplazar con las credenciales de la red Wi-Fi en uso
const char* ssid     = "*******";
const char* password = "*******";

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
#define CLIENT_ADDRESS 1  //4 para topologia Y
#define SERVER1_ADDRESS 2
#define SERVER2_ADDRESS 3
#define SERVER3_ADDRESS 4 //1 para topologia Y
 
uint8_t routes[NODES];               // Arreglo de direcciones de siguiente salto para determinados destinos
int16_t rssi[NODES];                 // Arreglo de mediciones de rssi desde el nodo enrutado mas cercano
uint8_t buf[RH_MESH_MAX_MESSAGE_LEN];// Buffer para almacenar mensajes:

//Mensajes posibles:
uint8_t data_c[] = "abecdefghijklmnopqrstuvwxyzabecdefghijklmnopqrstuabecdefghijklmnopqrstuvwxyzabecdefghijklmnopqrstuvabecdefghijklmnopqrstuvwxyzabecdefghijklmnopqrstuabecdefghijklmnopqrstuvwxyzabecdefghijklmno";
//MaxDefault 245Bytes: abecdefghijklmnopqrstuvwxyzabecdefghijklmnopqrstuabecdefghijklmnopqrstuvwxyzabecdefghijklmnopqrstuvabecdefghijklmnopqrstuvwxyazabaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaecdefghijklmnopqrstuabecdefghijklmnopqrstuvwxyzabecdefghijklmnopqrstuvwaaaaaa
//200Bytes: abecdefghijklmnopqrstuvwxyzabecdefghijklmnopqrstuabecdefghijklmnopqrstuvwxyzabecdefghijklmnopqrstuvabecdefghijklmnopqrstuvwxyzabecdefghijklmnopqrstuabecdefghijklmnopqrstuvwxyzabecdefghijklmnopqrstuvw
//192Bytes: abecdefghijklmnopqrstuvwxyzabecdefghijklmnopqrstuabecdefghijklmnopqrstuvwxyzabecdefghijklmnopqrstuvabecdefghijklmnopqrstuvwxyzabecdefghijklmnopqrstuabecdefghijklmnopqrstuvwxyzabecdefghijklmno
//160Bytes: abecdefghijklmnopqrstuvwxyzabecdefghijklmnopqrstuabecdefghijklmnopqrstuvwxyzabecdefghijklmnopqrstuvabecdefghijklmnopqrstuvwxyzabecdefghijklmnopqrstuvabecdefgha
//150Bytes: abecdefghijklmnopqrstuvwxyzabecdefghijklmnopqrstuabecdefghijklmnopqrstuvwxyzabecdefghijklmnopqrstuvabecdefghijklmnopqrstuvwxyzabecdefghijklmnopqrstuv
//128Bytes: abecdefghijklmnopqrstuvwxyzabecdefghijklmnopqrstuabecdefghijklmnopqrstuvwxyzabecdefghijklmnopqrstuvabecdefghijklmnopqrstuvwxyza
//100Bytes: abecdefghijklmnopqrstuvwxyzabecdefghijklmnopqrstuabecdefghijklmnopqrstuvwxyzabecdefghijklmnopqrstuv
//96Bytes: abecdefghijklmnopqrstuvwxyzabecdefghijklmnopqrstuabecdefghijklmnopqrstuvwxyzabecdefghijklmnopqr
//64Bytes: abecdefghijklmnopqrstuvwxyzabecdefghijklmnopqrstuabecdefghijklm
//50Bytes: abecdefghijklmnopqrstuvwxyzabecdefghijklmnopqrstu
//32Bytes: abecdefghijklmnopqrstuvwxyzabcd
//30Bytes: abecdefghijklmnopqrstuvwxyzab 
//25Bytes: abecdefghijklmnopqrstuvw
//20Bytes: abecdefghijklmnopqr 
//15Bytes: abecdefghijklm 
//10Bytes: abecdefgh 
//8Bytes: abecdef
//5Bytes: abec
int counter = 0;
int Nmuestras = 50;
float latencias[50]; //Para guardar las latencias medidas y luego promediarlas
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
    my_time_client.setTimeOffset(2880000);
    //my_time_client.setTimeOffset(28800);
    my_time_client.setUpdateInterval(2000000);
    //my_time_client.setUpdateInterval(20000);
    my_time_client.setPoolServerName("0.ubuntu.pool.ntp.org");
    //my_time_client.setPoolServerName("192.168.1.200");
    //Un timeout mas pequeno brinda mejor precision
    //pero una mayor probabilidade fallar en el ajuste
    my_time_client.setTimeout(800); 
    Serial.println("syncing...");

    while(my_time_client.update()!=1){
        delay(2000);
        my_time_client.forceUpdate();
    }
    //my_time_client.forceUpdate();

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
    uint8_t sf= 7;
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
    uint16_t timeout =1200;
    manager->setTimeout(timeout);
    }
    if(sf==11||sf==12){
    uint16_t timeout =2200;
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
String stringTiempos(float t1,float ts1,float t2,float ts2){
    float Dif = totalMilis(t1,ts1,t2,ts2);
    String var="Tiempo inicial: seg(";
    var+=ts1;
    var+=") mili(";
    var+=t1;
    var+=") Tiempo final: seg(";
    var+=ts2;
    var+=") mili(";
    var+=t2;
    var+=").";
    var+=" Intervalo: ";
    var+=Dif;
    return var;
}
// Funcion para calcular el promedio de un array de 50
float promedio (float array[]) 
{
  float y=0, suma = 0;
  for ( byte i = 0; i < Nmuestras; i++)
  {
    suma += array[i]; 
  }
  y=suma/Nmuestras; 
  return y;
}
// Funcion para calcular la desviacion estardar del array de muestras
float getStdDev(float array[]) {
  float prom = promedio(array);
  long total = 0;
  for (byte i = 0; i < Nmuestras; i++) {
    total = total + (array[i] - prom) * (array[i] - prom);
  }
  float variance = total/Nmuestras;
  float stdDev = sqrt(variance);
  return stdDev;
}
// Variables logicas para manejar las primeras corridas de envio
bool firstrun =true;
bool secondrun =false;
// Variables para guardar tiempos de interes
float time1;
float times1;
float time2;
float times2;
float intervalo1;

void loop()
{  
    if(client){
        if(counter<Nmuestras){   
            Serial.print("Tamano del paquete:");
            Serial.println(sizeof(data_c));
            //Seleccionamos el servidor o nivel al cual transmitir
            getNodeSelection(1);
            status="Sending to servers";
            OLEDprint(status,gotmsg);  
            // Enviar un mensaje a un server rf95_mesh
            // La ruta hacia la direccion destino sera automaticamente descubierta
            if(firstrun==true){
            my_time_client.update();
            time1=my_time_client.get_millis();
            times1=my_time_client.getSeconds();
            }
            uint8_t error = manager->sendtoWait(data_c, sizeof(data_c), PROGRAMING_ADDRESS);
            if (error == RH_ROUTER_ERROR_NONE) 
                {
                    Serial.print("paquete enviado #");
                    Serial.println(counter);
                    // Fue entregado con confirmacion el paquete
                    // Y Ahora esperamos la respuesta del server
                    uint8_t len = sizeof(buf);
                    uint8_t from;    
                    //delay(3000);
                    if (manager->recvfromAckTimeout(buf, &len, 5000, &from))
                        {
                            status="Receiving";
                            Serial.print("Mensaje de respuesta: ");
                            Serial.println((char*)buf);
                            String gotmsg = (char*)buf;
                            OLEDprint(status,gotmsg);
                            Serial.print("Obteniendo respuesta de: 0x");
                            Serial.print(from, HEX);
                            Serial.print(": ");
                            Serial.println((char*)buf);
                            if(firstrun==true){//Toma de tiempo de cierre del intervalo 
                            my_time_client.update();
                            time2=my_time_client.get_millis();
                            times2=my_time_client.getSeconds();
                            //firstrun=false;
                            Serial.println("Tiempos primera corrida:");
                            intervalo1=totalMilis(time1,times1,time2,times2);
                            String primerosTiempos=stringTiempos(time1,times1,time2,times2);
                            Serial.println(primerosTiempos);
                            if(secondrun==true){
                            latencias[counter]=intervalo1/2;
                            counter++;
                            }
                            secondrun=true;
                            delay(500);
                            }
                            rssi[PROGRAMING_ADDRESS-1] = rf95.lastRssi();
                        }
                        else
                            {
                                Serial.println("sin respuesta");
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
        refreshRoutingTable();
        RoutingTable=getRoutesString();
        }else{
            Serial.print("La Latencia promedio es: ");
            Serial.println((int)promedio(latencias));
            Serial.print("con una desviacion de: ");
            Serial.println(getStdDev(latencias));
            delay(5000);
            for (int i = 0; i < Nmuestras; i++)
            {
                Serial.print("Latencia #");
                Serial.print(i);
                Serial.print(": ");
                Serial.println(latencias[i]);
            }
        }
    }else{
        //Rutina para programar uno de los Servidores
        uint8_t len = sizeof(buf);
        uint8_t from;
        //Recepcion continua o descubrimiento de rutas, en espera de mensaje del cliente 
        if (manager->recvfromAckTimeout(buf, &len,20000, &from))
        {
        status="Receiving";
        String gotmsg = (char*)buf;
        OLEDprint(status,gotmsg);
        Serial.print("got request from: 0x");
        Serial.print(from, HEX);
        Serial.print(": ");
        Serial.println((char*)buf);
            // Enviar respuesta al cliente de origen
            if (manager->sendtoWait(buf, strlen((char*)buf), from) != RH_ROUTER_ERROR_NONE)
                {
                status="sendtoWait failed";
                OLEDprint(status,gotmsg);
                }else
                {
                    status="Sending reply";
                    OLEDprint(status,gotmsg);
                    rssi[from]= rf95.lastRssi();                 
                }
        }else{
                status="Waiting... xd";
                OLEDprint(status,gotmsg);
        }
    refreshRoutingTable();
    RoutingTable=getRoutesString();
    }
}
/*Proyecto de Grado de Ingenieria Electronica
            Red Mesh con LoRa
        Universidad Simon Bolivar 
        Alejandro Rivas 13-11208                
    Tester de Alcance Punto a Punto          */

#include "SSD1306.h" 
#include <RH_RF95.h>
#include <SPI.h>
#include "BluetoothSerial.h" 

#define GREENLED 25  //Pin del LED verde del ESP32

// Variables para conexion LoRa ESP32 por SPI
                     // TTGO   -- SX1278
#define SCK     5    // GPIO5  -- SCLK
#define MISO    19   // GPIO19 -- MISO
#define MOSI    27   // GPIO27 -- MOSI
#define SS      18   // GPIO18 -- CS
#define RST     23   // GPIO23 -- RESET 
#define DI0     26   // GPIO26 -- IRQ(Interrupt Request)

//Variables Globales
BluetoothSerial SerialBT;
SSD1306 display(0x3c, 21, 22); 
String rssi ="";
bool receptor = false; //Para seleccionar si se programa un receptor (true) o un transmisor (false)
String packet ;
String packSize = "--";
uint8_t counter = 0;
String message="";
String nada ="no llego";
String null ="null";
// Instancia del Driver RH_RF95 para LoRa en el TTGO LoRa 
RH_RF95 rf95(SS,DI0);
// Inicializacion de Variables String para impresiones OLED
String SFUpdate="Bluetooth Input: ";
String Current="SF = 7"; //Siempre empezamos con Factor de Propagacion de 7 por defecto
// Funcion de impresion OLED
void OLEDprint(String rssi_count, String msg, String rssi_op,String SPUp, String curr){
    if(receptor){
      display.clear();
      display.setTextAlignment(TEXT_ALIGN_LEFT);
      display.setFont(ArialMT_Plain_10);
      display.drawString(0 , 0 , "Recived: "+ msg); //Imprime el contador del mensaje recibido
      display.drawString(0 , 12, rssi_count);       //Imprime la relacion de potencia recibida 
      display.drawString(0 , 24 , SPUp);            //Imprime el mensaje que llega por Bluetooth
      display.drawString(0 , 36 , curr);            //Imprime el SF actual
      display.display();
    }else{
      display.clear();
      display.setTextAlignment(TEXT_ALIGN_LEFT);
      display.setFont(ArialMT_Plain_10);
      display.drawString(0 , 0 , rssi_count);       //Imprime el contador del mensaje que envia
      display.drawString(0 , 12 , "Return: "+ msg); //Imprime el mensaje recibido y que opcionalmente puede devolver
      display.drawString(0 , 24 , rssi_op);            //Impresion de RSSI, 0 si no escucha respuestas
      display.drawString(0 , 36 , SPUp);            //Imprime el mensaje que llega por Bluetooth
      display.drawString(0 , 48 , curr);            //Imprime el SF actual
      display.display();
    }
}
// Lector por caracteres de paquete recibido
void read_pkch(uint8_t packetSize, uint8_t buf[]) {
  packet ="";
  packSize = String(packetSize,DEC);
  for (int i = 0; i < packetSize; i++) { packet += (char)buf[i]; }
}

// Ajuste individual de Factor de Propagacion por Bluetooth
void ChangeSP(String read){
  if(read.equals("SF7")){
    uint8_t sf = 7;
    rf95.setSpreadingFactor(sf);
    String cur="SF = ";
    cur+=sf;
    Current=cur;
  }
  if(read.equals("SF8")){
    uint8_t sf = 8;
    rf95.setSpreadingFactor(sf);
    String cur="SF = ";
    cur+=sf;
    Current=cur;
  }
  if(read.equals("SF9")){
    uint8_t sf = 9;
    rf95.setSpreadingFactor(sf);
    String cur="SF = ";
    cur+=sf;
    Current=cur;
  }
  if(read.equals("SF10")){
    uint8_t sf = 10;
    rf95.setSpreadingFactor(sf);
    String cur="SF = ";
    cur+=sf;
    Current=cur;
  }
  if(read.equals("SF11")){
    uint8_t sf = 11;
    rf95.setSpreadingFactor(sf);
    String cur="SF = ";
    cur+=sf;
    Current=cur;
  }
  if(read.equals("SF12")){
    uint8_t sf = 12;
    rf95.setSpreadingFactor(sf);
    String cur="SF = ";
    cur+=sf;
    Current=cur;
  }
}

void setup() {
  //Se asigna el nombre identifcador de Bluetooth del dispositivo
  //SerialBT.begin("ESP32testGATO");
  SerialBT.begin("ESP32testPERRO");
  //SerialBT.begin("ESP32testPEZ");
  //SerialBT.begin("ESP32testOVEJA"); 
  pinMode(GREENLED,OUTPUT); //Led para evento exitoso de recepcion o transmision de paquete 
  pinMode(16,OUTPUT); 
  digitalWrite(16, LOW);    // Pone el GPIO16 low para resetear el OLED
  delay(50); 
  digitalWrite(16, HIGH);   // Mientras el OLED esta encendido, se debe poner el GPIO16 en high
  //Inicializaciones de comunicacion por puerto serial
  Serial.begin(115200);
  while (!Serial);
  Serial.println();
  Serial.println("LoRa Callback");
  SPI.begin(SCK,MISO,MOSI,SS);// Inicializa la SPI 
  if (!rf95.init())           //Chequeo de correcta inicializacion del driver RH_RF95
    Serial.println("init failed");  
  //  Parametros LoRa Iniciales
  int8_t power = 20;          //Potencia de salida, 20 es el maximo de +20dBm
  rf95.setTxPower(power, false);// De no ser asignada una potencia de salida especifica +13dBm por defecto
  //Inicializacion de pantalla OLED
  display.init();
  display.flipScreenVertically();  
  display.setFont(ArialMT_Plain_10);
  Serial.println("init ok");
  delay(1500);
}

void loop() {
    //Ajuste individual de Factor de Propagacion por Bluetooth
    if (SerialBT.available()) {
      String var="Bluetooth Input: ";
      //String message;
      char incomingChar = SerialBT.read();
      if (incomingChar != '\n'){
        //Concatenamos los caracteres recibidos en cada ciclo para formar el mensaje
        message += String(incomingChar);
      }
      else{
        //En caso de salto de linea vaciamos el mensaje
        message = "";
      }
      //Serial.write(incomingChar); //Debug opcional
      ChangeSP(message); //Segun el mensaje formado cambiamos el Factor de Propagacion
      rf95.printRegisters(); //Debug opcional para verificar SF cambiado y registros generales por impresion Serial
      var+=message;
      SFUpdate=var;
    }
    delay(20);

    if(receptor){
    //Rutina para programar a un receptor
    if (rf95.available())
    {
        String rssi= "RSSI: ";   
        uint8_t buf[RH_RF95_MAX_MESSAGE_LEN];
        uint8_t len = sizeof(buf);
        rf95.recv(buf,&len);          //Recibimos paquete
        read_pkch(len,buf);           //Leemos y actualizamos la variable global "packet"
        int16_t var =rf95.lastRssi(); //Capturamos el ultimo RSSI
        rssi += var;
        OLEDprint(rssi, packet,null,SFUpdate,Current);//Actualizamos el OLED
        delay(10);
        /*//Opcional enviar respuesta
        rf95.send(buf, strlen((char*)buf));
        rf95.waitPacketSent();*/
        digitalWrite(GREENLED, HIGH);  // Encendemos el LED 
        delay(900);                    // Esperamos 
        digitalWrite(GREENLED, LOW);   // Apagamos el LED
        delay(900);                    // Delay extra que permite la correcta recepcion de mensajes Bluetooth
    }else{
      delay(1800);                     // Delay que permite la correcta recepcion de mensajes Bluetooth
    }
  }else{
    // Rutina para programar a un transmisor
    String rssi= "RSSI: "; 
    uint8_t data[] = "Cota Mil ";
    //Construimos el paquete, mensaje + contador
    uint8_t mydata[RH_RF95_MAX_MESSAGE_LEN];
    mydata[0] = 0;
    char*  MyMessage = "Cota Mil ";
    char counter1[12];
    strcat ((char*)mydata, MyMessage);
    itoa(counter,counter1,10);
    strcat ((char*)mydata, counter1);
    rf95.send(mydata, strlen((char*)mydata)); //Enviamos paquete
    //rf95.send(data, sizeof(data)); //Opcional enviar solo mensaje sin contador
    String cuenta = "Contador: "; //Para actualizacion OLED
    cuenta += counter;
    int16_t var =rf95.lastRssi(); //Para actualizacion OLED
    rssi += var;
    OLEDprint(cuenta,(char*)data,rssi,SFUpdate,Current); //Actualizamos el OLED
    rf95.waitPacketSent();
    delay(10);
    /*//Opcional recibir respuesta
    if (rf95.available())
    {
        uint8_t buf[RH_RF95_MAX_MESSAGE_LEN];
        uint8_t len = sizeof(buf);
        rf95.recv(buf,&len);
        read_pkch(len,buf);
        String rssi= "RSSI: ";   
        int16_t var =rf95.lastRssi();
        rssi += var;
        OLEDprint(cuenta,packet,rssi,SFUpdate,Current);
    }*/
    counter++;                     //Incrementamos el contador +1
    digitalWrite(GREENLED, HIGH);  // Encendemos el LED 
    delay(900);                    // Esperamos 
    digitalWrite(GREENLED, LOW);   // Apagamos el LED
    delay(900);                    // Delay extra que permite la correcta recepcion de mensajes Bluetooth
  }
}
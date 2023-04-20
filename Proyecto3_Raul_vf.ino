//Librerias RFID
#include <Arduino.h>
#include <SPI.h>
#include <MFRC522.h>

#include <WiFi.h>
#include <WiFiClient.h>
#include <WebServer.h>
#include <ESPmDNS.h>

#include <NTPClient.h>
#include <WiFiUdp.h>

WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP);

#include <HTTPClient.h>
#include <ArduinoJson.h>

#include <Wire.h>
#include <Adafruit_Sensor.h>

#define SEALEVELPRESSURE_HPA (1013.25)

//Definimos las variables de RFID
#if defined(ESP32)
  #define SS_PIN 5
  #define RST_PIN 22
#elif defined(ESP8266)
  #define SS_PIN D8
  #define RST_PIN D0
#endif

MFRC522 rfid(SS_PIN, RST_PIN); // Instance of the class
MFRC522::MIFARE_Key key;
// Init array that will store new NUID
byte nuidPICC[4];

//xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx
String DatoHex;
const String UserReg_1 = "D08E323B";
const String UserReg_2 = "B33786A3";
const String UserReg_3 = "7762C83B";
//xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx

const char *ssid = "UTD";
const char *password = "a1b1c1d1e1";
//const char *ssid = "prueba";
//const char *password = "123456789";
//const char* ssid = "INFINITUM736A_2.4";
//const char* password = "cAdtTx3acr";

const char* ntpServer = "pool.ntp.org";
const char* serverName = "https://us-east-1.aws.data.mongodb-api.com/app/lector_rfid-rswsa/endpoint/RFID";
StaticJsonDocument<500> doc;



// the setup routine runs once when you press reset:
void setup() {

  Serial.begin(115200);
    
  // initialize serial communication at 9600 bits per second:
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  Serial.println("");

  // Wait for connection
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
    digitalWrite(5,LOW);
  }

  Serial.println("");
  Serial.print("Connected to ");
  digitalWrite(5,HIGH);
  Serial.println(ssid);
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());

   timeClient.begin();
  timeClient.setTimeOffset(-21600);

  if (MDNS.begin("esp32")) {
    Serial.println("MDNS responder started");
      SPI.begin(); // Init SPI bus
   rfid.PCD_Init(); // Init MFRC522
   Serial.println();
   Serial.print(F("Reader :"));
   rfid.PCD_DumpVersionToSerial();
   for (byte i = 0; i < 6; i++) {
     key.keyByte[i] = 0xFF;
   } 
   DatoHex = printHex(key.keyByte, MFRC522::MF_KEY_SIZE);
   Serial.println();
   Serial.println();
   Serial.println("Iniciando el Programa");
  }
}

// the loop routine runs over and over again forever:
void loop() {
       // Reset the loop if no new card present on the sensor/reader. This saves the entire process when idle.
     if ( ! rfid.PICC_IsNewCardPresent()){return;}
     
     // Verify if the NUID has been readed
     if ( ! rfid.PICC_ReadCardSerial()){return;}
     
     Serial.print(F("PICC type: "));
     MFRC522::PICC_Type piccType = rfid.PICC_GetType(rfid.uid.sak);
     Serial.println(rfid.PICC_GetTypeName(piccType));
     // Check is the PICC of Classic MIFARE type
     if (piccType != MFRC522::PICC_TYPE_MIFARE_MINI && piccType != MFRC522::PICC_TYPE_MIFARE_1K && piccType != MFRC522::PICC_TYPE_MIFARE_4K)
     {
       Serial.println("Su Tarjeta no es del tipo MIFARE Classic.");
       return;
     }
     
     if (rfid.uid.uidByte[0] != nuidPICC[0] || rfid.uid.uidByte[1] != nuidPICC[1] || rfid.uid.uidByte[2] != nuidPICC[2] || rfid.uid.uidByte[3] != nuidPICC[3] )
     {
       Serial.println("Se ha detectado una nueva tarjeta.");
       
       // Store NUID into nuidPICC array
       for (byte i = 0; i < 4; i++) {nuidPICC[i] = rfid.uid.uidByte[i];}
    
       DatoHex = printHex(rfid.uid.uidByte, rfid.uid.size);
       Serial.print("Codigo Tarjeta: "); Serial.println(DatoHex);
    
       if(UserReg_1 == DatoHex)
       {
        Serial.println("USUARIO 1 - PUEDE INGRESAR");
        doc["RFID"] = DatoHex;
        doc["estatus"] = "activo";     
       }
       else if(UserReg_2 == DatoHex)
       {
        Serial.println("USUARIO 2 - PUEDE INGRESAR");
       }
       else if(UserReg_3 == DatoHex)
       {
        Serial.println("USUARIO 3 - PUEDE INGRESAR");
       }
       else
       {
        Serial.println("NO ESTA REGISTRADO - PROHIBIDO EL INGRESO");
        doc["RFID"] = DatoHex;
        doc["estatus"] = "no_activo"; 
       }  
       Serial.println();
     }
     else 
     {
      Serial.println("Tarjeta leida previamente");
     }
     // Halt PICC
     rfid.PICC_HaltA();
     // Stop encryption on PCD
     rfid.PCD_StopCrypto1();
  while(!timeClient.update()) {
    timeClient.forceUpdate();
  }

  //Variable que se va a mandar a mongoDB en json
  
  //doc["RFID"] = DatoHex;
  
 Serial.println("Uploading data... "); 
       POSTData();
  delay(3000);        // delay in between reads for stability
}
String printHex(byte *buffer, byte bufferSize)
{  
   String DatoHexAux = "";
   for (byte i = 0; i < bufferSize; i++) 
   {
       if (buffer[i] < 0x10)
       {
        DatoHexAux = DatoHexAux + "0";
        DatoHexAux = DatoHexAux + String(buffer[i], HEX);  
       }
       else { DatoHexAux = DatoHexAux + String(buffer[i], HEX); }
   }
   
   for (int i = 0; i < DatoHexAux.length(); i++) {DatoHexAux[i] = toupper(DatoHexAux[i]);}
   return DatoHexAux;
}


void POSTData()
{
    if(WiFi.status()== WL_CONNECTED){
      HTTPClient http;

      http.begin(serverName);
      http.addHeader("Content-Type", "application/json");

      String json;
      serializeJson(doc, json);

      Serial.println(json);
      int httpResponseCode = http.POST(json);
      Serial.println(httpResponseCode);

      if (httpResponseCode == 204) {
        Serial.println("Data uploaded.");
        delay(3000);
        digitalWrite(18,HIGH);
        delay(100);
        digitalWrite(18,LOW);
      } else {
        Serial.println("ERROR: Couldn't upload data.");
        delay(3000);
      }
      delay(3000);
    }
    delay(3000);
}

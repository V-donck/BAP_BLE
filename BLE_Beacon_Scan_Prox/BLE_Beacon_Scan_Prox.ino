
#include <Arduino.h>

#include <NimBLEDevice.h>
#include <NimBLEAdvertisedDevice.h>
#include "NimBLEEddystoneURL.h"
#include "NimBLEEddystoneTLM.h"
#include "NimBLEBeacon.h"
#include <WiFi.h>
#include <WiFiClient.h>
#include <WiFiAP.h>

//#define ENDIAN_CHANGE_U16(x) ((((x)&0xFF00) >> 8) + (((x)&0xFF) << 8))

int scanTime = 5; //In seconds
BLEScan *pBLEScan;

const char *ssid = "yourAP";
const char *password = "yourPassword";
WiFiServer server(80);

// Tasks
TaskHandle_t Task1;
TaskHandle_t Task2;

class MyAdvertisedDeviceCallbacks : public BLEAdvertisedDeviceCallbacks
{


    /*** Only a reference to the advertised device is passed now
      void onResult(BLEAdvertisedDevice advertisedDevice) { **/
    void onResult(BLEAdvertisedDevice *advertisedDevice)
    {      
      //Serial.println("string: ");
      //Serial.print(advertisedDevice->toString().c_str());
      if (advertisedDevice->haveName())
      {
        std::string namedevice = advertisedDevice->getName();
        std::string pr = "Prox";
        if(namedevice==pr){
          Serial.print("Device name: ");
          Serial.println(advertisedDevice->getName().c_str());
          Serial.println("");
          Serial.println("found Prox!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!");
          Serial.println("");
          Serial.println("");
          Serial.println("");
          if (advertisedDevice->haveManufacturerData())
          {
            std::string strManufacturerData = advertisedDevice->getManufacturerData();
            uint8_t cManufacturerData[20];
            char be[8];
            char le[4];
            strManufacturerData.copy((char *)cManufacturerData, strManufacturerData.length(), 0);
            Serial.printf("strManufacturerData: %d ", strManufacturerData.length());
            Serial.printf("\n");
            sprintf(le,"%X",cManufacturerData[2]);
            sprintf(be,"%X",cManufacturerData[3]); 
            strcat(be,le);
            Serial.println(be);
        
          }
        }
      }
    return;
    }
};


void setup_ble(){
  //BLE
  Serial.println("Scanning...");
  BLEDevice::init("");
  pBLEScan = BLEDevice::getScan(); //create new scan
  pBLEScan->setAdvertisedDeviceCallbacks(new MyAdvertisedDeviceCallbacks());
  pBLEScan->setActiveScan(true); //active scan uses more power, but get results faster
  pBLEScan->setInterval(100);
  pBLEScan->setWindow(99); // less or equal setInterval value
}

void setup_wifi(){
  Serial.println("setup------------------------------------------------------------------------------");
  //WIFI
  Serial.println("Configuring access point...");

  WiFi.softAP(ssid, password);
  IPAddress myIP = WiFi.softAPIP();
  Serial.print("AP IP address: ");
  Serial.println(myIP);
  server.begin();

  Serial.println("Server started");
}

void setup()
{
  Serial.begin(115200);

    //create a task that will be executed in the Task1code() function, with priority 1 and executed on core 0
  xTaskCreatePinnedToCore(
                    Task1code,   /* Task function. */
                    "Task1",     /* name of task. */
                    10000,       /* Stack size of task */
                    NULL,        /* parameter of the task */
                    1,           /* priority of the task */
                    &Task1,      /* Task handle to keep track of created task */
                    0);          /* pin task to core 0 */                  
  delay(500);  
    //create a task that will be executed in the Task2code() function, with priority 1 and executed on core 1
  xTaskCreatePinnedToCore(
                    Task2code,   /* Task function. */
                    "Task2",     /* name of task. */
                    10000,       /* Stack size of task */
                    NULL,        /* parameter of the task */
                    1,           /* priority of the task */
                    &Task2,      /* Task handle to keep track of created task */
                    1);          /* pin task to core 1 */
  delay(500); 
}

//Task1code: Handle BLE
void Task1code( void * pvParameters ){
  Serial.print("Task1 running on core ");
  Serial.println(xPortGetCoreID());

  setup_ble();

  for(;;) {
    // put your main code here, to run repeatedly:
    BLEScanResults foundDevices = pBLEScan->start(scanTime, false);
    Serial.print("Devices found: ");
    Serial.println(foundDevices.getCount());
    Serial.println("Scan done!");
    delay(2000);
    pBLEScan->clearResults(); // delete results fromBLEScan buffer to release memory
  } 
}


//Task2code: Handle WiFi
void Task2code( void * pvParameters ){
  Serial.print("Task2 running on core ");
  Serial.println(xPortGetCoreID());
  setup_wifi();
  for(;;) {
   /* if (WiFi.status() != WL_CONNECTED) {
      setup_wifi();
    }

    if (!mqttClient.connected()) {
      // MQTT client is disconnected, connect
      connectMQTT();
    }

    String message = "Test message"; 
    publishMessage(message);
*/
    WiFiClient client = server.available();   // listen for incoming clients

    if (client) {                             // if you get a client,
      Serial.println("New Client.");           // print a message out the serial port
      String currentLine = "";                // make a String to hold incoming data from the client
      while (client.connected()) {            // loop while the client's connected
        if (client.available()) {             // if there's bytes to read from the client,
          char c = client.read();             // read a byte, then
          Serial.write(c);                    // print it out the serial monitor
          if (c == '\n') {                    // if the byte is a newline character
  
            // if the current line is blank, you got two newline characters in a row.
            // that's the end of the client HTTP request, so send a response:
            if (currentLine.length() == 0) {
              // HTTP headers always start with a response code (e.g. HTTP/1.1 200 OK)
              // and a content-type so the client knows what's coming, then a blank line:
              client.println("HTTP/1.1 200 OK");
              client.println("Content-type:text/html");
              client.println();
  
              // the content of the HTTP response follows the header:
              client.print("Click <a href=\"/H\">here</a> to turn ON the LED.<br>");
              client.print("Click <a href=\"/L\">here</a> to turn OFF the LED.<br>");
  
              // The HTTP response ends with another blank line:
              client.println();
              // break out of the while loop:
              break;
            } else {    // if you got a newline, then clear currentLine:
              currentLine = "";
            }
          } else if (c != '\r') {  // if you got anything else but a carriage return character,
            currentLine += c;      // add it to the end of the currentLine
          }
  
          // Check to see if the client request was "GET /H" or "GET /L":
          if (currentLine.endsWith("GET /H")) {
            digitalWrite(LED_BUILTIN, HIGH);               // GET /H turns the LED on
          }
          if (currentLine.endsWith("GET /L")) {
            digitalWrite(LED_BUILTIN, LOW);                // GET /L turns the LED off
          }
        }
      }
      // close the connection:
      client.stop();
      Serial.println("Client Disconnected.");
    }
    
    delay(5000);
  }
}



void loop(){}

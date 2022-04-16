
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

const char *ssid = "SmartFeeder";
const char *password = "password";
const int LISTLENGTH = 100;
WiFiServer server(80);
String lijst1[LISTLENGTH];
String lijst2[LISTLENGTH];
byte index1 = 0;
byte index2=0;

// Tasks
TaskHandle_t Task1;
TaskHandle_t Task2;

// check if id is already in lijst1
boolean checkArray1(String id){
  for(int i=0;i<index1;i++){
    if(lijst1[i].equals(id)){
      return true;
    }
  }
  return false;
}

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
            // als id in lijst 1 -> voederbak1 open
            if(checkArray1(be)){
              // open voederbak1
              // nu voor test led aan
              digitalWrite(LED_BUILTIN, HIGH);
              Serial.println("voederbak1 open!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!");
              
            }
            //if(checkArray2(be)){
              //open voederbak2
            //}
        
          }
        }
      }
    return;
    }
};



// check if id is already in lijst2
boolean checkArray2(String id){
  for(int i=0;i<index2;i++){
    if(lijst2[i].equals(id)){
      return true;
    }
  }
  return false;
}

boolean checkHex(String s){
  for(int i = 0;i<s.length();i++){
    char ch = s[i];
    if((ch<'0' || ch >'9') && (ch<'A' ||ch>'F')){
      return false;
    }
  }
  return true;
}



void setup_ble(){
  //BLE
  Serial.println("Scanning...");
  BLEDevice::init("");
  pBLEScan = BLEDevice::getScan(); //create new scan
  pBLEScan->setAdvertisedDeviceCallbacks(new MyAdvertisedDeviceCallbacks());
  pBLEScan->setActiveScan(true); //active scan uses more power, but get results faster
  pBLEScan->setInterval(1000); // here was 100
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
  // here 
  pinMode(LED_BUILTIN,OUTPUT);
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
    WiFiClient client = server.available();   // listen for incoming clients

    if (client) {                             // if you get a client,
      Serial.println("New Client.");           // print a message out the serial port
      String currentLine = "";                // make a String to hold incoming data from the client
      while (client.connected()) {            // loop while the client's connected
        if (client.available()) {             // if there's bytes to read from the client,
          char c = client.read();             // read a byte, then
          //Serial.write(c);                    // print it out the serial monitor
          if (c == '\n') {                    // if the byte is a newline character
  
            // if the current line is blank, you got two newline characters in a row.
            // that's the end of the client HTTP request, so send a response:
            if (currentLine.length() == 0) {
              String stringlist1= "";
             
              String stringlist2;
              for(int i=0;i<index1;i++){
                if(lijst1[i] != "0"){
                  stringlist1 = stringlist1 + " \n" + lijst1[i];
                }    
              }
              for(int i=0;i<index2;i++){
                if(lijst2[i] != "0"){
                  stringlist2 = stringlist2 + " \n" + lijst2[i]; 
                }  
              }
              
              // HTTP headers always start with a response code (e.g. HTTP/1.1 200 OK)
              // and a content-type so the client knows what's coming, then a blank line:
              client.println("HTTP/1.1 200 OK");
              client.println("Content-type:text/html");
              client.println();
              // the content of the HTTP response follows the header:
              client.write("<style>{box-sizing: border-box;}.column {float: left;width: 50%;}.row:after { content: \"\";display: table;clear: both; }</style>");
              //client.print("Click <a href=\"/H\">here</a> to turn ON the LED.<br>");
              //client.print("Click <a href=\"/L\">here</a> to turn OFF the LED.<br>");

              client.write("<form method=GET>Add to Food 1: <input type=text name=AF1><input type=submit></form>");
              client.write("<form method=GET>remove from Food 1: <input type=text name=RF1><input type=submit></form>");
              client.write("<form method=GET>Add to Food 2: <input type=text name=AF2><input type=submit></form>");
              client.write("<form method=GET>remove from Food 2: <input type=text name=RF2><input type=submit></form>");
              client.write("<div class=\"row\"><div class=\"column\" style=\"background-color:#FFB695;\"><h2>Food 1</h2><p>");
              client.print(stringlist1);
              client.write("</p></div><div class=\"column\" style=\"background-color:#96D1CD;\"><h2>Food 2</h2><p>");
              client.print(stringlist2);
              client.print("</p></div></div>");

              Serial.println("einde normal request");
  
              // The HTTP response ends with another blank line:
              //client.println();// here was not comment
              // break out of the while loop:
             // break;// here was break
            } else {    // if you got a newline, then clear currentLine:
              currentLine = "";
            }
          } else if (c != '\r') {  // if you got anything else but a carriage return character,
            currentLine += c;      // add it to the end of the currentLine
          }else if (c == '\r'){
            currentLine = "";
          }
          
          if(currentLine.endsWith("HTTP/1.1")){
            if(currentLine.startsWith("GET /?AF1")){
              if(index1<LISTLENGTH-1){
                Serial.println(currentLine);
                String idlogger = currentLine.substring(currentLine.indexOf('=')+1,currentLine.indexOf(' ',currentLine.indexOf('=')));
                Serial.println("idlogger");
                idlogger.toUpperCase();
                if(checkHex(idlogger) && idlogger.length()== 4){
                  Serial.println(idlogger);
                  if (!checkArray1(idlogger)){
                    lijst1[index1]=idlogger;
                    index1++;
                    Serial.println("added to list1");
                  }
                  else{
                    client.println("<script>alert(\"deze id zit al in deze lijst\");</script>");
                  }
                  if (checkArray2(idlogger)){
                    client.print("<script>alert(\"deze id zit al in de andere lijst\");</script>");
                  }
                }
                else{
                  client.print("<script>alert(\"deze id is niet juist\");</script>");
                }
              }
              else{
                client.print("<script>alert(\"Er zitten te veel id's in deze lijst\");</script>");
              }
            }

        if(currentLine.startsWith("GET /?AF2")){
          if(index2<LISTLENGTH-1){
            String idlogger = currentLine.substring(currentLine.indexOf('=')+1,currentLine.indexOf(' ',currentLine.indexOf('=')));
            idlogger.toUpperCase();
            Serial.println(idlogger);
            if(checkHex(idlogger) && idlogger.length()== 4){
              if (!checkArray2(idlogger)){
                lijst2[index2]=idlogger;
                index2++;
                Serial.println("added to list2");
              }
              else{
                client.print("<script>alert(\"deze id zit al in deze lijst\");</script>");
              }
              if (checkArray1(idlogger)){
                client.print("<script>alert(\"deze id zit al in de andere lijst\");</script>");
              }
            }
            else{
              client.print("<script>alert(\"deze id is niet juist\");</script>");
            } 
          }
          else{
            client.print("<script>alert(\"Er zitten te veel id's in deze lijst\");</script>");
          }
        }
              
         boolean inlist= false;
         boolean removeF = false;
         if(currentLine.startsWith("GET /?RF1")){
          removeF = true;
          String idlogger = currentLine.substring(currentLine.indexOf('=')+1,currentLine.indexOf(' ',currentLine.indexOf('=')));
          idlogger.toUpperCase();
          Serial.println(idlogger);
          if(checkHex(idlogger) && idlogger.length()== 4){              
           for(int i=0;i<index1;i++){
            if(lijst1[i].equals(idlogger)){
             lijst1[i] = "0";
             inlist=true;
            }
           }
          }
          else{
           client.print("<script>alert(\"deze id is niet juist\");</script>");
          } 
         }

         if(currentLine.startsWith("GET /?RF2")){
              removeF = true;
              String idlogger = currentLine.substring(currentLine.indexOf('=')+1,currentLine.indexOf(' ',currentLine.indexOf('=')));
              idlogger.toUpperCase();
              Serial.println(idlogger);
             if(checkHex(idlogger) && idlogger.length()== 4){
              for(int i=0;i<index2;i++){
                if(lijst2[i].equals(idlogger)){
                  lijst2[i] = "0";
                  inlist=true;
                }
              }
             }
             else{
              client.print("<script>alert(\"deze id is niet juist\");</script>");
             } 
         }

         // warning message that id is not in list
         if (!inlist & removeF){
            client.print("<script>alert(\"deze id zit niet in deze lijst\");</script>");
          }


        
// remove zeros in list
        if(index1>LISTLENGTH-2){
          int newindex;
          Serial.println("removezeros");

           Serial.println("lijst1:");
              for(int i=0;i<index1;i++){
                Serial.print(lijst1[i]);    
                }
                Serial.println("");
          newindex = 0;
          for(int i=0;i<index1;i++){
            lijst1[newindex]=lijst1[i];
            if(!lijst1[i].equals("0")){
              newindex++;
            }
          }
          index1=newindex;
        }

        if(index2>LISTLENGTH-2){
          int newindex;
          newindex = 0;
          for(int i=0;i<index2;i++){
            lijst2[newindex]=lijst2[i];
            if(!lijst2[i].equals("0")){
              newindex++;
            }
          }
          index2=newindex;
        }

         
         // print in serialmonitor           
 Serial.println("lijst1:");
              for(int i=0;i<index1;i++){
                Serial.print(lijst1[i]);    
                }
                Serial.println("");

                 Serial.println("lijst2:");
              for(int i=0;i<index2;i++){
                Serial.print(lijst2[i]);  
              }
              Serial.println(" ");
          }

          
            
        if (c == '\n') {// if the byte is a newline character
          // if the current line is blank, you got two newline characters in a row.
          // that's the end of the client HTTP request, so send a response: and this is the end
          if (currentLine.length() == 0) {
            client.println();
            break;
          }
        }
          
        }
      }
      // close the connection:
      client.stop();
      Serial.println("Client Disconnected.");
    }
    
    delay(2000); //here was 5000
  }
}


// to do 
/*
 * melding als verwijder maar niet in lijst -> nu komen de eerste 2 lijnen er bij op die nog bij de header horen, .write werkt niet
 * melding als al in andere lijst
 * VV               lijst toevoegen aan webpagina
 * andere rommel eruit
 * VV          error bij 100 dieren
 * VV error bij foute naam
 * VV            lijst met 0 leegmaken
 * miss lijst in eeprom zetten??? daarnaar kijken
 * VV      kleine letters automatisch veranderen naar hoofdletters bij intypen
 */

void loop(){}

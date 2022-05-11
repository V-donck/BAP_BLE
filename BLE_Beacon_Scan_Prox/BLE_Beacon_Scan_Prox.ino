#include <ESP32Time.h>


#include <Arduino.h>

#include <NimBLEDevice.h>
#include <NimBLEAdvertisedDevice.h>
#include "NimBLEBeacon.h"
#include <WiFi.h>
#include <WiFiClient.h>
#include <WiFiAP.h>
#include <ESP32Servo.h>


// Constants
int scanTime = 5; //In seconds
const char *ssid = "SmartFeeder";
const char *password = "password";
WiFiServer server(80);
const int LISTLENGTH = 1000;
Servo servo1;
Servo servo2;
int maxlogs = 1000;

// some global variables
BLEScan *pBLEScan;
int threshold = -70;
uint16_t lijst1[LISTLENGTH];
uint16_t lijst2[LISTLENGTH];
int index1 = 0;
int index2 = 0;
int closeFood = 0;
int openFood = 180;
boolean food1Open = false;
boolean food2Open = false;
boolean notFood1 = false;
boolean notFood2 = false;
boolean allowAllFood1 = false;
boolean allowAllFood2 = false;
boolean loggerspage = false;
ESP32Time rtc;



struct logid{
  String timelog;
  uint16_t id;
  byte feeder;
};

logid loglist[1000];
int count = 0;

// Tasks
TaskHandle_t Task1;
TaskHandle_t Task2;


//function declarations
boolean checkArray(uint16_t id, int nummer);
boolean checkId(String idlogger);
boolean checkThreshold(String threshold);
void removeZeros(byte nummer);
void Task1code( void * pvParameters );
void Task2code( void * pvParameters );


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
        int RSSi = advertisedDevice->getRSSI();
        if (RSSi >= threshold) {
          std::string pr = "Prox";
          if (namedevice == pr) {
            Serial.print("Device name: ");
            Serial.println(advertisedDevice->getName().c_str());
            Serial.println("");
            Serial.println("found Prox!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!");
            Serial.println("");
            Serial.println("");
            Serial.println(RSSi);
            Serial.println("rrsi");
            if (advertisedDevice->haveManufacturerData())
            {
              std::string strManufacturerData = advertisedDevice->getManufacturerData();
              uint8_t cManufacturerData[20];
              char be[8];
              char le[4];
              strManufacturerData.copy((char *)cManufacturerData, strManufacturerData.length(), 0);
              Serial.printf("strManufacturerData: %d ", strManufacturerData.length());
              Serial.printf("\n");
              Serial.println(cManufacturerData[2]);
              sprintf(le, "%X", cManufacturerData[2]);
              sprintf(be, "%X", cManufacturerData[3]);
              strcat(be, le);
              Serial.println(be);
              uint16_t receivedId;
              char * pEnd;
              receivedId = strtol(be, &pEnd, 16);
              Serial.println(receivedId);
              if (count<maxlogs){
                loglist[count] = {rtc.getTime("%d-%m-%Y, %H:%M:%S"),receivedId,5};
                count++;
              }
              if (checkArray(receivedId, 1)) {
                food1Open = true;
              }
              else {
                notFood1 = true;
              }
              if (checkArray(receivedId, 2)) {
                food2Open = true;
              }
              else {
                notFood2 = true;
              }
            }
          }
        }
      }
      return;
    }
};



void setup_ble() {
  rtc.setTime(0, 0, 0, 1, 1, 2000);
  Serial.println("Scanning...");
  BLEDevice::init("");
  pBLEScan = BLEDevice::getScan(); //create new scan
  pBLEScan->setAdvertisedDeviceCallbacks(new MyAdvertisedDeviceCallbacks());
  pBLEScan->setActiveScan(true); //active scan uses more power, but get results faster
  pBLEScan->setInterval(800); // here was 100
  pBLEScan->setWindow(300); // less or equal setInterval value
}

void setup_wifi() {
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
  servo1.attach(12);
  servo2.attach(A0);

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
void Task1code( void * pvParameters ) {
  // here led weg
  pinMode(LED_BUILTIN, OUTPUT);
  Serial.print("Task1 running on core ");
  Serial.println(xPortGetCoreID());

  setup_ble();

  // this is an infinite loop that scans for BLE devices
  for (;;) {
    BLEScanResults foundDevices = pBLEScan->start(scanTime, false);
    Serial.print("Devices found: ");
    Serial.println(foundDevices.getCount());
    Serial.println("Scan done!");

    if (!(allowAllFood1 | allowAllFood2)) {// if no allowall
      // if one of them, then open it, otherwise close both
      if (!(food1Open && food2Open)) {
        if (food1Open) {
          servo1.write(openFood);
          servo2.write(closeFood);
          Serial.println("food1 open");
        }
        else if (food2Open) {
          servo2.write(openFood);
          servo1.write(closeFood);
          Serial.println("food2 open");
        }
        else {
          Serial.println("allebei gesloten");
          servo1.write(closeFood);
          servo2.write(closeFood);
        }
      }
      else {
        Serial.println("allebei moeten open, dus allebei gesloten");
        servo1.write(closeFood);
        servo2.write(closeFood);
      }
    }
    else if (allowAllFood1 & allowAllFood2) { // both all allowed
      if (food1Open | food2Open | notFood1 | notFood2) { // animal in the neighborhood
        servo1.write(openFood);
        servo2.write(openFood);
        Serial.println("food1 open");
        Serial.println("food2 open");
        Serial.println("both");

      }
      else { // no animalin the neighborhood
        servo1.write(closeFood);
        servo2.write(closeFood);
        Serial.println("closed no animal in the neighborhood");
      }
    }
    else if (allowAllFood1) { // Food 1 allowed for all
      if (food2Open & !notFood2) { // only food2
        servo1.write(closeFood);
        servo2.write(openFood);
        Serial.println("open 2, terwijl 1 voor iedereen mag, maar enkel een food2 in de buurt");
      }
      else if (food2Open | !(food1Open | notFood1)) { //there is an animal that must have food2 or no animal in the neighborhood
        servo1.write(closeFood);
        servo2.write(closeFood);
        Serial.println("allebei gesloten  there is an animal that must have food2 or no animal in the neighborhood");
      }
      else { // only animals in Food1 or in no list
        servo1.write(openFood);
        servo2.write(closeFood);
        Serial.println("food1 open");
      }
    }
    else if (allowAllFood2) { // Food 2 allowed for all
      if (food1Open & !notFood1) { // only Food1
        servo1.write(openFood);
        servo2.write(closeFood);
        Serial.println("food1 open, terwijl 2 voor iedereen openstaat, maar enkel animals van food1 aanwezig");
      }
      else if (food1Open | !(food2Open | notFood2)) { //there is an animal that must have food1 or no animal in the neighborhood
        servo1.write(closeFood);
        servo2.write(closeFood);
        Serial.println("allebei gesloten there is an animal that must have food1 or no animal in the neighborhood ");
      }
      else { // only animals in food2 or in no list
        servo1.write(closeFood);
        servo2.write(openFood);
        Serial.println("food2 open");
      }
    }
    food1Open = false;
    food2Open = false;
    notFood1 = false;
    notFood2 = false;
    //delay(2000);
    pBLEScan->clearResults(); // delete results fromBLEScan buffer to release memory
  }

}

//Task2code: Handle WiFi
void Task2code( void * pvParameters ) {
  Serial.print("Task2 running on core ");
  Serial.println(xPortGetCoreID());
  setup_wifi();
  boolean badInputError = false;
  //a infinite for loop that creates the webserver
  for (;;) {
    WiFiClient client = server.available();   // listen for incoming clients
    if (client) {                             // if you get a client,
      Serial.println("New Client.");           // print a message out the serial port
      String currentLine = "";                // make a String to hold incoming data from the client
      while (client.connected()) {            // loop while the client's connected
        if (client.available()) {             // if there's bytes to read from the client,
          char c = client.read();             // read a byte
          if (c == '\n') {                    // if the byte is a newline character

            // if the current line is blank, you got two newline characters in a row.
            // that's the end of the client HTTP request, so send a response:
            if (currentLine.length() == 0) {
              // HTTP headers always start with a response code (e.g. HTTP/1.1 200 OK)
              // and a content-type so the client knows what's coming, then a blank line:
              if (!badInputError) {//if popup warning, the header isn't needed
                client.println("HTTP/1.1 200 OK");
                client.println("Content-type:text/html");
              }
              client.println();
              // the content of the HTTP response follows the header:
              //write style and head and text input fields
              client.write(
                //"<style> body { background-color: rgb(213, 215, 215); } form { text-align: left; /*here kan ook centre*/ font-family: verdana; } p { font-family: verdana; margin: 10px; } .button { border: none; color: white; padding: 15px 32px; text-align: center; text-decoration: none; display: inline-block; font-size: 16px; margin-bottom: 5px; margin-right: 5px; cursor: pointer; font-family: verdana; } .submitbutton { border: none; color: white; padding: 10px 20px; text-align: center; text-decoration: none; display: inline-block; font-size: 10px; margin: 4px 10px; cursor: pointer; box-sizing: border-box; font-family: verdana; } .button1 { background-color: #4CAF50; } .button2 { background-color: #008CBA; } .column { float: left; width: 48%; font-family: verdana; margin: 5px; } .row:after { display: table; clear: both; } div {font-family: verdana; } label { display: inline-block; width: 200px; text-align: left; font-family: verdana; margin-left: 10px } .columnlist { float: left; width: 48%; font-family: verdana; height: 200px; margin: 5px } h1 { text-align: center; font-family: verdana; } h2 { margin-left: 10px; }</style><head> <title>SmartFeeder</title></head><body> <h1>Smart feeder</h1> <div class=row> <div class=column> <form action=/form method=GET> <div> <label>Add to Food 1:</label> <input type=text name=AF1 maxlength=5 size=7> <input type=submit class=\"submitbutton button1\"> </div> </form> <form action=/form method=GET> <div> <label>Remove from Food 1:</label> <input type=text name=RF1 maxlength=5 size=7> <input type=submit class=\"submitbutton button1\"> </div> </form> </div> <div class=column> <form action=/form method=GET> <div> <label>Add to Food 2: </label> <input type=text name=AF2 maxlength=5 size=7> <input type=submit class=\"submitbutton button2\"> </div> </form> <form action=/form method=GET> <div> <label>Remove from Food 2: </label> <input type=text name=RF2 maxlength=5 size=7> <input type=submit class=\"submitbutton button2\"> </div> </form> </div> </div>");
                R"===(
                <style>
                  body {
                      background-color: rgb(213, 215, 215);
                  }
                  form {
                      text-align: left;
                      /*here kan ook centre*/
                      font-family: verdana;
                  }
                  p {
                      font-family: verdana;
                      margin: 10px;
                  }
                  .button {
                      border: none;
                      color: white;
                      padding: 15px 32px;
                      text-align: center;
                      text-decoration: none;
                      display: inline-block;
                      font-size: 16px;
                      margin-bottom: 5px;
                      cursor: pointer;
                      font-family: verdana;
                  margin-right: 5px;
                  
                  }
                  .submitbutton {
                      border: none;
                      color: white;
                      padding: 10px 20px;
                      text-align: center;
                      text-decoration: none;
                      display: inline-block;
                      font-size: 10px;
                      margin: 4px 10px;
                      cursor: pointer;
                      box-sizing: border-box;
                      font-family: verdana;
                  }
                  .button1 {
                      background-color: #4CAF50;
                  }
                  .button2 {
                      background-color: #008CBA;
                  }
                  .column {
                      float: left;
                      width: 48%;
                      font-family: verdana;
                      margin: 5px;
                  }
                  .row:after {
                      display: table;
                      clear: both;
                  }
                  div {
                      font-family: verdana;
                  }
                  label {
                      display: inline-block;
                      width: 200px;
                      text-align: left;
                      font-family: verdana;
                      margin-left: 10px
                  }
                  .columnlist {
                      float: left;
                      width: 48%;
                      font-family: verdana;
                      height: 200px;
                      margin: 5px
                  }
                  h1 {
                      text-align: center;
                      font-family: verdana;
                  }
                  h2 {
                      margin-left: 10px;
                  }
                </style>
                <head>
                  <title>SmartFeeder</title>
                </head>)===");
              if (!loggerspage) {
                client.write(R"===(
                <body>
                <div><a href=/LP><button class style="float: right";=\"submitbutton\">loggerpage</button></a>
                <div><a href=/><button class style="float: right";=\"submitbutton\">refresh</button></a>
                    <h1>Smart Feeder</h1>
                
                    <div class=row>
                        <div class=column>
                            <form action=/form method=GET>
                                <div>
                                    <label>Add to Food 1:</label>
                                    <input type=text name=AF1 maxlength=5 size=7>
                                    <input type=submit class="submitbutton button1">
                                </div>
                            </form>
                            <form action=/form method=GET>
                                <div>
                                    <label>Remove from Food 1:</label>
                                    <input type=text name=RF1 maxlength=5 size=7>
                                    <input type=submit class="submitbutton button1">
                                </div>
                            </form>
                        </div>
                        <div class=column>
                            <form action=/form method=GET>
                                <div>
                                    <label>Add to Food 2: </label>
                                    <input type=text name=AF2 maxlength=5 size=7>
                                    <input type=submit class="submitbutton button2">
                                </div>
                            </form>
                            <form action=/form method=GET>
                                <div>
                                    <label>Remove from Food 2: </label>
                                    <input type=text name=RF2 maxlength=5 size=7>
                                    <input type=submit class="submitbutton button2">
                                </div>
                            </form>
                        </div>
                    </div>)===");

                //table with all current id's
                // food 1
                client.write("<div class=row> <div class=columnlist style=background-color:#6eb069;> <h2>Food 1</h2>");
                if (allowAllFood1) {//check if allowAll is enabled for food1
                  client.write("<p> (all allowed");
                  if (!allowAllFood2) {
                    client.write(" exept the animals that are in Food2)</p><br>");
                  }
                  else {
                    client.write(")</p><br>");
                  }
                }
                client.write("<p>");
                for (int i = 0; i < index1; i++) {
                  if (lijst1[i] != 0) {
                    client.print(String(lijst1[i]) + " \n");
                  }
                }
                client.write("</p></div>");
                //food2
                client.write("<div class=columnlist style=background-color:#96D1CD;><h2>Food 2</h2>");
                if (allowAllFood2) {//check if allowAll is enabled for food2
                  client.write("<p> (all allowed");
                  if (!allowAllFood1) {
                    client.write(" exept the animals that are in Food1)</p><br>");
                  }
                  else {
                    client.write(")</p><br>");
                  }
                }
                client.write("<p>");
                for (int i = 0; i < index2; i++) {
                  if (lijst2[i] != 0) {
                    client.print(String(lijst2[i]) + " \n");
                  }
                }
                client.write("</p></div>");


                //buttons
                client.print("<div class=column><a href=/form/CF1><button class=\"button button1\">Clear Food1</button></a>");
                if (allowAllFood1) {
                  client.write("<a href=/form/AAF1><button class=\"button button1\">do not allow anymore all to food1 exept the animals in food2</button></a></div>");
                }
                else {
                  client.write("<a href=/form/AAF1><button class=\"button button1\">allow all to food1 exept the animals in food2</button></a></div>");
                }
                client.write("<div class=column><a href=/form/CF2><button class=\"button button2\">Clear Food2</button></a>");
                if (allowAllFood2) {
                  client.write("<a href=/form/AAF2><button class=\"button button2\">do not allow anymore all to food2 exept the animals infood1</button></a></div>");
                }
                else {
                  client.write("<a href=/form/AAF2><button class=\"button button2\">allow all to food2 exept the animals infood1</button></a></div>");
                }
                client.write("</div>");


                //threshold
                client.print("<p>Current threshold : ");
                client.print(String(threshold));
                client.write(" dB</p>");
                client.write(" <form action=/form method=GET> <div> <label>Set threshold: </label> <input type=text name=ST maxlength=5 size=7> <input type=submit class=submitbutton style=background-color:#3f3f3e> </div> </form></body>");

                badInputError = false;

                Serial.println("einde normal request");

                // The HTTP response ends with another blank line:
                //client.println();// here was not comment
                // break out of the while loop:
                // break;// here was break
              }
              else // logerpage // here edit
              {
                badInputError = false;
                client.write(R"===(
                <body>
                    <div><a href=/B><button class style="float: right";=\"submitbutton\">back</button></a></div>
                    <h1>logs</h1>
                    <form action=/form method=GET>
                                <div>
                                    <label>set time:</label>
                                    <input type=text name=AF1 maxlength=5 size=7>
                                    <input type=submit class="submitbutton button1">
                                </div>
                            </form>)===");
                           client.write("<div><p>");
                for (int i = 0; i<count;i++) {
                  logid idlog = loglist[i];
                    client.print(idlog.timelog + "; " + idlog.id + "; " + idlog.feeder + "<br>");
                  }
                
                client.write("</p></div></body>"); 
                    
                    
                    
              }
            } else {    // if you got a newline, then clear currentLine:
              currentLine = "";
            }
          } else if (c != '\r') {  // if you got anything else but a carriage return character,
            currentLine += c;      // add it to the end of the currentLine
          } else if (c == '\r') {
            currentLine = "";
          }

          if (currentLine.endsWith("HTTP/1.1")) {

            //add id to Food1
            if (currentLine.startsWith("GET /form?AF1")) {
              if (index1 < LISTLENGTH - 1) {
                //Serial.println(currentLine);
                String idlogger = currentLine.substring(currentLine.indexOf('=') + 1, currentLine.indexOf(' ', currentLine.indexOf('=')));
                if (checkId(idlogger)) {
                  long idlong = idlogger.toInt();
                  if (idlong > 65535) {
                    client.print("<script>alert(\"deze id is niet juist\");</script>");
                    badInputError = true;
                  }
                  else {
                    uint16_t idInt = idlong;
                    if (!checkArray(idInt, 1)) {
                      lijst1[index1] = idInt;
                      index1++;
                      Serial.println("added to list1");
                    }
                    else {
                      client.println("<script>alert(\"deze id zit al in deze lijst\");</script>");
                      badInputError = true;
                    }
                    if (checkArray(idInt, 2)) {
                      client.print("<script>alert(\"deze id zit al in de andere lijst\");</script>");
                      badInputError = true;
                    }
                  }
                }
                else {
                  client.print("<script>alert(\"deze id is niet juist\");</script>");
                  badInputError = true;
                }
              }
              else {
                client.print("<script>alert(\"Er zitten te veel id's in deze lijst\");</script>");
                badInputError = true;
              }
            }


            //add id to Food2
            if (currentLine.startsWith("GET /form?AF2")) {
              if (index2 < LISTLENGTH - 1) {
                String idlogger = currentLine.substring(currentLine.indexOf('=') + 1, currentLine.indexOf(' ', currentLine.indexOf('=')));
                Serial.println(idlogger);
                if (checkId(idlogger)) {
                  long idlong = idlogger.toInt();
                  if (idlong > 65535) {
                    client.print("<script>alert(\"deze id is niet juist\");</script>");
                    badInputError = true;
                  }
                  else {
                    uint16_t idInt = idlong;
                    if (!checkArray(idInt, 2)) {
                      lijst2[index2] = idInt;
                      index2++;
                      Serial.println("added to list2");
                    }
                    else {
                      client.print("<script>alert(\"deze id zit al in deze lijst\");</script>");
                      badInputError = true;
                    }
                    if (checkArray(idInt, 1)) {
                      client.print("<script>alert(\"deze id zit al in de andere lijst\");</script>");
                      badInputError = true;
                    }
                  }
                }
                else {
                  client.print("<script>alert(\"deze id is niet juist\");</script>");
                  badInputError = true;
                }
              }
              else {
                client.print("<script>alert(\"Er zitten te veel id's in deze lijst\");</script>");
                badInputError = true;
              }
            }

            boolean inlist = false;
            boolean removeF = false;

            //remove from Food1
            if (currentLine.startsWith("GET /form?RF1")) {
              removeF = true;
              String idlogger = currentLine.substring(currentLine.indexOf('=') + 1, currentLine.indexOf(' ', currentLine.indexOf('=')));
              Serial.println(idlogger);
              if (checkId(idlogger)) {
                long idlong = idlogger.toInt();
                if (idlong > 65535) {
                  client.print("<script>alert(\"deze id is niet juist\");</script>");
                  badInputError = true;
                }
                else {
                  uint16_t idInt = idlong;
                  for (int i = 0; i < index1; i++) {
                    if (lijst1[i] == idInt) {
                      lijst1[i] = 0;
                      inlist = true;
                    }
                  }
                }
              }
              else {
                client.print("<script>alert(\"deze id is niet juist\");</script>");
                badInputError = true;
              }
            }

            //remove from Food2
            if (currentLine.startsWith("GET /form?RF2")) {
              removeF = true;
              String idlogger = currentLine.substring(currentLine.indexOf('=') + 1, currentLine.indexOf(' ', currentLine.indexOf('=')));
              Serial.println(idlogger);
              if (checkId(idlogger)) {
                long idlong = idlogger.toInt();
                if (idlong > 65535) {
                  client.print("<script>alert(\"deze id is niet juist\");</script>");
                  badInputError = true;
                }
                else {
                  uint16_t idInt = idlong;
                  for (int i = 0; i < index2; i++) {
                    if (lijst2[i] == idInt) {
                      lijst2[i] = 0;
                      inlist = true;
                    }
                  }
                }
              }
              else {
                client.print("<script>alert(\"deze id is niet juist\");</script>");
                badInputError = true;
              }
            }

            // warning message that id is not in list
            if (!inlist & removeF) {
              client.print("<script>alert(\"deze id zit niet in deze lijst\");</script>");
              badInputError = true;
            }

            //set Threshold
            if (currentLine.startsWith("GET /form?ST")) {
              String thresholdstring = currentLine.substring(currentLine.indexOf('=') + 1, currentLine.indexOf(' ', currentLine.indexOf('=')));

              if (checkThreshold(thresholdstring)) {
                threshold = thresholdstring.toInt();
              }
              else {
                client.print("<script>alert(\"deze threshold is niet juist\");</script>");
                badInputError = true;
              }
            }


            // remove zeros in list1
            if (index1 > LISTLENGTH - 2 || index1 == LISTLENGTH / 2) {
              removeZeros(1);
            }

            //remove zeros in list2
            if (index2 > LISTLENGTH - 2 || index2 == LISTLENGTH / 2) {
              removeZeros(2);
            }

            // print in serialmonitor
            Serial.println("lijst1:");
            for (int i = 0; i < index1; i++) {
              Serial.print(lijst1[i]);
              Serial.print(", ");
            }
            Serial.println("");

            Serial.println("lijst2:");
            for (int i = 0; i < index2; i++) {
              Serial.print(lijst2[i]);
              Serial.print(", ");
            }
            Serial.println(" ");
          }

          //clear food 1
          if (currentLine.endsWith("GET /form/CF1")) {
            for (int i = 0; i <= index1; i++) {
              lijst1[i] = 0;
            }
            index1 = 0;
          }

          //Allow all to food 1
          if (currentLine.endsWith("GET /form/AAF1")) {
            allowAllFood1 = not(allowAllFood1);
          }

          //clear food 2
          if (currentLine.endsWith("GET /form/CF2")) {
            for (int i = 0; i <= index2; i++) {
              lijst2[i] = 0;
            }
            index2 = 0;
          }

          //Allow all to food 2
          if (currentLine.endsWith("GET /form/AAF2")) {
            allowAllFood2 = not(allowAllFood2);
          }

          //loggerpage
          if (currentLine.endsWith("GET /LP")) {
            loggerspage = true;
          }

          //back to normal
          if (currentLine.endsWith("GET /B")) {
            loggerspage = false;
          }




          //end of HTTP page
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
    //delay(2000); //here was 5000
  }
}


boolean checkArray(uint16_t id, int nummer) {
  if (nummer == 1) {
    for (int i = 0; i < index1; i++) {
      if (lijst1[i] == id) {
        return true;
      }
    }
    return false;
  }
  else if (nummer == 2) {
    for (int i = 0; i < index2; i++) {
      if (lijst2[i] == id) {
        return true;
      }
    }
    return false;
  }
  return false;
}

//check if string is int
boolean checkId(String s) {
  if (s.length() == 0) {
    return false;
  }
  for (int i = 0; i < s.length(); i++) {
    if (!isDigit(s.charAt(i))) {
      return false;
    }
  }
  return true;
}

boolean checkThreshold(String s) {
  if (s.charAt(0) != '-') {
    return false;
  }
  for (int i = 1; i < s.length(); i++) {
    if (!isDigit(s.charAt(i))) {
      return false;
    }
  }
  return true;
}

void removeZeros(byte nummer) {
  if (nummer == 0) {
    for (int i = 0; i < 2; i++) {
      removeZeros(i);
    }
  }
  else if (nummer == 1) {
    int newindex;
    newindex = 0;
    for (int i = 0; i < index1; i++) {
      lijst1[newindex] = lijst1[i];
      if (!lijst1[i] == 0) {
        newindex++;
      }
    }
    index1 = newindex;
  }
  else if (nummer == 2) {
    int newindex;
    newindex = 0;
    for (int i = 0; i < index2; i++) {
      lijst2[newindex] = lijst2[i];
      if (!lijst2[i] == 0) {
        newindex++;
      }
    }
    index2 = newindex;
  }
}


// to do
/*
   VV melding als verwijder maar niet in lijst -> nu komen de eerste 2 lijnen er bij op die nog bij de header horen, .write werkt niet
   VV melding als al in andere lijst
   VV               lijst toevoegen aan webpagina
   andere rommel eruit
   VV          error bij 100 dieren
   VV error bij foute naam
   VV            lijst met 0 leegmaken
   miss lijst in eeprom zetten??? daarnaar kijken
   VV      kleine letters automatisch veranderen naar hoofdletters bij intypen
   vvkijken of int die ingetyped is niet te groot is
   VV bij 2 voederbakken geen enkele open
   VVVthreshold
   VV in webserver threshold instellen
   vvin webserver zeggen bv alles 1

mooiere buttons,
set time
refresh button bij logs
miss downloadbaar
feeder open or closed -> eerst op het juiste zetten, en dan in de open/close terug op 0 zetten als die toch dicht moest


   
*/

void loop() {}

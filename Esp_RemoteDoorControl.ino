#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266WiFiMulti.h> 
#include <ESP8266mDNS.h>
#include <ESP8266WebServer.h>
#include <EEPROM.h>

#define BARKER 0x1234

//const char* ssid = "TP-LINK_AP_9C2C";             //Add your WiFi ssid
//const char* password =  "Danik57WIFIsafe!12";    //Add your WiFi password

#define PIN_LATCH_POWER 4   // latch power
#define PIN_IR_DETECTOR_EN 5
#define PIN_ENGINE_UP 14
#define PIN_ENGINE_DOWN 12
#define PIN_DOOR_CLOSE_BTN 13
#define SM_ITER_TIME 10
#define MAX_IDLE_TIME 150000
#define ROTATION_TIMEOUT 120000
 const char* ssid = "Arnon";             //Add your WiFi ssid
 const char* password =  "ar0765401922";    //Add your WiFi password
const int led = 2;

String Daniel_apiKey = "129959";              //Add your Token number that bot has sent you on WhatsApp messenger
String Daniel_phone_number = "+972542565256"; //Add your WhatsApp app registered phone number (same number that bot send you in url)

String Roni_apiKey = "9331675";              //Add your Token number that bot has sent you on WhatsApp messenger
String Roni_phone_number = "+972544682818"; //Add your WhatsApp app registered phone number (same number that bot send you in url)

int gBtn_isValid = 0; // we don't know if the btn is valid yet.
int gSM_state = 0;
unsigned int idle_time = 0;
const int port = 443;//SSL_PORT; // server port number

String url;                            //url String will be used to store the final generated URL
//std::unique_ptr<BearSSL::WiFiClientSecure> client(new BearSSL::WiFiClientSecure);
ESP8266WebServer server(80);    // Create a webserver object that listens for HTTP request on port 80
String newHostname = "ESP_DoorController-DD";
IPAddress subnet(255, 255, 255, 0);                  // Subnet Mask
IPAddress gateway(10, 100, 102, 1);                  // Default Gateway
IPAddress local_IP(10, 100, 102, 253);             // Static IP Address for esp8266

int RotationPol = 1;
unsigned int ReverseHoldOff = 2000;

void RunEngine(int direction);
void Handle_SM();
void ShutDown();

void eeprom_handler()
{
    unsigned int data;
    EEPROM.begin(12);
    EEPROM.get(0, data);
    if ( data != BARKER)
    {
        // EEPROM is empty
        Serial.println("EEPROM empty");
        EEPROM.put(0, BARKER);
        EEPROM.put(4, RotationPol);
        EEPROM.put(8, ReverseHoldOff);
        EEPROM.commit();
        return;
    }
    else{
        EEPROM.get(4, RotationPol);
        EEPROM.get(8, ReverseHoldOff);
        Serial.println("RotationPol and ReverseHoldOff = ");
        Serial.print(RotationPol);
        Serial.print(" ");
        Serial.print(ReverseHoldOff);
    }
    EEPROM.end();
}

void setup() 
{
    Serial.begin(115200);
    
    eeprom_handler();
    pinMode(PIN_LATCH_POWER, OUTPUT);
    digitalWrite(PIN_LATCH_POWER, HIGH);
    pinMode(PIN_DOOR_CLOSE_BTN, INPUT_PULLUP); // 1 - is open, 0 - is close
    pinMode(PIN_IR_DETECTOR_EN, OUTPUT);
    pinMode(PIN_ENGINE_UP, OUTPUT);
    pinMode(PIN_ENGINE_DOWN, OUTPUT);
    digitalWrite(PIN_IR_DETECTOR_EN, 0);

    WiFi.mode(WIFI_STA);
    WiFi.setPhyMode(WIFI_PHY_MODE_11G);
    if (WiFi.config(local_IP, gateway, subnet)) {
      Serial.println("Static IP Configured");
    }
    else {
      Serial.println("Static IP Configuration Failed");
    }
  
    WiFi.begin(ssid, password);   
   
    int8_t status = WiFi.waitForConnectResult(30000);
    if(status == WL_CONNECTED)
    {
       // Serial.println("Connected");
         Serial.println(WiFi.SSID());              // Tell us what network we're connected to
         delay(100);
       // Serial.print("IP address:\t");
        Serial.println(WiFi.localIP());           // Send the IP address of the ESP8266 to the computer

      

        server.on("/", HTTP_GET, handleRoot);     // Call the 'handleRoot' function when a client requests URI "/"
        server.on("/turnon", handleTurnOn);       // New handler for turning on the output
        server.on("/reverse", HTTP_GET, handleRevese);// New handler for reverse turn to ease the stress
        server.on("/check_password", HTTP_POST, handlePasswordCheck);
        server.on("/config", handleConfigPage);            // Interior page with configuration options
        server.on("/apply_config", HTTP_POST, handleApplyConfig);     // Apply configuration
       // server.on("/LED", HTTP_POST, handleLED);  // Call the 'handleLED' function when a POST request is made to URI "/LED"
        server.onNotFound(handleNotFound);        // When a client requests an unknown URI (i.e. something other than "/"), call function "handleNotFound"

        server.begin();                           // Actually start the server
  
    }

}

void loop() {
  // wait for WiFi connection
    server.handleClient();                    // Listen for HTTP requests from clients
    Handle_SM();

}

void handleApplyConfig() {
  if (server.method() == HTTP_POST) {
    String checkboxState = server.arg("checkbox");
    String numberValue = server.arg("numberInput");

    // You can now process these values as needed
    Serial.println("Checkbox State: " + checkboxState);
    delay(100);
    Serial.println("Number Value: " + numberValue);

    String response = "Configuration Applied: <br>";
    response += "Checkbox State: " + checkboxState + "<br>";
    response += "Number Value: " + numberValue;

    RotationPol = (checkboxState=="1") ? 1 : -1;
    ReverseHoldOff =  abs(numberValue.toInt());

    gSM_state = 4;
    server.send(200, "text/plain", response);
  }
}


void handleConfigPage() {
    String checkboxStr="";
    String holdoffValueStr = String(ReverseHoldOff);
    if(RotationPol == 1)
        checkboxStr ="checked";

  String page = "<html><body>";
  page += "<h1>Configuration Page</h1>";
  page += "<form id='configForm'>";
  page += "<label for='checkbox'  style=\"font-size: 24px;\">Positive rotation: </label>";
  page += "<input type='checkbox' style=\"width: 100px; height: 40px; font-size: 20px; padding: 10px;\" id='checkbox' "+checkboxStr +"><br><br>";
  page += "<label for='numberInput' style=\"font-size: 24px;\">Reverse hold off time[ms]: </label>";
  page += "<input type='number' style=\"width: 150px; height: 40px; font-size: 20px; padding: 10px;\" id='numberInput' min='0' max='10000' value='"+holdoffValueStr+"'><br><br>";
  page += "<input type='button' style=\"width: 200px; height: 40px; font-size: 20px; padding: 10px;\" value='Apply' onclick='applyConfig()'>";
  page += "<input type='button' style=\"width: 200px; height: 40px; font-size: 20px; padding: 10px;\" value='Home' onclick=\"location.href='/'\">";
  page += "</form>";
  page += "<div id='response'></div>";
  page += "<script>";
  page += "function applyConfig() {";
  page += "  var xhr = new XMLHttpRequest();";
  page += "  xhr.open('POST', '/apply_config', true);";
  page += "  xhr.setRequestHeader('Content-Type', 'application/x-www-form-urlencoded');";
  page += "  xhr.onreadystatechange = function() {";
  page += "    if (xhr.readyState == 4 && xhr.status == 200) {";
  page += "      document.getElementById('response').innerHTML = xhr.responseText;";
  page += "    }";
  page += "  };";
  page += "  var checkbox = document.getElementById('checkbox').checked ? 1 : 0;";
  page += "  var numberInput = document.getElementById('numberInput').value;";
  page += "  xhr.send('checkbox=' + checkbox + '&numberInput=' + numberInput);";
  page += "}";
  page += "</script>";
  page += "</body></html>";

  server.send(200, "text/html", page);
}



void handleRoot() {                         // When URI / is requested, send a web page with a button to toggle the LED
    String color="#00ff00"; //green
  if(digitalRead(PIN_DOOR_CLOSE_BTN)) 
    color="#ff0000"; //red
  String html = "<!DOCTYPE html><html><head><title>ESP8266 Web Server</title></head><body>";
  html += "<h1>ESP8266 Web Server ver 3</h1>";
  html += "<div style='display: flex; flex-direction: column; align-items: flex-start; padding: 20px;'>";  // Container with flexbox
  html += "<button id='turnButton' style='font-size: 100px; padding: 57px; background-color: "+color+"; margin-bottom: 20px;' onclick='turnOn()'>Open door</button>";
  html += "<button id='RevButton' style='font-size: 50px; padding: 26px; margin-bottom: 20px; background-color: #33fff9' onclick='revTurn()'>Reverse</button>";
  html += "<input type='password' id='pwd' style=\"width: 200px; height: 40px; font-size: 20px; padding: 10px;\"'><input type='button' style=\"width: 200px; height: 40px; font-size: 20px; padding: 10px;\" value='Settings' onclick='checkPassword()'>";
  html += "</form>";
  html += "<div id='response'></div>";
  html += "</div>";
  html += "<script>function turnOn(){var xhr=new XMLHttpRequest();xhr.open('GET','/turnon',true);xhr.send();document.getElementById('turnButton').style.backgroundColor='#ff0000';}</script>";
  html += "<script>function revTurn(){var xhr=new XMLHttpRequest();xhr.open('GET','/reverse',true);xhr.send();}</script>";
    // JavaScript to handle the AJAX request
  html += "<script>";
  html += "function checkPassword() {";
  html += "  var xhr = new XMLHttpRequest();";
  html += "  xhr.open('POST', '/check_password', true);";
  html += "  xhr.setRequestHeader('Content-Type', 'application/x-www-form-urlencoded');";
  html += "  xhr.onreadystatechange = function() {";
  html += "    if (xhr.readyState == 4 && xhr.status == 200) {";
  html += "      var response = xhr.responseText;";
  html += "      document.getElementById('response').innerHTML = response;";

// Check if the password is correct
  html += "      if (response === 'Password Correct') {";
  html += "        window.location.href = '/config';";
  html += "      }";
  html += "    }";
  html += "  };";
  html += "  var pwd = document.getElementById('pwd').value;";
  html += "  xhr.send('pwd=' + encodeURIComponent(pwd));";
  html += "}";
  html += "</script>";
  html += "</body></html>";

  server.send(200, "text/html", html);
}

void handlePasswordCheck() {
  String passwordInput = server.arg("pwd");
  if (passwordInput == "daniel") {
     server.send(200, "text/plain", "Password Correct");
  } else {
    server.send(200, "text/plain", "Incorrect Password");
  }
}

void handleTurnOn() {
    String response = "";
    if(!digitalRead(PIN_DOOR_CLOSE_BTN))    
    {
        response = "Going to open";
        Serial.println("Going to open");
        gSM_state = 1;
        idle_time = 0;
        response = "Ack";
    }
    else
    {
       gSM_state = 0;
       Serial.println("Already open");
       response = "Already open";
    }
    server.send(200, "text/plain", response);
}

void handleRevese()
{
    //Serial.println("hello world gm ");
    //Serial.print(gSM_state);
    String response = "Can't now";
    if(gSM_state==0)
    {
        RunEngine(-1);
        delay(1000);
        RunEngine(0);
        response = "Ack";
    }
    server.send(200, "text/plain", response);
}

void handleNotFound(){
  server.send(404, "text/plain", "404: Not found"); // Send HTTP status 404 (Not Found) when there's no handler for the URI in the request
}


void Handle_SM()
{
    unsigned int rotation_timeout = ( (ReverseHoldOff*13) /10 ) + 2400; 
    delay(SM_ITER_TIME);
    switch(gSM_state)
    {
        case 0:
            idle_time++;
            if(idle_time >= MAX_IDLE_TIME/SM_ITER_TIME)
                ShutDown();
            RunEngine(0);
            // if( !digitalRead(PIN_DOOR_CLOSE_BTN)) // both button and IR signals the door is CLOSE -> the Button is valid to use
            //     gBtn_isValid = 1;
        break;
        case 1: // open the door
            RunEngine(1);
            idle_time++;
            if((idle_time >= rotation_timeout/SM_ITER_TIME) ||
                    (digitalRead(PIN_DOOR_CLOSE_BTN)) )
            {
                RunEngine(0);
                gSM_state = 2;
            }
        break;
        case 2:
            Serial.println("Starting reverse");
            RunEngine(-1);
            delay(ReverseHoldOff);
            Serial.println("Off engine");
            gSM_state = 0;
            ShutDown();
           
        break;
        case 3:
            Serial.println("time pass");
            break;
        case 4:
            EEPROM.begin(12);
            EEPROM.put(4, RotationPol);
            EEPROM.put(8, ReverseHoldOff);
            EEPROM.commit();
            EEPROM.end();
            gSM_state = 0;
        default:
            RunEngine(0);
            break;
    }
}

        
void RunEngine(int direction)
{
    direction *= RotationPol;
    switch(direction)
    {
        case 1:
            digitalWrite(PIN_ENGINE_UP, 1);
            delay(1);
            digitalWrite(PIN_ENGINE_DOWN, 0);
        break;
        case -1:
            digitalWrite(PIN_ENGINE_UP, 0);
            delay(1);
            digitalWrite(PIN_ENGINE_DOWN, 1);
        break;
        case 0:
        default:
            digitalWrite(PIN_ENGINE_UP, 0);
            digitalWrite(PIN_ENGINE_DOWN, 0);
        break;
    }
}

void ShutDown()
{
    WiFi.disconnect(); 
    WiFi.mode(WIFI_OFF);
    delay(100);
    ESP.deepSleep(0); 
}
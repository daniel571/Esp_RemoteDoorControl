#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266WiFiMulti.h> 
#include <ESP8266mDNS.h>
#include <ESP8266WebServer.h>

const char* ssid = "TP-LINK_AP_9C2C";             //Add your WiFi ssid
const char* password =  "Danik57WIFIsafe!12";    //Add your WiFi password

#define PIN_IR_DETECTOR_IN 4
#define PIN_IR_DETECTOR_EN 5
#define PIN_ENGINE_UP 14
#define PIN_ENGINE_DOWN 12
#define SM_ITER_TIME 10
#define MAX_IDLE_TIME 150000
// const char* ssid = "Arnon";             //Add your WiFi ssid
// const char* password =  "ar0765401922";    //Add your WiFi password
const int led = 2;

String Daniel_apiKey = "129959";              //Add your Token number that bot has sent you on WhatsApp messenger
String Daniel_phone_number = "+972542565256"; //Add your WhatsApp app registered phone number (same number that bot send you in url)

String Roni_apiKey = "9331675";              //Add your Token number that bot has sent you on WhatsApp messenger
String Roni_phone_number = "+972544682818"; //Add your WhatsApp app registered phone number (same number that bot send you in url)

int gSM_state = 0;
unsigned int idle_time = 0;
const int port = 443;//SSL_PORT; // server port number

String url;                            //url String will be used to store the final generated URL
//std::unique_ptr<BearSSL::WiFiClientSecure> client(new BearSSL::WiFiClientSecure);
ESP8266WebServer server(80);    // Create a webserver object that listens for HTTP request on port 80
String newHostname = "ESP_DoorController-DD";
IPAddress subnet(255, 255, 255, 0);                  // Subnet Mask
IPAddress gateway(192, 168, 1, 1);                  // Default Gateway
IPAddress local_IP(192, 168, 1, 253);             // Static IP Address for esp8266


void RunEngine(int direction);
void Handle_SM();
void ShutDown();

void setup() 
{
    Serial.begin(115200);
    pinMode(PIN_IR_DETECTOR_IN, INPUT);
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

void handleRoot() {                         // When URI / is requested, send a web page with a button to toggle the LED
    String color="#00ff00"; //green
  if(digitalRead(PIN_IR_DETECTOR_IN)) 
    color="#ff0000"; //red
  String html = "<!DOCTYPE html><html><head><title>ESP8266 Web Server</title></head><body>";
  html += "<h1>ESP8266 Web Server ver 2</h1>";
  html += "<div style='display: flex; flex-direction: column; align-items: flex-start; padding: 20px;'>";  // Container with flexbox
  html += "<button id='turnButton' style='font-size: 100px; padding: 57px; background-color: "+color+"; margin-bottom: 20px;' onclick='turnOn()'>Open door</button>";
  html += "<button id='RevButton' style='font-size: 50px; padding: 26px; background-color: #33fff9' onclick='revTurn()'>Reverse</button>";
  html += "</div>";
  html += "<script>function turnOn(){var xhr=new XMLHttpRequest();xhr.open('GET','/turnon',true);xhr.send();document.getElementById('turnButton').style.backgroundColor='"+color+"';}</script>";
  html += "<script>function revTurn(){var xhr=new XMLHttpRequest();xhr.open('GET','/reverse',true);xhr.send();}</script>";
  html += "</body></html>";

  server.send(200, "text/html", html);
}

void handleTurnOn() {
    String response = "";
    if(!digitalRead(PIN_IR_DETECTOR_IN))    
    {
        response = "Going to open";
        Serial.println("Going to open");
        gSM_state = 1;
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
    delay(SM_ITER_TIME);
    switch(gSM_state)
    {
        case 0:
            idle_time++;
            if(idle_time >= MAX_IDLE_TIME/SM_ITER_TIME)
                ShutDown();
            RunEngine(0);
        break;
        case 1: // open the door
            RunEngine(1);
            if(digitalRead(PIN_IR_DETECTOR_IN)) 
            {
                RunEngine(0);
                gSM_state = 2;
            }
        break;
        case 2:
            Serial.println("Starting reverse");
            RunEngine(-1);
            delay(2000);
            Serial.println("Off engine");
            gSM_state = 0;
            ShutDown();
           
        break;
        case 3:
            Serial.println("time pass");
            break;
        default:
            RunEngine(0);
            break;
    }
}

        
void RunEngine(int direction)
{
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
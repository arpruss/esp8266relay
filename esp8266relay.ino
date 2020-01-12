#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266mDNS.h>
#include <ESP8266WebServer.h>

#define MAX_TRIES    3
#define TRY_TIME     60000

uint32 numTries = 0;
uint32 lastTries[MAX_TRIES] = {0};

#include "c:/users/alexander_pruss/Arduino/private.h"

#define OPEN_TIME 333

#define RELAY     D1
#define RELAY_ON  1
#define RELAY_OFF 0

#define LED 2
#define LED_ON 0
#define LED_OFF 1

ESP8266WebServer server(80);    

void handleRoot();              
void handlePost();
void handleNotFound();


void setup(void){
  Serial.begin(115200);         // Start the Serial communication to send messages to the computer
  delay(10);
  Serial.println('\n');
  pinMode(LED,OUTPUT);
  digitalWrite(LED,LED_OFF);
  pinMode(RELAY,OUTPUT);
  digitalWrite(RELAY,RELAY_OFF);

  IPAddress myIP(RELAY_IP);
  IPAddress myGateway(RELAY_GATEWAY);
  IPAddress mySubnet(RELAY_SUBNET);

  WiFi.begin(ssid, psk);
  WiFi.config(myIP, myGateway, mySubnet);

  Serial.println("Connecting ...");

  while (WiFi.status() != WL_CONNECTED) {
    delay(250);
    Serial.print('.');
  }
  Serial.println('\n');
  Serial.println("Connected!");

  /* MDNS.begin("garageRELAY"); */

  server.on("/", HTTP_GET, handleRoot);              
  server.on("/RELAY", HTTP_POST, handlePost);
  server.onNotFound(handleNotFound);

  server.begin();                           // Actually start the server
  Serial.println("HTTP server started");
}

void loop(void){
  server.handleClient();                    // Listen for HTTP requests from clients
}

bool checkTry() {
  if (numTries >= MAX_TRIES && millis() < lastTries[0] + TRY_TIME) {
     return false;
  }

  return true;
}

void addTry() {
  if (numTries == MAX_TRIES) {
    memmove(lastTries,lastTries+1,sizeof(lastTries[0])*(MAX_TRIES-1));
    numTries--;
  }
  lastTries[numTries++] = millis();
}

void openSesame(){
  digitalWrite(LED, LED_ON);
  digitalWrite(RELAY, RELAY_ON);
  delay(OPEN_TIME);
  digitalWrite(LED, LED_OFF);
  digitalWrite(RELAY, RELAY_OFF);
}

void handleRootWithPrefix(String prefix) {
  String msg = "<html><title>Enter Code</title><body>";
  String code = "";
  
  if (! checkTry()) {
    server.send(200, "text/html", 
    "<html><title>Time Out</title><body>"
    "Too many tries.<br/><a href='/'>Try again later.</a>"
    "</body></html");
    return;
  }

  server.send(200, "text/html", 
  prefix + "<form action='/RELAY' method='post'>"
  "<input type='number' name='code' value='" + code + "'/><br/>"
  "<input type='submit' value='Submit'/><br/>"
  "</form>"
  "</body></html>");
}

void handleRoot() {
  handleRootWithPrefix("");
}

void handlePost() {
  if (! checkTry()) {
    handleRoot();
    return;
  }
  
  if (server.args() == 1 && server.argName(0) == "code") {
    addTry(); 
    String code = server.arg(0);
    if (code == RELAY_PIN) {
      server.send(200, "text/html", 
        "<html><title>Success</title><body>"
        "Sesame opening.<br/><a href='/'>Try again.</a>"
        "</body></html>");
      openSesame();
      numTries = 0;
      return;
    }    
    else {
      handleRootWithPrefix("<font color='red'>Incorrect entry.</font><br/>");
    }
  }
}

void handleNotFound(){
  server.send(404, "text/plain", "404: Not found"); // Send HTTP status 404 (Not Found) when there's no handler for the URI in the request
}

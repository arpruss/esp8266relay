//#include <ESP8266WiFi.h>
#include <WiFiClient.h>
//#include <ESP8266mDNS.h>
#include <ESP8266WebServer.h>
#include <ESP8266WebServerSecure.h>

#define SUPPORT_HTTP
#define SUPPORT_HTTPS

#define MAX_TRIES    3
#define TRY_TIME     60000
#define FONT_STYLE "style='font-size:6vw;'"
#define FONT_START "<p " FONT_STYLE ">"
#define VIEWPORT "<meta name='viewport' content='width=device-width, initial-scale=1.0'>"
#define FONT_END "</p>"

uint32 numTries = 0;
uint32 lastTries[MAX_TRIES] = {0};

#include "c:/arduino_private/private.h"

#define OPEN_TIME 333

#define RELAY     D1
#define RELAY_ON  0
#define RELAY_OFF 1

#define LED 2
#define LED_ON 0
#define LED_OFF 1

#ifdef SUPPORT_HTTP
ESP8266WebServer server(80);    
#endif
#ifdef SUPPORT_HTTPS
BearSSL::ESP8266WebServerSecure secureServer(443);

static const char serverCert[] PROGMEM = RELAY_CERT_TXT;

static const char serverKey[] PROGMEM =  RELAY_KEY_TXT;
#endif

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

#ifdef SUPPORT_HTTP
  server.on("/", handleRoot);              
  server.on("/RELAY", HTTP_GET, handleRoot);              
  server.on("/RELAY", HTTP_POST, handlePost);
  server.onNotFound(handleNotFound);
  server.begin();                           // Actually start the server
  Serial.println("HTTP server started");
#endif

#ifdef SUPPORT_HTTPS
  configTime(3 * 3600, 0, "pool.ntp.org", "time.nist.gov");
  secureServer.getServer().setRSACert(new BearSSL::X509List(serverCert), new BearSSL::PrivateKey(serverKey));
//  secureServer.on("/", handleRootHTTPS);              
  secureServer.on(RELAY_DIRECTORY, HTTP_GET, handleRootHTTPS);              
  secureServer.on(RELAY_DIRECTORY, HTTP_POST, handlePostHTTPS);
  secureServer.onNotFound(handleNotFoundHTTPS);
  secureServer.begin();
  Serial.println("HTTPS server started");
#endif  
}

void loop(void){
#ifdef SUPPORT_HTTP  
  server.handleClient();                    // Listen for HTTP requests from clients
#endif  
#ifdef SUPPORT_HTTPS  
  secureServer.handleClient();
#endif
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

#ifdef SUPPORT_HTTP
void handleRootWithPrefix(String prefix) {
  String top = "<html>" VIEWPORT "<title>Enter Code</title>" VIEWPORT "<body onload='clickme()'>";
  String code = "";
  
  if (! checkTry()) {
    server.send(200, "text/html", 
    "<html><title>Time Out</title>" VIEWPORT "<body>"
    FONT_START "Too many tries.<br/><a href='/'>Try again later.</a>" FONT_END
    "</body></html");
    return;
  }

  server.send(200, "text/html", 
  top + prefix + "<form action='/RELAY' method='post'>"
  FONT_START "<input " FONT_STYLE " type='number' size='6' name='code' id='code' value='" + code + "' autofocus /><br/>"
  "<input " FONT_STYLE " type='submit' value='Submit'/><br/>" FONT_END
  "</form>"
  "</body>"
  "<script>function clickme() {var el = document.getElementById('code'); el.focus(); el.click(); el.dispatchEvent(new Event('touchstart')); }</script>" 
  "</html>");
}
#endif

#ifdef SUPPORT_HTTPS
void handleRootWithPrefixHTTPS(String prefix) {
  Serial.print("https ");
  Serial.println(prefix);
  String top = "<html>" VIEWPORT "<title>Enter Code</title>" VIEWPORT "<body onload='clickme()'>";
  String code = "";
  
  if (! checkTry()) {
    secureServer.send(200, "text/html", 
    "<html><title>Time Out</title>" VIEWPORT "<body>"
    FONT_START "Too many tries.<br/><a href='/'>Try again later.</a>" FONT_END
    "</body></html");
    return;
  }

  secureServer.send(200, "text/html", 
  top + prefix + "<form action='" + RELAY_DIRECTORY + "' method='post'>"
  FONT_START "<input " FONT_STYLE " type='number' size='6' name='code' id='code' value='" + code + "' autofocus /><br/>"
  "<input " FONT_STYLE " type='submit' value='Submit'/><br/>" FONT_END
  "</form>"
  "</body>"
  "<script>function clickme() {var el = document.getElementById('code'); el.focus(); el.click(); el.dispatchEvent(new Event('touchstart')); }</script>" 
  "</html>");
}
#endif

#ifdef SUPPORT_HTTP
void handleRoot() {
  handleRootWithPrefix("");
}
#endif

#ifdef SUPPORT_HTTPS
void handleRootHTTPS() {
  handleRootWithPrefixHTTPS("");
}
#endif

#ifdef SUPPORT_HTTP
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
        "<html>" VIEWPORT "<title>Success</title><body>" 
        FONT_START
        "Sesame opening.<br/><a href='/'>Try again.</a>" FONT_END
        "</body></html>");
      openSesame();
      numTries = 0;
      return;
    }    
    else {
      handleRootWithPrefix(FONT_START "<font color='red'>Incorrect entry.</font>" FONT_END);
    }
  }
}
#endif

#ifdef SUPPORT_HTTPS
void handlePostHTTPS() {
  Serial.println("https post");
  if (! checkTry()) {
    handleRootHTTPS();
    return;
  }
  
  if (secureServer.args() == 1 && secureServer.argName(0) == "code") {
    addTry(); 
    String code = secureServer.arg(0);
    if (code == RELAY_PIN) {
      secureServer.send(200, "text/html", 
        "<html>" VIEWPORT "<title>Success</title><body>" 
        FONT_START
        "Sesame opening.<br/><a href='"  RELAY_DIRECTORY "'>Try again.</a>" FONT_END
        "</body></html>");
      openSesame();
      numTries = 0;
      return;
    }    
    else {
      handleRootWithPrefixHTTPS(FONT_START "<font color='red'>Incorrect entry.</font>" FONT_END);
    }
  }
}
#endif

#ifdef SUPPORT_HTTP
void handleNotFound(){
  server.send(404, "text/plain", "404: Not found"); // Send HTTP status 404 (Not Found) when there's no handler for the URI in the request
}
#endif

#ifdef SUPPORT_HTTPS
void handleNotFoundHTTPS(){
  // don't respond, hide better
  //secureServer.send(404, "text/plain", "404: Not found"); // Send HTTP status 404 (Not Found) when there's no handler for the URI in the request
}
#endif

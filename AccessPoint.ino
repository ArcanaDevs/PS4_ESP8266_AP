/* Create a WiFi access point and provide a web server on it. */
/* https://gist.github.com/Cyclenerd/7c9cba13360ec1ec9d2ea36e50c7ff77/e8bafbc16c6d30531d8fa2e843cb26df59e0d825 */
/* https://tttapa.github.io/ESP8266/Chap11%20-%20SPIFFS.html */

#include <ESP8266WiFi.h>
#include "./DNSServer.h"                  // Patched lib
#include <ESP8266WebServer.h>
#include <FS.h>   // Include the SPIFFS library

const byte        DNS_PORT = 53;          // Capture DNS requests on port 53
IPAddress         apIP(10, 10, 10, 1);    // Private network for server
DNSServer         dnsServer;              // Create the DNS object
ESP8266WebServer  webServer(80);          // HTTP server

String getContentType(String filename); // convert the file extension to the MIME type
bool handleFileRead(String path);       // send the right file to the client (if it exists)

void setup() {
  Serial.begin(9600);                  
  Serial.print("Hello World");
  
  // turn the LED on (HIGH is the voltage level)
  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, HIGH);

  // configure access point
  WiFi.mode(WIFI_AP);
  WiFi.softAPConfig(apIP, apIP, IPAddress(255, 255, 255, 0));
  WiFi.softAP("PS4"); // WiFi name

  // if DNSServer is started with "*" for domain name, it will reply with
  // provided IP to all DNS request
  dnsServer.start(DNS_PORT, "*", apIP);

  SPIFFS.begin();                           // Start the SPI Flash Files System

  // replay to all requests with same HTML
  webServer.onNotFound([]() {
    if (!handleFileRead(webServer.uri())) {
      webServer.sendHeader("Location", String("/"), true);
      webServer.send(301, "text/plain", "");
    };
  });
  webServer.begin();
}

void loop() {
  dnsServer.processNextRequest();
  webServer.handleClient();
}

String getContentType(String filename){
  if(filename.endsWith(".htm")) return "text/html";
  else if(filename.endsWith(".html")) return "text/html";
  else if(filename.endsWith(".js")) return "application/javascript";
  else if(filename.endsWith(".gz")) return "application/x-gzip";
  else if(filename.endsWith(".zst")) return "application/x-zstd";
  else if(filename.endsWith(".br")) return "application/x-br";
  else if(filename.endsWith(".bin")) return "application/octet-stream";
  return "text/plain";
}

bool handleFileRead(String path) { // send the right file to the client (if it exists)
  Serial.println("handleFileRead: " + path);
  if (path.endsWith("/")) path += "index.html";         // If a folder is requested, send the index file
  String contentType = getContentType(path);            // Get the MIME type
  String pathWithGz = path + ".gz";

  if (SPIFFS.exists(pathWithGz) || SPIFFS.exists(path)) {                            // If the file exists
    if (SPIFFS.exists(pathWithGz)) {
      path = path + ".gz";
    }
    
    File file = SPIFFS.open(path, "r");                 // Open it
    size_t sent = webServer.streamFile(file, contentType); // And send it to the client
    file.close();                                       // Then close the file again
    Serial.println(String("\tSent file: ") + path);
    return true;
  }
  Serial.println("\tFile Not Found");
  return false;                                         // If the file doesn't exist, return false
}

#include <Arduino.h>
#include <WiFi.h>
#include <AsyncTCP.h>

#include <ESPAsyncWebServer.h>

AsyncWebServer server(80);

typedef struct
{
  char ssid[32];
  char password[128];
} EpromWifi;

const char* ssid = "Beaucamp";
const char* password = "jfytj2cXC6LLt3NrWgMX52m8";

const char* softap_ssid = "SiPIN Detector";
const char* softap_password = "cernwebfest2021";

// === Experimental Data =========================================================================================

//unsigned long measurementTotalTime = 3600000; // milliseconds
unsigned long measurementStartTime = 0; // milliseconds

// Counter-----------------------------------------
#define COUNTER_HIST_LEN 2048
double binTime = 50;
unsigned int counterHist[COUNTER_HIST_LEN];

// ======================== Index HTML ===========================================================================

const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE HTML><html><head>
  <title>SiPIN Detector config</title>
  <meta name="viewport" content="width=device-width, initial-scale=1">
  </head><body>
  <h1>SiPIN Detector config</h1>
  <p>Welcome to the SiPIN detector config page! Use this page to setup the experiment.</p>

  <form action="/">
    Measurement ID: <input type="text" name="ID">
    <input type="submit" value="Ok">
  </form><br>
  <form action="/">
    Experiment:
    <select name="experiment_type">
      <option value="Count">Counter</option>
      <option value="Energy">Energy spectrum</option>
    </select>
    <input type="submit" value="Submit">
  </form><br>
  <form action="/">
    Trigger level: <input type="text" name="trigger">
    <input type="submit" value="Ok">
  </form><br>
  <form action="/">
    <input type="submit" value="Reset Histogram">
  </form><br>

  <div style="width:400px;height=100px;display:flex;justify-content:center;align-items:center;">ADC raw-triggered-input placeholder</div>
</body></html>)rawliteral";

// ======================== WIFI SETUP ===========================================================================

const char wifi_setup_index_html_1[] PROGMEM = R"rawliteral(
<!DOCTYPE HTML><html><head>
  <title>SiPIN Detector WiFi config</title>
  <meta name="viewport" content="width=device-width, initial-scale=1">
  </head><body>
  <h1>SiPIN Detector WiFi config</h1>
  <p>Welcome to the SiPIN detector config page! Use this page to setup the WiFi connection.</p><br>
 )rawliteral";

const char wifi_setup_index_html_2[] PROGMEM = R"rawliteral(
To access the WiFi config, press the button. Your current connection will disconnect.
<form action="/wifi"> <input type="submit"/> </form>
</body></html>
 )rawliteral";

const char wifi_setup_html_1[] PROGMEM = R"rawliteral(
<!DOCTYPE HTML><html><head>
  <title>SiPIN Detector WiFi config</title>
  <meta name="viewport" content="width=device-width, initial-scale=1">
  </head><body>
  <h1>SiPIN Detector WiFi config</h1>
  <p>Welcome to the SiPIN detector config page! Please select a WiFi network and input it's password. Open (unsecured) networks are marked with a '(*)'.</p><br>

Available networks:
)rawliteral";

const char wifi_setup_html_2[] PROGMEM = R"rawliteral(
</select><br>
Password: <input type="password", name="password"> (Leave empty if WiFi network is open)<br>
<input type="submit" value="Connect">
</form>
)rawliteral";

const char wifi_setup_index_html_from_STA[] PROGMEM = R"rawliteral(
<!DOCTYPE HTML><html><head>
  <title>SiPIN Detector WiFi config</title>
  <meta name="viewport" content="width=device-width, initial-scale=1">
  </head><body>
  <h1>SiPIN Detector WiFi config</h1>
  <pstyle="color:red;">Error! To change the WiFi connection config, you should connect to the internal SSID (SiPIN Detector), and go to <a href="http://192.168.4.1">192.168.4.1</a>.</p>
  </body></html>
 )rawliteral";

 String wifiConfig;

// ======================== / WIFI SETUP =========================================================================

String IpAddress2String(const IPAddress& ipAddress)
{
  return String(ipAddress[0]) + String(".") +\
  String(ipAddress[1]) + String(".") +\
  String(ipAddress[2]) + String(".") +\
  String(ipAddress[3])  ; 
}

void notFound(AsyncWebServerRequest *request) {
  request->send(404, "text/plain", "Not found");
}

void setup() {
  Serial.begin(115200);
  //CONFIG WIFI:
  
  WiFi.mode(WIFI_MODE_APSTA);
  WiFi.disconnect();
  delay(100);
  
  wifiConfig = wifi_setup_html_1;
  WiFi.disconnect();
  int n = WiFi.scanNetworks();
  if (n == 0) {
    wifiConfig += "No WiFi networks found!";
  } else {
    wifiConfig += "<form action=\"SetWifi\">SSID: <select name=\"ssid\">";
    for (int i = 0; i < n; ++i) {
      // Print SSID and RSSI for each network found
      wifiConfig += "<option value=\"";
      wifiConfig += WiFi.SSID(i);
      wifiConfig += "\">";
      wifiConfig += WiFi.SSID(i);
      wifiConfig += (WiFi.encryptionType(i) == WIFI_AUTH_OPEN) ? "(*)" : "";
      wifiConfig += "</option>";
    }
    wifiConfig += wifi_setup_html_2;
  }
  wifiConfig += "</body></html>";

  
  WiFi.softAP(softap_ssid, "");

  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
    if (ON_STA_FILTER(request)) {
      request->send(200, "text/html", index_html);
    } else if (ON_AP_FILTER(request)) {
      String wifiIndex = String(wifi_setup_index_html_1);
      if (WiFi.status() == WL_CONNECTED) {
        wifiIndex += "<p>You are now connected to SSID: ";
        wifiIndex += WiFi.SSID();
        wifiIndex += ", with IP: ";
        wifiIndex += IpAddress2String(WiFi.localIP());
        wifiIndex += ".</p><br>";
      } else if (WiFi.status() == WL_CONNECT_FAILED) {
        wifiIndex += "<p>Failed to connect to WiFi.</p><br>";
      } else if (WiFi.status() == WL_DISCONNECTED || WiFi.status() == WL_CONNECTION_LOST) {
        wifiIndex += "<p>Disconnected from WiFi network.</p><br>";
      }
      wifiIndex += wifi_setup_index_html_2;
      request->send(200, "text/html", wifiIndex);
    }
  });

  server.on("/wifi", HTTP_GET, [](AsyncWebServerRequest *request){
    if (ON_STA_FILTER(request)) {
      request->send(200, "text/html", wifi_setup_index_html_from_STA);
    } else if (ON_AP_FILTER(request)) {
      request->send(200, "text/html", wifiConfig);
    }
  });

  
  // Send a GET request to <ESP_IP>/wifi?ssid=<inputMessage>
  server.on("/SetWifi", HTTP_GET, [] (AsyncWebServerRequest *request) {
    if (ON_STA_FILTER(request)) {
      request->send(200, "text/html", wifi_setup_index_html_from_STA);
    } else if (ON_AP_FILTER(request)) {
      String inputSSID;
      String inputPassword;
      bool isNetworkOpen;
      
      // GET input1 value on <ESP_IP>/get?input1=<inputMessage>
      if (request->hasParam("ssid")) {
        inputSSID = request->getParam("ssid")->value(); 
        inputPassword = request->getParam("password")->value();
  
        Serial.println(inputSSID);
        Serial.println(inputPassword);
        
        isNetworkOpen = (inputPassword.length() == 0);
        if(isNetworkOpen) {
          WiFi.begin(inputSSID.c_str());
        } else {
          WiFi.begin(inputSSID.c_str(), inputPassword.c_str());
        }
        
        //Serial.print("Connecting to WiFi ..");
        int failCounter = 0;
        while (WiFi.status() != WL_CONNECTED) {
          //Serial.print('.');
          delay(1000);
          if(failCounter == 10) {
            //WiFi.disconnect();
            String wifiSetReturn = String("WiFi failed to connect! Try again");
            request->send(200, "text/html", wifiSetReturn);
          }
        }
        Serial.println(WiFi.localIP());
  
        String wifiSetReturn = String("WiFi set! Connect to device on the new network to set the trigger points, with IP address: ");
        wifiSetReturn += IpAddress2String(WiFi.localIP());
        
        request->send(200, "text/html", wifiSetReturn);
      } else {
        request->send(200, "text/html", "Error! Go to 192.168.4.1 to change WiFi settings.");
      }
    }
  });

  
  server.onNotFound(notFound);
  server.begin();

  for(int i = 0; i < COUNTER_HIST_LEN; i++) {
    counterHist[i] = 0;
  }
}
    
  
  
/*
  // Send a GET request to <ESP_IP>/get?input1=<inputMessage>
  server.on("/get", HTTP_GET, [] (AsyncWebServerRequest *request) {
    String inputMessage;
    String inputParam;
    // GET input1 value on <ESP_IP>/get?input1=<inputMessage>
    if (request->hasParam(PARAM_INPUT_1)) {
      inputMessage = request->getParam(PARAM_INPUT_1)->value();
      inputParam = PARAM_INPUT_1;
    }
    // GET input2 value on <ESP_IP>/get?input2=<inputMessage>
    else if (request->hasParam(PARAM_INPUT_2)) {
      inputMessage = request->getParam(PARAM_INPUT_2)->value();
      inputParam = PARAM_INPUT_2;
    }
    // GET input3 value on <ESP_IP>/get?input3=<inputMessage>
    else if (request->hasParam(PARAM_INPUT_3)) {
      inputMessage = request->getParam(PARAM_INPUT_3)->value();
      inputParam = PARAM_INPUT_3;
    }
    else {
      inputMessage = "No message sent";
      inputParam = "none";
    }
    Serial.println(inputMessage);
    request->send(200, "text/html", "HTTP GET request sent to your ESP on input field (" 
                                     + inputParam + ") with value: " + inputMessage +
                                     "<br><a href=\"/\">Return to Home Page</a>");
  }); 
  server.onNotFound(notFound);
  server.begin();
  */

void loop() {
  
}

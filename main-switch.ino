#include <ESP8266WiFi.h>
#include <ESPAsyncWebServer.h>
#include <map>

// Relay Normally Open true
#define RELAY_NO true
// No. of relays
#define NUM_RELAYS 4

// Assign descriptive names and GPIOs for each relay
std::map<int, String> relayNames = {
  {12, "Main Light"},
  {4, "Warm LED"},
  {13, "Blue LED"},
  {14, "Stereo"}
};

int relayGPIOs[NUM_RELAYS] = {12, 4, 13, 14};

// Replace with your network credentials
const char* ssid = "router nguo";
const char* password = "sipeanangibure2";

const char* PARAM_INPUT_1 = "relay";  
const char* PARAM_INPUT_2 = "state";

// Create AsyncWebServer object on port 80
AsyncWebServer server(80);

const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE HTML>
<html>
  <head>
    <meta name="viewport" content="width=device-width, initial-scale=1">
    <style>
      html {
        font-family: Arial;
        display: inline-block;
        text-align: center;
        height: 100vh;
      }
      body {
        min-height: 90vh;
        max-width: 100vw;
        margin: 0px auto;
        padding: 1rem;
        padding-bottom: 20px;
        padding-top: 0;
        background-image: linear-gradient(
          to right,
          rgba(255, 0, 101, 0.1),
          rgba(255, 0, 201, 1)
        );
      }
      .row {
        display: flex;
        display: -webkit-flex;
        flex-direction: row;
        gap: 20px;
      }
      .col {
        display: flex;
        display: -webkit-flex;
        flex-direction: column;
        gap: 10px;
      }
      section {
        min-height: 70vh;
      }
      .container {
        height: 100%;
        justify-content: center;
        align-items: center;
        background-color: rgba(0, 50, 50, 0.1);
        border-radius: 15px;
        padding: 25px;
        flex-wrap: wrap;
        overflow: hidden;
      }
      header {
        justify-content: flex-start;
      }
      h2 {
        font-size: 3rem;
        text-align: left;
        margin: 0;
        padding: 1rem;
        padding-left: 0;
      }
      p {
        font-size: 1.5rem;
      }
      .toggle {
        justify-content: center;
        align-items: center;
        border-radius: 10px;
        background-color: whitesmoke;
        padding: 15px;
      }
      .switch {
        position: relative;
        width: 80px;
        height: 44px;
      }
      .switch input {
        display: none;
      }
      .slider {
        display: flex;
        justify-content: center;
        align-items: center;
        cursor: pointer;
        position: absolute;
        top: 0;
        left: 0;
        right: 0;
        bottom: 0;
        background-color: #ccc;
        border-radius: 34px;
      }
      .slider:before {
        position: absolute;
        content: "";
        height: 36px;
        width: 36px;
        left: 8px;
        bottom: 4px;
        right: 4px;
        background-color: #fff;
        -webkit-transition: 0.4s;
        transition: 0.4s;
        border-radius: 34px;
      }
      input:checked + .slider {
        background-color: #09ff00;
      }
      input:checked + .slider:before {
        -webkit-transform: translateX(30px);
        -ms-transform: translateX(30px);
        transform: translateX(30px);
      }
    </style>
  </head>
  <body class="col">
    <header class="col">
      <h2>Kwa Kunnoh</h2>
    </header>
    <div class="row container">
      %BUTTONPLACEHOLDER%
    </div>
    <footer class="col">
      <p>&copy; 2024</p>
    </footer>

    <script>
      function toggleCheckbox(el) {
        var xhr = new XMLHttpRequest();
        if(el.checked){ xhr.open("GET", "/update?relay="+el.id+"&state=1", true); }
        else { xhr.open("GET", "/update?relay="+el.id+"&state=0", true); }
        xhr.send();
      }
    </script>
  </body>
</html>
)rawliteral";

// Replaces placeholder with button section in your web page
String processor(const String& var) {
  if (var == "BUTTONPLACEHOLDER") {
    String buttons = "";
    for (int i = 0; i < NUM_RELAYS; i++) {
      int gpio = relayGPIOs[i];
      String relayName = relayNames[gpio];
      String relayStateValue = relayState(gpio);
      buttons += "<div class=\"col toggle\"><h4>" + relayName + "</h4>";
      buttons += "<label class=\"switch\"><input type=\"checkbox\" onchange=\"toggleCheckbox(this)\" id=\"" + String(i + 1) + "\" " + relayStateValue + ">";
      buttons += "<span class=\"row slider\"></span></label></div>";
    }
    return buttons;
  }
  return String();
}

String relayState(int gpio) {
  if (RELAY_NO) {
    return digitalRead(gpio) ? "" : "checked";
  } else {
    return digitalRead(gpio) ? "checked" : "";
  }
}

void setup() {
  // Serial port for debugging purposes
  Serial.begin(115200);

  // Set all relays to off when the program starts
  for (int i = 0; i < NUM_RELAYS; i++) {
    int gpio = relayGPIOs[i];
    pinMode(gpio, OUTPUT);
    digitalWrite(gpio, RELAY_NO ? HIGH : LOW);
  }

  // Connect to Wi-Fi
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Connecting to WiFi...");
  }

  // Print ESP8266 Local IP Address
  Serial.println(WiFi.localIP());

  // Route for root / web page
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send_P(200, "text/html", index_html, processor);
  });

  // Update relay state
  server.on("/update", HTTP_GET, [] (AsyncWebServerRequest *request) {
    if (request->hasParam(PARAM_INPUT_1) && request->hasParam(PARAM_INPUT_2)) {
      int relayIndex = request->getParam(PARAM_INPUT_1)->value().toInt() - 1;
      int state = request->getParam(PARAM_INPUT_2)->value().toInt();

      if (relayIndex >= 0 && relayIndex < NUM_RELAYS) {
        int gpio = relayGPIOs[relayIndex];
        digitalWrite(gpio, RELAY_NO ? !state : state);
        Serial.println("Updated GPIO " + String(gpio) + " (" + relayNames[gpio] + ") to state " + String(state));
        request->send(200, "text/plain", "OK");
        return;
      }
    }
    request->send(400, "text/plain", "Invalid request");
  });

  // Start server
  server.begin();
}

void loop() {
}

#include <ESP8266WiFi.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include <PubSubClient.h>

#define wifi_ssid "WIFI_SSID"
#define wifi_password "WIFI_PASS"

#define mqtt_server "MQTT_SERVER"
#define mqtt_user "MQTT_USER"
#define mqtt_password "MQTT_PASS"
#define temperature_topic "sensor/temp"

#define DELAY 10000 //Update every 10 seconds
#define ONE_WIRE_BUS 2
WiFiClient espClient;
PubSubClient client(espClient);

//Declare onewire bus details
OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature DS18B20(&oneWire);

void setup() {
  //Setup Serial Port
  Serial.begin(115200);
  Serial.print("Starting\n");

  //Setup Wifi
  setup_wifi();
  client.setServer(mqtt_server, 1883);

  // setup OneWire bus
  DS18B20.begin();
}

void setup_wifi() {
  delay(10);
  // We start by connecting to a WiFi network
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(wifi_ssid);

  WiFi.begin(wifi_ssid, wifi_password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
}

void reconnect() {
  // Loop until we're reconnected
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    // Attempt to connect
    // If you do not want to use a username and password, change next line to
    // if (client.connect("ESP8266Client")) {
    if (client.connect("ESP8266Client", mqtt_user, mqtt_password)) {
      Serial.println("connected");
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}

bool checkBound(float newValue, float prevValue, float maxDiff) {
  return !isnan(newValue) &&
         (newValue < prevValue - maxDiff || newValue > prevValue + maxDiff);
}

long lastMsg = 0;
float temp = 0.0;
float diff = 1.0;
float newTemp = 0.0;

void loop() {
  if (!client.connected()) {
    reconnect();
  }
  client.loop();

  long now = millis();
  if (now - lastMsg > 1000) {
    lastMsg = now;

    //Getting the temperature
    do {
      DS18B20.requestTemperatures();
      newTemp = DS18B20.getTempCByIndex(0);
      delay(100);
      Serial.print("Temperature: ");
      Serial.println(temp);
    } while (temp == 85.0 || temp == (-127.0));
 
    if (checkBound(newTemp, temp, diff)) {
      temp = newTemp;
      Serial.print("New temperature:");
      Serial.println(String(temp).c_str());
      client.publish(temperature_topic, String(temp).c_str(), true);
    }
  }
}

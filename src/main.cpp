#include <Arduino.h>
#include <Wire.h>
#include <ESP8266WiFi.h>

#include <ESP8266HTTPClient.h>
#include <tr064.h>
#include <PubSubClient.h>

#define LED_BUILTIN 2

// Replace with your network credentials
const char* ssid     = "FRITZ!Box 6591 Cable GX";
const char* password = "87811180949226958397";

const char USER[] = "doorbell";
const char PASSWORD[] = "doorbell.12683.82B";
const char FRITZBOX_IP[] = "192.168.178.1";
const int FRITZBOX_PORT = 49000;

const char* mqtt_broker = "192.168.178.55";
const char* topic_doorbell = "doorbell/ring";

unsigned long ulStart = 0;

WiFiClient wifi_client;
PubSubClient mqtt_client(wifi_client);


TR064 tr064_connection(FRITZBOX_PORT, FRITZBOX_IP, USER, PASSWORD);

void reconnect_mqtt()
{
    while (!mqtt_client.connected())
    {
        Serial.print("Reconnecting MQTT ...\r\n");
        if (!mqtt_client.connect("ESP8266Client"))
        {
            Serial.print("failed, rc=\r\n");
            Serial.print(mqtt_client.state());
            Serial.println("retrying in 100 milliseconds\r\n");
            delay(100);
        }
    }
}

void ringAllPhones()
{
  Serial.println("TR-064 - try round call\r\n");

  /*Serial.println("TR-064 - init ...");
  tr064_connection.init();

  Serial.println(" done\r\n");*/

  String tr064_service = "urn:dslforum-org:service:X_VoIP:1";
  String call_params[][2] = {{"NewX_AVM-DE_PhoneNumber", "**9"}}; //Die Telefonnummer **9 ist der Fritzbox-Rundruf.

  //if (tr064_connection.action(tr064_service, "X_AVM-DE_DialNumber", call_params, 1) == true)
  if (tr064_connection.action(tr064_service, "X_AVM-DE_DialNumber", call_params, 1, "/upnp/control/x_voip") == true)
  {
    Serial.println("TR-064 action successfull\r\n");
  }
  else
  {
    Serial.println("TR-064 action failed\r\n");
  }

  //Warte vier Sekunden bis zum auflegen
  delay(4000);

  tr064_connection.action(tr064_service, "X_AVM-DE_DialHangup");
}

void sendMqtt(const char* const payload)
{
  if (!mqtt_client.connected())
    reconnect_mqtt();

  Serial.printf("sending '%s'\r\n", payload);
  mqtt_client.publish(topic_doorbell, "ring");

  mqtt_client.loop();

  unsigned long ulTime = millis() - ulStart;
  Serial.printf("time from reset: %lu ms\r\n", ulTime);
}

void setup() 
{
  ulStart = millis();

  pinMode(LED_BUILTIN, OUTPUT);

  Serial.begin(9600);
  Serial.setDebugOutput(true);

  // Connect to Wi-Fi network with SSID and password
  unsigned long wifiloop = 0;
  delay(100);
  Serial.print("\r\nConnecting to ");
  Serial.println(ssid);

  WiFi.disconnect();
  WiFi.enableSTA(true);
  WiFi.setPhyMode(WIFI_PHY_MODE_11N);
  WiFi.setOutputPower(20.5);
  WiFi.enableInsecureWEP();

  WiFi.begin(ssid, password);
  wl_status_t wifiStatus = WiFi.status();
  while (wifiStatus != WL_CONNECTED)
  {
    delay(2000);
    Serial.println(WiFi.status());
    digitalWrite(LED_BUILTIN, (wifiloop++ % 2 == 1) ? HIGH : LOW);

    WiFi.printDiag(Serial);
    //WiFi.printStatus(Serial);

    if ((wifiStatus == WL_CONNECT_FAILED) || (wifiStatus == WL_CONNECTION_LOST))
    {
        WiFi.begin(ssid, password);
    }  

    wifiStatus = WiFi.status();
  }

  // Print local IP address
  Serial.println("\r\n");
  Serial.println("WiFi connected.\r\n");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
  Serial.println("\r\n");

  mqtt_client.setServer(mqtt_broker, 1883);

  tr064_connection.debug_level = TR064::DEBUG_VERBOSE;

  //ESP.wdtDisable();
}

void loop()
{
  //ringAllPhones();

  sendMqtt("ring");

  ESP.deepSleep(0); 
}
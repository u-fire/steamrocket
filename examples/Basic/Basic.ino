// Visit https://steamrocket.co to register an account
// Provision a device and add the ID and key below
// Go to https://steamrocket.co/develop/basics/ for how the system works

#include <Arduino.h>
#include <steamrocket.h>

#ifdef ESP32
#include <WiFi.h>
#include <HTTPClient.h>
#elif ESP8266
#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#endif

const char *ssid = "";
const char *password = "";

HTTPClient http;
String steamrocket_key = "";
String steamrocket_id = "";

String data;
SteamRocket steamrocket;

void setup()
{
  Serial.begin(9600);
  Serial.println();
  Serial.println("🚀 steamrocket Device Starting");

  WiFi.begin(ssid, password);
  Serial.print("📡 Connecting to WiFi");
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    Serial.print(".");
  }
  Serial.println("!");

  steamrocket.begin(http, steamrocket_id, steamrocket_key);

  data = "test: 1";

  Serial.println("🤐 Encrypting and sending data");
  if (steamrocket.send(data) == true)
  {
    Serial.println("👍 OK!");
  }
  else
  {
    Serial.println("👎 something didn't work");
  }

  if (steamrocket.returned_data.length())
  {
    Serial.println("💬 returned: " + steamrocket.returned_data);
  }
}

void loop()
{
}
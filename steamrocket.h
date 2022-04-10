#pragma once
#include <Arduino.h>
#ifdef ESP32
#include <HTTPClient.h>
#elif ESP8266
#include <ESP8266HTTPClient.h>
#include <WiFiClient.h>
#endif

#include <hydrogen.h>

#define DEBUG 1

class SteamRocket
{
public:
    const String &returned_data = _returned_data;

    bool begin(HTTPClient &http, const String &public_key, const String &secret_key);
    bool send(const String &data);

private:
    HTTPClient *_http;
    #ifdef ESP8266
    WiFiClient _client;
    #endif
    hydro_kx_state st_client;
    hydro_kx_keypair client_static_kp;
    hydro_kx_session_keypair client_kp;

    uint8_t packet1[hydro_kx_XX_PACKET1BYTES];
    uint8_t packet3[hydro_kx_XX_PACKET3BYTES];
    char hex_encoded[200];
    uint8_t hex_decoded[100];
    char data[400];
    String state;
    String url_header = "application/x-www-form-urlencoded";
    String content_header = "Content-Type";
    String _sk;
    String _pk;
    String payload;
    static String _returned_data;

    bool _packet1();
    bool _packet3();
    bool _encrypt(const String &data);
    bool _decrypt(const String &data);
};
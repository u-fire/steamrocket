#include <steamrocket.h>

String SteamRocket::_returned_data = "";

// init library
// return true on success, false on failure
bool SteamRocket::begin(HTTPClient &http, const String &public_key, const String &secret_key)
{
    _http = &http;
    _pk = public_key;
    _sk = secret_key;

#if DEBUG == 1
    Serial.println("::begin() ok");
#endif
    return true;
}

bool SteamRocket::_packet1()
{

    if (hydro_init() != 0)
    {
#if DEBUG == 1
        Serial.println("::begin() fail");
#endif
        return false;
    }

    hydro_kx_keygen(&client_static_kp);
    hydro_kx_xx_1(&st_client, packet1, NULL);
    hydro_bin2hex(hex_encoded, sizeof hex_encoded, packet1, sizeof packet1);

    #if ESP8266
    _http->begin(_client, "http://steamrocket.co/v1/sns");
    #elif ESP32
    _http->begin("http://steamrocket.co/v1/sns");
    #endif
    _http->addHeader(content_header, url_header);
    payload = "step=1&packet=";
    payload.concat(hex_encoded);
    int httpResponseCode = _http->POST(payload);

    if (httpResponseCode != 200)
    {
#if DEBUG == 1
        Serial.println("::send() packet1 error");
#endif
        return false;
    }
    payload = _http->getString();
    strncpy(data, payload.c_str(), sizeof data);
#if DEBUG == 1
    Serial.println("::_packet1() ok");
#endif
    return true;
}

bool SteamRocket::_packet3()
{
    state = payload.substring(0, payload.indexOf('\n'));
    String p2 = payload.substring(payload.indexOf('\n') + 1);

    hydro_hex2bin(hex_decoded, sizeof hex_decoded, p2.c_str(), p2.length(), NULL, NULL);
    if (hydro_kx_xx_3(&st_client, &client_kp, packet3, NULL, hex_decoded, NULL, &client_static_kp) != 0)
    {
#if DEBUG == 1
        Serial.println("::send() packet2 error");
#endif
        return false;
    }
    hydro_bin2hex(hex_encoded, sizeof hex_encoded, packet3, sizeof packet3);

    _http->addHeader(content_header, url_header);
    sprintf(data, "step=3&packet=%s&id=%s", hex_encoded, state.c_str());
    int httpResponseCode = _http->POST(data);

    if (httpResponseCode != 200)
    {
#if DEBUG == 1
        Serial.println("::send() packet3 error");
#endif
        return false;
    }
#if DEBUG == 1
    Serial.println("::_packet3() ok");
#endif
    return true;
}

bool SteamRocket::_encrypt(const String &data)
{
#define CONTEXT "Examples"
    char msg[200];                                                // transfer data into defined msg size
    char hex_data[475];                                           // for storing hex-encoded encrypted msg
    String payload;                                               // POST request
    uint8_t ciphertext[hydro_secretbox_HEADERBYTES + sizeof msg]; // raw encrypted binary
    uint8_t signature[hydro_sign_BYTES];                          // signature
    char skey_decoded[64];
    char signature_encoded[130]; // hex-encoded signature

    // Copy data into msg, needed to ensure consistent data-block size
    strcpy(msg, data.c_str());

    // encrypt msg into ciphertext
    hydro_secretbox_encrypt(ciphertext, msg, sizeof msg, 0, CONTEXT, client_kp.tx);

    // transform ciphertext into hex-encoded data
    memset(hex_data, '0', sizeof hex_data);
    hydro_bin2hex(hex_data, sizeof hex_data, ciphertext, sizeof ciphertext);

    // sign ciphertext
    hydro_hex2bin((uint8_t *)skey_decoded, sizeof skey_decoded, (char *)_sk.c_str(), 128, NULL, NULL);
    hydro_sign_create(signature, hex_data, sizeof hex_data, CONTEXT, (uint8_t *)skey_decoded);
    hydro_bin2hex(signature_encoded, sizeof signature_encoded, signature, sizeof signature);

    // get ready to send data to server
    _http->addHeader(content_header, url_header);
    payload = "step=5&packet=";
    payload.concat(hex_data);
    payload += "&id=";
    payload.concat(state.c_str());
    payload += "&pk=";
    payload.concat(_pk);
    payload += "&sig=";
    payload.concat(signature_encoded);
    int httpResponseCode = _http->POST(payload);

    // server will respond 200 if no error
    if (httpResponseCode != 200)
    {
#if DEBUG == 1
        Serial.println("::_encrypt() error");
#endif
        _http->end();
        return false;
    }
    payload = _http->getString();
    _decrypt(payload);
    _http->end();
#if DEBUG == 1
    Serial.println("::_encrypt() ok");
#endif
    // if we got this far, there were no errors
    return true;
}

bool SteamRocket::_decrypt(const String &data)
{
    char hex_data[475];
    char public_key[130];
    char signature[130];
    char sig_decoded[64];
    char pkey_decoded[32];

    String encrypted = data.substring(0, data.indexOf('\n'));
    String sig = data.substring(data.indexOf('\n') + 1);

    memset(hex_data, '0', sizeof hex_data);
    memset(public_key, '0', sizeof public_key);
    memset(signature, '0', sizeof signature);

    strcpy(hex_data, encrypted.c_str());
    strcpy(public_key, _pk.c_str());
    strcpy(signature, sig.c_str());

    hydro_hex2bin((uint8_t *)pkey_decoded, sizeof pkey_decoded, (char *)public_key, 64, NULL, NULL);
    hydro_hex2bin((uint8_t *)sig_decoded, sizeof sig_decoded, (char *)signature, 128, NULL, NULL);
    if (hydro_sign_verify((uint8_t *)sig_decoded, hex_data, sizeof hex_data, CONTEXT, (uint8_t *)pkey_decoded) != 0)
    {
#if DEBUG == 1
        Serial.println("::_decrypt() signature error");
#endif
        return false;
    }

    char decrypted[200];
    uint8_t encoded[hydro_secretbox_HEADERBYTES + 200];
    hydro_hex2bin(encoded, sizeof encoded, hex_data, sizeof hex_data, NULL, NULL);
    if (hydro_secretbox_decrypt(decrypted, encoded, sizeof encoded, 0, CONTEXT, client_kp.rx) != 0)
    {
#if DEBUG == 1
        Serial.println("::_decrypt() decryption error");
#endif
        return false;
    }
    _returned_data = decrypted;

    return true;
}

bool SteamRocket::send(const String &message)
{
    if (message.length() > 200)
    {
#if DEBUG == 1
        Serial.println("::send() message too long");
#endif
        return false;
    }

    _returned_data = "";
    if (!_packet1())
        return false;
    if (!_packet3())
        return false;
    if (!_encrypt(message))
        return false;

#if DEBUG == 1
    Serial.println("::send() ok");
#endif
    return true;
}
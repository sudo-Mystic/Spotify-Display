#include <Arduino.h>
#include <ArduinoJson.h>
#include <ESP8266HTTPClient.h>
#include <WiFiClientSecure.h>
#include <ESP8266WiFi.h>
#include <base64.h>
#include <resources.h>

// WiFi credentials
const char *ssid = "JaiHanuman";
const char *password = "Rishabh1869";

class SpotConn {
public:
  SpotConn() : accessTokenSet(false), tokenExpireTime(0), tokenStartTime(0) {
    client.setInsecure();
  }

  bool getUserCode(const String& serverCode) {
    if (sendAuthRequest("grant_type=authorization_code&code=" + serverCode + "&redirect_uri=" + String(REDIRECT_URI))) {
      saveTokens();
      return true;
    }
    return false;
  }

  bool refreshAuth() {
    if (refreshToken.isEmpty()) {
      Serial.println("Error: Refresh token is missing.");
      return false;
    }
    if (sendAuthRequest("grant_type=refresh_token&refresh_token=" + String(refreshToken))) {
      saveTokens();
      return true;
    }
    return false;
  }

  unsigned long getTokenStartTime() const { return tokenStartTime; }
  unsigned long getTokenExpireTime() const { return tokenExpireTime; }

  }

private:
  BearSSL::WiFiClientSecure client;
  String accessToken;
  String refreshToken;
  unsigned long tokenExpireTime;
  unsigned long tokenStartTime;
  bool accessTokenSet;


  bool sendAuthRequest(const String& requestBody) {
    HTTPClient https;
    https.begin(client, "https://accounts.spotify.com/api/token");
    https.addHeader("Authorization", "Basic " + base64::encode(String(CLIENT_ID) + ":" + String(CLIENT_SECRET)));
    https.addHeader("Content-Type", "application/x-www-form-urlencoded");
    int httpResponseCode = https.POST(requestBody);

    Serial.println(https.getString());
    https.end();
    return false;
  }
};

void setup() {
  Serial.begin(115200);
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.print(".");

  }
}

void loop() {

}

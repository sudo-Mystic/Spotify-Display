#include <ESP8266WiFi.h>
#include <WiFiClientSecure.h>
#include <ESP8266WebServer.h>
#include <ArduinoJson.h>
#include <ESP8266mDNS.h>
#include <ESP8266HTTPClient.h>
#include <base64.h>
#include <LittleFS.h>
#include <resources.h>

// WiFi credentials
const char *ssid = "ssid";
const char *password = "pass";

// Spotify API credentials
const char *CLIENT_ID = "xxxxxxxxxxxxx";
const char *CLIENT_SECRET = "xxxxxxxxxxxxxxxxxx";
const char *REDIRECT_URI = "http://esp-ip/callback/";
const char *scope = "user-read-playback-state";


class SpotConn {
public:
  SpotConn() : accessTokenSet(false), tokenExpireTime(0), tokenStartTime(0) {
    client.setInsecure();
    if (!LittleFS.begin()) {
      Serial.println("Failed to mount file system");
    } else {
      loadTokens();
    }
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

  bool getTrackInfo() {
    HTTPClient https;
    String url = "https://api.spotify.com/v1/me/player/currently-playing";
    https.useHTTP10(true);
    https.begin(client, url);
    https.addHeader("Authorization", "Bearer " + String(accessToken));
    int httpResponseCode = https.GET();
    bool success = false;

    if (httpResponseCode == HTTP_CODE_OK) {
      StaticJsonDocument<512> filter;
      setupFilter(filter);
      DynamicJsonDocument doc(4096);
      DeserializationError error = deserializeJson(doc, https.getStream(), DeserializationOption::Filter(filter));

      if (error) {
        Serial.print(F("deserializeJson() failed: "));
        Serial.println(error.f_str());
      } else {
        if (strcmp(doc["currently_playing_type"], "track") == 0) {
          const char *songName = doc["item"]["name"];
          const char *artist = doc["item"]["artists"][0]["name"];
        } else {
        }
        success = true;
      }
    } else {
      handleHttpErrors(httpResponseCode);
    }
    https.end();
    return success;
  }

  unsigned long getTokenStartTime() const { return tokenStartTime; }
  unsigned long getTokenExpireTime() const { return tokenExpireTime; }

  void factoryReset() {
    LittleFS.remove("/tokens.json");
    accessToken = "";
    refreshToken = "";
    accessTokenSet = false;
    Serial.println("Tokens cleared. Please reauthorize.");
  }

private:
  BearSSL::WiFiClientSecure client;
  String accessToken;
  String refreshToken;
  unsigned long tokenExpireTime;
  unsigned long tokenStartTime;
  bool accessTokenSet;

  void saveTokens() {
    DynamicJsonDocument doc(512);
    doc["access_token"] = accessToken;
    doc["refresh_token"] = refreshToken;
    doc["expires_in"] = tokenExpireTime;
    File file = LittleFS.open("/tokens.json", "w");
    if (file) {
      serializeJson(doc, file);
      file.close();
    } else {
      Serial.println("Failed to open file for writing");
    }
  }

  void loadTokens() {
    File file = LittleFS.open("/tokens.json", "r");
    if (file) {
      DynamicJsonDocument doc(512);
      DeserializationError error = deserializeJson(doc, file);
      if (!error) {
        accessToken = doc["access_token"].as<String>();
        refreshToken = doc["refresh_token"].as<String>();
        tokenExpireTime = doc["expires_in"];
        tokenStartTime = millis();
        accessTokenSet = true;
      } else {
        Serial.println("Failed to read file, using default configuration");
      }
      file.close();
    } else {
      Serial.println("Failed to open file for reading");
    }
  }

  bool sendAuthRequest(const String& requestBody) {
    HTTPClient https;
    https.begin(client, "https://accounts.spotify.com/api/token");
    https.addHeader("Authorization", "Basic " + base64::encode(String(CLIENT_ID) + ":" + String(CLIENT_SECRET)));
    https.addHeader("Content-Type", "application/x-www-form-urlencoded");
    int httpResponseCode = https.POST(requestBody);

    if (httpResponseCode == HTTP_CODE_OK) {
      DynamicJsonDocument doc(512);
      deserializeJson(doc, https.getStream());
      accessToken = doc["access_token"].as<String>();
      refreshToken = doc["refresh_token"].as<String>();
      tokenExpireTime = doc["expires_in"];
      tokenStartTime = millis();
      accessTokenSet = true;
      return true;
    }
    Serial.println(https.getString());
    https.end();
    return false;
  }

  void setupFilter(StaticJsonDocument<512>& filter) {
    filter["timestamp"] = true;
    filter["progress_ms"] = true;
    JsonObject filter_item = filter.createNestedObject("item");
    JsonObject filter_item_album = filter_item.createNestedObject("album");
    filter_item_album["artists"] = true;
    filter_item_album["images"] = true;
    filter_item_album["name"] = true;
    filter_item["artists"] = true;
    filter_item["duration_ms"] = true;
    filter_item["name"] = true;
    filter["currently_playing_type"] = true;
    filter["is_playing"] = true;
  }

  void handleHttpErrors(int httpResponseCode) {
    if (httpResponseCode == HTTP_CODE_NO_CONTENT) {
    } else if (httpResponseCode == HTTP_CODE_UNAUTHORIZED) {
      refreshAuth();
    }
    Serial.print("Error getting track info: ");
    Serial.println(httpResponseCode);
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

// *** WIFI and SERVERS *** //
struct Config {
    char mqtt_server[41];
    uint16_t mqtt_port;
    char ntp_server[41];
    uint16_t ntp_port;
    char token[21];
    char wifi_ssid[41];
    char wifi_psk[41];
    uint8_t pub_int;
};

Config config;

bool saveConfigSPIFFS() {
    // Save the config to SPIFFS
    Serial.println("Saving config");
    // Create a JSON
    StaticJsonDocument<NETBUFF_SIZE> jsonBuffer;
    JsonObject json = jsonBuffer.to<JsonObject>();

    json["mqtt_server"] = config.mqtt_server;
    json["mqtt_port"] = config.mqtt_port;
    json["ntp_server"] = config.ntp_server;
    json["ntp_port"] = config.ntp_port;
    json["token"] = config.token;
    json["wifi_ssid"] = config.wifi_ssid;
    json["wifi_psk"] = config.wifi_psk;
    json["pub_int"] = config.pub_int;

    File configFile = SPIFFS.open("/config.json", "w");
    if (configFile) {
        serializeJson(json, configFile);
        configFile.write('\n');
        configFile.close();
        return true;
    } else {
        Serial.println("Failed to open config file");
        return false;
    }
}

void printSDConfig(const char *filename, Config &config) {
    // Print the content of a file in the SD-Card
    File file = SD.open(filename);

    if (!file) {
        Serial.println(F("Failed to read file"));
        return;
    }

    // Extract each characters by one by one
    while (file.available()) {
        Serial.print((char)file.read());
    }
    Serial.println();
    file.close();
}

bool loadConfigSD(const char *filename, Config &config) {
    // Load config file from SD card
    File file = SD.open(filename);

    if (!file) {
        Serial.println(F("Failed to read file"));
        return false;
    }

    StaticJsonDocument<512> json;

    // Deserialize the JSON document
    DeserializationError error = deserializeJson(json, file);
    if (error) {
        Serial.println(F("Failed to read json file in SD-card"));
    } else {
        Serial.println("File exists");

        if (json.containsKey("mqtt_server")) strcpy(config.mqtt_server, json["mqtt_server"]);
        if (json.containsKey("mqtt_port")) config.mqtt_port = json["mqtt_port"];
        if (json.containsKey("ntp_server")) strcpy(config.ntp_server, json["ntp_server"]);
        if (json.containsKey("ntp_port")) config.ntp_port = json["ntp_port"];
        if (json.containsKey("token")) strcpy(config.token, json["token"]);
        if (json.containsKey("wifi_ssid")) strcpy(config.wifi_ssid, json["wifi_ssid"]);
        if (json.containsKey("wifi_psk")) strcpy(config.wifi_psk, json["wifi_psk"]);
        if (json.containsKey("pub_int")) config.pub_int=json["pub_int"];
    }

    // Close the file
    file.close();
    return true;
}

bool loadConfigSPIFFS(){
    if (SPIFFS.begin()) {

        Serial.println("SPIFFS mounted");
        if (SPIFFS.exists("/config.json")) {

            Serial.println("Config json exists in SPIFFS");
            File configFile = SPIFFS.open("/config.json", "r");
            StaticJsonDocument<NETBUFF_SIZE> jsonBuf;
            deserializeJson(jsonBuf, configFile);
            JsonObject json = jsonBuf.as<JsonObject>();

            if (json) {
                Serial.println("JSON object is valid");
                if (json.containsKey("mqtt_server")) strcpy(config.mqtt_server, json["mqtt_server"]);
                if (json.containsKey("mqtt_port")) config.mqtt_port = json["mqtt_port"];
                if (json.containsKey("ntp_server")) strcpy(config.ntp_server, json["ntp_server"]);
                if (json.containsKey("ntp_port")) config.ntp_port = json["ntp_port"];
                if (json.containsKey("token")) strcpy(config.token, json["token"]);
                if (json.containsKey("wifi_ssid")) strcpy(config.wifi_ssid, json["wifi_ssid"]);
                if (json.containsKey("wifi_psk")) strcpy(config.wifi_psk, json["wifi_psk"]);
                if (json.containsKey("pub_int")) config.pub_int=json["pub_int"];
                return true;
            } else {
                Serial.println("/config.json is not valid");
                return false;
            }
        } else {
            Serial.println("Config.json doesn't exist");
            return false;
        }
    } else {
        Serial.println("Failed to mount FS");
        return false;
    }
}

#include <FS.h>
#include <WiFiManager.h>
#include "ArduinoJson.h"
#include <PubSubClient.h>
#include "Config.h"
#include "NTPTime.h"
#include <SD.h> //Load SD library
#include <SoftwareSerial.h>
#include "RTClib.h" //include Adafruit RTC library

#define NETBUFF_SIZE 512

Config config;

WiFiManager wifiManager;
WiFiClient espClient;
PubSubClient mqttClient(espClient);

// *** SD CARD *** //
int chipSelect = 15; //chip select pin for the MicroSD Card Adapter
char dataFileName[16]; // make it long enough to hold your longest file name, plus a null terminator
const char *configFileName = "/config.json";  // <- SD library uses 8.3 filenames

// *** RTC *** //
RTC_PCF8523 rtc;
String timestamp;

// *** general *** //
uint8_t status_publish = 0;
bool status_write = 0;
bool time_to_publish;
time_t last_publish;
uint16_t sensorID = 91;
#define ECHO_PIN 14
#define TRIG_PIN 12

float sensorReading;

float readDistance() {
    float timing;
    float distance;
    digitalWrite(TRIG_PIN, LOW);
    delay(2);
    digitalWrite(TRIG_PIN, HIGH);
    delay(10);
    digitalWrite(TRIG_PIN, LOW);

    timing = pulseIn(ECHO_PIN, HIGH);
    distance = (timing * 0.034) / 2;
    return distance;
}

//function that send the startup message
void startup_message() {
    Serial.println();
    Serial.println("***Start-up********************************************************");
   // ** get the timestamp
    String timeString = ISOtime();

    char mqtt_clientid[15];
    if (mqttClient.connect(config.token)) {

        Serial.println("MQTT connected.");
        long rssi = WiFi.RSSI();

        // send JSON startup message
        StaticJsonDocument<NETBUFF_SIZE> jsonBuffer;
        JsonObject json = jsonBuffer.to<JsonObject>();
        json["ts"] = timeString;
        json["rssi"] = rssi;
        char buf[NETBUFF_SIZE];
        serializeJson(json, buf, NETBUFF_SIZE);

        Serial.print("Publish message: ");
        Serial.println(buf);

        char topic_buf[50];
        sprintf(topic_buf, "device/sck/%s/readings/raw", config.token);
        mqttClient.publish(topic_buf, buf);
    }
}

void saveConfigJson() {
    //save the custom parameters to FS
    Serial.println("saving config");
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
        Serial.println("failed to open config file for writing");
        serializeJson(json, configFile);
        configFile.write('\n');
        configFile.close();
    }
    //end save
}

bool loadConfiguration(const char *filename, Config &config) {
    // Open file for reading
    File file = SD.open(filename);

    if (!file) {
        Serial.println(F("Failed to read file"));
        return false;
    }

    // // Extract each characters by one by one
    // while (file.available()) {
    //     Serial.print((char)file.read());
    // }
    // Serial.println();

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

    // Close the file (Curiously, File's destructor doesn't close the file)
    file.close();
    return true;
}

void setup() {
    Serial.begin(115200);
    bool sdCardInit = false;
    bool configInit = false;

    // ************************** INITIALIZE CARD ** //
    pinMode(chipSelect, OUTPUT); // chip select pin must be set to OUTPUT mode
    Serial.println("");
    Serial.println("***SD-CARD********************************************************");
    if (!SD.begin(chipSelect)) { // Initialize SD card
        Serial.println("Could not initialize SD card."); // if return value is false, something went wrong.
    } else {
        Serial.println("Card initialized.");
        sdCardInit = true;
    }

    if (sdCardInit) {
        if (loadConfiguration(configFileName, config)) {
            configInit = true;
            Serial.println("Config init, saving to SPIFFS");
            saveConfigJson();
        }
    }

    //read configuration from FS json
    Serial.println("mounting FS...");

    if (SPIFFS.begin()) {
        Serial.println("mounted file system");
        if (!configInit) {
            Serial.println("No config loaded from SD-card, loading from SPIFFS");
            if (SPIFFS.exists("/config.json")) {
                Serial.println("Config json exists in SPIFFS");
                //file exists, reading and loading
                Serial.println("reading config file");
                File configFile = SPIFFS.open("/config.json", "r");

                // File configFile = SPIFFS.open("/config.json", "r");
                StaticJsonDocument<NETBUFF_SIZE> jsonBuffer;
                deserializeJson(jsonBuffer, configFile);
                JsonObject json = jsonBuffer.as<JsonObject>();

                if (json) {
                    Serial.println("File exists");

                    if (json.containsKey("mqtt_server")) strcpy(config.mqtt_server, json["mqtt_server"]);
                    if (json.containsKey("mqtt_port")) config.mqtt_port = json["mqtt_port"];
                    if (json.containsKey("ntp_server")) strcpy(config.ntp_server, json["ntp_server"]);
                    if (json.containsKey("ntp_port")) config.ntp_port = json["ntp_port"];

                    if (json.containsKey("token")) strcpy(config.token, json["token"]);
                    if (json.containsKey("wifi_ssid")) strcpy(config.wifi_ssid, json["wifi_ssid"]);
                    if (json.containsKey("wifi_psk")) strcpy(config.wifi_psk, json["wifi_psk"]);
                    if (json.containsKey("pub_int")) config.pub_int=json["pub_int"];

                } else {
                    Serial.println("/config.json is not valid");
                }
            } else {
                Serial.println("Config.json doesn't exist");
            }
        }
    } else {
        Serial.println("Failed to mount FS");
    }

    // wifiManager.setSaveConfigCallback(saveConfigCallback);
    boolean errorConnecting = false;

    WiFi.mode(WIFI_STA);
    WiFi.begin(config.wifi_ssid, config.wifi_psk);

    Serial.println();
    Serial.print("Connecting to:\t");
    Serial.println(WiFi.SSID());

    WiFi.waitForConnectResult();
    if (WiFi.status() != WL_CONNECTED) {
        errorConnecting = true;
    }

    if (errorConnecting) {
        while (1) {
            Serial.println("Failed to connect Wifi");
            delay(1000);
        }
    }

    Serial.println();
    Serial.print("Connected a:\t");
    Serial.println(WiFi.SSID());
    Serial.print("IP address:\t");
    Serial.println(WiFi.localIP());

    mqttClient.setServer(config.mqtt_server, config.mqtt_port);
    Serial.print("Attempting connection to broker:\t");
    Serial.println(config.mqtt_server);
    Serial.print("At port:\t");
    Serial.println(config.mqtt_port);

    while (!mqttClient.connect(config.token)) {
        Serial.println("Can't connect to broker");
        delay(1000);
    }

    Serial.println("Connected to broker");
    delay(1000);

    // NTP
    Serial.println("****NTP****");
    Serial.println("Setting NTP Provider");
    setNTPprovider();
    Serial.println("Requesting NTP");
    time_t ntpTime = getNtpTime();
    String timeString = epoch2iso(ntpTime);
    Serial.println(timeString);

    // ************************** INITIALIZE RTC ** //
    Serial.println();
    Serial.println("****RTC****");

    if (!rtc.begin()) {
        Serial.println("Couldn't find RTC");
        while (1);
    } else {
        Serial.println("RTC module ready");
    }

    if (ntpTime) {
        Serial.println("RTC module ready");
        rtc.adjust(now());

        Serial.println(ISOtime());
    }

    startup_message();
    Serial.println("****");
    Serial.println();

    int n = 0;
    snprintf(dataFileName, sizeof(dataFileName), "data%03d.csv", n); // includes a three-digit sequence number in the file name

    while(SD.exists(dataFileName)) {
        n++;
        snprintf(dataFileName, sizeof(dataFileName), "data%03d.csv", n);
    }

    Serial.print("Data file name: ");
    Serial.println(dataFileName);

    // ** Write Log File Header
    File dataFile; // file object that is used to read and write data
    dataFile = SD.open(dataFileName, FILE_WRITE);
    if (dataFile) {
        dataFile.println(" "); //Just a leading blank line, in case there was previous data
        String header = "TIME,LEVEL (cm)";
        dataFile.println("TIME, LEVEL");
        dataFile.close();
        Serial.print("Header: ");
        Serial.println(header);
        Serial.println();
    } else {
        Serial.println("Couldn't open log file");
    }

    // SENSOR
    pinMode(ECHO_PIN, INPUT);
    pinMode(TRIG_PIN, OUTPUT);

    Serial.println("********************************************************");
    Serial.println("********************************************************");
    Serial.println("Start to measure.");
    last_publish = now();

    delay(500); //wait before to start
}

//LOOP
void loop() {

    mqttClient.loop();

    time_to_publish = (now() - last_publish) > config.pub_int;
    Serial.print("Pending to read... ");
    Serial.println(config.pub_int - (now()-last_publish));

    if (time_to_publish){

        sensorReading = readDistance();

        status_publish = 0;     //before to send, clean the status publish flag

        Serial.println("");
        timestamp = ISOtime();         //get the timestamp
        Serial.print("Timestamp : "); Serial.println(timestamp);
        Serial.print("Distance (cm) : "); Serial.println(sensorReading);
        Serial.println();

        //send payload
        status_publish = 0;
        if (mqttClient.connect(config.token)) {
            Serial.println("MQTT connected.");
            long rssi = WiFi.RSSI();

            char buf[NETBUFF_SIZE];
            sprintf(buf, "{t:%s,%i:%.2f}", timestamp.c_str(), sensorID, sensorReading);
            Serial.print("Publish message: ");
            Serial.println(buf);

            char topic_buf[50];
            sprintf(topic_buf, "device/sck/%s/readings/raw", config.token);
            status_publish = mqttClient.publish(topic_buf, buf);
            Serial.print("Publish status:"); Serial.println(status_publish);
        }
        delay(1);

        // Save in SD-card
        status_write = false;
        // ** Write Log File Header
        File file = SD.open(dataFileName, FILE_WRITE);
        if (file) {
            Serial.println("Log file opened");
            file.print(timestamp);
            file.print(",");
            file.println(sensorReading);
            file.close();
            delay(10);
            status_write = true;
        } else {
            Serial.println("Couldn't open log file");
            status_write = false;
        }

        //if the publish was success => blink green
        if(status_publish == 1){
            Serial.println("The payload reached the server.");
        }
        last_publish = now();
    }
    delay(1000);
}


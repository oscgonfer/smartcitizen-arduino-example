#define NETBUFF_SIZE 512

#include <WiFiManager.h>
#include "ArduinoJson.h"
#include <SD.h> //Load SD library
#include <FS.h>
#include <PubSubClient.h>
#include "RTClib.h" //include Adafruit RTC library
#include "Config.h"
#include "NTPTime.h"

extern Config config;

WiFiManager wifiManager;
WiFiClient espClient;
PubSubClient mqttClient(espClient);

// *** SD CARD *** //
int chipSelect = 15; //chip select pin for the MicroSD Card Adapter
char dataFileName[16];
const char *configFileName = "/config.json";

// *** RTC *** //
RTC_PCF8523 rtc;
String timestamp;

// *** GENERIC *** //
uint8_t publishStatus = 0;
bool writeStatus = 0;
bool isTimeToPublish;
time_t lastPublish;

// *** SENSOR *** //
// In this case we use a single reading, with a simple distance sensor
// The sensorID here comes from https://api.smartcitizen.me/v0/sensors
uint16_t sensorID = 91;
float sensorReading;
#define ECHO_PIN 14
#define TRIG_PIN 12

float readSensor() {
    // Make this function take readings from your sesnor, whatever it is
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

void setup() {
    Serial.begin(115200);
    bool configInit = false;
    bool configSaved = false;
    bool timeAvailable = false;

    // SD-CARD
    pinMode(chipSelect, OUTPUT);
    Serial.println("***SD-CARD INIT ***");
    if (!SD.begin(chipSelect)) { // Initialize SD card
        Serial.println("Could not initialize SD card."); // if return value is false, something went wrong.
    } else {
        Serial.println("Card initialized. Loading config from it");

        if (loadConfigSD(configFileName, config)) {
            configInit = true;
            Serial.println("Config init, saving to SPIFFS");
            configSaved = saveConfigSPIFFS();
        } else {
            Serial.println("Couldn't load config from SD-card, attempting from SPIFFS");
            configInit = loadConfigSPIFFS();
        }
    }

    // Wi-Fi
    Serial.println("****WIFI****");
    WiFi.mode(WIFI_STA);
    WiFi.begin(config.wifi_ssid, config.wifi_psk);

    Serial.print("Connecting to:\t");
    Serial.println(WiFi.SSID());

    WiFi.waitForConnectResult();
    if (WiFi.status() != WL_CONNECTED) {
        Serial.println("Failed to connect Wifi");
    }

    Serial.print("Connected to:\t");
    Serial.println(WiFi.SSID());
    Serial.print("IP address:\t");
    Serial.println(WiFi.localIP());

    // MQTT
    Serial.println("****MQTT****");
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

    // NTP
    Serial.println("****NTP****");
    Serial.println("Setting NTP Provider");
    setNTPprovider();
    Serial.println("Requesting NTP");
    time_t ntpTime = getNtpTime();
    String timeString = epoch2iso(ntpTime);
    Serial.println(timeString);

    // RTC
    Serial.println("****RTC****");

    if (!rtc.begin()) {
        Serial.println("Couldn't find RTC");
    } else {
        Serial.println("RTC module ready");
        if (ntpTime) {
            timeAvailable = true;
            Serial.println("RTC module ready");
            rtc.adjust(now());
            Serial.println(ISOtime());
        } else {
            Serial.println('Using existing RTC');
        }
    }

    // FILE
    // Includes a three-digit sequence number in the file name
    int n = 0;
    snprintf(dataFileName, sizeof(dataFileName), "data%03d.csv", n);

    while(SD.exists(dataFileName)) {
        n++;
        snprintf(dataFileName, sizeof(dataFileName), "data%03d.csv", n);
    }

    Serial.print("Data filename: ");
    Serial.println(dataFileName);

    // ** Write Log File Header
    File dataFile;
    dataFile = SD.open(dataFileName, FILE_WRITE);
    if (dataFile) {
        dataFile.println(" "); //Just a leading blank line, in case there was previous data
        String header = "TIME,LEVEL (cm)";
        dataFile.println(header);
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

    Serial.println("Start to measure.");
    lastPublish = now();
}

//LOOP
void loop() {

    mqttClient.loop();
    isTimeToPublish = (now() - lastPublish) > config.pub_int;
    Serial.print("Pending to read... ");
    Serial.println(config.pub_int - (now()-lastPublish));

    if (isTimeToPublish){

        sensorReading = readSensor();
        timestamp = ISOtime();

        Serial.print("Timestamp : "); Serial.println(timestamp);
        Serial.print("Distance (cm) : "); Serial.println(sensorReading);
        Serial.println();

        // Send payload
        publishStatus = 0;
        if (mqttClient.connect(config.token)) {
            Serial.println("MQTT connected.");
            long rssi = WiFi.RSSI();

            char buf[NETBUFF_SIZE];
            sprintf(buf, "{t:%s,%i:%.2f}", timestamp.c_str(), sensorID, sensorReading);
            Serial.print("Publish message: ");
            Serial.println(buf);

            char topic_buf[50];
            sprintf(topic_buf, "device/sck/%s/readings/raw", config.token);
            publishStatus = mqttClient.publish(topic_buf, buf);
            Serial.print("Publish status:");
            Serial.println(publishStatus);
        }

        // Save in SD-card
        writeStatus = false;
        // ** Write Log File Header
        File file = SD.open(dataFileName, FILE_WRITE);
        if (file) {
            Serial.println("Log file opened");
            file.print(timestamp);
            file.print(",");
            file.println(sensorReading);
            file.close();
        } else {
            Serial.println("Couldn't open log file");
        }

        lastPublish = now();
    }
    delay(1000);
}


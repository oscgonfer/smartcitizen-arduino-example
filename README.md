# Smart Citizen Arduino Example

This is a **very** simple example to send data to Smart Citizen MQTT broker, using an Arduino. This particular example was developed with a Adafruit HUZZAH ESP8266 + Adalogger module (RTC + SD-card), taking components from the [smartcitizen-kit-2x firmware](https://github.com/fablabbcn/smartcitizen-kit-2x) and previous projects. It is by no means an attempt to replace the complexity of that firmware, as this is only meant for educational purposes or very simple prototypes, as a demo. For real-world deployments, use the [Smart Citizen Kit](https://docs.smartcitizen.me).

## Changing the sensors

You can customize the sensors you read data from. Simply by modifying the code in `main.cpp`:

```
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
```

## Data logging

Data is collected and sent via MQTT to a broker. If an SD-card is present, it will store it there as well, in `CSV` format.

## Configuration

Configuration is done via SD-card file. An example config.json file is provided. Make sure to name the file `config.json` and put it in the root folder.

Example configuration:

```
{
    "mqtt_server": "mqtt.smartcitizen.me",
    "mqtt_port": 1883,
    "ntp_server": "ntp.smartcitizen.me",
    "ntp_port": 80,
    "token": "83d9ja",
    "wifi_ssid": "WIFI",
    "wifi_psk": "PASSWORD",
    "pub_int": 60
}
```

- `mqtt_server"`: Address of the MQTT broker to send data to. Eg: `"mqtt.smartcitizen.me"``
- `mqtt_port"`: MQTT broker port. No quotes. Eg: `1883`
- `ntp_server"`: Address of the NTP server, to get network time from. Eg: `"ntp.smartcitizen.me"``
- `ntp_port"`: NTP server port. No quotes. Eg: `80`
- `token"`: Token from Smart Citizen platform, to identify your device. Get it on https://start.smartcitizen.me. Eg: `"83d9ja"`
- `wifi_ssid"`: Wi-Fi network name. Eg: `"MYWIFI"`
- `wifi_psk"`: Wi-Fi network password `"PASSWORD"`.

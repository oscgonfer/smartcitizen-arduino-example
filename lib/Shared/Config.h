
// *** WIFI and SERVER *** //
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
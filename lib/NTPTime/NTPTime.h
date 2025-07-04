#include <TimeLib.h>

WiFiUDP Udp;
byte packetBuffer[48];
char last_modified[50];

extern Config config;

String leadingZeros(String original, int decimalNumber)
{
    for (uint8_t i=0; i < (decimalNumber - original.length()); ++i) {
        original = "0" + original;
    }
    return original;
}

void sendNTPpacket(IPAddress &address)
{
    memset(packetBuffer, 0, 48);

    // Initialize values needed to form NTP request
    packetBuffer[0] = 0b11100011;
    packetBuffer[1] = 0;
    packetBuffer[2] = 6;
    packetBuffer[3] = 0xEC;
    packetBuffer[12] = 49;
    packetBuffer[13] = 0x4E;
    packetBuffer[14] = 49;
    packetBuffer[15] = 52;

    Udp.beginPacket(address, config.ntp_port);
    Udp.write(packetBuffer, 48);
    Udp.endPacket();
}

time_t getNtpTime()
{
    IPAddress ntpServerIP;

    while (Udp.parsePacket() > 0) ; // discard any previously received packets
    WiFi.hostByName(config.ntp_server, ntpServerIP);
    Serial.print("NTP Server IP: ");
    Serial.println(ntpServerIP);

    sendNTPpacket(ntpServerIP);
    uint32_t beginWait = millis();

    while (millis() - beginWait < 1500) {
        int size = Udp.parsePacket();

        if (size >= 48) {
            Udp.read(packetBuffer, 48);  // read packet into the buffer
            unsigned long secsSince1900;
            // convert four bytes starting at location 40 to a long integer
            secsSince1900 =  (unsigned long)packetBuffer[40] << 24;
            secsSince1900 |= (unsigned long)packetBuffer[41] << 16;
            secsSince1900 |= (unsigned long)packetBuffer[42] << 8;
            secsSince1900 |= (unsigned long)packetBuffer[43];

            Serial.println(F("Time updated!!!"));
            String epochStr = String(secsSince1900 - 2208988800UL);
            sprintf(last_modified, "%s %s GMT", __DATE__, __TIME__);
            return secsSince1900 - 2208988800UL;
        }
    }
    Serial.println(F("No NTP Response!!!"));
    return 0;
}

time_t ntpProvider()
{
    return getNtpTime();
}

void setNTPprovider()
{
    // Update time
    Serial.println(F("Trying NTP Sync..."));
    Udp.begin(8888);
    setSyncProvider(ntpProvider);
    setSyncInterval(300);
}

String ISOtime()
{
    // Return string.format("%04d-%02d-%02dT%02d:%02d:%02dZ", tm["year"], tm["mon"], tm["day"], tm["hour"], tm["min"], tm["sec"])
    if (timeStatus() == timeSet) {
        String isoTime = String(year()) + "-" +
            leadingZeros(String(month()), 2) + "-" +
            leadingZeros(String(day()), 2) + "T" +
            leadingZeros(String(hour()), 2) + ":" +
            leadingZeros(String(minute()), 2) + ":" +
            leadingZeros(String(second()), 2) + "Z";
        return isoTime;
    } else {
        return "0";
    }
}

String epoch2iso(uint32_t toConvert)
{

    time_t tc = toConvert;

    String isoTime = String(year(tc)) + "-" +
        leadingZeros(String(month(tc)), 2) + "-" +
        leadingZeros(String(day(tc)), 2) + "T" +
        leadingZeros(String(hour(tc)), 2) + ":" +
        leadingZeros(String(minute(tc)), 2) + ":" +
        leadingZeros(String(second(tc)), 2) + "Z";

    return isoTime;
}
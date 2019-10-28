#include <Arduino.h>
#include <SPI.h>
#include <RF24.h>
#include "Config.h"

RF24 radio(RadioCePin, RadioCsPin);
char inputBuffer[32];
uint32_t lastPacketTimestamp = 0;

void PollIncomingMessages();

void ParseMessage();

void setup() {
    Serial.begin(115200);
    while (!Serial);

    if (IsDebugMode) {
        Serial.println(F("Setting up..."));
    }

    radio.begin();
    radio.setChannel(RadioChannelNumber);
    radio.setPALevel(RF24_PA_MAX);
    radio.setDataRate(RF24_250KBPS);
    radio.setAutoAck(1); // Ensure autoACK is enabled
    radio.setRetries(2, 3); // Optionally, increase the delay between retries & # of retries
    radio.setCRCLength(RF24_CRC_8); // Use 8-bit CRC for performance
    radio.enableDynamicPayloads();
    radio.openReadingPipe(1, RadioAddress);
    radio.startListening();

    if (IsDebugMode) {
//        radio.printDetails();
        Serial.println(F("Radio setup done"));
    }

    if (IsDebugMode) {
        Serial.println(F("Setup done :)"));
    }
}

void loop() {
    PollIncomingMessages();
}

void PollIncomingMessages() {
    if (Serial.available()) {
        Serial.readBytes(inputBuffer, 32);
        ParseMessage();
    }
    if (radio.available()) {
        int len = radio.getDynamicPayloadSize();
        radio.read(inputBuffer, len);
        ParseMessage();
    }
}

void ParseMessage() {
    if (IsDebugMode) {
        Serial.print(F("T:"));
        Serial.print(millis());
        Serial.print(F(" (>) MSG="));
        Serial.println(String(inputBuffer));
        Serial.flush();
    }

    uint32_t now = millis();

    if (now - lastPacketTimestamp >= MessageDebouncingTimeMs) {
        lastPacketTimestamp = now;

        if (memcmp_P(inputBuffer, F("Served;"), 7) == 0) {
            Serial.println(ServeTracks[random(NumOfServedTracks)]);
            Serial.flush();
        } else if (memcmp_P(inputBuffer, F("BallOut;"), 8) == 0) {
            Serial.println(BallOutTracks[random(NumOfBallOutTracks)]);
            Serial.flush();
        } else if (memcmp_P(inputBuffer, F("BallLost;"), 9) == 0) {
            Serial.println(BallLostTrackNumber);
            Serial.flush();
        }
    }
}
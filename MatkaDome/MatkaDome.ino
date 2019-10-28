#include <Arduino.h>
#include <NodeID.h>
#include <Fsm.h>
#include <Animation.h>
#include <Adafruit_NeoPixel.h>
#include <SPI.h>
#include <RF24.h>
#include <EEPROM.h>
#include "Config.h"
#include "Headers.h"
#include "Matka.h"
#include "Ball.h"


NodeID nodeID(A0, A7);

Animation display(1, DisplayStep);
Animation animBackground(BackgroundAnimationDurationMs, BackgroundAnimationStep);
Animation animStars(StarDurationMs, StarsAnimationStep);
Animation animIdle(IdleAnimationDurationMs, IdleAnimationStep, Animation::eTimingType::PowN3, Animation::eEasingType::EaseInOut);
Animation animServe(0, ServeAnimationStep);
Animation animBallComing(0, BallComingAnimationStep);

Adafruit_NeoPixel strip(StripLedCount, StripPin, NEO_GRB + NEO_KHZ800);

State Start([]() {
    animServe.Stop();
    animBallComing.Stop();
    animBackground.Start();
    animStars.Start();
    animIdle.Start();
}, NULL, NULL);
State Serving(NULL, NULL, NULL);
State BallComing(NULL, NULL, NULL);
State WaitingForOthers(NULL, NULL, NULL);
State WaitingForMe(NULL, NULL, NULL);
Fsm fsm(&Start);

String inputStr, outputStr;
char inputBuffer[32];
char outputBuffer[32];

Matka matka;

Ball ball;

RF24 radio(RadioCePin, RadioCsPin);

uint8_t starIndex = 0;

uint32_t lastPiezoTime = 0;

void setup() {
    if (IsDebugMode) {
        Serial.begin(115200);
        while (!Serial);
    }

    if (IsDebugMode) {
        Serial.println(F("Setting up..."));
    }

    nodeID.Setup();

    if (IsDebugMode) {
        Serial.print(F("NodeID: K="));
        Serial.print(nodeID.getID());
        Serial.print(F(" N="));
        Serial.println(nodeID.getN());
    }

    fsm.add_transition(&Start, &Serving, Serve, DoServe); // Via Piezo
    fsm.add_timed_transition(&Start, &Serving, IdleTimeoutSeconds * 1000, DoRandomServe); // V
    fsm.add_transition(&Start, &WaitingForOthers, ServedToOthers, []() {}); // V
    fsm.add_transition(&Start, &BallComing, ServedToMe, DoBallComing); // V
    fsm.add_transition(&Serving, &WaitingForOthers, BallOut, DoBallOut); // Via animation step
    fsm.add_transition(&Serving, &Start, ServedToMe, DoBallLost); // V
    fsm.add_transition(&Serving, &Start, ServedToOthers, DoBallLost); // V
    fsm.add_transition(&WaitingForOthers, &Start, BallLost, MakeServerAvailable); // V
    fsm.add_transition(&WaitingForOthers, &BallComing, ServedToMe, DoBallComing); // V
    fsm.add_timed_transition(&WaitingForOthers, &Start, WaitingTimeoutSeconds * 1000, []() {}); // V
    fsm.add_transition(&BallComing, &WaitingForMe, ResponseTime, []() {}); // Via animation step
    fsm.add_transition(&WaitingForMe, &Serving, Serve, DoServe); // Via Piezo
    fsm.add_transition(&WaitingForMe, &Start, BallLost, DoBallLost); // Via animation step

    matka.SetID(&nodeID);

    radio.begin();
    radio.setChannel(RadioChannelNumber);
    radio.setPALevel(RF24_PA_MAX);
    radio.setDataRate(RF24_250KBPS);
    radio.setAutoAck(1); // Ensure autoACK is enabled
    radio.setRetries(2, 3); // Optionally, increase the delay between retries & # of retries
    radio.setCRCLength(RF24_CRC_8); // Use 8-bit CRC for performance
    radio.enableDynamicPayloads();
    radio.openReadingPipe(1, RadioNodeAddresses[nodeID.getID() - 1]);
    radio.startListening();

    if (IsDebugMode) {
//        radio.printDetails();
        Serial.println(F("Radio setup done"));
    }

    display.SetStepTimeMilliseconds(20);
    display.Start();

    strip.begin();
    colorFill(strip.Color(255, 0, 0), 300); // Red
    colorFill(strip.Color(0, 255, 0), 300); // Green
    colorFill(strip.Color(0, 0, 255), 300); // Blue
    colorFill(strip.Color(0, 0, 0), 100); // Off

    uint32_t seed = analogRead(A0);
    randomSeed(seed);

    if (IsDebugMode) {
        Serial.print(F("Seed = "));
        Serial.println(seed);
    }

    if (IsDebugMode) {
        Serial.println(F("Setup done :)"));
    }
}

void loop() {
    display.Refresh();

    PollIncomingMessages();

    PollPiezo();

    fsm.run_machine();
}

void PollPiezo() {
    uint32_t now = millis();
    if (now - lastPiezoTime >= PiezoDebounceTime && analogRead(PiezoPin) > PiezoThreshold) {
        lastPiezoTime = now;
        fsm.trigger(Event::Serve);
    }
}

void PollIncomingMessages() {
    // todo: do we need to wait for other nodes? delay((n - i -1) * sending time)
    if (Serial.available()) {
        inputStr = Serial.readString();
        ParseMessage();
    }

    if (radio.available()) {
        int len = radio.getDynamicPayloadSize();
        radio.read(inputBuffer, len);
        inputStr = String(inputBuffer);
        ParseMessage();
    }
}

void ParseMessage() {
    if (IsDebugMode) {
        Serial.print(F("T:"));
        Serial.print(millis());
        Serial.print(F(" (>) MSG="));
        Serial.println(inputStr);
    }

    if (inputStr.startsWith(F("Served;"))) {
        uint16_t serverID = GetParamString(inputStr, "I:").toInt();
        uint32_t targets = GetParamString(inputStr, "T:").toDouble();
        uint32_t duration = GetParamString(inputStr, "D:").toDouble();
        uint16_t color = GetParamString(inputStr, "C:").toInt();

        matka.SetServerID(serverID);
        matka.SetTargets(targets);
        ball.SetDuration(duration);
        ball.SetColor(color);

        fsm.trigger(targets & (1u << (matka.GetNodeId() - 1u)) ? ServedToMe : ServedToOthers);
    } else if (inputStr.startsWith(F("BallLost;"))) {
        uint16_t serverID = GetParamString(inputStr, "I:").toInt();
        matka.SetServerID(serverID);

        fsm.trigger(BallLost);
    } else if (IsDebugMode && inputStr.startsWith(F("Status;"))) {
        Serial.print(F("Occupied targets: "));
        Serial.print(matka.GetTargets());
        Serial.println();
    } else if (IsDebugMode && inputStr.startsWith(F("Serve;"))) {
        Serial.println(F("Manual serving... "));
        fsm.trigger(Serve);
    } else if (IsDebugMode && inputStr.startsWith(F("Loose;"))) {
        Serial.println(F("Manual ball lost... "));
        fsm.trigger(BallLost);
    }
}

String GetParamString(const String &i_Str, const String &i_Param) {
    unsigned int start = i_Str.indexOf(i_Param) + i_Param.length();
    unsigned int end = i_Str.indexOf(';', start);
    String result = i_Str.substring(start, end);

    return result;
}

void BroadcastMessage(Event i_Event) {
    if (IsDebugMode) {
        Serial.print(F("T:"));
        Serial.print(millis());
        Serial.print(F(" Broadcasting: "));
        Serial.println(i_Event);
    }
    switch (i_Event) {
        case Serve:
            outputStr = String(F("Served;"));
            outputStr.concat(F("I:"));
            outputStr.concat(matka.GetNodeId());
            outputStr.concat(F(";T:"));
            outputStr.concat(matka.GetTargets());
            outputStr.concat(F(";D:"));
            outputStr.concat(ball.GetDuration());
            outputStr.concat(F(";C:"));
            outputStr.concat(ball.GetColor());
            outputStr.concat(F(";"));
            break;
        case BallLost:
            outputStr = String(F("BallLost;"));
            outputStr.concat(F("I:"));
            outputStr.concat(matka.GetNodeId());
            outputStr.concat(F(";"));
        case BallOut:
            outputStr = String(F("BallOut;"));
        default:
            break;
    }

    outputStr.toCharArray(outputBuffer, 32);

    radio.stopListening();

    // Send to MP3 player
    radio.openWritingPipe(RadioMP3Address);
    radio.write(outputBuffer, outputStr.length());

    // Send to all nodes
    for (auto RadioAddress : RadioNodeAddresses) {
        if (RadioAddress == RadioNodeAddresses[nodeID.getID() - 1]) {
            continue;
        }
        radio.openWritingPipe(RadioAddress);
        radio.write(outputBuffer, outputStr.length());
    }

    radio.startListening();

    if (IsDebugMode) {
        Serial.print(F("(<) MSG="));
        Serial.println(outputStr);
    }
}

void DoRandomServe() {
    uint16_t randomNumber = random(0, 100);
    if (randomNumber < RandomServePercent) {
        DoServe();
    }
}

void DoServe() {
    // Serving, stop other animations
    if (IsDebugMode) {
        Serial.println(F("Stopping animations"));
    }
    animIdle.Stop();
    animBallComing.Stop();

    if (IsDebugMode) {
        Serial.println(F("Settings random target(s) and server"));
    }
    matka.SetServerID(matka.GetNodeId());
    matka.SetRandomTargets(1);

    if (IsDebugMode) {
        Serial.println(F("Settings duration and color"));
    }

    // Ball out (speed, color)
    ball.SetDuration(random(BallDurationMinimumMs, BallDurationMaximumMs));
    ball.SetColor((uint8_t) random(0, sizeof(BallColorsPallete) / sizeof(long)));

    if (IsDebugMode) {
        Serial.println(F("Broadcasting"));
    }
    // Board cast "Served To Me/Others" (served, node#, Targets#, speed, color)
    BroadcastMessage(Serve);

    if (IsDebugMode) {
        Serial.println(F("Starting serve animation"));
    }

    animServe.SetDuration(ball.GetDuration());
    // Only after broadcasting start the animation. Give the targets time to initialize
    animServe.Start();
}

void DoBallComing() {
    if (IsDebugMode) {
        Serial.println(F("Ball Coming!"));
    }
    animIdle.Stop();
    MakeServerAvailable();

    animBallComing.SetTimeDelayMilliseconds(ball.GetDuration());
    animBallComing.SetDuration(ball.GetDuration());
    animBallComing.Start();

    if (IsDebugMode) {
        Serial.println(F("Ball Coming animations handle done"));
    }
}

void DoBallLost() {
    if (IsDebugMode) {
        Serial.println(F("Ball Lost :("));
    }
    MakeServerAvailable();
    BroadcastMessage(BallLost);
}

void DoBallOut() {
    if (IsDebugMode) {
        Serial.println(F("Ball Out"));
    }
    BroadcastMessage(BallOut);
}

void MakeServerAvailable() {
//    matka.ClearTargetID(matka.GetServerID());
    matka.ClearTargets();
}

// This makes the rainbow equally distributed throughout the strip
void BackgroundAnimationStep(Animation *const i_Animation, float i_Progress) {
    uint16_t j = 5 * 256 * i_Progress; // 5 cycles of all colors on wheel

    for (uint16_t i = 0; i < strip.numPixels(); i++) {
        uint32_t color = WheelBlueTurquoise(((i * 256u / strip.numPixels()) + j) & 255u, BackgroundBrightness);
//        color = 0x000000;
        strip.setPixelColor(i, color);
    }

    if (i_Progress == 1) {
        i_Animation->Start();
    }
}

void StarsAnimationStep(Animation *const i_Animation, float i_Progress) {
    if (starIndex > 0) {
        strip.setPixelColor(starIndex, StarColor);
    } else {
        starIndex = random(StarLowerIndex, StarUpperIndex);
    }

    if (i_Progress == 1) {
        starIndex = 0;
        i_Animation->SetTimeDelayMilliseconds(random(StarMinimumRandomDelayMs, StarMaximumRandomDelayMs));
        i_Animation->Start();
    }
}

void IdleAnimationStep(Animation *const i_Animation, float i_Progress) {
    uint16_t range = lround(IdleAnimationLedCount * i_Progress);
    uint8_t offset = BaseColorLedCount;

    for (uint16_t i = 0; i < range; i++) {
        strip.setPixelColor(offset + i, 0xFFFFFF);
    }

    if (i_Progress == 1) {
        i_Animation->Start();
    }
}

void ServeAnimationStep(Animation *const i_Animation, float i_Progress) {
    uint8_t offset = BaseColorLedCount;
    int iLength = strip.numPixels() + BallLedCount - offset;
    int start = (iLength * i_Progress) - BallLedCount + offset;
    int end = start + BallLedCount;

    for (uint16_t i = max(0, start); i < min(strip.numPixels(), end); i++) {
        strip.setPixelColor(i, BallColorsPallete[ball.GetColor()]);
    }

    if (i_Progress == 1) {
        fsm.trigger(BallOut);
    }
}

void BallComingAnimationStep(Animation *const i_Animation, float i_Progress) {
    uint8_t offset = BaseColorLedCount;
    int iLength = strip.numPixels() + BallLedCount - offset;
    int start = (iLength * i_Progress) - BallLedCount + offset;
    int end = start + BallLedCount;

    for (uint16_t i = max(0, start); i < min(strip.numPixels(), end); i++) {
        strip.setPixelColor(strip.numPixels() - i, BallColorsPallete[ball.GetColor()]);
    }

    if (i_Progress >= BallComingResponseWindow) {
        fsm.trigger(ResponseTime);
    }

    // If the animation is finished properly = no response from user = ball lost.
    if (i_Progress == 1) {
        fsm.trigger(BallLost);
    }
}

void DisplayStep(Animation *const i_Animation, float i_Progress) {
    // Call the individual animation "Refresh" method
    // The order of the "Refresh" calls, defines the order of writing to the strip (bottom up)
    animBackground.Refresh();
    animStars.Refresh();
    animIdle.Refresh();
    animBallComing.Refresh();
    animServe.Refresh();

    // Set base color
    for (int i = 0; i < BaseColorLedCount; ++i) {
        strip.setPixelColor(i, BaseColors[nodeID.getID() - 1]);
    }

    // Show the final result
    strip.show();

    if (i_Progress == 1) {
        i_Animation->Start();
    }
}

// Fill the dots one after the other with a color
void colorFill(uint32_t c, uint32_t wait) {
    for (uint16_t i = 0; i < strip.numPixels(); i++) {
        strip.setPixelColor(i, c);
    }
    strip.show();
    delay(wait);
}


// Input a value 0 to 255 to get a color value.
// The colours are a transition r - g - b - back to r.
uint32_t Wheel(uint8_t i_Position, float i_Brightness = 1.0f) {
    i_Position = 255 - i_Position;
    if (i_Position < 85) {
        return Adafruit_NeoPixel::Color((255 - i_Position * 3) * i_Brightness, 0, (i_Position * 3) * i_Brightness);
    } else if (i_Position < 170) {
        i_Position -= 85;
        return Adafruit_NeoPixel::Color(0, (i_Position * 3) * i_Brightness, (255 - i_Position * 3) * i_Brightness);
    } else {
        i_Position -= 170;
        return Adafruit_NeoPixel::Color((i_Position * 3) * i_Brightness, (255 - i_Position * 3) * i_Brightness, 0);
    }
}

uint32_t WheelBlueTurquoise(uint8_t i_Position, float i_Brightness = 1.0f) {
    return Adafruit_NeoPixel::Color(0, (i_Position < 127 ? i_Position * 2 : 255 - (i_Position - 127) * 2) * i_Brightness, 255 * i_Brightness);
}
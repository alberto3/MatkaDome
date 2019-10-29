#ifndef __Config_H__
#define __Config_H__


static const bool IsDebugMode = false;
static const uint16_t IdleTimeoutSeconds = 30;
static const uint16_t WaitingTimeoutSeconds = 7;
static const uint16_t IdleAnimationDurationMs = 700;
static const uint8_t IdleAnimationLedCount = 6;
static const uint16_t BackgroundAnimationDurationMs = 15000;
static const float BackgroundBrightness = 0.5f;
static const uint16_t StripLedCount = 150;
static const uint8_t StripPin = 3;
static const uint8_t PiezoPin = A5;
static const uint16_t PiezoThreshold = 80;
static const uint16_t PiezoDebounceTime = 100;
static const uint8_t RandomServePercent = 10;
static const uint8_t RadioCePin = 10;
static const uint8_t RadioCsPin = 9;
static const uint8_t RadioChannelNumber = 88;
static const uint8_t RadioMP3Address[10] = "AE2019MP3";
static const uint8_t RadioNodeAddresses[][10] = {
        "AE2019N1", "AE2019N2", "AE2019N3", "AE2019N4",
        "AE2019N5", "AE2019N6", "AE2019N7", "AE2019N8"/*,
        "AE2019N9", "AE2019N10", "AE2019N11", "AE2019N12"*/
};
static const uint32_t StarColor = 0xFFFFFF;
static const uint32_t StarDurationMs = 100;
static const uint16_t StarLowerIndex = StripLedCount - (StripLedCount / 2); // Upper half of the strip
static const uint16_t StarUpperIndex = StripLedCount - 1; // Last led
static const uint16_t StarMinimumRandomDelayMs = 2545; // 2.545 sec
static const uint16_t StarMaximumRandomDelayMs = 5123; // 55.123 sec
static const uint8_t BallLedCount = 10;
static const uint16_t BallDurationMinimumMs = 1500;
static const uint16_t BallDurationMaximumMs = 2500;
static const uint32_t BallColorsPallete[] = {0x8E44AD, 0xF7DC6F, 0xF7F9F9, 0xF57ABC};
static const float BallComingResponseWindow = 0.85f; // We allow some tolerance. Actually should be StripLedCount / (StripLedCount + BallLedCount)
static const uint32_t BaseColors[] = {
    0xff0000, 0xffff00, 0x7fff00, 0x00ffff,
    0x007fff, 0x0000ff, 0x7f00ff, 0xff00ff
};
static const uint8_t BaseColorLedCount = 27;

#endif

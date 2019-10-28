#ifndef __Config_H__
#define __Config_H__


static const bool IsDebugMode = false;
static const int BallLostTrackNumber = 20;
static const int RadioCePin = 10;
static const int RadioCsPin = 9;
static const int RadioChannelNumber = 88;
static const uint8_t NumOfServedTracks = 4;
static const uint8_t NumOfBallOutTracks = 4;
static const uint32_t MessageDebouncingTimeMs = 1000;
static const uint8_t RadioAddress[10] = "AE2019MP3";
static const uint8_t ServeTracks[NumOfServedTracks] = {1, 2, 3, 4};
static const uint8_t BallOutTracks[NumOfBallOutTracks] = {10, 20, 30, 40};

#endif

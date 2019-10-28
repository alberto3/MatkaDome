#ifndef HEADERS_H
#define HEADERS_H

enum Event {
    Serve,
    ServedToMe,
    ServedToOthers,
    BallLost,
    BallOut,
    ResponseTime
};

void BackgroundAnimationStep(Animation *const i_Animation, float i_Progress);

void IdleAnimationStep(Animation *const i_Animation, float i_Progress);

void ServeAnimationStep(Animation *const i_Animation, float i_Progress);

void BallComingAnimationStep(Animation *const i_Animation, float i_Progress);

void StarsAnimationStep(Animation *const i_Animation, float i_Progress);

void DisplayStep(Animation *const i_Animation, float i_Progress);

void colorFill(uint32_t c, uint32_t wait);

uint32_t WheelRainbow(uint8_t i_Position, float i_Brightness);

uint32_t WheelBlueTurquoise(uint8_t i_Position, float i_Brightness);

void PollIncomingMessages();

void PollPiezo();

void ParseMessage();

String GetParamString(const String &i_Str, const String &i_Param);

void BroadcastMessage(Event i_Event);

void DoServe();

void DoRandomServe();

void DoBallComing();

void DoBallLost();

void DoBallOut();

void MakeServerAvailable();

#endif

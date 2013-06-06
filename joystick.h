#ifndef JOY_H
#define JOY_H

extern int joy;

extern float joyX;
extern float joyY;
extern float joyZ;
extern float joyR;
extern float joyPOV;
extern int joyButtons[32];

extern float joyXLast;
extern float joyYLast;
extern float joyZLast;
extern float joyRLast;
extern float joyPOVLast;
extern int joyButtonsLast[32];

int joyInit();
int joySample();

#endif
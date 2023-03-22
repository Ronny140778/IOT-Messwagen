/*
 * messwagen.h
 *
 *  Created on: 16.01.2023
 *      Author: Ronny
 */

#ifndef MESSWAGEN_H_
#define MESSWAGEN_H_

//scale H0 87, TT 120, N 160
const int scale = 160;
//wheel circumference 2*r*pi or pi*d in mm
const float wheelCircumFerence = 1.948;
//number of magnets
const byte magnets = 1;
//pin hallsensor
const byte interruptPin = 2;
//IP for access point mode
const IPAddress ip(192,168,1,1);
//number of values for average angles
const int sizeAvg = 10;

typedef struct {
  float angleData[sizeAvg];
  uint8_t index;
  float sumAngles;
  float avg;
} avgFilter;

const int WRITTEN_SIGNATURE = 0xBEEFDEED;
const int START_ADDRESS     = 0;

typedef struct {
  float x;
  float y;
} Center;

#define ANGLE_X "angleX"
#define ANGLE_Y "angleY"
#define SPEED_MOD "speedMod"
#define SPEED_ORG "speedOrg"
#define SPEED_ORG_MAX "speedOrgMax"
#define DISTANCE_MOD "distanceMod"
#define DISTANCE_ORG "distanceOrg"
#define ANGLE_X_MIN_MAX "angleXMinMax"
#define ANGLE_Y_MIN_MAX "angleYMinMax"
#define BUTTON_RST_MIN_MAX "1"
#define BUTTON_RST_DST "2"
#define BUTTON_CALIBRATE "3"

#endif /* MESSWAGEN_H_ */

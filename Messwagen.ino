#include <Arduino_LSM6DS3.h>
#include <SPI.h>
#include <WiFiNINA.h>
#include <avr/dtostrf.h>
#include <FlashStorage_SAMD.h>
#include "arduino_secrets.h"
#include "Messwagen.h"
#include "DataView.h"

char ssid[] = SECRET_SSID;
char pass[] = SECRET_PASS;

int status = WL_IDLE_STATUS;
WiFiServer server(80);
DataView dataView;
Center center;
FlashStorage(center_storage, Center);
int counterPrintValues = 0;

const int centerAdressEEPROM = START_ADDRESS + sizeof(WRITTEN_SIGNATURE);

volatile unsigned long cntMillis;
volatile unsigned long lastMillis;
volatile unsigned int cntRotation;
unsigned int lastRotation;

float minAngleX;
float maxAngleX;
float minAngleY;
float maxAngleY;
float maxSpeed;
float distanceM = 0.0;
float distanceOrg = 0.0;
float currSpeedKmH = 0.0;
float currSpeedCmS = 0.0;

avgFilter avgX;
avgFilter avgY;

boolean connectToAP = false;

void isr() {
	unsigned long currMillis = millis();
	cntMillis = currMillis - lastMillis;
	lastMillis = currMillis;
	cntRotation++;
}

void printWifiStatus() {
	// print the SSID of the network you're attached to:
	Serial.print("SSID: ");
	Serial.println(WiFi.SSID());

	// print your board's IP address:
	IPAddress localIp = WiFi.localIP();
	Serial.print("To see this page in action, open a browser to http://");
	Serial.println(localIp);
}

void addToFilter(float angle, avgFilter *p_avg_values) {
	float temp = p_avg_values->angleData[p_avg_values->index];
	p_avg_values->angleData[p_avg_values->index] = angle;
	p_avg_values->sumAngles = p_avg_values->sumAngles + angle - temp;
	p_avg_values->avg = p_avg_values->sumAngles / sizeAvg;
	p_avg_values->index++;
	p_avg_values->index %= sizeAvg;
}

void calcAnglesAddToFilter() {
	float x, y, z;
	IMU.readAcceleration(x, y, z);

	float angleX = atan2(-y, z) * 180 / PI;
	float angleY = atan2(-x, sqrt(y * y + z * z)) * 180 / PI;

	angleX = angleX - center.x;
	angleY = angleY - center.y;

	addToFilter(angleX, &avgX);
	addToFilter(angleY, &avgY);

	if (avgY.avg > maxAngleY) {
		maxAngleY = avgY.avg;
	} else if (avgY.avg < minAngleY) {
		minAngleY = avgY.avg;
	}
	if (avgX.avg > maxAngleX) {
		maxAngleX = avgX.avg;
	} else if (avgX.avg < minAngleX) {
		minAngleX = avgX.avg;
	}
}

void calcSpeedAndDst(int currRotation, long currMillis) {
	distanceM = (currRotation * wheelCircumFerence) / (100 * magnets);
	distanceOrg = distanceM * scale / 1000;
	currSpeedCmS = wheelCircumFerence * 1000 / (currMillis * magnets);
	currSpeedKmH = currSpeedCmS * scale * 3.6 / 100;
	if (currSpeedKmH > maxSpeed) {
		maxSpeed = currSpeedKmH;
	}
}

void resetMaxValues() {
	minAngleX = 0;
	maxAngleX = 0;
	minAngleY = 0;
	maxAngleY = 0;
	maxSpeed = 0;
}

void resetDstValues() {
	distanceM = 0;
	distanceOrg = 0;
	cntRotation = 0;
}

void setup() {
	//Init Interrupt
	pinMode(interruptPin, INPUT_PULLUP);
	attachInterrupt(digitalPinToInterrupt(interruptPin), isr, FALLING);

	//Init Serial
	Serial.begin(9600);
	//	while (!Serial)
	//		;

	//Init IMU
	while (!IMU.begin())
		;
	while (!IMU.accelerationAvailable())
		;
	delay(500);

	initWlan();
	initCenter();

	cntRotation = 0;
	lastRotation = 0;
	resetMaxValues();

	dataView.setRefreshInterval(300);
	dataView.setTitle("Messwagen");
	dataView.addField("Geschw. (Org.): ", SPEED_ORG, "0.0", " km/h");
	dataView.addField("Dist. (Org.): ", DISTANCE_ORG, "0.0", " km");
	dataView.addField("Steigung: ", ANGLE_Y, "0.0", " %");
	dataView.addField("Neigung: ", ANGLE_X, "0.0", " %");
	dataView.addField("Geschwindigkeit: ", SPEED_MOD, "0.0", " cm/s");
	dataView.addField("Distanz: ", DISTANCE_MOD, "0.0", " m");
	dataView.addField("Steig. min/max: ", ANGLE_Y_MIN_MAX, "-000.0 / 000.0",
			" %");
	dataView.addField("Neig. min/max: ", ANGLE_X_MIN_MAX, "-000.0 / 000.0",
			" %");
	dataView.addField("Geschw. max: ", SPEED_ORG_MAX, "0.0", " km/h");
	dataView.addButton("Reset min/max", BUTTON_RST_MIN_MAX);
	dataView.addButton("Reset Distanz", BUTTON_RST_DST);
	dataView.addButton("Kalibrieren", BUTTON_CALIBRATE);
}

void loop() {
	String button = dataView.handleClient(server);
	if (button != NO_BUTTON) {
		if (button.equals(BUTTON_CALIBRATE)) {
			resetCenter();
		} else if (button.equals(BUTTON_RST_DST)) {
			resetDstValues();
		} else if (button.equals(BUTTON_RST_MIN_MAX)) {
			resetMaxValues();
		}
	}
	loopCalculate();
}

void loopCalculate() {
	counterPrintValues++;
	unsigned long currMillis;
	unsigned long millisSinceLastCount;
	unsigned int currRotation;
	noInterrupts();
	currMillis = cntMillis;
	currRotation = cntRotation;
	millisSinceLastCount = millis() - lastMillis;
	interrupts();

	if (millisSinceLastCount > 2000) {
		currSpeedCmS = 0;
		currSpeedKmH = 0;
	} else {
		if (lastRotation < currRotation || millisSinceLastCount > currMillis) {
			calcSpeedAndDst(currRotation, currMillis);
			lastRotation = currRotation;
		}
	}

	calcAnglesAddToFilter();

	setCurrentData(formatFloat(avgY.avg), ANGLE_Y);
	setCurrentData(formatFloat(avgX.avg), ANGLE_X);
	setCurrentData(formatFloat(currSpeedKmH), SPEED_ORG);
	setCurrentData(formatFloat(distanceOrg), DISTANCE_ORG);
	setCurrentData(formatFloat(currSpeedCmS), SPEED_MOD);
	setCurrentData(formatFloat(distanceM), DISTANCE_MOD);
	setCurrentData(formatFloat(maxSpeed), SPEED_ORG_MAX);
	setCurrentData(formatFloat(minAngleY) + " / " + formatFloat(maxAngleY),
	ANGLE_Y_MIN_MAX);
	setCurrentData(formatFloat(minAngleX) + " / " + formatFloat(maxAngleX),
	ANGLE_X_MIN_MAX);
}

void setCurrentData(String value, String fieldID) {
	dataView.setCurrentData(fieldID, value);
}

String formatFloat(float value) {
	char buffer[10];
	dtostrf(value, 6, 1, buffer);
	return buffer;
}

void initWlan() {
	if (strlen(ssid) > 0) {
		Serial.println("SSID configured: scan");
		boolean networkAvailable = false;
		int numSsid = WiFi.scanNetworks();
		for (int thisNet = 0; thisNet < numSsid; thisNet++) {
			if (strcmp(ssid, WiFi.SSID(thisNet)) == 0) {
				networkAvailable = true;
				Serial.println("SSID found");
				break;
			}
		}
		if (networkAvailable) {
			//connect to Wifi-AP
			while (status != WL_CONNECTED) {
				Serial.print("connect to Network: ");
				Serial.println(ssid);
				WiFi.setHostname("Messwagen");
				status = WiFi.begin(ssid, pass);
				connectToAP = true;
				// wait for connection:
			}
		}
	}

	if (!connectToAP) {
		Serial.println("Start Wifi-AP");
		status = WiFi.beginAP("Messwagen");
		WiFi.config(ip);
		if (status != WL_AP_LISTENING) {
			Serial.println("Creating access point failed");
			// don't continue
			while (true)
				;
		}
	}

	delay(1000);
	server.begin();
	printWifiStatus();
}

void initCenter() {
	int signature;
	EEPROM.get(START_ADDRESS, signature);
	// If the EEPROM doesn't store valid data, then no WRITTEN_SIGNATURE
	if (signature != WRITTEN_SIGNATURE) {
		EEPROM.put(START_ADDRESS, WRITTEN_SIGNATURE);
		if (!EEPROM.getCommitASAP()) {
			EEPROM.commit();
		}
		resetCenter();
	} else {
		EEPROM.get(centerAdressEEPROM, center);
	}
}

void resetCenter() {
	//initCenter
	center.x = 0;
	center.y = 0;
	for (int i = 0; i < sizeAvg; i++) {
		calcAnglesAddToFilter();
	}
	center.x = avgX.avg;
	center.y = avgY.avg;

	EEPROM.put(centerAdressEEPROM, center);
	if (!EEPROM.getCommitASAP()) {
		EEPROM.commit();
	}

}

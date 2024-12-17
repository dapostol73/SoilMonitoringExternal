
/*********************************************************************
  This is an example for our Monochrome OLEDs based on SH110X drivers

  This example is for a 128x64 size display using I2C to communicate
  3 pins are required to interface (2 I2C and one reset)

  Adafruit invests time and resources providing this open source code,
  please support Adafruit and open-source hardware by purchasing
  products from Adafruit!

  Written by Limor Fried/Ladyada  for Adafruit Industries.
  BSD license, check license.txt for more information
  All text above, and the splash screen must be included in any redistribution

  i2c SH1106 modified by Rupert Hirst  12/09/21
*********************************************************************/

#include <Arduino.h>
#include <Wire.h>
#include <WiFi.h>

#include "DisplayMain.h"
#include "SensorData.h"

#define SERIAL_LOGGING
#ifndef SERIAL_LOGGING
// disable Serial output
#define Serial KillDefaultSerial
static class {
public:
    void begin(...) {}
    void print(...) {}
    void println(...) {}
} Serial;
#endif

#define MS_SENSOR0 A3
#define SERIAL_BAUD_RATE 115200

ApplicationSettings appSettings; //change to pointer
NetworkManager netManager;
DisplayMain displayMain;

const uint8_t SENSORS_COUNT = 8;
const uint16_t SENSORS_UPDATE_SECS = 1; // Sensor query every minute
const uint8_t sensorPins[SENSORS_COUNT] = { A3, A4, A5, A6, A14, A15, A16, A17 };
SensorData sensorData[SENSORS_COUNT];
long sensorTimeLastUpdate = LONG_MIN;

/* Uncomment the initialize the I2C address , uncomment only one, If you get a totally blank screen try the other*/
#define i2c_Address 0x3c //initialize with the I2C addr 0x3C Typically eBay OLED's
//#define i2c_Address 0x3d //initialize with the I2C addr 0x3D Typically Adafruit OLED's

int16_t clamp(int16_t value, int16_t minimum, int16_t maximum);
void blinkLed(uint8_t times, uint8_t interval);
void configureWiFi();
void configureSensors();
void updateSensors();

void setup()
{
    Serial.begin(SERIAL_BAUD_RATE);
	#ifdef SERIAL_LOGGING
		while(!Serial);
	#endif

    // initialize digital pin PB2 as an output.
    pinMode(BUILTIN_LED, OUTPUT);

    delay(250); // wait for the OLED to power up
    displayMain.init();
	//configureWiFi();
	configureSensors();
	blinkLed(3, 250);	
	Serial.println("Setup complete!");
}

void loop()
{
    if (millis() - sensorTimeLastUpdate > (1000L*SENSORS_UPDATE_SECS))
    {
        #ifdef SERIAL_LOGGING
        Serial.println("Setting updateCurrentWeather to true");
        #endif
        updateSensors();
        sensorTimeLastUpdate = millis();
    }

    TouchPoint touch = displayMain.DisplayTouch->getTouch();

	// Display touches that have a pressure value (Z)
	if (touch.zRaw != 0)
    {
        #ifdef SERIAL_LOGGING
        Serial.print("Touch at X: ");
        Serial.print(touch.x);
        Serial.print(", Y: ");
        Serial.println(touch.y);
        #endif
        char info[48] = "";
        sprintf(info, "Last Touch at X: %d Y: %d", touch.x, touch.y);
        displayMain.DisplayGFX->fillRect(0, 0, 480, 32, BACKGROUND_COLOR);
        displayMain.drawString(info, 4, 4, TEXT_LEFT_TOP, TEXT_MAIN_COLOR);
		//uint16_t x = clamp(touch.x, 5, 475);
		//uint16_t y = clamp(touch.y, 5, 315);
		//displayMain.DisplayGFX->fillCircle(x, y, 4, GREEN);
    }
}

void configureSensors()
{
    uint16_t w = 60;
    for (uint16_t i = 0; i < SENSORS_COUNT; i++)
    {
		sensorData[i].init(sensorPins[i], i, (i * w), 32);
        displayMain.updateMositureMeter(&sensorData[i], true);
    }
}

void updateSensors()
{
    for (uint16_t i = 0; i < SENSORS_COUNT; i++)
    {
        sensorData[i].readValue();
        displayMain.updateMositureMeter(&sensorData[i]);
    }
}

void blinkLed(uint8_t times, uint8_t interval)
{
    for (uint8_t i = 0; i < times; i++)
    {
        digitalWrite(BUILTIN_LED, HIGH);   // turn the LED on (HIGH is the voltage level)
        delay(interval);               // wait for 100mS
        digitalWrite(BUILTIN_LED, LOW);    // turn the LED off by making the voltage LOW
        delay(interval);               // wait for 100mS
    }
}

void printConnectInfo()
{
	// get your MAC address
	byte mac[6];
	WiFi.macAddress(mac);
	IPAddress ip = WiFi.localIP();
	
	// SSID
	char ssidInfo[34] = "";
	sprintf(ssidInfo, "WiFi SSID: %s", WiFi.SSID());

	// MAC address
	char macInfo[34] = "";
	sprintf(macInfo, "MAC address: %02X:%02X:%02X:%02X:%02X:%02X", mac[5], mac[4], mac[3], mac[2], mac[1], mac[0]);

	// IP address
	char ipInfo[34] = "";
	sprintf(ipInfo, "IP address: %u.%u.%u.%u", ip[0], ip[1], ip[2], ip[3]);

    displayMain.clearDisplay();
	displayMain.printLine(ssidInfo, SUCCESS_COLOR);
	displayMain.printLine(ipInfo, TEXT_MAIN_COLOR);
	displayMain.printLine(macInfo, TEXT_MAIN_COLOR);

	#ifdef SERIAL_LOGGING
	Serial.println(ssidInfo);
	Serial.println(ipInfo);	
	Serial.println(macInfo);
	#endif
}

void resolveAppSettings()
{
	int8_t numNetworks = WiFi.scanNetworks();
	#ifdef SERIAL_LOGGING
	Serial.println("Number of networks found: " + String(numNetworks));
	#endif

	if (numNetworks == 0)
	{
		delay(2500);
		numNetworks = WiFi.scanNetworks();
	}

	for (uint8_t i=0; i<numNetworks; i++)
	{
		String ssid = WiFi.SSID(i);
		#ifdef SERIAL_LOGGING
		Serial.println(ssid + " (" + String(WiFi.RSSI(i)) + ")");
		#endif
		for (uint8_t j=0; j < AppSettingsCount; j++)
		{
			const char* appSsid = AppSettings[j].WifiSettings.SSID;
			#ifdef SERIAL_LOGGING
			Serial.println("Checking: " + String(appSsid));
			#endif
			if (strcasecmp(appSsid, ssid.c_str()) == 0)
			{
				#ifdef SERIAL_LOGGING
				Serial.println("Found: " + String(ssid));
				#endif
				AppSettings[j].WifiSettings.Avialable = true;
				AppSettings[j].WifiSettings.Strength = WiFi.RSSI(i);

				if (AppSettings[j].WifiSettings.Strength > appSettings.WifiSettings.Strength)
				{
					appSettings = AppSettings[j];
				}
			}
		}
	}

	#ifdef SERIAL_LOGGING
	Serial.println("Using WiFi " + String(appSettings.WifiSettings.SSID));
	#endif
	WiFi.disconnect();	
}

bool connectToWiFi(uint8_t retries)
{
	String scanMsg = "Scanning for WiFi SSID.";
    displayMain.printLine(scanMsg);
	#ifdef SERIAL_LOGGING
	Serial.println(scanMsg);
	#endif
	resolveAppSettings();

    char infoMsg[] = "Waiting for connection to WiFi";
	if (!appSettings.WifiSettings.Avialable)
	{
		char connectErr[48] = "";
		sprintf(connectErr, "No WiFi connections found for %s!", appSettings.WifiSettings.SSID);
		#ifdef SERIAL_LOGGING
		Serial.println(connectErr);
		#endif
		displayMain.fillScreen(ERROR_COLOR);
		displayMain.drawString(connectErr, 400, 240, TEXT_CENTER_MIDDLE, BLACK, ERROR_COLOR);
		return false;
	}

	WiFi.setSleep(false);
	WiFi.begin(appSettings.WifiSettings.SSID, appSettings.WifiSettings.Password);
    displayMain.printLine(infoMsg);
	#ifdef SERIAL_LOGGING
	Serial.println(infoMsg);
	#endif

	uint8_t attempts = 0;
	uint8_t attemptMax = 3;
	while (WiFi.status() != WL_CONNECTED && attempts < attemptMax)
	{
		delay(1000);
        //display.print('.');
        //display.display();
		#ifdef SERIAL_LOGGING
		Serial.print('.');
		#endif
		++attempts;
	}
    //display.println();
    //display.display();
	#ifdef SERIAL_LOGGING
	Serial.println();
	#endif

	uint8_t retry = 0;
	while(WiFi.status() != WL_CONNECTED)
	{
		if (retry < retries)
		{
			connectToWiFi(0);
			++retry;
		}
		else
		{
			return false;
		}
		
	}

	return true;
}

void configureWiFi()
{
	String intialMsg = "Intializing WiFi module.";
    displayMain.clearDisplay();
    displayMain.printLine(intialMsg);
	#ifdef SERIAL_LOGGING
	Serial.println(intialMsg);
	#endif

	WiFi.mode(WIFI_MODE_STA);
	delay(1000);
	WiFi.disconnect();
	WiFi.setAutoConnect(true);

	if (WiFi.status() == WL_NO_SHIELD)
	{
		String initialErr = "Communication with WiFi module failed!";
        displayMain.printLine(initialErr);
		#ifdef SERIAL_LOGGING
		Serial.println(initialErr);
		#endif
		// don't continue
		while (true)
			;
	}

	if (!connectToWiFi(2))
	{
		// don't continue
		while (true)
			;
	}
		
	printConnectInfo();
	delay(2000);
}

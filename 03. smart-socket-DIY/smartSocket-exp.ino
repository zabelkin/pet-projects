#include <ESP8266WiFi.h> 	// addressing the board
#include <WiFiUdp.h> 		// wifi connections
#include <PubSubClient.h>	// MQTT interactions

#define timeLong 60000L // pub/sub timeout - period in seconds to connect to MQTT server
#define timeShort 20000L // UDP timeout - period for UDP try to time server
#define GTM_MSK 10800L // Moscow time zone (GTM+3) - feel free to set your own

unsigned long timeMarkLong;   // pub/sub last time
unsigned long timeMarkShort;  // UDP last time
unsigned long lastNTPResponse;// last succesful UDP session time
uint32_t timeUNIX;

const char* ssid     = "xxx"; // your home wifi SSID
const char* password = "xxx"; // your home wifi netword password
const char* NTPServerName = "ntp3.stratum2.ru"; // time server
IPAddress timeServerIP;
const int NTP_PACKET_SIZE = 48;  // NTP time stamp is in the first 48 bytes of the message
byte NTPBuffer[NTP_PACKET_SIZE]; // buffer to hold incoming and outgoing packets
WiFiUDP UDP;

// you get all MQTT credential at registration at service (or by setting up your own)
const char *mqtt_server = "xxx"; // MQTT server name
const int mqtt_port = xxx; // MQTT server port ()
const char *mqtt_user = "xxx"; // login
const char *mqtt_pass = "xxx"; // password
WiFiClient wclient; 
PubSubClient client(wclient, mqtt_server, mqtt_port);

uint8_t portOnOff;
uint8_t portManualOnOff;
float portTimeOn;
float portTimeOff;
String portMode;
uint16_t portDutyMin;
uint16_t portPauseMin;


/*
 * declarations and utility functions
 */
void sendNTPpacket(IPAddress address);
uint32_t getTime();
void callback(const MQTT::Publish& pub);
inline int getSeconds(uint32_t UNIXTime)
{
  return UNIXTime % 60;
}
inline int getMinutes(uint32_t UNIXTime)
{
  return UNIXTime / 60 % 60;
}
inline int getHours(uint32_t UNIXTime)
{
  return UNIXTime / 3600 % 24;
}

/*
* setup code
*/
void setup()
{
  Serial.begin(115200);
  Serial.print("\n\nConnecting to ");
  Serial.println(ssid);
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);

	while (WiFi.status() != WL_CONNECTED)
	{
		delay(500);
		Serial.print(".");
	}
	Serial.println("\nWiFi connected");
	Serial.print("IP address: ");
	Serial.println(WiFi.localIP());
  
	UDP.begin(123); 
	if(!WiFi.hostByName(NTPServerName, timeServerIP))
	{ // Get the IP address of the NTP server
		Serial.println("DNS lookup failed. Rebooting.");
		Serial.flush();
		ESP.reset();
	}
  // Serial.print("Time server IP:\t");
  // Serial.println(timeServerIP);
	timeMarkShort = millis();
	timeMarkLong = timeMarkShort + timeLong + 1;
	lastNTPResponse = timeMarkShort;
	timeMarkShort = timeMarkShort + timeShort + 1 ;
	timeUNIX = 0;
	
	client.set_callback(callback);
	
	portOnOff = 0;
	pinMode(0, OUTPUT);
	digitalWrite(0, 0);
	pinMode(2, OUTPUT);
	digitalWrite(2, portOnOff);
	portManualOnOff = 0;
}

//
// main cycle
//
void loop()
{
	uint32_t actualTime = 0;
	float f_actualTime; // to compare in timer mode, in "hh.mm" format
	String payload;
	
	
	if (millis() - timeMarkShort > timeShort)
	{
		// UDP action
		timeMarkShort = millis();
		// Serial.println("\r\nSending NTP request ...");
		sendNTPpacket(timeServerIP); 
	}
	// trying to parse server response every cycle of loop()
	uint32_t time = getTime();          // Check if an NTP response has arrived and get the (UNIX) time
	if (time)
	{                                  // If a new timestamp has been received
		timeUNIX = time;
		// Serial.print("NTP response:\t"); // debug NTP server message, if needed
		// Serial.println(timeUNIX);
		lastNTPResponse = millis();
	} else if ((millis() - lastNTPResponse) > 300000UL) 
    {
		Serial.println("More than 15 minutes hsince last NTP response. Rebooting."); // well, it is just a cheap ESP8266, sometimes reboots
		Serial.flush();
		ESP.reset();
    }

	if (millis() - timeMarkLong > timeLong)
	{
		// pub/sub action
		timeMarkLong = millis();
		if (timeUNIX>0)
		{ // at least one of UDP request succeded 
			actualTime = timeUNIX + (millis() - lastNTPResponse)/1000 + GTM_MSK;
			// Serial.printf("\rActual time:\t%02d:%02d:%02d   \n", getHours(actualTime), getMinutes(actualTime), getSeconds(actualTime)); // debug 
			f_actualTime = (float) (getHours(actualTime) + getMinutes(actualTime)/100.0);
		}
		// sorting out what to put into portOnOff once a minute, based on subs
		// here you descipher your MQTT subscriptions
		if (portMode=="manual")
		{
			portOnOff = portManualOnOff;
		}
		if (portMode=="timer")
		{
			portOnOff = 0;
			if (portTimeOn<=f_actualTime&&portTimeOff>=f_actualTime) portOnOff = 1;
		}
		if (portMode=="duty")
		{
			portOnOff = 0; // a plug. Never od a need, so never programmed :)
		}
		digitalWrite(2, portOnOff); // get the value to port finally!
		Serial.print("portOnOff: ");
		Serial.println(String(portOnOff));
		if (WiFi.status() == WL_CONNECTED) // подключаемся к MQTT серверу
		{
			if (!client.connected())
			{
				Serial.print("Connecting to MQTT server, ");
				if (client.connect(MQTT::Connect("arduinoClient2").set_auth(mqtt_user, mqtt_pass)))
				{
					digitalWrite(0, 1); // "all good" indicator - attempt
					Serial.println("connected.");
					client.subscribe("socket1/Mode"); // subscribe
					client.subscribe("socket1/onTime");
					client.subscribe("socket1/offTime");
					client.subscribe("socket1/dutyMinutes");
					client.subscribe("socket1/pauseMinutes");
					client.subscribe("socket1/manualSwitch");
						//Serial.print("portOnOff: ");
						//Serial.println(String(portOnOff));
					client.publish("socket1/_status", String(portOnOff));
					if (actualTime>0)
					{
						payload = String((float) getHours(actualTime)+getMinutes(actualTime)/100.0, 2);
						client.publish("_systemTime", payload); // saving system time
					}
				} else
				{
					Serial.println(" failed! Could not connect to MQTT server");
					digitalWrite(0, 0); // failed so internet connection indicator goes Off
				}
			} else // already connected to MQTT servser
			{
				client.publish("socket1/_status", String(portOnOff));
					//Serial.print("portOnOff: "); // debug
					//Serial.println(String(portOnOff));
				if (actualTime>0)
				{
					payload = String((float) getHours(actualTime)+getMinutes(actualTime)/100.0, 2);
					client.publish("_systemTime", payload);
				}
			}
		}
		if (client.connected())
			client.loop();
		
	} // end of 1 min actions
}

/*
*	functions
* go to callback function to denote your MQTT subscribtions
* two anotehr functions are auxilary one for real time management
*/

void sendNTPpacket(IPAddress address)
{
	memset(NTPBuffer, 0, NTP_PACKET_SIZE);  // set all bytes in the buffer to 0
	// Initialize values needed to form NTP request
	NTPBuffer[0] = 0b11100011;   // LI, Version, Mode
	// send a packet requesting a timestamp:
	UDP.beginPacket(address, 123); // NTP requests are to port 123
	UDP.write(NTPBuffer, NTP_PACKET_SIZE);
	UDP.endPacket();
}

uint32_t getTime()
{
	if (UDP.parsePacket() == 0)
	{ // If there's no response (yet)
		return 0;
	}
	UDP.read(NTPBuffer, NTP_PACKET_SIZE); // read the packet into the buffer
	// Combine the 4 timestamp bytes into one 32-bit number
	uint32_t NTPTime = (NTPBuffer[40] << 24) | (NTPBuffer[41] << 16) | (NTPBuffer[42] << 8) | NTPBuffer[43];
	// Convert NTP time to a UNIX timestamp:
	// Unix time starts on Jan 1 1970. That's 2208988800 seconds in NTP time:
	const uint32_t seventyYears = 2208988800UL;
	// subtract seventy years:
	uint32_t UNIXTime = NTPTime - seventyYears;
  return UNIXTime;
}

void callback(const MQTT::Publish& pub)
{
	/*Serial.print(pub.topic()); // just a debug code
	Serial.print(" => ");
	Serial.println(pub.payload_string());
	*/
	String payload = pub.payload_string();
	if(String(pub.topic()) == "socket1/Mode")
	{
		portMode = payload; // operating mode change
		Serial.print("\nOperating mode has changed to "); Serial.println(portMode);
		// portOnOff = 0; // switch it off just in case
	}
	if(String(pub.topic()) == "socket1/manualSwitch")
	{
		portManualOnOff = payload.toInt();
		Serial.print("\nManual switch has changed to "); Serial.println(portManualOnOff);
	}
	if(String(pub.topic()) == "socket1/onTime")
	{
		portTimeOn = (float) (payload.substring(0,2).toInt() + payload.substring(3,5).toInt()/100.0);
		Serial.print("\nOn time changed to "); Serial.println(portTimeOn);
	}
	if(String(pub.topic()) == "socket1/offTime")
	{
		portTimeOff = (float) (payload.substring(0,2).toInt() + payload.substring(3,5).toInt()/100.0);
		Serial.print("\nOff time changed to "); Serial.println(portTimeOff);
	}
	if(String(pub.topic()) == "socket1/dutyMinutes")
	{
		portDutyMin = payload.toInt();
		Serial.print("\nDuty minutes have changed to "); Serial.println(portDutyMin);
	}
	if(String(pub.topic()) == "socket1/pauseMinutes")
	{
		portPauseMin = payload.toInt();
		Serial.print("\nPause minutes have changed to "); Serial.println(portPauseMin);
	}
}

#include <WiFiNINA.h>
#include <utility/wifi_drv.h>
#include <EEPROM.h>

const int pin = 2;
const int addr = 0;
const long reportFreq = 3600000;
const int heartBeat = 10000;
long lastUpdate = 0;
volatile long counter = 0;
String incoming = "";
unsigned long lastPing = 0;
volatile unsigned long loopTime = 0;
boolean alreadyConnected = false;
boolean wifiConnected = false;

//WiFi Settings
char ssid[] = "us007shop";
char pass[] = "MdcW31r#";

//MTC Adapter
WiFiServer server(7878);
WiFiClient client;

//determine status LEDs
void setStatus()
{
  if (checkCon())
  {
    WiFiDrv::digitalWrite(25, LOW);
    WiFiDrv::digitalWrite(26, LOW);
    WiFiDrv::digitalWrite(27, HIGH);
  }
  else if (wifiConnected)
  {
    WiFiDrv::digitalWrite(25, LOW);
    WiFiDrv::digitalWrite(26, HIGH);
    WiFiDrv::digitalWrite(27, LOW);
  }
  else
  {
    WiFiDrv::digitalWrite(25, HIGH);
    WiFiDrv::digitalWrite(26, LOW);
    WiFiDrv::digitalWrite(27, LOW);
  }
}

//check connection
boolean checkCon()
{
  if (alreadyConnected)
  {
    if (!(loopTime - lastPing >= 2 * heartBeat))
    {
      return true;
    }
    alreadyConnected = false;
    client.stop();
  }
  return false;
}

void connectWifi()
{ //Connect to WiFi
  setStatus();
  int statusWiFi = WiFi.status();
  if (statusWiFi != WL_CONNECTED)
  {
    wifiConnected = false;
    statusWiFi = WiFi.begin(ssid, pass);
    delay(7000); //Wait for XXXXms seconds for connection
    if (statusWiFi == WL_CONNECTED)
    {
      wifiConnected = true;
    }
  }
}

void updateCounters()
{ //Update the EEPROM and Agent
  if (loopTime - lastUpdate >= reportFreq && alreadyConnected)
  {
    lastUpdate = loopTime;
    saveTotal();
    noInterrupts();
    client.println("|flow|" + String(counter));
    interrupts();
  }
}

void increment()
{ //Counter
  counter += 1;
}

void readTotal()
{ //Read Counter from EEPROM
  counter = EEPROM.get(addr, counter);
}

void saveTotal()
{ //Save counter to EEPROM
  EEPROM.put(addr, counter);
}

void setup()
{ //Run Before Loop
  //Device
  WiFiDrv::pinMode(25, OUTPUT); //RED
  WiFiDrv::pinMode(26, OUTPUT); //GREEN
  WiFiDrv::pinMode(27, OUTPUT); //BLUE
  WiFiDrv::digitalWrite(25, HIGH);
  pinMode(pin, INPUT_PULLUP);
  attachInterrupt(pin, increment, RISING);
  readTotal();

  //MTC Adapter
  server.begin();
}

void loop()
{ //Main Loop
  connectWifi();
  loopTime = millis();
  client = server.available();
  if (client)
  {
    while (client.available())
    {
      char c = client.read();
      incoming += c;
      if (incoming == "* PING\n")
      {
        lastPing = loopTime;
        noInterrupts();
        client.println("* PONG " + String(heartBeat));
        interrupts();
        incoming = "";
        if (!alreadyConnected)
        {
          noInterrupts();
          client.println("|avail|AVAILABLE|flow|" + String(counter));
          interrupts();
          alreadyConnected = true;
          lastUpdate = loopTime;
        }
        else
        {
          updateCounters();
        }
      }
    }
  }
}
/*********WICAN Sistem Intrumentasi dan kontrol Competition *********/
/*********Source Code Belongs to David Fauzi  *********/
/*********https://github.com/davidf1000  *********/
// Declare Library
//SEND
#include <ESP8266WiFi.h>
#include "Adafruit_MQTT.h"
#include "Adafruit_MQTT_Client.h"
/************************* WiFi Access Point *********************************/

#define WLAN_SSID "Redmi"
#define WLAN_PASS "12345678"

/************************* Adafruit.io Setup *********************************/

#define AIO_SERVER "io.adafruit.com"
#define AIO_SERVERPORT 1883 // use 8883 for SSL
#define AIO_USERNAME "davidfauzi"
#define AIO_KEY "0f7873c4c7bf48bb95e79d4112889e8e"

/************************* Ticker Declaration *********************************/
/************ Global State (Don't Change) ******************/

// Create an ESP8266 WiFiClient class to connect to the MQTT server.
WiFiClient client;
// or... use WiFiFlientSecure for SSL
//WiFiClientSecure client;

// Setup the MQTT client class by passing in the WiFi client and MQTT server and login details.
Adafruit_MQTT_Client mqtt(&client, AIO_SERVER, AIO_SERVERPORT, AIO_USERNAME, AIO_KEY);

/****************************** Feeds ***************************************/

// Setup a feed called 'photocell' for publishing.
// Notice MQTT paths for AIO follow the form: <username>/feeds/<feedname>
Adafruit_MQTT_Publish humidity = Adafruit_MQTT_Publish(&mqtt, AIO_USERNAME "/feeds/rawsensor.humidity");
Adafruit_MQTT_Publish temperature = Adafruit_MQTT_Publish(&mqtt, AIO_USERNAME "/feeds/rawsensor.temperature");
Adafruit_MQTT_Publish gasleak = Adafruit_MQTT_Publish(&mqtt, AIO_USERNAME "/feeds/rawsensor.gasleakage");
Adafruit_MQTT_Publish sound = Adafruit_MQTT_Publish(&mqtt, AIO_USERNAME "/feeds/rawsensor.noise");
Adafruit_MQTT_Publish dust = Adafruit_MQTT_Publish(&mqtt, AIO_USERNAME "/feeds/rawsensor.airquality");
Adafruit_MQTT_Publish notif = Adafruit_MQTT_Publish(&mqtt, AIO_USERNAME "/feeds/dataguru.notif");

//SENSOR DATA

//DUST SENSOR
#define USE_AVG //calculate average value in N mesared values (dust )
#include <Wire.h>
#include <Adafruit_ADS1015.h>

// DHT
#include <Adafruit_Sensor.h>
#include <DHT.h>
#include <DHT_U.h>

//TIMER
#include <Ticker.h>
Ticker storedata;
//filtering
#include <Filter.h>
ExponentialFilter<float> dustparticlefilter(30, 0);
ExponentialFilter<float> temperaturefilter(60, 20);
ExponentialFilter<float> humidityfilter(60, 50);
ExponentialFilter<float> gasleakfilter(85, 0);
ExponentialFilter<int> filter(20, 55);
// MQ
#include <MQ2.h>
const int pin = A0;
int lpg, co, smoke;

MQ2 mq2(pin);
//relay 1 and relay 2
#define relay1 D0
#define relay2 D8
//EEPROM
#include <EEPROM.h>

// Declare define

//DUST SENSOR

//#ifdef USE_AVG
//DUST

#define N 50

// DHT
#define DHTPIN 13
#define DHTTYPE DHT11

//SOUND

// RELAY
#define relay1 D0
#define relay2 D8
// DRIVER
#define driverdigital1 D4
#define driverdigital2 D5
#define driverpwm D6

// Inisiasi library

//DUST SENSOR
Adafruit_ADS1115 ads(0x48);

// MQ2

// DHT
DHT_Unified dht(DHTPIN, DHTTYPE);

//SOUND

int cal;

//inisiasi variabel
//DUST SENSOR
float Voltage = 0.0;
const int sharpLEDPin = D3; // Arduino digital pin 7 connect to sensor LED.
static unsigned long VoRawTotal = 0;
static int VoRawCount = 0;
static float Voc = 0.6;  //Voc is 0.6 Volts for dust free acordind sensor spec
static float VocT = 0.6; //Self calibration overvrite the Coc with minimum measered value - this variable you need to have the treshold for the sensor  (facturi calibration)
const float K = 0.5;
float dataDust;
// MQ2
long lpgLevel;
// DHT
int dhtTemp;
int dhtHum;
int dataTemp;
int dataHum;
uint32_t delayMS;
//SOUND
//DECLARE FUNGSI
void MQTT_connect();
void MQTT_send();
void upgrade_cumm();
void store_cumm();
void dhtget();
int calibratesound();
void measureLPG();
void measureSound();
void dustSensor();

void setup()
{
    Wire.begin(D2, D1);
    Serial.begin(115200);
    ads.begin();
    //relay
    pinMode(relay1, OUTPUT);
    pinMode(relay2, OUTPUT);
    // DUST
    pinMode(sharpLEDPin, OUTPUT);
    VocT = Voc;
    // MQ2

    // DHT
    pinMode(DHTPIN, INPUT_PULLUP);
    dht.begin();
    sensor_t sensor;
    dht.temperature().getSensor(&sensor);
    dht.humidity().getSensor(&sensor);
    delayMS = sensor.min_delay / 1000;
    //SOUND
    int soundsensor;
    //EEPROM
    EEPROM.begin(512);

    // TICKER

    //WIFI PART
    Serial.println();
    Serial.println();
    Serial.print("Connecting to ");
    Serial.println(WLAN_SSID);
    WiFi.begin(WLAN_SSID, WLAN_PASS);
    while (WiFi.status() != WL_CONNECTED)
    {
        delay(500);
        Serial.print(".");
    }
    Serial.println();
    Serial.println("WiFi connected");
    Serial.println("IP address: ");
    Serial.println(WiFi.localIP());
}

void loop()
{
    // put your main code here, to run repeatedly:
    dustSensor();
    measureLPG();
    measureSound();
    dhtget();
    MQTT_send();
    delay(100);
}

void dustSensor()
{
    // Turn on the dust sensor LED by setting digital pin LOW.
    digitalWrite(sharpLEDPin, LOW);
    // Wait 0.28ms before taking a reading of the output voltage as per spec.
    delayMicroseconds(280);
    // Record the output voltage. This operation takes around 100 microseconds.
    int VoRaw = ads.readADC_SingleEnded(0);
    // Turn the dust sensor LED off by setting digital pin HIGH.
    digitalWrite(sharpLEDPin, HIGH);
    // Wait for remainder of the 10ms cycle = 10000 - 280 - 100 microseconds.
    delayMicroseconds(9620);
    // Print raw voltage value (number from 0 to 1023).
    // Use averaging if needed.
    float Vo = VoRaw;
    VoRawTotal += VoRaw;
    VoRawCount++;
    if (VoRawCount >= N)
    {
        Vo = 1.0 * VoRawTotal / N;
        VoRawCount = 0;
        VoRawTotal = 0;
    }
    else
    {
        return;
    }

    // Compute the output voltage in Volts.
    Vo = (Vo * 0.1875) / 1000;
    //------------ Dust density calculated by Voc set in the begining - like in sensor spec  ------------
    // Calculate Dust Density in units of ug/m3 for the initial Voc (as in sensor specs)
    float dV = Vo - VocT;
    if (dV < 0)
    {
        dV = 0;
    }
    float dustDensity = dV / K * 100.0;
    String SPT1 = ""; // add this to serial print the set Voc
    SPT1 = ("Dust Density(Voc ");
    SPT1 += VocT;
    SPT1 += (" V)");

    // printFValue(SPT1, dustDensity, "ug/m3", false); // Print the values
    dV = 0;          // Reset the dV to 0 for the next calculation
    dustDensity = 0; // Reset the dustDensity to 0 for the next calculation
    //------------ Dust density calculated by lowest output voltage during runtime  ------------
    // Convert to Dust Density in units of ug/m3.
    // During runtime, the Voc value is updated dynamically whenever a lower output voltage is sense.
    // this cover the dust sensing below the sensors specefied range from 0 to Voc (0,6V) as specified in the beginin
    dV = Vo - Voc;
    if (dV < 0)
    {
        dV = 0;
        Voc = Vo;
    }
    dustDensity = dV / K * 100.0;
    SPT1 = "";
    SPT1 = ("Dust Density(Voc ");
    SPT1 += Voc;
    SPT1 += (" V)");
    dustparticlefilter.Filter(dustDensity);
    dataDust = dustparticlefilter.Current();
    //printFValue(SPT1, dustDensity, "ug/m3", true);
    Serial.print("Density :");
    Serial.print(dustDensity);
    Serial.print(" ug/m3");
    Serial.println();
    Serial.print(" Density :");
    Serial.print(dataDust);
    Serial.print(" ug/m3 filtered");
    Serial.println();
}
void measureLPG() // pindahin ke pin A0 biar lgsung bisa dipake library
{
    lpg = mq2.readCO();
    gasleakfilter.Filter(lpg);
    lpgLevel = gasleakfilter.Current();
    Serial.print("Gas Leak level : ");
    Serial.print(lpgLevel);
    Serial.println(" ppm");
}
void measureSound() //harus kalibrasi
{
    int adc1;
    double cum;
    for (int i = 0; i < 10; i++)
    {
        adc1 = ads.readADC_SingleEnded(1);
        //Serial.println (Analog);
        cum += abs(adc1 - 20766);
        delay(20);
    }
    cum = cum / 10;
    cum = map(cum, 0, 60, 30, 85);
    //Serial.print("sound cum : ");
    //Serial.println (cum);
    // kalman process
    Pc = P + varProcess;
    G = Pc / (Pc + varVolt); // kalman gain
    P = (1 - G) * Pc;
    Xp = Xe;
    Zp = Xp;
    Xe = G * (cum - Zp) + Xp; // the kalman estimate of the sensor voltage
    Serial.print("cum : ");
    cum = 0;
    if (Xe > 120)
    {
        Xe = 120;
    }
    Serial.println(Xe);
    //sound =map(cum,0,60,30,85);
}
int calibratesound()
{
    int adc1;
    double cum;
    for (int i = 0; i < 20; i++)
    {
        adc1 = ads.readADC_SingleEnded(1);
        //Serial.println (Analog);
        cum += abs(adc1);
    }
    return (cum / 20);
}
void dhtget()
{
    delay(delayMS);
    sensors_event_t event;
    dht.temperature().getEvent(&event);
    dhtTemp = event.temperature;
    dht.humidity().getEvent(&event);
    dhtHum = event.relative_humidity;
    Serial.print("temp : ");
    Serial.println(dhtTemp);
    Serial.print("hum : ");
    Serial.println(dhtHum);
    temperaturefilter.Filter(dhtTemp); //filtering
    dataTemp = temperaturefilter.Current();
    humidityfilter.Filter(dhtHum);
    dataHum = humidityfilter.Current();
    Serial.print("temp filtered : ");
    Serial.println(dataTemp);
    Serial.print("hum filtered  : ");
    Serial.println(dataHum);
}
void store_cumm()
{
}
//store nilai ke eeprom, cuma setiap 1 menit saja utk backup
void upgrade_cumm()
{
}
void MQTT_send()
{

    //publish temperatur
    if (!temperature.publish(0))
    {
        Serial.println(F("Failed"));
    }
    else
    {
        Serial.println(F("OK!"));
    };
    //publish humidity
    if (!humidity.publish(0))
    {
        Serial.println(F("Failed"));
    }
    else
    {
        Serial.println(F("OK!"));
    };
    //publish gasleak
    if (!gasleak.publish(0))
    {
        Serial.println(F("Failed"));
    }
    else
    {
        Serial.println(F("OK!"));
    };
    //publish noise
    if (!sound.publish(0))
    {
        Serial.println(F("Failed"));
    }
    else
    {
        Serial.println(F("OK!"));
    };
    //publish AirQuality
    if (!dust.publish(0))
    {
        Serial.println(F("Failed"));
    }
    else
    {
        Serial.println(F("OK!"));
    };
}

void MQTT_connect()
{
    int8_t ret;

    // Stop if already connected.
    if (mqtt.connected())
    {
        Serial.println(F("connected, go back to loop"));
        return;
    }

    Serial.print("Connecting to MQTT... ");

    uint8_t retries = 3;
    while ((ret = mqtt.connect()) != 0)
    { // connect will return 0 for connected
        Serial.println(mqtt.connectErrorString(ret));
        Serial.println("Retrying MQTT connection in 5 seconds...");
        mqtt.disconnect();
        delay(5000); // wait 5 seconds
        retries--;
        if (retries == 0)
        {
            // basically die and wait for WDT to reset me
            while (1)
                ;
        }
    }
    Serial.println("MQTT Connected!");
}

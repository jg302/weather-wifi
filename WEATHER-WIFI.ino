#include <ArduinoHttpClient.h>
#include <SPI.h>
#include <Wire.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include <DHT.h>
#include <WiFiNINA.h>

// Pins
#define DHTPIN 5
#define thermistorPin A0
#define ONE_WIRE_BUS 4

float outdoorTemp;
float waterTemp;
float indoorTemp;
byte digitalHumidity;
byte digitalT;
int status = WL_IDLE_STATUS;

// Setup
#define DHTTYPE DHT11
OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature sensors(&oneWire);
DHT dht(DHTPIN, DHTTYPE);
int total_devices;
DeviceAddress sensor_address;

// Other constants
#define R1 1000 // value of R1 on board
#define c1 0.001129148
#define c2 0.000234125
#define c3 0.0000000876741 // steinhart-hart coeficients for thermistor
#define kelvin 273.15;

char ssid[] = SECRET_SSID;        // your network SSID (name)
char pass[] = SECRET_PASS;    // your network password (use for WPA, or use as key for WEP)
char addr[] = SECRET_SERVER_ADDR;

// Initialise wifi
int port = 80;
WiFiClient wifi;

long seconds(long ms)
{
  return (ms * 1000);
}

void setup() {
  Serial.begin(115200);
  delay(40);

  sensors.begin();
  dht.begin();
  pinMode(LED_BUILTIN, OUTPUT);

  total_devices = sensors.getDeviceCount();
  delay(100);
}

void loop() {
  // Temperature & humidity
  digitalHumidity = dht.readHumidity();
  digitalT = dht.readTemperature();

  byte Vo;
  float logR2, T, vOut;

  // Therminsor calculations
  Vo = analogRead(thermistorPin);
  logR2 = log(R1 * (1023.0 / (float)Vo - 1.0)); // calculate resistance on thermistor
  T = (1.0 / (c1 + c2 * logR2 + c3 * logR2 * logR2 * logR2)); // temperature in Kelvin
  T = T - kelvin;                                    // convert Kelvin to Celcius

  Serial.print("Indoor T : ");
  Serial.println(T);
  Serial.print("humididty ");
  Serial.println(digitalHumidity);
  Serial.print("temp digital ");
  Serial.println(digitalT);
  indoorTemp = T;

  // ONE WIRE TEMPS
  sensors.requestTemperatures();

  for (int i = 0; i < total_devices; i++) {

    if (sensors.getAddress(sensor_address, i)) {


      float temperature_degreeCelsius = sensors.getTempC(sensor_address);

      if (i == 0) {
        Serial.print("outdoorTemp: ");
        outdoorTemp = temperature_degreeCelsius;
      } else {
        Serial.print("waterTemp: ");
        waterTemp = temperature_degreeCelsius;
      }

      Serial.println(temperature_degreeCelsius);
    }
  }

  // End temperature reading, do wifi
  delay(seconds(10));

  if (status != WL_CONNECTED) {
    digitalWrite(LED_BUILTIN, HIGH);
    status = WiFi.begin(ssid, pass);
    delay(seconds(15));
    digitalWrite(LED_BUILTIN, LOW);
  }

  // Make POST request
  if (status == WL_CONNECTED) {
    WiFiClient client;
    HttpClient http = HttpClient(wifi, SECRET_SERVER_ADDR, port);

    // Data to send with HTTP POST
    String httpRequestData = "/readings?indoorT=" + String(T) + "&out=" + String(outdoorTemp) + "&waterT=" + String(waterTemp) + "&=humidity" + String(digitalHumidity);
    int httpResponseCode = http.post(httpRequestData);
  }
}

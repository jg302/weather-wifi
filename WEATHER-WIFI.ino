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
#define ONE_WIRE_BUS 1
#define ERROR_LED 2

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

WiFiClient wifi;
HttpClient client = HttpClient(wifi, serverAddress, 8000);

long seconds(long ms)
{
  return (ms * 1000);
}

#define NOWIFI 1
#define APINOT200 2
#define TSENSOR 3
#define HUMIDSENSOR 4 // full deets at https://github.com/amcewen/HttpClient/blob/master/HttpClient.h#L12C1-L24C50

// Todo: this code could delay temperature reading cycle
void handleError(int code) {
  Serial.print("Error : ");
  Serial.println(code);
  
  // Blink the light for the error code
  int i = 0;
  while (i != code) {
    digitalWrite(ERROR_LED, HIGH);
    delay(500);
    digitalWrite(ERROR_LED, LOW);
    delay(500);
    i++;
  }
}

void pulse(int pulses) {
  int i = 0;
  while (i != pulses) {
    digitalWrite(ERROR_LED, HIGH);
    delay(100);
    digitalWrite(ERROR_LED, LOW);
    delay(100);
    i = i + 1;
  }
}

void setup() {
  Serial.begin(115200);
  delay(40);

  sensors.begin();
  dht.begin();
  pinMode(LED_BUILTIN, OUTPUT);
  pinMode(ERROR_LED, OUTPUT);


  pulse(4);

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
  
  // Error handling
  if (T <= -40 | T >= 60) {
    handleError(TSENSOR);
  }
  
  if (digitalHumidity <= 0 | digitalHumidity >= 100) {
    handleError(HUMIDSENSOR);
  }

  // End temperature reading, do wifi
  delay(seconds(2));

  if (status != WL_CONNECTED) {
    digitalWrite(LED_BUILTIN, HIGH);
    status = WiFi.begin(ssid, pass);
    delay(seconds(15));
    digitalWrite(LED_BUILTIN, LOW);
  }

  // Make POST request
  if (status == WL_CONNECTED) {
    // Data to send with HTTP POST
    String httpRequestData = "key=" + String(SECRET_KEY) + "&indoorT=" + String(T) + "&outT=" + String(outdoorTemp) + "&waterT=" + String(waterTemp) + "&=humidity" + String(digitalHumidity);
    String contentType = "application/x-www-form-urlencoded";

    int httpResponseCode = client.post("/readings", contentType, httpRequestData);
    
    int dave = client.get("/readings");
      // read the status code and body of the response
  int statusCode = client.responseStatusCode();
  String response = client.responseBody();
  Serial.print("Status code: ");
  Serial.println(statusCode);
  Serial.print("Response: ");
  Serial.println(response);

    
    Serial.println(httpResponseCode);
    
    if (httpResponseCode != 0) {
      handleError(APINOT200);
    } else {
      pulse(2);
    }
  } else {
    handleError(NOWIFI); 
  }
}

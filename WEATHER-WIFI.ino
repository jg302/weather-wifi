#include <SPI.h>
#include <Wire.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include <DHT.h>

// Pins
#define DHTPIN 5
#define DHTTYPE DHT11
DHT dht(DHTPIN, DHTTYPE);
#define thermistorPin A0

#define ONE_WIRE_BUS 8

float outdoorTemp;
float waterTemp;
float indoorTemp;
byte digitalHumidity;
byte digitalT;

OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature sensors(&oneWire);

int total_devices;
DeviceAddress sensor_address; 

// Other constants
#define R1 1000 // value of R1 on board
#define c1 0.001129148
#define c2 0.000234125
#define c3 0.0000000876741 // steinhart-hart coeficients for thermistor
#define kelvin 273.15;

long seconds(long ms)
{
  return (ms * 1000);
}

void setup() {
  Serial.begin(115200);

  total_devices = sensors.getDeviceCount();
  delay(100);
  
  sensors.begin();
  dht.begin();

  total_devices = sensors.getDeviceCount();
  delay(100);
}

void loop() {
    // Temperature & humidity
    digitalHumidity = dht.readHumidity();
    digitalT = dht.readTemperature();
   
    byte Vo;
    float logR2,T, vOut;
   
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
  
  for(int i=0;i<total_devices; i++){
    
    if(sensors.getAddress(sensor_address, i)){

     
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
  // End temperature reading

  delay(seconds(2));
}

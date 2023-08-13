/*
 * Health Monitor - Arduino Code
 * Author: Md. Tanvir Hassan
 * Version: V3
 * Date: [Insert Date Here]
 *
 * This code is authored by Md. Tanvir Hassan and is provided for educational purposes.
 * You may use, modify, and distribute this code freely, but please retain this notice.
 * The author is not liable for any consequences arising from the use of this code.
 *
 * This code is not for sale and is intended for educational and non-commercial use only.
 */

#include <math.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <SoftwareSerial.h>
#include <MAX30100_PulseOximeter.h>

SoftwareSerial mySerial(3, 2); // SIM800L Tx & Rx is connected to Arduino #3 & #2
LiquidCrystal_I2C lcd(0x27, 16, 2); // I2C address, columns, rows
PulseOximeter pox;

unsigned long previousMillis = 0;
const unsigned long interval = 60000; // 1 minute in milliseconds

uint32_t tsLastReport = 0;
#define REPORTING_PERIOD_MS 1000

int i, bpm, spo2;
double temperatureF;

void setup()
{
  Serial.begin(9600);
  mySerial.begin(9600);
  
  lcd.begin();
  lcd.backlight();
  
  lcd.print("Initializing...");
  delay(2000); // Allow time for SIM800L to initialize
  
  mySerial.println("AT");
  delay(1000); // Allow time for SIM800L response
  lcd.clear();
  lcd.print("AT: ");
  lcd.print(mySerial.readString());
  
  mySerial.println("AT+CMGF=1");
  delay(1000); // Allow time for SIM800L response
  lcd.setCursor(0, 0);
  lcd.print("Health Monitor");
  
  delay(2000);
  lcd.clear();
  
  // Initialize MAX30100 sensor
  if (!pox.begin()) 
  {
    Serial.println("Error initializing PulseOximeter!");
  }
  pox.setIRLedCurrent(MAX30100_LED_CURR_7_6MA);
  pox.setOnBeatDetectedCallback(onBeatDetected);
}

void loop()
{
  unsigned long currentMillis = millis();
  
  i = analogRead(A0);
  double temperature = Thermister(i);
  temperatureF = celsiusToFahrenheit(temperature);

  // Read MAX30100 sensor data
  pox.update();
  
  bpm = pox.getHeartRate();
  spo2 = pox.getSpO2();
  
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("BPM:");
  lcd.print(bpm);
  lcd.setCursor(8, 0);
  lcd.print("SpO2:");
  lcd.print(spo2);
  lcd.print("%");
  lcd.setCursor(0, 1);
  lcd.print("Temp:");
  lcd.print(temperatureF);
  lcd.print(char(223));
  lcd.print("F");

  // Check conditions and send warning
  if ((bpm > 60 && bpm < 80) || (bpm > 120) || (spo2 > 80 && spo2 < 90) || (temperatureF > 104)) 
  {
    if (currentMillis - previousMillis >= interval)
    {
      previousMillis = currentMillis;
      String warningMessage = constructWarningMessage();
      sendWarningSMS(warningMessage);
    }
  }
}

String constructWarningMessage()
{
  String message = "Warning - ";
  
  if (bpm > 40 && bpm < 60) {
    message += "Low Pressure - BPM: " + String(bpm) + " ;  SpO2: " + String(spo2) + "% ;" + " Temperature: " + String(temperatureF) + "F ;  ";
  }
  if (bpm > 110) {
    message += "High Pressure - BPM: " + String(bpm) + " ;  SpO2: " + String(spo2) + "% ;" + " Temperature: " + String(temperatureF) + "F ;  ";
  }
  if (spo2 > 80 && spo2 < 90) {
    message += "Low SpO2 - SpO2: " + String(spo2) + ";    BPM: " + String(bpm) + " ;" + "Temperature: " + String(temperatureF) + "F ; ";
  }
  if (temperatureF > 104) {
    message += "High Fever || Temperature: " + String(temperatureF) + "F ;           "+ "BPM: " + String(bpm) + " ;  SpO2: " + String(spo2) + "%;" ;
    
  }
  
  return message;
}

void onBeatDetected()
{
  Serial.println("Beat!!!");
}

double Thermister(int data)
{
  double temp;
  temp = log(10000.0 * ((1024.0 / data) - 1));
  temp = 1 / (0.001129148 + (0.000234125 + (0.0000000876741 * temp * temp)) * temp);
  temp = temp - 273.15;
  return temp;
}

double celsiusToFahrenheit(double celsius)
{
  return (celsius * 9.0 / 5.0) + 32.0;
}

void sendWarningSMS(String message)
{
  mySerial.println("AT+CMGS=\"+8801580536906\""); // Change with your phone number
  delay(1000); // Wait for module response
  
  mySerial.print(message);
  mySerial.write(26); // ASCII code for Ctrl+Z
  
  delay(1000); // Wait for module to send SMS
  
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Warning Sent");
}

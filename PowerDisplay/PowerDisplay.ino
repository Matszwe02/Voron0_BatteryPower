#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>


#define BUTTON_PIN 2
#define CURRENT_SENSOR_PIN A0
#define VOLTAGE_SENSOR_PIN A1

Adafruit_SSD1306 display(128, 64, &Wire, -1);


#define BATTERY_CAPACITY 5.000          // Ah
#define CURRENT_SENSOR_SENSITIVITY 5.54 // A / V
#define CURRENT_ZERO 511                // ADC
#define VOLTAGE_SENSE_RATIO 5.08        // V / V
#define BATTERY_LOW_VOLTAGE 3.0         // V
#define BATTERY_HIGH_VOLTAGE 4.0        // V
#define DISPLAY_TIMEOUT 60000           // s
#define PERCENT_WARNING 20              // %
#define CELL_INTERNAL_RESISTANCE 0.58   // Ohm

float voltage = 0;                      // V
float fixedVoltage = 0;                 // V
float cellVoltage = 0;                  // V
float fixedCellVoltage = 0;             // V
float current = 0;                      // A
float power = 0;                        // W
float percent = 0;                      // %
float fixedPercent = 0;                 // %
float remainingTime = 0;                // h


int displayMode;
unsigned long lastPress;
unsigned long lastWarning;
bool buttonPress = false;

void setup()
{
    pinMode(BUTTON_PIN, INPUT_PULLUP);
    
    pinMode(A2, OUTPUT);
    pinMode(A3, OUTPUT);
    pinMode(3, OUTPUT);
    digitalWrite(A2, LOW);
    digitalWrite(A3, HIGH);
    digitalWrite(3, LOW);
    delay(100);
    
    display.begin(SSD1306_SWITCHCAPVCC, 0x3C);
    display.clearDisplay();
    display.setTextColor(SSD1306_WHITE);
    display.display();
    lastPress = millis();
    displayMode = 1;
    
    delay(100);
    
    voltage = getVoltage();
    
    current = getCurrent();
    fixedVoltage = voltage + (current * CELL_INTERNAL_RESISTANCE);
    
    cellVoltage = voltage / 6;
    fixedCellVoltage = fixedVoltage / 6;
    
    percent = getPercent(cellVoltage);
    fixedPercent = getPercent(fixedCellVoltage);
    
    while(millis() < 2000)
    {
        getVariables();
    }
}

void loop()
{
    
    for(int i=0; i<10; i++)
    {
        delay(5);
        getVariables();
        if(readButton())displayMode ++;
    }
    
    
    
    if(displayMode > 2) displayMode = 1;
    
    if((millis() - lastPress > DISPLAY_TIMEOUT) && (millis() - lastWarning > 5000)) displayMode = 0;
    
    if(percent > PERCENT_WARNING || power < -10)
        switch (displayMode)
        {
            case 0:
                displayOff();
            break;
            case 1:
                displayPercentage(fixedPercent, power);
            break;
            case 2:
                displayPower(power, remainingTime);
            break;
        }
    else
    {
        lastWarning = millis();
        switch (displayMode)
        {
            case 2:
                if(millis() % 1000 > 500)
                    displayOff();
                else
                    displayPower(power, remainingTime);
            break;
            default:
                if(millis() % 1000 > 500)
                    displayOff();
                else
                    displayPercentage(fixedPercent, power);
            break;
        }
    }
}


void displayPercentage(float percentFloat, float powerFloat)
{    
    display.clearDisplay();
    display.setTextSize(2);
    
    String str = String(percentFloat, 1) + "%";
    int16_t x, y;
    uint16_t w, h;
    display.getTextBounds(str, 0, 0, &x, &y, &w, &h);
    
    display.setCursor(64 - (w/2), 10);
    display.print(str);
    
    if(powerFloat < 0 && millis() % 1000 > 500) display.print(" +");
    
    display.fillRect(6, 35 + 1, 115, 18, 0);
    display.drawRect(5, 35 + 0, 117, 17, 1);
    display.fillRect(6, 35 + 0, map(percentFloat, 0, 100, 0, 115), 17, 1);
    display.display();
}

void displayPower(float power, float remainingTime)
{
    int minutes = int(remainingTime * 60) % 60;
    int hours = int(remainingTime - (minutes / 60));
    display.clearDisplay();
    display.setTextSize(2);
    display.setCursor(5, 0);
    display.print(String(power, 2));
    display.print("W");
    display.setCursor(5, 20);
    display.print(String(hours));
    display.print("h ");
    display.print(String(minutes));
    display.print("m");
    display.setCursor(5, 40);
    display.setTextSize(1);
    display.print(String(fixedVoltage));
    display.print("V  (");
    display.print(String(voltage));
    display.print("V) ");
    
    display.setCursor(5, 50);
    display.print(String(current));
    display.print("A   ");
    display.print(String(fixedPercent));
    display.print("% ");
    display.display();
}

void displayOff()
{
    display.clearDisplay();
    display.display();
}

float getVoltage()
{
    return analogRead(VOLTAGE_SENSOR_PIN) / 1024.0 * 5.0 * VOLTAGE_SENSE_RATIO;
}

float getCurrent()
{
    return (analogRead(CURRENT_SENSOR_PIN) - CURRENT_ZERO) / 1024.0 * -5.0 * CURRENT_SENSOR_SENSITIVITY;
}

float getPercent(float percentVoltage)
{  
   float newPercent = map(percentVoltage * 10000, BATTERY_LOW_VOLTAGE * 10000, BATTERY_HIGH_VOLTAGE * 10000, 0, 10000);
   newPercent = constrain(newPercent, 0, 10000);
   newPercent /= 100;
   return newPercent;
}


float multiReads(float value, float newValue, long ratio)
{
    float output = (value*1000 * (ratio-1) + newValue*1000) / ratio;
    output /= 1000;
    return output;
}


void getVariables()
{
    float newVoltage = getVoltage();
    voltage = multiReads(voltage, newVoltage, (abs(newVoltage - voltage) > 0.1)? 100 : 1000000);
    cellVoltage = voltage / 6;
    
    float newCurrent = getCurrent();
    current = multiReads(current, newCurrent, (abs(newCurrent - current) > 0.1)? 100 : 1000000);
        
    fixedVoltage = voltage + (current * CELL_INTERNAL_RESISTANCE);
    fixedCellVoltage = fixedVoltage / 6;
    
    power = fixedVoltage * current;
    
    float newPercent = getPercent(cellVoltage);
    
    float newFixedPercent = getPercent(fixedCellVoltage);
    
    percent = multiReads(percent, newPercent, abs(percent - newPercent) > 5 ? 100 : 10000);
    fixedPercent = multiReads(fixedPercent, newFixedPercent, abs(fixedPercent - newFixedPercent) > 5 ? 100 : 10000);
    
    if(abs(current) > 0.01)
    {
        if(power > 0)
            remainingTime = BATTERY_CAPACITY / current * (percent / 100);
        else
            remainingTime = BATTERY_CAPACITY / (-current) * ((100 - percent) / 100);
    }
    else
        remainingTime = 99;
    
}

bool readButton()
{
    if(millis() - lastPress < 200) return false;
    while(!digitalRead(BUTTON_PIN) && buttonPress);
    buttonPress = !digitalRead(BUTTON_PIN);
    if(buttonPress) lastPress = millis();
    return buttonPress;
}
#include <Wire.h>
#include "MAX30105.h"
#include "heartRate.h"
#include <LiquidCrystal_I2C.h>

MAX30105 particleSensor;
LiquidCrystal_I2C lcd(0x27, 16, 2);

// Beat timing
unsigned long lastBeatTime = 0;
unsigned long previousBeatTime = 0;

float bpm = 0;
int beatAvg = 0;

const byte RATE_SIZE = 4;
byte rates[RATE_SIZE];
byte rateSpot = 0;

// Finger detection
const long FINGER_THRESHOLD = 20000;

// Minimum time between beats
const unsigned long MIN_BEAT_INTERVAL = 600;

// Beat detection tuning
const int HYSTERESIS = 300;

// Variables for detection
long lastIR = 0;
long lastDelta = 0;

bool possibleBeat = false;
unsigned long possibleBeatTime = 0;

void setup()
{
  Serial.begin(9600);
  Serial.println("Starting");

  Wire.begin();

  lcd.init();
  lcd.backlight();
  lcd.setCursor(0,0);
  lcd.print("Heart Monitor");
  lcd.setCursor(0,1);
  lcd.print("Starting...");
  delay(2000);
  lcd.clear();

  if (!particleSensor.begin(Wire, I2C_SPEED_STANDARD))
  {
    Serial.println("MAX30102 not found");

    lcd.setCursor(0,0);
    lcd.print("Sensor Error");

    while (1);
  }

  Serial.println("Sensor ready");

  particleSensor.setup();
  particleSensor.setPulseAmplitudeRed(0x3F);
  particleSensor.setPulseAmplitudeIR(0x3F);
}

bool detectBeat(long irValue)
{
  long delta = irValue - lastIR;
  lastIR = irValue;

  if (delta < -HYSTERESIS && lastDelta > HYSTERESIS)
  {
    if (!possibleBeat)
    {
      possibleBeat = true;
      possibleBeatTime = millis();
    }
  }
  else
  {
    if (millis() - possibleBeatTime > 120)
    {
      possibleBeat = false;
    }
  }

  lastDelta = delta;

  if (possibleBeat && (millis() - lastBeatTime > MIN_BEAT_INTERVAL))
  {
    previousBeatTime = lastBeatTime;
    lastBeatTime = millis();
    possibleBeat = false;
    return true;
  }

  return false;
}

void loop()
{
  long irValue = particleSensor.getIR();

  Serial.print("IR=");
  Serial.print(irValue);

  if (irValue > FINGER_THRESHOLD)
  {
    Serial.print(" (finger)");

    if (detectBeat(irValue))
    {
      if (previousBeatTime != 0)
      {
        unsigned long delta = lastBeatTime - previousBeatTime;

        bpm = 60.0 / (delta / 1000.0);

        if (bpm > 40 && bpm < 180)
        {
          rates[rateSpot++] = (byte)bpm;
          rateSpot %= RATE_SIZE;

          beatAvg = 0;
          for (byte i = 0; i < RATE_SIZE; i++)
            beatAvg += rates[i];

          beatAvg /= RATE_SIZE;
        }
      }
    }

    lcd.setCursor(0,0);
    lcd.print("BPM: ");
    lcd.print((int)bpm);
    lcd.print("   ");

    lcd.setCursor(0,1);
    lcd.print("Avg: ");
    lcd.print(beatAvg);
    lcd.print("   ");
  }
  else
  {
    Serial.print(" (no finger)");

    beatAvg = 0;
    lastBeatTime = 0;
    previousBeatTime = 0;
    bpm = 0;
    possibleBeat = false;

    lcd.setCursor(0,0);
    lcd.print("Place Finger   ");
    lcd.setCursor(0,1);
    lcd.print("                ");
  }

  Serial.print(", BPM=");
  Serial.print(bpm);
  Serial.print(", Avg BPM=");
  Serial.println(beatAvg);

  delay(10);   // 0.01 second between readings
}
/*
  Optical Heart Rate Detection (PBA Algorithm) using the MAX30105 Breakout
  By: Nathan Seidle @ SparkFun Electronics
  Date: October 2nd, 2016
  https://github.com/sparkfun/MAX30105_Breakout

  This is a demo to show the reading of heart rate or beats per minute (BPM) using
  a Penpheral Beat Amplitude (PBA) algorithm.

  It is best to attach the sensor to your finger using a rubber band or other tightening
  device. Humans are generally bad at applying constant pressure to a thing. When you
  press your finger against the sensor it varies enough to cause the blood in your
  finger to flow differently which causes the sensor readings to go wonky.

  Hardware Connections (Breakoutboard to Arduino):
  -5V = 5V (3.3V is allowed)
  -GND = GND
  -SDA = A4 (or SDA)
  -SCL = A5 (or SCL)
  -INT = Not connected

  The MAX30105 Breakout can handle 5V or 3.3V I2C logic. We recommend powering the board with 5V
  but it will also run at 3.3V.
*/

#include <Wire.h>
#include "MAX30105.h"

#include "heartRate.h"
#include <Adafruit_NeoPixel.h>

// ========== Configuración de NeoPixel ==========
#define LED_PIN     6
#define LED_COUNT   64
#define BRIGHTNESS  255
Adafruit_NeoPixel strip(LED_COUNT, LED_PIN, NEO_GRB + NEO_KHZ800);


MAX30105 particleSensor;



const byte RATE_SIZE = 2; //Increase this for more averaging. 4 is good.
byte rates[RATE_SIZE]; //Array of heart rates
byte rateSpot = 0;
long lastBeat = 0; //Time at which the last beat occurred

float beatsPerMinute;
int beatAvg;


// Variables para animación no bloqueante
unsigned long lastBreatheUpdate = 0;
int breatheStep = 0;
bool subiendo = true;

void setup()
{
  Serial.begin(115200);
  Serial.println("Initializing...");

  // Initialize sensor
  if (!particleSensor.begin(Wire, I2C_SPEED_FAST)) //Use default I2C port, 400kHz speed
  {
    Serial.println("MAX30105 was not found. Please check wiring/power. ");
    while (1);
  }
  Serial.println("Place your index finger on the sensor with steady pressure.");

  particleSensor.setup(); //Configure sensor with default settings
  particleSensor.setPulseAmplitudeRed(0x0A); //Turn Red LED to low to indicate sensor is running
  particleSensor.setPulseAmplitudeGreen(0); //Turn off Green LED

  strip.begin();
  strip.setBrightness(BRIGHTNESS);
  strip.show(); // apagar todos los LEDs
  colorWipe(strip.Color(255, 0, 0), 5); // Red
}

void loop()
{
  long irValue = particleSensor.getIR();

  if (checkForBeat(irValue) == true)
  {
    //We sensed a beat!
    long delta = millis() - lastBeat;
    lastBeat = millis();

    beatsPerMinute = 60 / (delta / 1000.0);

    if (beatsPerMinute < 255 && beatsPerMinute > 20)
    {
      rates[rateSpot++] = (byte)beatsPerMinute; //Store this reading in the array
      rateSpot %= RATE_SIZE; //Wrap variable

      //Take average of readings
      beatAvg = 0;
      for (byte x = 0 ; x < RATE_SIZE ; x++)
        beatAvg += rates[x];
      beatAvg /= RATE_SIZE;
    }
  }

  Serial.print("IR=");
  Serial.print(irValue);
  Serial.print(", BPM=");
  Serial.print(beatsPerMinute);
  Serial.print(", Avg BPM=");
  Serial.print(beatAvg);

  if (irValue < 50000) {
    Serial.print(" No finger?");
    beatAvg = 0;
    if (beatsPerMinute >= 0)
      beatsPerMinute -= 20;
    if (beatsPerMinute < 0)
      beatsPerMinute =0;
  }

  Serial.println();
  if (beatAvg != 0)
    breatheOrangeNonBlocking(abs(80 - beatAvg));
  if (beatAvg == 0)
    breatheOrangeNonBlocking(abs(80 - beatsPerMinute));
}

void breatheOrangeNonBlocking(uint8_t speed) {
  unsigned long ahora = millis();
  const int maxStep = 128;

  if (ahora - lastBreatheUpdate >= speed) {
    lastBreatheUpdate = ahora;

    uint8_t factor = strip.gamma8(breatheStep);
    uint8_t r = (255 * factor) / 255;
    uint8_t g = (100 * factor) / 255;
    strip.fill(strip.Color(r, g, 0));
    strip.show();

    if (subiendo) {
      breatheStep += 5;
      if (breatheStep >= maxStep) {
        breatheStep = maxStep;
        subiendo = false;
      }
    } else {
      breatheStep -= 5;
      if (breatheStep <= 0) {
        breatheStep = 0;
        subiendo = true;
      }
    }
  }
}

void colorWipe(uint32_t c, uint8_t wait) {
  for(uint16_t i=0; i<strip.numPixels(); i++) {
    strip.setPixelColor(i, c);
    strip.show();
    delay(wait);
  }
}

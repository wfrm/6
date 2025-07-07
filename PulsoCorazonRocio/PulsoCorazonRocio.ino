#include <Wire.h>
#include "MAX30105.h"
#include "heartRate.h"
#include <FastLED.h>

// ========== Sensor MAX30105 ==========
MAX30105 particleSensor;
const byte RATE_SIZE = 4;
byte rates[RATE_SIZE];
byte rateSpot = 0;
long lastBeat = 0;
float beatsPerMinute;
int beatAvg;

// ========== FastLED ==========
#define LED_PIN     6
#define NUM_LEDS    64
#define BRIGHTNESS  255
#define LED_TYPE    WS2812B
#define COLOR_ORDER GRB
CRGB leds[NUM_LEDS];

// ========== Estados ==========
bool dedoPresente = false;
unsigned long ultimoCambioDedo = 0;
const long debounceTiempo =500;

// ========== Efecto respiración ==========
uint8_t breatheStep = 0;
bool breathingUp = true;
unsigned long lastBreatheUpdate = 0;

// ========== Pulso visual ==========
bool latidoActivo = false;
unsigned long latidoInicio = 0;
const int duracionLatido = 150;  // ms

void setup() {
  Serial.begin(115200);
  Serial.println("Inicializando...");

  if (!particleSensor.begin(Wire, I2C_SPEED_FAST)) {
    Serial.println("MAX30105 no encontrado. Revisa conexiones.");
    while (1);
  }

  particleSensor.setup();
  particleSensor.setPulseAmplitudeRed(0x0A);
  particleSensor.setPulseAmplitudeGreen(0);

  FastLED.addLeds<LED_TYPE, LED_PIN, COLOR_ORDER>(leds, NUM_LEDS);
  FastLED.setBrightness(BRIGHTNESS);
  FastLED.clear();
  FastLED.show();
}

void loop() {
  long irValue = particleSensor.getIR();
  unsigned long ahora = millis();

  // === Detectar dedo con debounce ===
  if (irValue > 50000 && !dedoPresente && ahora - ultimoCambioDedo > debounceTiempo) {
    dedoPresente = true;
    ultimoCambioDedo = ahora;
    rates[rateSpot++]=60;
    Serial.println("🟢 Dedo detectado");
  } else if (irValue < 50000 && dedoPresente && ahora - ultimoCambioDedo > debounceTiempo) {
    dedoPresente = false;
    ultimoCambioDedo = ahora;
    latidoActivo = false;
    Serial.println("🔴 Dedo retirado");
  }

  // === Calcular BPM si hay dedo ===
  if (dedoPresente) {
    if (checkForBeat(irValue)) {
      long delta = ahora - lastBeat;
      lastBeat = ahora;

      beatsPerMinute = 60.0 / (delta / 1000.0);
      if (beatsPerMinute < 255 && beatsPerMinute > 20) {
        rates[rateSpot++] = (byte)beatsPerMinute;
        rateSpot %= RATE_SIZE;

        beatAvg = 0;
        for (byte x = 0 ; x < RATE_SIZE ; x++)
          beatAvg += rates[x];
        beatAvg /= RATE_SIZE;
      }

      // Activar efecto de latido visual
      latidoActivo = true;
      latidoInicio = ahora;
    }

    Serial.print("IR="); Serial.print(irValue);
    Serial.print(", BPM="); Serial.print(beatsPerMinute);
    Serial.print(", Avg BPM="); Serial.println(beatAvg);
  }

  // === Mostrar animación ===
  if (dedoPresente) {
    mostrarPulso(ahora);
  } else {
    respirar(ahora, 120);  // respiración lenta
  }
}

void mostrarPulso(unsigned long ahora) {
  if (latidoActivo) {
    if (ahora - latidoInicio < duracionLatido) {
      fill_solid(leds, NUM_LEDS, CRGB::OrangeRed);
    } else {
      
      FastLED.clear();
    }
    FastLED.show();
  }
}

void respirar(unsigned long ahora, uint8_t velocidad) {
  if (ahora - lastBreatheUpdate >= velocidad) {
    lastBreatheUpdate = ahora;

    uint8_t factor = breathingUp ? breatheStep+=20 : breatheStep-=20;
    uint8_t brillo = dim8_video(factor);
    CRGB color = CRGB(brillo, brillo * 0.4, 0);  // Naranja cálido

    fill_solid(leds, NUM_LEDS, color);
    FastLED.show();

    if (breathingUp && breatheStep >= 255) breathingUp = false;
    if (!breathingUp && breatheStep <= 0) breathingUp = true;
  }
}

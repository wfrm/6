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

float level = 1.0;
const byte RATE_SIZE = 4;
byte rates[RATE_SIZE];
byte rateSpot = 0;
long lastBeat = 0;

float beatsPerMinute;
int beatAvg;

bool dedoPresente = false;
unsigned long ultimoCambioDedo = 0;
const long debounceTiempo = 1000;  // 1 segundo para evitar fluctuaciones

// Variables para animación no bloqueante
unsigned long lastBreatheUpdate = 0;
int breatheStep = 0;
bool subiendo = true;

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

  strip.begin();
  strip.setBrightness(BRIGHTNESS);
  strip.show(); // apagar todos los LEDs
}

void loop() {
  long irValue = particleSensor.getIR();
  unsigned long ahora = millis();

  // === Detectar dedo presente con debounce ===
  if (irValue > 50000 && !dedoPresente && ahora - ultimoCambioDedo > debounceTiempo) {
    dedoPresente = true;
    ultimoCambioDedo = ahora;
    Serial.println("🟢 Dedo detectado");
  } else if (irValue < 50000 && dedoPresente && ahora - ultimoCambioDedo > debounceTiempo) {
    dedoPresente = false;
    ultimoCambioDedo = ahora;
    Serial.println("🔴 Dedo retirado");
  }

  // === Calcular BPM si el dedo está presente ===
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
    }

    Serial.print("IR="); Serial.print(irValue);
    Serial.print(", BPM="); Serial.print(beatsPerMinute);
    Serial.print(", Avg BPM="); Serial.println(beatAvg);
  }

  // === Control de animación ===
  uint8_t velocidad = dedoPresente ? constrain(beatAvg / 2, 10, 50) : 120;
  breatheOrangeNonBlocking(velocidad);
}

// === Animación no bloqueante ===
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
      breatheStep++;
      if (breatheStep >= maxStep) {
        breatheStep = maxStep;
        subiendo = false;
      }
    } else {
      breatheStep--;
      if (breatheStep <= 0) {
        breatheStep = 0;
        subiendo = true;
      }
    }
  }
}

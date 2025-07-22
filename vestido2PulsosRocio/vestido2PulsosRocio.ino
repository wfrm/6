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
boolean primeraVez = false;
unsigned long tempo=180000;

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
const long debounceTiempo = 500;

// ========== Efecto respiración ==========
uint8_t breatheStep = 0;
bool breathingUp = true;
unsigned long lastBreatheUpdate = 0;

// ========== Pulso visual ==========
bool latidoActivo = false;
unsigned long latidoInicio = 0;
const int duracionLatido = 150;  // ms

uint8_t BeatsPerMinute = 58;//62;//la persona esta relajada
unsigned long tiempoEspera = 0;

uint8_t gHue = 0; // rotating "base color" used by many of the patterns
#define FRAMES_PER_SECOND  120
CRGBPalette16 orange_palette_1 =
{ 0x1A0500, 0x2A0A00, 0x3A1000, 0x4A1500,
  0x5A1A00, 0x6A1F00, 0x7A2500, 0x8A2A00,
  0x9A2F00, 0xAA3400, 0xBB3A00, 0xCC3F00,
  0xDD4400, 0xEE4A00, 0xFF5500, 0xFFAA33
};
CRGBPalette16 orange_fire_palette =
{ 0x1A0000, 0x2A0800, 0x3A1000, 0x4B1800,
  0x5C2000, 0x6C2800, 0x7D3000, 0x8E3800,
  0x9E4000, 0xAF4800, 0xC05000, 0xD15800,
  0xE16000, 0xF26800, 0xFF7000, 0xFFAA33
};
CRGBPalette16 amber_glow_palette =
{ 0x331000, 0x442000, 0x553000, 0x664000,
  0x775000, 0x886000, 0x997000, 0xAA8000,
  0xBB9000, 0xCC9F00, 0xDDAF00, 0xEEBF00,
  0xFFCF00, 0xFFD74A, 0xFFE080, 0xFFF0B0
};

CRGBPalette16 saturated_orange_palette =
{ 0xAA1400, 0xBE1900, 0xD21E00, 0xE62300,
  0xFA2800, 0xFF2D00, 0xFF3200, 0xFF3700,
  0xFF3700, 0xFF3200, 0xFF2D00, 0xFA2800,
  0xE62300, 0xD21E00, 0xBE1900, 0xAA1400
};

CRGBPalette16 white_palette =
{ 0xC0C0C0, 0xD0D0D0, 0xE0E0E0, 0xF0F0F0,
  0xF8F8F8, 0xFCFCFC, 0xFEFEFE, 0xFFFFFF,
  0xFFFFFF, 0xFEFEFE, 0xFCFCFC, 0xF8F8F8,
  0xF0F0F0, 0xE0E0E0, 0xD0D0D0, 0xC0C0C0
};


long deltaIR = 0;
long irValueAnterior = 0;
void setup() {
  tiempoEspera = millis();
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
  irValueAnterior = particleSensor.getIR();
}

void pacifica_loop()
{
  // Increment the four "color index start" counters, one for each wave layer.
  // Each is incremented at a different speed, and the speeds vary over time.
  static uint16_t sCIStart1, sCIStart2, sCIStart3, sCIStart4;
  static uint32_t sLastms = 0;
  uint32_t ms = GET_MILLIS();
  uint32_t deltams = ms - sLastms;
  sLastms = ms;
  uint16_t speedfactor1 = beatsin16(3, 179, 269);
  uint16_t speedfactor2 = beatsin16(4, 179, 269);
  uint32_t deltams1 = (deltams * speedfactor1) / 256;
  uint32_t deltams2 = (deltams * speedfactor2) / 256;
  uint32_t deltams21 = (deltams1 + deltams2) / 2;
  sCIStart1 += (deltams1 * beatsin88(1011, 10, 13));
  sCIStart2 -= (deltams21 * beatsin88(777, 8, 11));
  sCIStart3 -= (deltams1 * beatsin88(501, 5, 7));
  sCIStart4 -= (deltams2 * beatsin88(257, 4, 6));

  // Clear out the LED array to a dim background blue-green
  fill_solid( leds, NUM_LEDS, CRGB( 2, 6, 10));

  // Render each of four layers, with different scales and speeds, that vary over time
  pacifica_one_layer( orange_palette_1, sCIStart1, beatsin16( 3, 11 * 256, 14 * 256), beatsin8( 10, 70, 130), 0 - beat16( 301) );
  pacifica_one_layer( orange_fire_palette, sCIStart2, beatsin16( 4,  6 * 256,  9 * 256), beatsin8( 17, 40,  80), beat16( 401) );
  pacifica_one_layer( amber_glow_palette, sCIStart3, 6 * 256, beatsin8( 9, 10, 38), 0 - beat16(503));
  pacifica_one_layer( saturated_orange_palette, sCIStart4, 5 * 256, beatsin8( 8, 10, 28), beat16(601));

  // Add brighter 'whitecaps' where the waves lines up more
  pacifica_add_whitecaps();

  // Deepen the blues and greens a bit
  pacifica_deepen_colors();
}

bool animacionSinelonActiva = false;
unsigned long inicioSinelon = 0;
const unsigned long duracionSinelon = 2000; // en milisegundos

void loop() {

  long irValue = particleSensor.getIR();
  deltaIR = irValue - irValueAnterior;
  //Serial.print(", deltaIR="); Serial.println(deltaIR);
  unsigned long ahora = millis();

  if (deltaIR > 300 && !animacionSinelonActiva) {
    animacionSinelonActiva = true;
    inicioSinelon = millis();
  }
  irValueAnterior = irValue;

  // === Detectar dedo con debounce ===
  if (irValue > 50000 && !dedoPresente && ahora - ultimoCambioDedo > debounceTiempo) {
    dedoPresente = true;
    ultimoCambioDedo = ahora;
    rates[rateSpot++] = 60;
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
       primeraVez = true;
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
  if (animacionSinelonActiva) {
    sinelonAmbar();
    FastLED.show();

    if (millis() - inicioSinelon >= duracionSinelon) {
      animacionSinelonActiva = false;
      FastLED.clear(); // limpia LEDs después de sinelon
    }
  }
  else if (dedoPresente) {
    bpm();
    FastLED.show();
    FastLED.delay(1000 / FRAMES_PER_SECOND);
  } else {
    if (primeraVez) {
      bpm();
      FastLED.show();
      FastLED.delay(1000 / FRAMES_PER_SECOND);

    }
    else {
      respirar(ahora,10);
    }

  }

  // do some periodic updates
  EVERY_N_MILLISECONDS( 20 ) {
    gHue++;  // slowly cycle the "base color" through the rainbow
  }
  if (gHue > 50) {
    gHue = 0;
  }

    if (millis() - tiempoEspera > tempo  && primeraVez ) //10 minutos
  {
    tiempoEspera=millis();
    primeraVez = false;
    Serial.println("fin");
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

    uint8_t factor = breathingUp ? breatheStep += 2 : breatheStep -= 1;
    uint8_t brillo = triwave8(factor);
    CRGB color = CRGB(brillo, brillo * 0.2, 0);  // Naranja cálido

    fill_solid(leds, NUM_LEDS, color);
    FastLED.show();

    if (breathingUp && breatheStep >= 255) breathingUp = false;
    if (!breathingUp && breatheStep <= 0) breathingUp = true;
  }
}
void bpm()
{

  // colored stripes pulsing at a defined Beats-Per-Minute (BPM)
  if (beatAvg != 0)
    BeatsPerMinute = (uint8_t)beatAvg;

  CRGBPalette16 palette = white_palette;//saturated_orange_palette ;//LavaColors_p;//PartyColors_p;
  ////////////////////////////////////////////////////
  /// si la persona supera los 90 bpm color ambar
  ////////////////////////////////////////
  if (BeatsPerMinute > 90) {
    palette = saturated_orange_palette;//LavaColors_p; //orange_fire_palette;//HeatColors_p ;// buscado el ambar
  }

  uint8_t beat = beatsin8( BeatsPerMinute, 64, 255);
  for ( int i = 0; i < NUM_LEDS; i++) { //9948
    leds[i] = ColorFromPalette(palette, gHue + (i * 6), beat - gHue + (i * 5));
  }
}

// Add one layer of waves into the led array
void pacifica_one_layer( CRGBPalette16& p, uint16_t cistart, uint16_t wavescale, uint8_t bri, uint16_t ioff)
{
  uint16_t ci = cistart;
  uint16_t waveangle = ioff;
  uint16_t wavescale_half = (wavescale / 2) + 20;
  for ( uint16_t i = 0; i < NUM_LEDS; i++) {
    waveangle += 250;
    uint16_t s16 = sin16( waveangle ) + 32768;
    uint16_t cs = scale16( s16 , wavescale_half ) + wavescale_half;
    ci += cs;
    uint16_t sindex16 = sin16( ci) + 32768;
    uint8_t sindex8 = scale16( sindex16, 240);
    CRGB c = ColorFromPalette( p, sindex8, bri, LINEARBLEND);
    leds[i] += c;
  }
}

// Add extra 'white' to areas where the four layers of light have lined up brightly
void pacifica_add_whitecaps()
{
  uint8_t basethreshold = beatsin8( 9, 55, 65);
  uint8_t wave = beat8( 7 );

  for ( uint16_t i = 0; i < NUM_LEDS; i++) {
    uint8_t threshold = scale8( sin8( wave), 20) + basethreshold;
    wave += 7;
    uint8_t l = leds[i].getAverageLight();
    if ( l > threshold) {
      uint8_t overage = l - threshold;
      uint8_t overage2 = qadd8( overage, overage);
      leds[i] += CRGB( overage, overage2, qadd8( overage2, overage2));
    }
  }
}

// Deepen the blues and greens
void pacifica_deepen_colors()
{
  for ( uint16_t i = 0; i < NUM_LEDS; i++) {
    leds[i].blue = scale8( leds[i].blue,  145);
    leds[i].green = scale8( leds[i].green, 200);
    leds[i] |= CRGB( 2, 5, 7);
  }
}

void respirarNaranja()
{
  EVERY_N_MILLISECONDS( 20) {
    pacifica_loop();
    FastLED.show();
  }
}

void sinelon()
{
  // a colored dot sweeping back and forth, with fading trails
  fadeToBlackBy( leds, NUM_LEDS, 30);
  int pos = beatsin16( 30, 0, NUM_LEDS - 1 );
  leds[pos] += CHSV( gHue, 255, 192);
}

void sinelonAmbar()
{
  // a colored dot sweeping back and forth, with fading trails
  fadeToBlackBy( leds, NUM_LEDS, 30);
  int pos = beatsin16( 30, 0, NUM_LEDS - 1 );
  leds[pos] = CRGB(200, 200 * 0.2, 0);
}

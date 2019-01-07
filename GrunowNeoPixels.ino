#include <Encoder.h>
#include <Adafruit_NeoPixel.h>
#ifdef __AVR__
  #include <avr/power.h>
#endif

#define PIN_NEOPIXELS 6

// Parameter 1 = number of pixels in strip
// Parameter 2 = Arduino pin number (most are valid)
// Parameter 3 = pixel type flags, add together as needed:
//   NEO_KHZ800  800 KHz bitstream (most NeoPixel products w/WS2812 LEDs)
//   NEO_KHZ400  400 KHz (classic 'v1' (not v2) FLORA pixels, WS2811 drivers)
//   NEO_GRB     Pixels are wired for GRB bitstream (most NeoPixel products)
//   NEO_RGB     Pixels are wired for RGB bitstream (v1 FLORA pixels, not v2)
//   NEO_RGBW    Pixels are wired for RGBW bitstream (NeoPixel RGBW products)
Adafruit_NeoPixel strip = Adafruit_NeoPixel(36, PIN_NEOPIXELS, NEO_GRB + NEO_KHZ800);

// IMPORTANT: To reduce NeoPixel burnout risk, add 1000 uF capacitor across
// pixel power leads, add 300 - 500 Ohm resistor on first pixel's data input
// and minimize distance between Arduino and first pixel.  Avoid connecting
// on a live circuit...if you must, connect GND first.

// AUDIO INPUT SETUP
int PIN_STROBE = 4;
int PIN_RESET = 5;
int PIN_AUDIO_LEFT = 0;
int PIN_AUDIO_RIGHT = 1;
int left[7];
int right[7];
int band;
int audio_input = 0;
int freq = 0;

// SELECTOR KNOB
int PIN_KNOBA = 8;
int PIN_KNOBB = 7;
Encoder knob(PIN_KNOBA, PIN_KNOBB);
long knobPosition = 0;

// VISUALIZER
uint32_t yellow = strip.Color(255, 255, 0);
uint32_t orange = strip.Color(236, 214, 33);
uint32_t red = strip.Color(255, 0, 0);
uint32_t blue = strip.Color(0, 0, 255);
uint32_t navy = strip.Color(0, 0, 160);
uint32_t purple = strip.Color(122, 0, 255);
uint32_t violet = strip.Color(85, 0, 150);
uint32_t green = strip.Color(0, 203, 0);
uint32_t darkgreen = strip.Color(0, 140, 0);
uint32_t magenta = strip.Color(255, 0, 255);
uint32_t rose = strip.Color(255, 122, 180);
uint32_t cyan = strip.Color(0, 255, 255);
uint32_t turqoise = strip.Color(0, 200, 255);
uint32_t white = strip.Color(255, 255, 255);
uint32_t black = strip.Color(0, 0, 0);

uint16_t leftChannel[6][3] = {{7, 25, 26}, {8, 27, 28}, {9, 29, 30}, {10, 31, 32}, {11, 33, 34}, {12, 35, 36}};
uint16_t rightChannel[6][3] = {{6, 24, 23}, {5, 22, 21}, {4, 20, 19}, {3, 18, 17}, {2, 16, 15}, {1, 14, 13}};
uint32_t colorSets[9][6] = {{white, yellow, green, red, blue, purple},
    {purple, blue, yellow, cyan, purple, magenta},
    {cyan, blue, purple, magenta, red, white},
    {darkgreen, green, white, white, red, green},
    {navy, blue, navy, turqoise, blue, navy},
    {violet, purple, violet, magenta, purple, violet},
    {yellow, green, yellow, blue, green, yellow},
    {red, magenta, red, rose, magenta, red},
    {black, black, black, black, black, black}};
int colorIndex = 0;

// INDICATOR LED
int PIN_RED = 11;
int PIN_GREEN = 10;
int PIN_BLUE = 9;

// MAIN
void setup() {
  Serial.begin(9600);
  Serial.println("Grunow booted");
  Serial.end();
  strip.begin();
  strip.show(); // Initialize all pixels to 'off'
  strip.setBrightness(32);

  //initialize eq
  pinMode(PIN_RESET, OUTPUT); // reset
  pinMode(PIN_STROBE, OUTPUT); // strobe
  digitalWrite(PIN_RESET, LOW); // reset low
  digitalWrite(PIN_STROBE, HIGH); //pin 5 is RESET on the shield

  //setup indicator
  pinMode(PIN_RED, OUTPUT);
  pinMode(PIN_GREEN, OUTPUT);
  pinMode(PIN_BLUE, OUTPUT);
}

void loop() {
  readMSGEQ7();
  checkKnobInput();
  illuminate();
  setIndicator();
  //colorTest(100); // ms
}

void checkKnobInput() {
  int colorSetCount = sizeof(colorSets) / sizeof(colorSets[0]);
  long newPosition;
  newPosition = knob.read();
  if (newPosition < knobPosition) {
    // Go back
    colorIndex -= 1;
    if (colorIndex < 0) {
      colorIndex = colorSetCount - 1;
    }
  }
  else if (newPosition > knobPosition) {
    // Go forward
    colorIndex += 1;
    if (colorIndex >= colorSetCount) {
      colorIndex = 0;
    }
  }
  knobPosition = newPosition;
}

void setIndicator() {
  uint32_t hexValue = colorSets[colorIndex][0];
  int r = 255 - ((hexValue >> 16) & 0xFF);
  int g = 255 - ((hexValue >> 8) & 0xFF);
  int b = 255 - ((hexValue) & 0xFF);
  analogWrite(PIN_RED, r);
  analogWrite(PIN_GREEN, g);
  analogWrite(PIN_BLUE, b);
}

void illuminate() {
  strip.clear();
  for(int i = 0; i < 6; i++) { // Loop through audio banks - ignore lowest frequency
    int leftValue = left[i];
    int rightValue = right[i];
    if (leftValue > 200) {
      strip.setPixelColor(leftChannel[i][0]-1, colorSets[colorIndex][0]);
    }
    if (leftValue > 350) {
      strip.setPixelColor(leftChannel[i][0]-1, colorSets[colorIndex][1]);
      strip.setPixelColor(leftChannel[i][1]-1, colorSets[colorIndex][2]);
    }
    if (leftValue > 500) {
      strip.setPixelColor(leftChannel[i][0]-1, colorSets[colorIndex][3]);
      strip.setPixelColor(leftChannel[i][1]-1, colorSets[colorIndex][4]);
      strip.setPixelColor(leftChannel[i][2]-1, colorSets[colorIndex][5]);
    }
    if (rightValue > 200) {
      strip.setPixelColor(rightChannel[i][0]-1, colorSets[colorIndex][0]);
    }
    if (rightValue > 350) {
      strip.setPixelColor(rightChannel[i][0]-1, colorSets[colorIndex][1]);
      strip.setPixelColor(rightChannel[i][1]-1, colorSets[colorIndex][2]);
    }
    if (rightValue > 500) {
      strip.setPixelColor(rightChannel[i][0]-1, colorSets[colorIndex][3]);
      strip.setPixelColor(rightChannel[i][1]-1, colorSets[colorIndex][4]);
      strip.setPixelColor(rightChannel[i][2]-1, colorSets[colorIndex][5]);
    }
  }
  strip.show();
  delay(50);
}

void readMSGEQ7() { // Function to read 7 band equalizers
  digitalWrite(PIN_RESET, HIGH);
  digitalWrite(PIN_RESET, LOW);
  for(band=0; band <7; band++)
  {
    digitalWrite(PIN_STROBE, LOW); // strobe pin on the shield - kicks the IC up to the next band 
    delayMicroseconds(50); // 
    left[band] = analogRead(PIN_AUDIO_LEFT); // store left band reading
    right[band] = analogRead(PIN_AUDIO_RIGHT); // ... and the right
    digitalWrite(PIN_STROBE, HIGH);
  }
}

// Fill the dots one after the other with a color
void colorTest(uint8_t wait) {
  // left side
  for(int i = 0; i < 6; i++) {
    for (int j = 0; j < 3; j++) {
      int light = leftChannel[i][j] - 1;
      uint32_t c = colorSets[colorIndex][j];
      strip.setPixelColor(light, c);
      strip.show();
      delay(wait);
      strip.clear();
    }
  }
  // right side
  for(int i = 0; i < 6; i++) {
    for (int j = 0; j < 3; j++) {
      int light = rightChannel[i][j] - 1;
      uint32_t c = colorSets[colorIndex][j];
      strip.setPixelColor(light, c);
      strip.show();
      delay(wait);
      strip.clear();
    }
  }
}

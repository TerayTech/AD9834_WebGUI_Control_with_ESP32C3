#include <Arduino.h>
#include <Wire.h>
#include <SPI.h>
#include <Preferences.h>
#include <math.h>
#include "ad983x.h"

// Comment out the line below to disable OLED functionality
// #define OLED

#ifdef OLED
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#endif

// AD9834 Pin Definitions
#define AD9834_SYNC_PIN    0   // SYNC (select_pin)
#define AD9834_RESET_PIN   1   // RESET
#define AD9834_SCLK_PIN    2   // SCLK
#define AD9834_DATA_PIN    3   // DATA (MOSI)
#define AD9834_FSE_PIN     4   // Frequency Select
#define AD9834_PSE_PIN     21  // Phase Select

#ifdef OLED
// OLED Display Settings
#define OLED_SDA_PIN       6
#define OLED_SCL_PIN       7
#define OLED_RESET_PIN     -1  // Share Arduino reset pin
#define OLED_SCREEN_WIDTH  128
#define OLED_SCREEN_HEIGHT 32
#define OLED_ADDRESS       0x3C
#endif

// AD9834 Settings
#define AD9834_CLOCK_FREQ  75000000  // 75MHz reference clock

// Global Variables
#ifdef OLED
Adafruit_SSD1306 display(OLED_SCREEN_WIDTH, OLED_SCREEN_HEIGHT, &Wire, OLED_RESET_PIN);
#endif
AD983X_PIN ad9834(AD9834_SYNC_PIN, AD9834_CLOCK_FREQ / 1000000, AD9834_RESET_PIN);
Preferences preferences;

// Signal parameters
uint32_t currentFreq0 = 1000;    // Default 1kHz
uint32_t currentFreq1 = 10000;   // Default 10kHz
uint16_t currentPhase0 = 0;      // Default 0 degrees
uint16_t currentPhase1 = 0;      // Default 0 degrees
byte activeFreqReg = 0;          // Default to frequency register 0
byte activePhaseReg = 0;         // Default to phase register 0
OutputMode currentMode = OUTPUT_MODE_SINE; // Default to sine wave

// Command buffer
const int MAX_COMMAND_LENGTH = 32;
char cmdBuffer[MAX_COMMAND_LENGTH];
int cmdIndex = 0;

// Function prototypes
void processCommand();
#ifdef OLED
void updateDisplay();
#endif
void printHelp();
void setFrequency(byte reg, uint32_t freq);
void setPhase(byte reg, uint16_t phase);
void setWaveform(OutputMode mode);
void selectFrequencyRegister(byte reg);
void selectPhaseRegister(byte reg);
void saveSettings();
void loadSettings();
#ifdef OLED
void drawWaveform();
#endif

void setup() {
  // Initialize Serial
  Serial.begin(115200);
  while (!Serial) delay(10);
  
  // Initialize SPI for AD9834
  SPI.begin(AD9834_SCLK_PIN, -1, AD9834_DATA_PIN, AD9834_SYNC_PIN);
  
  // Initialize FSE and PSE pins
  pinMode(AD9834_FSE_PIN, OUTPUT);
  pinMode(AD9834_PSE_PIN, OUTPUT);
  digitalWrite(AD9834_FSE_PIN, LOW);  // Select FREQ0 by default
  digitalWrite(AD9834_PSE_PIN, LOW);  // Select PHASE0 by default
  
  // Initialize AD9834
  ad9834.begin();
  
  // Load saved settings or use defaults
  loadSettings();
  
  // Apply loaded settings
  setFrequency(0, currentFreq0);
  setFrequency(1, currentFreq1);
  setPhase(0, currentPhase0);
  setPhase(1, currentPhase1);
  setWaveform(currentMode);
  selectFrequencyRegister(activeFreqReg);
  selectPhaseRegister(activePhaseReg);
  
  #ifdef OLED
  // Initialize I2C for OLED
  Wire.begin(OLED_SDA_PIN, OLED_SCL_PIN);
  
  // Initialize OLED display
  if(!display.begin(SSD1306_SWITCHCAPVCC, OLED_ADDRESS)) {
    Serial.println(F("SSD1306 allocation failed"));
    for(;;); // Don't proceed, loop forever
  }
  
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 0);
  display.println(F("AD9834 Signal Generator"));
  display.println(F("Initializing..."));
  display.display();
  delay(1000);
  
  // Update display with current settings
  updateDisplay();
  #endif
  
  // Print welcome message and help
  Serial.println(F("\nAD9834 Signal Generator"));
  Serial.println(F("Based on 75MHz reference clock"));
  printHelp();
}

void loop() {
  // Check for serial commands
  while (Serial.available() > 0) {
    char c = Serial.read();
    
    // Process command on newline or carriage return
    if (c == '\n' || c == '\r') {
      if (cmdIndex > 0) {
        cmdBuffer[cmdIndex] = '\0';  // Null-terminate the string
        processCommand();
        cmdIndex = 0;  // Reset buffer for next command
      }
    } 
    // Add character to buffer if not full
    else if (cmdIndex < MAX_COMMAND_LENGTH - 1) {
      cmdBuffer[cmdIndex++] = c;
    }
  }
}

void processCommand() {
  char cmd = cmdBuffer[0];
  char subCmd = (cmdIndex > 1) ? cmdBuffer[1] : '\0';
  char* param = (cmdIndex > 2) ? &cmdBuffer[2] : NULL;
  
  uint32_t value = 0;
  if (param != NULL) {
    value = atol(param);
  }
  
  switch (cmd) {
    case 'F': // Set Frequency
    case 'f':
      if (subCmd == '0') {
        setFrequency(0, value);
        Serial.print(F("Frequency 0 set to: "));
        Serial.println(value);
      } else if (subCmd == '1') {
        setFrequency(1, value);
        Serial.print(F("Frequency 1 set to: "));
        Serial.println(value);
      } else {
        Serial.println(F("Invalid frequency register. Use F0 or F1."));
      }
      break;
      
    case 'P': // Set Phase
    case 'p':
      if (subCmd == '0') {
        if (value <= 4095) { // 12-bit phase value
          setPhase(0, value);
          Serial.print(F("Phase 0 set to: "));
          Serial.println(value);
        } else {
          Serial.println(F("Phase value must be 0-4095"));
        }
      } else if (subCmd == '1') {
        if (value <= 4095) { // 12-bit phase value
          setPhase(1, value);
          Serial.print(F("Phase 1 set to: "));
          Serial.println(value);
        } else {
          Serial.println(F("Phase value must be 0-4095"));
        }
      } else {
        Serial.println(F("Invalid phase register. Use P0 or P1."));
      }
      break;
      
    case 'S': // Select Register
    case 's':
      if (subCmd == 'F' || subCmd == 'f') {
        if (param != NULL && (*param == '0' || *param == '1')) {
          byte reg = *param - '0';
          selectFrequencyRegister(reg);
          Serial.print(F("Selected frequency register: "));
          Serial.println(reg);
        } else {
          Serial.println(F("Invalid register. Use SF0 or SF1."));
        }
      } else if (subCmd == 'P' || subCmd == 'p') {
        if (param != NULL && (*param == '0' || *param == '1')) {
          byte reg = *param - '0';
          selectPhaseRegister(reg);
          Serial.print(F("Selected phase register: "));
          Serial.println(reg);
        } else {
          Serial.println(F("Invalid register. Use SP0 or SP1."));
        }
      } else {
        Serial.println(F("Invalid selection command. Use SF or SP."));
      }
      break;
      
    case 'W': // Set Waveform
    case 'w':
      if (subCmd == 'S' || subCmd == 's') {
        setWaveform(OUTPUT_MODE_SINE);
        Serial.println(F("Waveform set to Sine"));
      } else if (subCmd == 'T' || subCmd == 't') {
        setWaveform(OUTPUT_MODE_TRIANGLE);
        Serial.println(F("Waveform set to Triangle"));
      } else {
        Serial.println(F("Invalid waveform. Use WS (sine) or WT (triangle)."));
      }
      break;
      
    case '?': // Help
    case 'H':
    case 'h':
      printHelp();
      break;
      
    default:
      Serial.println(F("Unknown command. Type ? for help."));
      break;
  }
  
  #ifdef OLED
  // Update display after any command
  updateDisplay();
  #endif
}

void setFrequency(byte reg, uint32_t freq) {
  if (reg == 0) {
    currentFreq0 = freq;
  } else {
    currentFreq1 = freq;
  }
  ad9834.setFrequency(reg, (long int)freq);
  saveSettings(); // Save settings after change
  #ifdef OLED
  updateDisplay();
  #endif
}

void setPhase(byte reg, uint16_t phase) {
  if (reg == 0) {
    currentPhase0 = phase;
  } else {
    currentPhase1 = phase;
  }
  ad9834.setPhaseWord(reg, phase);
  saveSettings(); // Save settings after change
}

void setWaveform(OutputMode mode) {
  currentMode = mode;
  ad9834.setOutputMode(mode);
  saveSettings(); // Save settings after change
}

void selectFrequencyRegister(byte reg) {
  activeFreqReg = reg;
  digitalWrite(AD9834_FSE_PIN, reg); // Set FSE pin based on register selection
  saveSettings(); // Save settings after change
}

void selectPhaseRegister(byte reg) {
  activePhaseReg = reg;
  digitalWrite(AD9834_PSE_PIN, reg); // Set PSE pin based on register selection
  saveSettings(); // Save settings after change
}

// Save current settings to non-volatile memory
void saveSettings() {
  preferences.begin("ad9834", false); // Open namespace in RW mode
  
  preferences.putULong("freq0", currentFreq0);
  preferences.putULong("freq1", currentFreq1);
  preferences.putUShort("phase0", currentPhase0);
  preferences.putUShort("phase1", currentPhase1);
  preferences.putUChar("activeFreq", activeFreqReg);
  preferences.putUChar("activePhase", activePhaseReg);
  preferences.putUChar("waveMode", (uint8_t)currentMode);
  
  preferences.end(); // Close the namespace
  
  Serial.println(F("Settings saved"));
}

// Load settings from non-volatile memory
void loadSettings() {
  preferences.begin("ad9834", true); // Open namespace in read-only mode
  
  // Load settings with defaults if not found
  currentFreq0 = preferences.getULong("freq0", 1000);
  currentFreq1 = preferences.getULong("freq1", 10000);
  currentPhase0 = preferences.getUShort("phase0", 0);
  currentPhase1 = preferences.getUShort("phase1", 0);
  activeFreqReg = preferences.getUChar("activeFreq", 0);
  activePhaseReg = preferences.getUChar("activePhase", 0);
  currentMode = (OutputMode)preferences.getUChar("waveMode", OUTPUT_MODE_SINE);
  
  preferences.end(); // Close the namespace
  
  Serial.println(F("Settings loaded"));
}

#ifdef OLED
// Draw a simple waveform visualization
void drawWaveform() {
  const int waveHeight = 8;   // Height of the waveform
  const int waveY = 24;       // Y position of the waveform (bottom of text area)
  const int waveWidth = 128;  // Width of the waveform (full display width)
  
  if (currentMode == OUTPUT_MODE_SINE) {
    // Draw sine wave
    for (int x = 0; x < waveWidth; x++) {
      // Convert x to radians (2*PI = full wave cycle)
      float angle = (x * 2.0 * PI) / 32.0; // 32 pixels per cycle
      int y = waveY + sin(angle) * (waveHeight / 2);
      
      // Draw a slightly thicker point for better visibility
      display.drawPixel(x, y, SSD1306_WHITE);
      display.drawPixel(x, y+1, SSD1306_WHITE); // Make it a bit thicker
    }
  } else {
    // Draw triangle wave
    const int period = 16;  // Pixels per triangle period
    for (int x = 0; x < waveWidth; x++) {
      int triangleX = x % period;
      int y;
      
      if (triangleX < period / 2) {
        // Rising edge
        y = waveY - waveHeight / 2 + triangleX * waveHeight / (period / 2);
      } else {
        // Falling edge
        y = waveY + waveHeight / 2 - (triangleX - period / 2) * waveHeight / (period / 2);
      }
      
      // Draw a slightly thicker point for better visibility
      display.drawPixel(x, y, SSD1306_WHITE);
      display.drawPixel(x, y+1, SSD1306_WHITE); // Make it a bit thicker
    }
  }
}

void updateDisplay() {
  display.clearDisplay();
  display.setCursor(0, 0);
  
  // Display active frequency
  display.print(F("F"));
  display.print(activeFreqReg);
  display.print(F(": "));
  display.print(activeFreqReg == 0 ? currentFreq0 : currentFreq1);
  display.println(F(" Hz"));
  
  // Display active phase
  display.print(F("P"));
  display.print(activePhaseReg);
  display.print(F(": "));
  display.print(activePhaseReg == 0 ? currentPhase0 : currentPhase1);
  display.println(F(" deg"));
  
  // Display waveform type
  display.print(F("Mode: "));
  display.println(currentMode == OUTPUT_MODE_SINE ? F("Sine") : F("Triangle"));
  
  // Draw waveform visualization
  drawWaveform();
  
  display.display();
}
#endif

void printHelp() {
  Serial.println(F("\n--- AD9834 Signal Generator Commands ---"));
  Serial.println(F("F0xxxxx  - Set Frequency 0 (in Hz)"));
  Serial.println(F("F1xxxxx  - Set Frequency 1 (in Hz)"));
  Serial.println(F("P0xxxx   - Set Phase 0 (0-4095)"));
  Serial.println(F("P1xxxx   - Set Phase 1 (0-4095)"));
  Serial.println(F("SF0      - Select Frequency Register 0"));
  Serial.println(F("SF1      - Select Frequency Register 1"));
  Serial.println(F("SP0      - Select Phase Register 0"));
  Serial.println(F("SP1      - Select Phase Register 1"));
  Serial.println(F("WS       - Set Waveform to Sine"));
  Serial.println(F("WT       - Set Waveform to Triangle"));
  Serial.println(F("?/H      - Show this help"));
  Serial.println(F("----------------------------------------"));
}

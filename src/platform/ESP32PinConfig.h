#ifndef _ESP32_PIN_CONFIG_H
#define _ESP32_PIN_CONFIG_H

// ============================================================================
// FLCL - FIRE STARTER: ESP32-S3 Handheld Hardware Pin Configuration
// Target Boards: Adafruit Feather ESP32-S3 (PID 5477) / Waveshare ESP32-S3-Pico
// Display:       ST7789 2.0" / 2.4" 240x320 IPS TFT (4-wire SPI)
// Audio:         MAX98357A I2S Class-D Mono Amplifier + 8 Ohm 1W Speaker
// Controls:      Active-LOW tactile switches using internal GPIO pull-ups
// ============================================================================

// --- ST7789 240x320 Portrait SPI Display Pins ---
#ifndef PIN_TFT_MOSI
#define PIN_TFT_MOSI   35  // SPI MOSI (DIN)
#endif
#ifndef PIN_TFT_SCLK
#define PIN_TFT_SCLK   36  // SPI SCLK (CLK)
#endif
#ifndef PIN_TFT_CS
#define PIN_TFT_CS     9   // Chip Select
#endif
#ifndef PIN_TFT_DC
#define PIN_TFT_DC     10  // Data / Command
#endif
#ifndef PIN_TFT_RST
#define PIN_TFT_RST    11  // Reset (-1 if tied to 3.3V/EN)
#endif
#ifndef PIN_TFT_BL
#define PIN_TFT_BL     12  // Backlight control (HIGH = ON)
#endif

// --- MAX98357A I2S Audio Amplifier Pins ---
#ifndef PIN_I2S_BCLK
#define PIN_I2S_BCLK   14  // Bit Clock (BCLK)
#endif
#ifndef PIN_I2S_LRCLK
#define PIN_I2S_LRCLK  15  // Word Select / Left-Right Clock (LRC)
#endif
#ifndef PIN_I2S_DOUT
#define PIN_I2S_DOUT   16  // Serial Data Out (DIN on MAX98357A)
#endif

// --- Handheld Control Buttons (Active LOW, wired between GPIO and GND) ---
// Left Circular Pad: 4-way D-Pad
#ifndef PIN_BTN_UP
#define PIN_BTN_UP     5
#endif
#ifndef PIN_BTN_DOWN
#define PIN_BTN_DOWN   6
#endif
#ifndef PIN_BTN_LEFT
#define PIN_BTN_LEFT   17
#endif
#ifndef PIN_BTN_RIGHT
#define PIN_BTN_RIGHT  18
#endif

// Right Circular Pad: A (Burn / Confirm) + optional B
#ifndef PIN_BTN_A
#define PIN_BTN_A      13
#endif
#ifndef PIN_BTN_B
#define PIN_BTN_B      1
#endif

// Right Bezel Angled Pill Buttons:
// Top Angled Button = START (Start Game / Advance Intro / Pause & City Map)
// Bottom Angled Button = SELECT (Switch Music Track)
#ifndef PIN_BTN_START
#define PIN_BTN_START  38
#endif
#ifndef PIN_BTN_SELECT
#define PIN_BTN_SELECT 39
#endif

// Audio mixer sample rate (22,050 Hz stereo/mono I2S for crisp chiptune + SFX)
#ifndef FIRESTARTER_AUDIO_SAMPLE_RATE
#define FIRESTARTER_AUDIO_SAMPLE_RATE 22050
#endif

#endif

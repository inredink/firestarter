#if defined(FIRESTARTER_PLATFORM_ESP32) && defined(ARDUINO)

#include <Arduino.h>
#include <SPI.h>
#include <driver/i2s.h>

#include "ESP32PinConfig.h"
#include "esp32_shim/allegro.h"
#include "../Main.h"
#include "../FireStarter.h"

extern "C" void esp32_tick_timers(int elapsed_ms);

// ============================================================================
// ST7789 240x320 Portrait SPI Driver (Self-Contained, Zero Config Files Needed)
// ============================================================================
static SPIClass *g_tft_spi = NULL;
static uint16_t g_line_swap_buf[240];

static inline void tft_write_cmd(uint8_t cmd) {
    digitalWrite(PIN_TFT_DC, LOW);
    digitalWrite(PIN_TFT_CS, LOW);
    g_tft_spi->transfer(cmd);
    digitalWrite(PIN_TFT_CS, HIGH);
}

static inline void tft_write_data8(uint8_t data) {
    digitalWrite(PIN_TFT_DC, HIGH);
    digitalWrite(PIN_TFT_CS, LOW);
    g_tft_spi->transfer(data);
    digitalWrite(PIN_TFT_CS, HIGH);
}

static void st7789_init_display(void) {
    pinMode(PIN_TFT_CS, OUTPUT);
    pinMode(PIN_TFT_DC, OUTPUT);
    digitalWrite(PIN_TFT_CS, HIGH);
    digitalWrite(PIN_TFT_DC, HIGH);

    if (PIN_TFT_RST >= 0) {
        pinMode(PIN_TFT_RST, OUTPUT);
        digitalWrite(PIN_TFT_RST, HIGH);
        delay(20);
        digitalWrite(PIN_TFT_RST, LOW);
        delay(50);
        digitalWrite(PIN_TFT_RST, HIGH);
        delay(120);
    }

    if (PIN_TFT_BL >= 0) {
        pinMode(PIN_TFT_BL, OUTPUT);
        digitalWrite(PIN_TFT_BL, HIGH);
    }

    g_tft_spi = new SPIClass(FSPI);
    g_tft_spi->begin(PIN_TFT_SCLK, -1, PIN_TFT_MOSI, PIN_TFT_CS);
    g_tft_spi->beginTransaction(SPISettings(40000000, MSBFIRST, SPI_MODE0));

    tft_write_cmd(0x01); // SWRESET
    delay(150);
    tft_write_cmd(0x11); // SLPOUT
    delay(120);

    tft_write_cmd(0x3A); // COLMOD: 16-bit RGB565 (0x55)
    tft_write_data8(0x55);

    tft_write_cmd(0x36); // MADCTL: Portrait 240x320 RGB order (0x00)
    tft_write_data8(0x00);

    tft_write_cmd(0x21); // INVON (ST7789 IPS panels require display inversion ON for accurate RGB)
    delay(10);
    tft_write_cmd(0x13); // NORON
    delay(10);
    tft_write_cmd(0x29); // DISPON
    delay(50);
}

extern "C" void esp32_flush_screen_from_allegro(const uint16_t *pixels, int w, int h) {
    if (!g_tft_spi || !pixels) return;
    int flushW = (w > 240) ? 240 : w;
    int flushH = (h > 320) ? 320 : h;

    // CASET (0 .. flushW - 1)
    tft_write_cmd(0x2A);
    digitalWrite(PIN_TFT_DC, HIGH);
    digitalWrite(PIN_TFT_CS, LOW);
    g_tft_spi->transfer(0x00);
    g_tft_spi->transfer(0x00);
    g_tft_spi->transfer((uint8_t)((flushW - 1) >> 8));
    g_tft_spi->transfer((uint8_t)((flushW - 1) & 0xFF));
    digitalWrite(PIN_TFT_CS, HIGH);

    // RASET (0 .. flushH - 1)
    tft_write_cmd(0x2B);
    digitalWrite(PIN_TFT_DC, HIGH);
    digitalWrite(PIN_TFT_CS, LOW);
    g_tft_spi->transfer(0x00);
    g_tft_spi->transfer(0x00);
    g_tft_spi->transfer((uint8_t)((flushH - 1) >> 8));
    g_tft_spi->transfer((uint8_t)((flushH - 1) & 0xFF));
    digitalWrite(PIN_TFT_CS, HIGH);

    // RAMWR (stream big-endian RGB565 scanlines)
    tft_write_cmd(0x2C);
    digitalWrite(PIN_TFT_DC, HIGH);
    digitalWrite(PIN_TFT_CS, LOW);
    for (int y = 0; y < flushH; y++) {
        const uint16_t *row = pixels + y * w;
        for (int x = 0; x < flushW; x++) {
            uint16_t px = row[x];
            g_line_swap_buf[x] = (uint16_t)((px >> 8) | (px << 8));
        }
        g_tft_spi->writeBytes((const uint8_t *)g_line_swap_buf, (uint32_t)(flushW * 2));
    }
    digitalWrite(PIN_TFT_CS, HIGH);
}

// ============================================================================
// Active-LOW GPIO Button Polling
// ============================================================================
static uint32_t g_combo_hold_start_ms = 0;

static void esp32_init_buttons(void) {
    pinMode(PIN_BTN_UP,     INPUT_PULLUP);
    pinMode(PIN_BTN_DOWN,   INPUT_PULLUP);
    pinMode(PIN_BTN_LEFT,   INPUT_PULLUP);
    pinMode(PIN_BTN_RIGHT,  INPUT_PULLUP);
    pinMode(PIN_BTN_A,      INPUT_PULLUP);
    pinMode(PIN_BTN_B,      INPUT_PULLUP);
    pinMode(PIN_BTN_START,  INPUT_PULLUP);
    pinMode(PIN_BTN_SELECT, INPUT_PULLUP);
}

extern "C" void esp32_poll_buttons(void) {
    key[KEY_UP]     = (digitalRead(PIN_BTN_UP)     == LOW) ? 1 : 0;
    key[KEY_DOWN]   = (digitalRead(PIN_BTN_DOWN)   == LOW) ? 1 : 0;
    key[KEY_LEFT]   = (digitalRead(PIN_BTN_LEFT)   == LOW) ? 1 : 0;
    key[KEY_RIGHT]  = (digitalRead(PIN_BTN_RIGHT)  == LOW) ? 1 : 0;
    key[KEY_A]      = (digitalRead(PIN_BTN_A)      == LOW) ? 1 : 0;
    key[KEY_Z]      = (digitalRead(PIN_BTN_B)      == LOW) ? 1 : 0;
    key[KEY_ENTER]  = (digitalRead(PIN_BTN_START)  == LOW) ? 1 : 0;
    key[KEY_RSHIFT] = (digitalRead(PIN_BTN_SELECT) == LOW) ? 1 : 0;

    // Holding START + SELECT together for 1.5 seconds resets to the Title Screen (F2)
    if (key[KEY_ENTER] && key[KEY_RSHIFT]) {
        if (g_combo_hold_start_ms == 0) {
            g_combo_hold_start_ms = millis();
        } else if (millis() - g_combo_hold_start_ms >= 1500) {
            key[KEY_F2] = 1;
            g_combo_hold_start_ms = millis();
        }
    } else {
        g_combo_hold_start_ms = 0;
        key[KEY_F2] = 0;
    }
}

// ============================================================================
// MAX98357A I2S Audio Output Task (Runs on ESP32-S3 Core 0)
// ============================================================================
static void esp32_audio_task(void *param) {
    (void)param;
    static int16_t audio_buf[512 * 2]; // 512 stereo frames per DMA slice (~23 ms)
    for (;;) {
        esp32_render_audio_stream(audio_buf, 512, FIRESTARTER_AUDIO_SAMPLE_RATE);
        size_t bytes_written = 0;
        i2s_write(I2S_NUM_0, audio_buf, sizeof(audio_buf), &bytes_written, portMAX_DELAY);
    }
}

static void esp32_init_i2s_audio(void) {
    i2s_config_t i2s_cfg = {};
    i2s_cfg.mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_TX);
    i2s_cfg.sample_rate = FIRESTARTER_AUDIO_SAMPLE_RATE;
    i2s_cfg.bits_per_sample = I2S_BITS_PER_SAMPLE_16BIT;
    i2s_cfg.channel_format = I2S_CHANNEL_FMT_RIGHT_LEFT;
    i2s_cfg.communication_format = I2S_COMM_FORMAT_STAND_I2S;
    i2s_cfg.intr_alloc_flags = ESP_INTR_FLAG_LEVEL1;
    i2s_cfg.dma_buf_count = 6;
    i2s_cfg.dma_buf_len = 256;
    i2s_cfg.use_apll = false;
    i2s_cfg.tx_desc_auto_clear = true;

    i2s_pin_config_t pin_cfg = {};
    pin_cfg.bck_io_num = PIN_I2S_BCLK;
    pin_cfg.ws_io_num = PIN_I2S_LRCLK;
    pin_cfg.data_out_num = PIN_I2S_DOUT;
    pin_cfg.data_in_num = I2S_PIN_NO_CHANGE;

    i2s_driver_install(I2S_NUM_0, &i2s_cfg, 0, NULL);
    i2s_set_pin(I2S_NUM_0, &pin_cfg);
    i2s_zero_dma_buffer(I2S_NUM_0);

    xTaskCreatePinnedToCore(
        esp32_audio_task,
        "FireStarterAudio",
        8192,
        NULL,
        2,
        NULL,
        0
    );
}

// ============================================================================
// Arduino Setup & Main Loop
// ============================================================================
static FireStarter *g_handheld_game = NULL;
static uint32_t g_last_tick_ms = 0;
static uint32_t g_accum_ms = 0;

void setup() {
    Serial.begin(115200);
    esp32_init_buttons();
    st7789_init_display();
    esp32_init_i2s_audio();

    gLog.start("log.txt", Log::vSystem);
    g_handheld_game = new FireStarter();
    g_handheld_game->initialize();
    g_last_tick_ms = millis();
    g_accum_ms = 0;
}

void loop() {
    uint32_t now_ms = millis();
    uint32_t elapsed = now_ms - g_last_tick_ms;
    if (elapsed > 0) {
        g_last_tick_ms = now_ms;
        if (elapsed > 250) elapsed = 250;
        g_accum_ms += elapsed;
        bool updated = false;
        while (g_accum_ms >= 10) { // 100 Hz original game logic rate
            g_accum_ms -= 10;
            g_handheld_game->updateGameLogic();
            updated = true;
        }
        if (updated) {
            g_handheld_game->presentScreen();
        }
    } else {
        delay(1);
    }
}

#endif // defined(FIRESTARTER_PLATFORM_ESP32) && defined(ARDUINO)

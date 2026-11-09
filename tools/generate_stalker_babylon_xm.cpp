#include <cstdio>
#include <cstdlib>
#include <cstdint>
#include <cstring>
#include <cmath>
#include <vector>

#include "babylon_score_data.h"

static const double PI = 3.14159265358979323846;

static void write_u8(std::vector<uint8_t> &buf, uint8_t v) {
    buf.push_back(v);
}

static void write_u16(std::vector<uint8_t> &buf, uint16_t v) {
    buf.push_back((uint8_t)(v & 0xFF));
    buf.push_back((uint8_t)((v >> 8) & 0xFF));
}

static void write_u32(std::vector<uint8_t> &buf, uint32_t v) {
    buf.push_back((uint8_t)(v & 0xFF));
    buf.push_back((uint8_t)((v >> 8) & 0xFF));
    buf.push_back((uint8_t)((v >> 16) & 0xFF));
    buf.push_back((uint8_t)((v >> 24) & 0xFF));
}

static void write_padded_str(std::vector<uint8_t> &buf, const char *s, size_t len) {
    size_t slen = s ? strlen(s) : 0;
    for (size_t i = 0; i < len; i++) {
        buf.push_back(i < slen ? (uint8_t)s[i] : 0);
    }
}

struct XmInstrumentSpec {
    const char *name;
    std::vector<int16_t> pcm;
    uint32_t loopStartSamples;
    uint32_t loopLenSamples;
    bool looped;
    uint8_t defaultVol;
    uint8_t defaultPan;
    int8_t finetune;
    int8_t relNote;
    bool useVolEnv;
    uint16_t volEnvPoints[12][2];
    uint8_t volEnvNodes;
    uint8_t volSustainPoint;
    uint16_t fadeout;
    uint8_t vibType;
    uint8_t vibSweep;
    uint8_t vibDepth;
    uint8_t vibSpeed;
};

static void append_instrument(std::vector<uint8_t> &out, const XmInstrumentSpec &spec) {
    // Instrument header size = 263 bytes (standard FT2 instrument header)
    write_u32(out, 263);
    write_padded_str(out, spec.name, 22);
    write_u8(out, 0);   // Instrument type = 0
    write_u16(out, 1);  // Number of samples = 1
    write_u32(out, 40); // Sample header size = 40 bytes

    // Sample map for 96 notes (all map to sample 0)
    for (int i = 0; i < 96; i++) {
        write_u8(out, 0);
    }

    // 12 Volume envelope points (x, y) -> 48 bytes
    for (int i = 0; i < 12; i++) {
        uint16_t x = (i < spec.volEnvNodes) ? spec.volEnvPoints[i][0] : 0;
        uint16_t y = (i < spec.volEnvNodes) ? spec.volEnvPoints[i][1] : 0;
        write_u16(out, x);
        write_u16(out, y);
    }

    // 12 Panning envelope points -> 48 bytes
    for (int i = 0; i < 12; i++) {
        write_u16(out, (uint16_t)(i * 16));
        write_u16(out, 32);
    }

    write_u8(out, spec.volEnvNodes); // Number of volume points
    write_u8(out, 0);                // Number of panning points
    write_u8(out, spec.volSustainPoint); // Volume sustain point
    write_u8(out, 0);                // Volume loop start
    write_u8(out, 0);                // Volume loop end
    write_u8(out, 0);                // Panning sustain point
    write_u8(out, 0);                // Panning loop start
    write_u8(out, 0);                // Panning loop end

    // Volume envelope type: bit 0 = On, bit 1 = Sustain
    uint8_t volType = spec.useVolEnv ? (1 | 2) : 0;
    write_u8(out, volType);
    write_u8(out, 0); // Panning type = Off

    write_u8(out, spec.vibType);
    write_u8(out, spec.vibSweep);
    write_u8(out, spec.vibDepth);
    write_u8(out, spec.vibSpeed);
    write_u16(out, spec.fadeout);

    // Pad remaining 22 bytes of 263-byte instrument header (241 bytes written so far)
    for (int i = 0; i < 22; i++) {
        write_u8(out, 0);
    }

    // Sample header (40 bytes) — lengths in BYTES for 16-bit samples!
    uint32_t lenBytes = (uint32_t)spec.pcm.size() * 2;
    uint32_t loopStartBytes = spec.loopStartSamples * 2;
    uint32_t loopLenBytes = spec.loopLenSamples * 2;
    write_u32(out, lenBytes);
    write_u32(out, loopStartBytes);
    write_u32(out, loopLenBytes);
    write_u8(out, spec.defaultVol);
    write_u8(out, (uint8_t)spec.finetune);
    // Sample type: bit 4 (0x10) = 16-bit sample, bit 0 (0x01) = forward loop
    uint8_t sampleType = 0x10 | (spec.looped ? 0x01 : 0x00);
    write_u8(out, sampleType);
    write_u8(out, spec.defaultPan);
    write_u8(out, (uint8_t)spec.relNote);
    write_u8(out, 0); // Reserved (0 = standard delta PCM)
    write_padded_str(out, spec.name, 22);

    // Sample waveform data: 16-bit signed delta-encoded little-endian
    int16_t prev = 0;
    for (size_t i = 0; i < spec.pcm.size(); i++) {
        int16_t cur = spec.pcm[i];
        int16_t delta = (int16_t)(cur - prev);
        prev = cur;
        write_u16(out, (uint16_t)delta);
    }
}

// ============================================================================
// Synthesize the 9 16-Bit Instruments for "Stalker Goes to Babylon"
// At relNote = 0, C-4 (XM note 49, MIDI 60) plays at 16,726 Hz.
// One cycle of 64 samples at 16,726 Hz = 261.34 Hz (Concert C4!).
// ============================================================================
static XmInstrumentSpec make_overdrive_guitar() {
    XmInstrumentSpec s = {};
    s.name = "16b Overdrive Guitar";
    s.loopStartSamples = 2048;
    s.loopLenSamples = 1024; // 16 cycles of 64 samples
    s.looped = true;
    s.defaultVol = 64;
    s.defaultPan = 128;
    s.finetune = 2;
    s.relNote = 0;
    s.useVolEnv = true;
    s.volEnvNodes = 4;
    s.volEnvPoints[0][0] = 0;   s.volEnvPoints[0][1] = 64;
    s.volEnvPoints[1][0] = 6;   s.volEnvPoints[1][1] = 54;
    s.volEnvPoints[2][0] = 24;  s.volEnvPoints[2][1] = 48;
    s.volEnvPoints[3][0] = 48;  s.volEnvPoints[3][1] = 0;
    s.volSustainPoint = 2;
    s.fadeout = 1400;
    s.vibType = 0; s.vibSweep = 20; s.vibDepth = 2; s.vibSpeed = 10;

    uint32_t total = s.loopStartSamples + s.loopLenSamples;
    s.pcm.resize(total);
    uint32_t rng = 0x5A17E901u;
    for (uint32_t i = 0; i < total; i++) {
        double ph = (2.0 * PI * (double)(i % 64)) / 64.0;
        double modPh = (2.0 * PI * (double)(i % 1024)) / 1024.0;
        // Rich guitar harmonic series + slight chorus modulation
        double raw = 1.00 * sin(ph + 0.15 * sin(modPh))
                   + 0.72 * sin(2.0 * ph + 0.3)
                   + 0.55 * sin(3.0 * ph - 0.2 * cos(modPh))
                   + 0.38 * sin(4.0 * ph)
                   + 0.25 * sin(5.0 * ph + 0.4)
                   + 0.15 * sin(6.0 * ph);
        // Add crisp pick transient in the first 380 samples (non-looped region)
        if (i < 380) {
            rng = rng * 1664525u + 1013904223u;
            double noise = ((double)(int32_t)rng / 2147483648.0);
            double env = 1.0 - ((double)i / 380.0);
            raw += noise * env * 0.9;
        }
        // Asymmetric tube-style soft overdrive clipping
        double driven = tanh(raw * 1.45 + 0.12) - tanh(0.12);
        int32_t val = (int32_t)(driven * 22000.0);
        if (val > 32760) val = 32760; else if (val < -32760) val = -32760;
        s.pcm[i] = (int16_t)val;
    }
    return s;
}

static XmInstrumentSpec make_fm_lead_double() {
    XmInstrumentSpec s = {};
    s.name = "16b FM Lead Double";
    s.loopStartSamples = 1024;
    s.loopLenSamples = 1024;
    s.looped = true;
    s.defaultVol = 46;
    s.defaultPan = 160;
    s.finetune = 4;
    s.relNote = 0;
    s.useVolEnv = true;
    s.volEnvNodes = 4;
    s.volEnvPoints[0][0] = 0;   s.volEnvPoints[0][1] = 62;
    s.volEnvPoints[1][0] = 8;   s.volEnvPoints[1][1] = 50;
    s.volEnvPoints[2][0] = 24;  s.volEnvPoints[2][1] = 44;
    s.volEnvPoints[3][0] = 44;  s.volEnvPoints[3][1] = 0;
    s.volSustainPoint = 2;
    s.fadeout = 1600;
    s.vibType = 0; s.vibSweep = 12; s.vibDepth = 3; s.vibSpeed = 12;

    uint32_t total = s.loopStartSamples + s.loopLenSamples;
    s.pcm.resize(total);
    for (uint32_t i = 0; i < total; i++) {
        double ph = (2.0 * PI * (double)(i % 64)) / 64.0;
        double lfo = (2.0 * PI * (double)(i % 1024)) / 1024.0;
        // 16-bit YM2612/SNES style FM + pulse lead
        double fmIndex = (i < 512) ? (1.8 - 0.8 * ((double)i / 512.0)) : (1.0 + 0.2 * sin(lfo));
        double fm = sin(ph + fmIndex * sin(2.0 * ph) + 0.35 * sin(3.0 * ph));
        double pulse = ((i % 64) < 24) ? 0.45 : -0.45;
        double mix = 0.72 * fm + 0.28 * pulse;
        int32_t val = (int32_t)(mix * 21000.0);
        if (val > 32760) val = 32760; else if (val < -32760) val = -32760;
        s.pcm[i] = (int16_t)val;
    }
    return s;
}

static XmInstrumentSpec make_picked_bass() {
    XmInstrumentSpec s = {};
    s.name = "16b Picked Rock Bass";
    s.loopStartSamples = 1536;
    s.loopLenSamples = 1024; // 8 cycles of 128 samples (or 16 cycles of 64)
    s.looped = true;
    s.defaultVol = 64;
    s.defaultPan = 120;
    s.finetune = 2;
    s.relNote = 0;
    s.useVolEnv = true;
    s.volEnvNodes = 4;
    s.volEnvPoints[0][0] = 0;   s.volEnvPoints[0][1] = 64;
    s.volEnvPoints[1][0] = 5;   s.volEnvPoints[1][1] = 56;
    s.volEnvPoints[2][0] = 20;  s.volEnvPoints[2][1] = 50;
    s.volEnvPoints[3][0] = 38;  s.volEnvPoints[3][1] = 0;
    s.volSustainPoint = 2;
    s.fadeout = 1800;

    uint32_t total = s.loopStartSamples + s.loopLenSamples;
    s.pcm.resize(total);
    for (uint32_t i = 0; i < total; i++) {
        double ph = (2.0 * PI * (double)(i % 64)) / 64.0;
        double subPh = (2.0 * PI * (double)(i % 128)) / 128.0;
        // Punchy FM picked bass with sub-octave reinforcement + upper harmonics
        double pickEnv = (i < 600) ? (1.0 - (double)i / 600.0) : 0.0;
        double wave = 0.95 * sin(ph + (0.9 + 1.4 * pickEnv) * sin(ph))
                    + 0.55 * sin(subPh)
                    + 0.45 * sin(2.0 * ph)
                    + 0.25 * sin(3.0 * ph);
        double clipped = tanh(wave * 1.35);
        int32_t val = (int32_t)(clipped * 26000.0);
        if (val > 32760) val = 32760; else if (val < -32760) val = -32760;
        s.pcm[i] = (int16_t)val;
    }
    return s;
}

static XmInstrumentSpec make_sub_bass() {
    XmInstrumentSpec s = {};
    s.name = "16b Sub Warm Bass";
    s.loopStartSamples = 512;
    s.loopLenSamples = 512;
    s.looped = true;
    s.defaultVol = 58;
    s.defaultPan = 136;
    s.finetune = 0;
    s.relNote = 0;
    s.useVolEnv = true;
    s.volEnvNodes = 3;
    s.volEnvPoints[0][0] = 0;   s.volEnvPoints[0][1] = 64;
    s.volEnvPoints[1][0] = 16;  s.volEnvPoints[1][1] = 54;
    s.volEnvPoints[2][0] = 36;  s.volEnvPoints[2][1] = 0;
    s.volSustainPoint = 1;
    s.fadeout = 1800;

    uint32_t total = s.loopStartSamples + s.loopLenSamples;
    s.pcm.resize(total);
    for (uint32_t i = 0; i < total; i++) {
        double ph = (2.0 * PI * (double)(i % 64)) / 64.0;
        double wave = 0.85 * sin(ph) + 0.40 * sin(2.0 * ph) + 0.15 * sin(3.0 * ph);
        int32_t val = (int32_t)(tanh(wave * 1.2) * 24000.0);
        s.pcm[i] = (int16_t)val;
    }
    return s;
}

static XmInstrumentSpec make_kick_drum() {
    XmInstrumentSpec s = {};
    s.name = "16b Rock Kick";
    s.looped = false;
    s.defaultVol = 64;
    s.defaultPan = 128;
    s.relNote = 0;
    s.useVolEnv = false;

    uint32_t len = 3400;
    s.pcm.resize(len);
    double phase = 0.0;
    uint32_t rng = 0x13579BDFu;
    for (uint32_t i = 0; i < len; i++) {
        double t = (double)i / 16726.0;
        double freq = 46.0 + 135.0 * exp(-t * 38.0) + 220.0 * exp(-t * 180.0);
        phase += (2.0 * PI * freq) / 16726.0;
        double env = exp(-t * 11.5);
        double body = sin(phase) + 0.25 * sin(2.0 * phase);
        rng = rng * 1664525u + 1013904223u;
        double click = ((double)(int32_t)rng / 2147483648.0) * exp(-t * 240.0) * 0.6;
        double out = tanh((body * env + click) * 1.55);
        s.pcm[i] = (int16_t)(out * 30000.0);
    }
    return s;
}

static XmInstrumentSpec make_snare_drum() {
    XmInstrumentSpec s = {};
    s.name = "16b Rock Snare";
    s.looped = false;
    s.defaultVol = 64;
    s.defaultPan = 128;
    s.relNote = 0;
    s.useVolEnv = false;

    uint32_t len = 4200;
    s.pcm.resize(len);
    double ph1 = 0.0, ph2 = 0.0;
    uint32_t rng = 0x2468ACE1u;
    double prevNoise = 0.0;
    for (uint32_t i = 0; i < len; i++) {
        double t = (double)i / 16726.0;
        double f1 = 125.0 + 95.0 * exp(-t * 55.0);
        double f2 = 215.0 + 110.0 * exp(-t * 65.0);
        ph1 += (2.0 * PI * f1) / 16726.0;
        ph2 += (2.0 * PI * f2) / 16726.0;
        double body = (0.65 * sin(ph1) + 0.35 * sin(ph2)) * exp(-t * 22.0);

        rng = rng * 1664525u + 1013904223u;
        double white = (double)(int32_t)rng / 2147483648.0;
        double wires = (white - 0.35 * prevNoise) * exp(-t * 12.5);
        prevNoise = white;

        double out = tanh((body * 0.85 + wires * 0.90) * 1.45);
        s.pcm[i] = (int16_t)(out * 29000.0);
    }
    return s;
}

static XmInstrumentSpec make_closed_hat() {
    XmInstrumentSpec s = {};
    s.name = "16b Closed Hi-Hat";
    s.looped = false;
    s.defaultVol = 52;
    s.defaultPan = 148;
    s.relNote = 0;
    s.useVolEnv = false;

    uint32_t len = 1300;
    s.pcm.resize(len);
    uint32_t rng = 0x9E3779B9u;
    double prev = 0.0;
    for (uint32_t i = 0; i < len; i++) {
        double t = (double)i / 16726.0;
        rng = rng * 1664525u + 1013904223u;
        double n = (double)(int32_t)rng / 2147483648.0;
        double hp = n - prev;
        prev = n;
        double metal = 0.25 * (sin(2.0 * PI * 3410.0 * t) + sin(2.0 * PI * 4870.0 * t) + sin(2.0 * PI * 6290.0 * t));
        double env = exp(-t * 42.0);
        double out = (0.75 * hp + 0.25 * metal) * env;
        int32_t v = (int32_t)(out * 22000.0);
        if (v > 32760) v = 32760; else if (v < -32760) v = -32760;
        s.pcm[i] = (int16_t)v;
    }
    return s;
}

static XmInstrumentSpec make_open_hat() {
    XmInstrumentSpec s = {};
    s.name = "16b Open Hi-Hat";
    s.looped = false;
    s.defaultVol = 50;
    s.defaultPan = 148;
    s.relNote = 0;
    s.useVolEnv = false;

    uint32_t len = 4800;
    s.pcm.resize(len);
    uint32_t rng = 0xCAFEBABEu;
    double prev = 0.0;
    for (uint32_t i = 0; i < len; i++) {
        double t = (double)i / 16726.0;
        rng = rng * 1664525u + 1013904223u;
        double n = (double)(int32_t)rng / 2147483648.0;
        double hp = n - prev;
        prev = n;
        double metal = 0.28 * (sin(2.0 * PI * 3240.0 * t) + sin(2.0 * PI * 4620.0 * t) + sin(2.0 * PI * 5980.0 * t));
        double env = exp(-t * 9.5);
        double out = (0.68 * hp + 0.32 * metal) * env;
        int32_t v = (int32_t)(out * 20000.0);
        if (v > 32760) v = 32760; else if (v < -32760) v = -32760;
        s.pcm[i] = (int16_t)v;
    }
    return s;
}

static XmInstrumentSpec make_crash_cymbal() {
    XmInstrumentSpec s = {};
    s.name = "16b Crash Cymbal";
    s.looped = false;
    s.defaultVol = 52;
    s.defaultPan = 104;
    s.relNote = 0;
    s.useVolEnv = false;

    uint32_t len = 11000;
    s.pcm.resize(len);
    uint32_t rng = 0xDEADBEEFu;
    double prev = 0.0;
    for (uint32_t i = 0; i < len; i++) {
        double t = (double)i / 16726.0;
        rng = rng * 1664525u + 1013904223u;
        double n = (double)(int32_t)rng / 2147483648.0;
        double hp = n - 0.65 * prev;
        prev = n;
        double metal = 0.30 * (sin(2.0 * PI * 2780.0 * t) + sin(2.0 * PI * 3950.0 * t) + sin(2.0 * PI * 5410.0 * t));
        double env = exp(-t * 3.8);
        double out = (0.65 * hp + 0.35 * metal) * env;
        int32_t v = (int32_t)(out * 21000.0);
        if (v > 32760) v = 32760; else if (v < -32760) v = -32760;
        s.pcm[i] = (int16_t)v;
    }
    return s;
}

static void emit_xm_cell(std::vector<uint8_t> &pat, uint8_t note, uint8_t inst, uint8_t vol, uint8_t eff, uint8_t effVal) {
    uint8_t mask = 0x80;
    if (note != 0)   mask |= 1;
    if (inst != 0)   mask |= 2;
    if (vol != 0)    mask |= 4;
    if (eff != 0)    mask |= 8;
    if (effVal != 0) mask |= 16;

    pat.push_back(mask);
    if (mask & 1)  pat.push_back(note);
    if (mask & 2)  pat.push_back(inst);
    if (mask & 4)  pat.push_back(vol);
    if (mask & 8)  pat.push_back(eff);
    if (mask & 16) pat.push_back(effVal);
}

int main() {
    std::vector<uint8_t> xm;

    const uint16_t numChannels = 10;
    const uint16_t numPatterns = BABYLON_NUM_PATTERNS; // 23 patterns of 64 rows = 92 measures
    const uint16_t numInstruments = 9;
    const uint16_t defaultSpeed = 6; // 6 ticks/row
    const uint16_t defaultBpm = 100; // 100 BPM (exact Stalker Goes to Babylon groove)

    // 1. XM Header (60 bytes + 276 bytes = 336 bytes)
    write_padded_str(xm, "Extended Module: ", 17);
    write_padded_str(xm, "Stalker Goes Babylon", 20);
    write_u8(xm, 0x1A);
    write_padded_str(xm, "FastTracker v2.00   ", 20);
    write_u16(xm, 0x0104); // Version 1.04
    write_u32(xm, 276);    // Header size from this offset
    write_u16(xm, numPatterns); // Song length in orders = 23
    write_u16(xm, 0);      // Restart position = 0 (loops full song)
    write_u16(xm, numChannels);
    write_u16(xm, numPatterns);
    write_u16(xm, numInstruments);
    write_u16(xm, 1);      // Flags: 1 = Linear frequency table
    write_u16(xm, defaultSpeed);
    write_u16(xm, defaultBpm);

    // Pattern order table (256 bytes)
    for (int i = 0; i < 256; i++) {
        write_u8(xm, (i < numPatterns) ? (uint8_t)i : 0);
    }

    // 2. Write 23 Patterns (64 rows each, 10 channels)
    for (int p = 0; p < numPatterns; p++) {
        std::vector<uint8_t> patData;
        for (int r = 0; r < 64; r++) {
            int globalRow = p * 64 + r;
            const BabylonRow &row = g_babylon_rows[globalRow];

            // Channels 0, 1, 2: 16-bit Overdrive Guitar (Instrument 1)
            for (int c = 0; c < 3; c++) {
                uint8_t n = row.g_note[c];
                uint8_t inst = (n >= 1 && n <= 96) ? 1 : 0;
                uint8_t vol = 0;
                if (inst != 0) {
                    vol = row.g_dead ? 0x30 : (c == 0 ? 0x50 : 0x46);
                }
                emit_xm_cell(patData, n, inst, vol, 0, 0);
            }

            // Channel 3: 16-bit FM Lead Double (Instrument 2) on top guitar melody
            {
                uint8_t n = row.g_note[0];
                uint8_t inst = (n >= 1 && n <= 96) ? 2 : 0;
                uint8_t vol = (inst != 0) ? (row.g_dead ? 0x24 : 0x3E) : 0;
                emit_xm_cell(patData, n, inst, vol, 0, 0);
            }

            // Channel 4: 16-bit Picked Rock Bass (Instrument 3), shifted +12 semitones for punchy clarity
            {
                uint8_t n = row.b_note;
                uint8_t playNote = (n >= 1 && n <= 84) ? (uint8_t)(n + 12) : n;
                uint8_t inst = (n >= 1 && n <= 96) ? 3 : 0;
                uint8_t vol = (inst != 0) ? 0x50 : 0;
                emit_xm_cell(patData, playNote, inst, vol, 0, 0);
            }

            // Channel 5: 16-bit Sub/Warm Bass (Instrument 4)
            {
                uint8_t n = row.b_note;
                uint8_t playNote = (n >= 1 && n <= 84) ? (uint8_t)(n + 12) : n;
                uint8_t inst = (n >= 1 && n <= 96) ? 4 : 0;
                uint8_t vol = (inst != 0) ? 0x48 : 0;
                emit_xm_cell(patData, playNote, inst, vol, 0, 0);
            }

            // Channel 6: Kick Drum (Instrument 5, note 49 = C-4)
            if (row.kick) {
                emit_xm_cell(patData, 49, 5, 0x50, 0, 0);
            } else {
                emit_xm_cell(patData, 0, 0, 0, 0, 0);
            }

            // Channel 7: Snare Drum (Instrument 6, note 49 = C-4)
            if (row.snare) {
                emit_xm_cell(patData, 49, 6, 0x50, 0, 0);
            } else {
                emit_xm_cell(patData, 0, 0, 0, 0, 0);
            }

            // Channel 8: Hi-Hats (Instrument 7 = Closed Hat, Instrument 8 = Open Hat)
            // Sharing Channel 8 makes Closed Hat naturally choke Open Hat!
            if (row.hat == 1) {
                emit_xm_cell(patData, 49, 7, 0x44, 0, 0);
            } else if (row.hat == 2) {
                emit_xm_cell(patData, 49, 8, 0x42, 0, 0);
            } else {
                emit_xm_cell(patData, 0, 0, 0, 0, 0);
            }

            // Channel 9: Crash / Ride Cymbal (Instrument 9 = Crash, Instrument 8 @ D#4 = Ride)
            if (row.cym == 1) {
                emit_xm_cell(patData, 49, 9, 0x48, 0, 0);
            } else if (row.cym == 2) {
                emit_xm_cell(patData, 52, 8, 0x44, 0, 0);
            } else {
                emit_xm_cell(patData, 0, 0, 0, 0, 0);
            }
        }

        // Write 9-byte pattern header + packed pattern data
        write_u32(xm, 9); // Pattern header length = 9
        write_u8(xm, 0);  // Packing type = 0
        write_u16(xm, 64); // Number of rows = 64
        write_u16(xm, (uint16_t)patData.size());
        xm.insert(xm.end(), patData.begin(), patData.end());
    }

    // 3. Write the 9 16-bit Instruments
    append_instrument(xm, make_overdrive_guitar());
    append_instrument(xm, make_fm_lead_double());
    append_instrument(xm, make_picked_bass());
    append_instrument(xm, make_sub_bass());
    append_instrument(xm, make_kick_drum());
    append_instrument(xm, make_snare_drum());
    append_instrument(xm, make_closed_hat());
    append_instrument(xm, make_open_hat());
    append_instrument(xm, make_crash_cymbal());

    const char *outPath = "C:/Users/patri/.gemini/antigravity/scratch/firestarter/data/stalker_babylon.xm";
    FILE *fp = fopen(outPath, "wb");
    if (!fp) {
        printf("Failed to open %s for writing\n", outPath);
        return 1;
    }
    fwrite(&xm[0], 1, xm.size(), fp);
    fclose(fp);
    printf("Successfully generated %s (%zu bytes)\n", outPath, xm.size());
    return 0;
}

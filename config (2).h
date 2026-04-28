#pragma once

/*
 * config.h — User-editable configuration for the RPM Meter project.
 *
 * Change values here rather than editing the main sketch.
 * After editing, re-compile and upload.
 */

// ─── Encoder ─────────────────────────────────────────────────────────────────

// GPIO pin connected to encoder CLK (Channel A). Must be interrupt-capable.
// On ESP32 all GPIO pins support interrupts EXCEPT: 6,7,8,9,10,11
#ifndef ENCODER_CLK
  #define ENCODER_CLK   34
#endif

// GPIO pin connected to encoder DT (Channel B).
#ifndef ENCODER_DT
  #define ENCODER_DT    35
#endif

// Pulses per revolution of your encoder.
// Check the datasheet of your encoder module.
//   Generic IR slot disc (20 holes)   : 20
//   Bourns PEC11R                     : 24
//   Generic optical (100 slot disc)   : 100
//   Incremental encoder, 400 CPR      : 400
#ifndef PPR
  #define PPR  20
#endif

// ─── Sampling ────────────────────────────────────────────────────────────────

// Measurement window in milliseconds.
// Shorter → faster response, noisier reading.
// Longer  → smoother reading, slower to react.
#ifndef SAMPLE_MS
  #define SAMPLE_MS  200
#endif

// Low-pass filter coefficient applied after sampling (0.0 – 1.0).
// 1.0 = no filtering (raw), 0.1 = heavy smoothing.
#ifndef ALPHA
  #define ALPHA  0.3f
#endif

// ─── Display ─────────────────────────────────────────────────────────────────

#ifndef OLED_WIDTH
  #define OLED_WIDTH   128
#endif

#ifndef OLED_HEIGHT
  #define OLED_HEIGHT  64
#endif

// I2C address — 0x3C on most modules, 0x3D on some.
#ifndef OLED_ADDRESS
  #define OLED_ADDRESS  0x3C
#endif

// I2C pins (ESP32 default)
#ifndef I2C_SDA
  #define I2C_SDA  21
#endif

#ifndef I2C_SCL
  #define I2C_SCL  22
#endif

// Maximum RPM shown on the bar gauge (does not clamp actual measurement).
#ifndef RPM_MAX
  #define RPM_MAX  9999
#endif

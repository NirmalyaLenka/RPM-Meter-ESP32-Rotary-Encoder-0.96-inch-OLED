# RPM Meter — ESP32 + Rotary Encoder + 0.96-inch OLED

A straightforward RPM measurement system built on the ESP32. A rotary encoder disc mounted on a rotating shaft generates pulses, the microcontroller counts them over a fixed time window, and the computed RPM value is shown on a small OLED screen in real time. Direction of rotation is also detected and displayed.

This project is deliberately kept simple so it is easy to adapt. All the parameters you are likely to need to change — encoder resolution, sample rate, pin assignments — live in a single header file.


## What you need

Hardware

- ESP32 development board (any 38-pin or 30-pin variant works)
- Rotary encoder module (KY-040, or a bare encoder disc with IR sensor) <img width="254" height="254" alt="image" src="https://github.com/user-attachments/assets/c5be11cf-42d1-4a18-b2eb-64c064b74582" />

- 0.96-inch OLED display, SSD1306 controller, I2C interface <img width="240" height="240" alt="image" src="https://github.com/user-attachments/assets/017366d4-8f3e-4140-93d1-3aadd69ab63c" />

- Breadboard and jumper wires

Software

- Arduino IDE 1.8.x or 2.x, or PlatformIO
- Board package: "esp32 by Espressif Systems" (install via Boards Manager)
- Libraries (install via Library Manager):
  - Adafruit SSD1306
  - Adafruit GFX Library


## Wiring

A plain text wiring diagram is in WIRING.txt. The short version:

| Encoder pin | ESP32 pin |
|-------------|-----------|
| CLK (A)     | GPIO 34   |
| DT (B)      | GPIO 35   |
| VCC         | 3.3V      |
| GND         | GND       |

| OLED pin | ESP32 pin |
|----------|-----------|
| SDA      | GPIO 21   |
| SCL      | GPIO 22   |
| VCC      | 3.3V      |
| GND      | GND       |

Do not connect either device to 5V. Both are 3.3V logic.


## Configuration

Open config.h before compiling. The two values you almost certainly need to change are:

PPR (pulses per revolution) — look up the spec of your encoder. The cheap 20-slot IR disc modules are PPR 20. Quality incremental encoders are often 100, 200, or 600.

SAMPLE_MS — how long the counter runs before an RPM calculation happens. 200 ms gives a reading five times per second, which feels responsive for most applications. If the shaft is very slow, increase this to 500 ms or 1000 ms so enough pulses accumulate for an accurate count.

Everything else — pin numbers, I2C address, smoothing coefficient — can also be changed in that file without touching the main sketch.


## How the measurement works

The encoder CLK pin is attached to a hardware interrupt. Every rising edge increments a counter in IRAM (fast memory). At the end of each sample window, the main loop reads and resets that counter atomically (interrupts briefly disabled), then applies:

    RPM = (count / PPR) * (60000 / SAMPLE_MS)

The result passes through an exponential moving average filter to suppress noise without adding significant lag. The smoothing factor ALPHA defaults to 0.3, which means 30 percent of the new raw reading is blended with 70 percent of the previous filtered value.

The OLED shows the RPM as a large three-digit number, a small direction indicator (CW or CCW), a running total of revolutions, and a bar gauge scaled to RPM_MAX.


## Serial output

Connect a USB cable and open the Serial Monitor at 115200 baud. Each sample window prints one line:

    RPM: 1420  Dir: CW  Revs: 48


## Troubleshooting

The OLED shows nothing
- Check that SDA and SCL are not swapped.
- Try changing OLED_ADDRESS to 0x3D in config.h.
- Some SSD1306 clones require a 10k pull-up resistor on SDA and SCL to 3.3V.

The RPM reads zero when the shaft is clearly spinning
- Confirm ENCODER_CLK is wired to a pin that supports interrupts. On the ESP32, GPIO 6-11 do not work for this.
- Open the Serial Monitor and slowly rotate the encoder by hand. You should see RPM values appear. If not, the interrupt is not firing — try swapping CLK and DT wires.

The reading is noisy or fluctuates wildly
- Lower ALPHA in config.h for heavier smoothing.
- Increase SAMPLE_MS to collect more pulses per window.
- Add a small ceramic capacitor (100 nF) across VCC and GND on the encoder module to suppress electrical noise.

The RPM value is consistently wrong by a fixed ratio
- The PPR value in config.h does not match your encoder. Count the slots in the disc or check the datasheet.


## Project structure

    rpm_meter_esp32/
    |-- rpm_encoder_oled.ino   Main sketch
    |-- config.h               All user-editable parameters
    |-- WIRING.txt             Plain-text wiring reference
    |-- README.md              This file
    |-- docs/
        |-- index.html         Interactive component guide (open in browser)


## License

MIT License. Use it, modify it, build products with it — attribution is appreciated but not required.


## Acknowledgements

Built with the Adafruit SSD1306 and Adafruit GFX libraries, which take the heavy lifting out of driving small displays. The ESP32 interrupt system makes high-frequency pulse counting trivial without needing external counter ICs.

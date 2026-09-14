# Kick Sensor

An Arduino-based wearable that detects a kick using an MPU6050 accelerometer/gyroscope, estimates kick speed, and logs the data to an SD card.

## Hardware

- Arduino Uno
- MPU6050 (accelerometer + gyroscope)
- SD card 

## Wiring

| Module | Uno Pin |
|---|---|
| MPU6050 SDA | A4 |
| MPU6050 SCL | A5 |
| SD card CS | D10 |
| SD card MOSI (DI) | D11 |
| SD card MISO (DO) | D12 |
| SD card SCK | D13 |


MPU6050 and SD module both share the Uno's 5V and GND rails.

## Libraries required

Install via Arduino Library Manager:
- Adafruit MPU6050
- Adafruit Unified Sensor
- Adafruit BusIO
- SD (built-in)

## How it works

1. On boot, the sketch initializes I2C, the MPU6050, and the SD card. If either fails, it prints an error over Serial and halts.
2. Once you see `Ready. Start kicking.` in Serial Monitor, the sketch continuously reads acceleration magnitude in `loop()`.
3. When acceleration crosses `TRIGGER_G` (default 3.0g), it captures a burst of `BUFFER_SIZE` samples, computes peak acceleration/gyro values and an estimated kick speed, then writes the burst to a new `kickN.csv` file on the SD card.

## Key constants (top of sketch)

- `LEG_LENGTH_M` — knee-to-sensor distance, used in the speed estimate. Measure and set this for your own leg.
- `TRIGGER_G` — acceleration threshold (in g) that counts as a kick.
- `BUFFER_SIZE` — number of samples captured per kick event.

## Usage

1. Upload the sketch, open Serial Monitor at 115200 baud.
2. Wait for `Ready. Start kicking.`
3. Kick. Each detected kick prints a summary line and writes a `kickN.csv` file to the SD card.
4. Pull the SD card afterward to review the logged kick data.

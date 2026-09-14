/*

*/

#include <Wire.h>
#include <Adafruit_MPU6050.h>
#include <Adafruit_Sensor.h>
#include <SPI.h>
#include <SD.h>

Adafruit_MPU6050 mpu;

const float LEG_LENGTH_M = 0.40;   // measure YOUR knee-to-sensor distance in meters
const float TRIGGER_G    = 3.0;
const int   BUFFER_SIZE  = 10;                 // total samples held at once (lowered from 20 - see header)
const int   POST_TARGET  = BUFFER_SIZE / 2;    // samples to keep collecting AFTER a trigger
const int   SD_CS_PIN    = 10;
const int   BUZZER_PIN   = 8;

float axBuf[BUFFER_SIZE], ayBuf[BUFFER_SIZE], azBuf[BUFFER_SIZE];
float gxBuf[BUFFER_SIZE], gyBuf[BUFFER_SIZE], gzBuf[BUFFER_SIZE];
unsigned long tBuf[BUFFER_SIZE];

int bufIndex = 0;
int kickFileNum = 0;
bool triggered = false;
int postCount = 0;
float peakAccelG = 0;
float peakGyroDegS = 0;

void setup() {
  Serial.begin(115200);
  delay(500); // give the Serial connection a moment to fully open
  Serial.println(F("[1] Serial started"));

  Wire.begin();
  Wire.setClock(400000);
  Serial.println(F("[2] Wire (I2C) started - about to try mpu.begin()..."));

  if (!mpu.begin()) {
    Serial.println(F("[3] MPU6050 not found - check wiring!"));
    while (1) delay(10);
  }
  Serial.println(F("[3] MPU6050 found OK"));

  mpu.setAccelerometerRange(MPU6050_RANGE_8_G);
  mpu.setGyroRange(MPU6050_RANGE_500_DEG);
  mpu.setFilterBandwidth(MPU6050_BAND_260_HZ);
  Serial.println(F("[4] MPU6050 configured - about to try SD.begin()..."));

  if (!SD.begin(SD_CS_PIN)) {
    Serial.println(F("[5] SD card not found - check wiring/card!"));
    while (1) delay(10);
  }
  Serial.println(F("[5] SD card found OK"));

  pinMode(BUZZER_PIN, OUTPUT);
  Serial.println(F("[6] Ready. Start kicking."));
}

void loop() {
  sensors_event_t a, g, temp;
  mpu.getEvent(&a, &g, &temp);

  float ax = a.acceleration.x / 9.81;
  float ay = a.acceleration.y / 9.81;
  float az = a.acceleration.z / 9.81;
  float accelMag = sqrt(ax*ax + ay*ay + az*az);
  float gyroMag = sqrt(sq(g.gyro.x) + sq(g.gyro.y) + sq(g.gyro.z));
  float gyroDegS = gyroMag * 57.2958;

  // always keep writing into the single ring buffer
  axBuf[bufIndex] = ax; ayBuf[bufIndex] = ay; azBuf[bufIndex] = az;
  gxBuf[bufIndex] = g.gyro.x; gyBuf[bufIndex] = g.gyro.y; gzBuf[bufIndex] = g.gyro.z;
  tBuf[bufIndex] = micros();
  bufIndex = (bufIndex + 1) % BUFFER_SIZE;

  if (!triggered && accelMag > TRIGGER_G) {
    // kick just started - begin the post-trigger countdown
    triggered = true;
    postCount = 0;
    peakAccelG = accelMag;
    peakGyroDegS = gyroDegS;
  } else if (triggered) {
    if (accelMag > peakAccelG) peakAccelG = accelMag;
    if (gyroDegS > peakGyroDegS) peakGyroDegS = gyroDegS;
    postCount++;
    if (postCount >= POST_TARGET) {
      saveKick();
      triggered = false; // ready to detect the next kick
    }
  }
}

void saveKick() {
  float estSpeedMS = (peakGyroDegS * 0.0174533) * LEG_LENGTH_M;
  float estSpeedKMH = estSpeedMS * 3.6;

  char fname[16];
  snprintf(fname, sizeof(fname), "kick%d.csv", kickFileNum++);
  File f = SD.open(fname, FILE_WRITE);
  if (f) {
    f.println(F("time_us,ax_g,ay_g,az_g,gx_rads,gy_rads,gz_rads"));
    for (int i = 0; i < BUFFER_SIZE; i++) {
      int idx = (bufIndex + i) % BUFFER_SIZE; // read out oldest-to-newest
      f.print(tBuf[idx]); f.print(",");
      f.print(axBuf[idx]); f.print(","); f.print(ayBuf[idx]); f.print(","); f.print(azBuf[idx]); f.print(",");
      f.print(gxBuf[idx]); f.print(","); f.print(gyBuf[idx]); f.print(","); f.println(gzBuf[idx]);
    }
    f.close();
  }

  Serial.print(F("Kick #")); Serial.print(kickFileNum);
  Serial.print(F(" | Peak accel: ")); Serial.print(peakAccelG); Serial.print(F(" g"));
  Serial.print(F(" | Est. speed: ")); Serial.print(estSpeedKMH); Serial.println(F(" km/h"));

  tone(BUZZER_PIN, 2000, 150);
  delay(300);
}

#include <Arduino_BMI270_BMM150.h>
#include <Arduino_APDS9960.h>
#include <Arduino_HS300x.h>

#define HUMIDSAMPLES 5
#define TEMPSAMPLES 5
#define MAGSAMPLES 10
#define LIGHTSAMPLES 10

// Thresholds (tune these)
#define HUMID_JUMP_THR 2.0
#define TEMP_RISE_THR  0.5
#define MAG_SHIFT_THR  5.0
#define LIGHT_CHANGE_THR 15.0

unsigned long lastEventTime = 0;
// Humidity
float humidBuffer[HUMIDSAMPLES];
int humidIndex = 0;
bool humidFilled = false;

// Temperature
float tempBuffer[TEMPSAMPLES];
int tempIndex = 0;
bool tempFilled = false;

// Magnetic field
float magBuffer[MAGSAMPLES];
int magIndex = 0;
bool magFilled = false;

// Light
int lightBuffer[LIGHTSAMPLES];
int lightIndex = 0;
bool lightFilled = false;

void setup() {
  Serial.begin(115200);
  delay(1500);

  HS300x.begin();

  if (!APDS.begin()) {
    Serial.println("Failed to initialize APDS9960 sensor.");
    while (1);
  }

  if (!IMU.begin()) {
    Serial.println("Failed to initialize IMU.");
    while (1);
  }
}

void loop() {

  int r, g, b = 0;
  static int clear;
  float temperature = HS300x.readTemperature();
  float humidity = HS300x.readHumidity();

  humidBuffer[humidIndex++] = humidity;
  if (humidIndex >= HUMIDSAMPLES) {
    humidIndex = 0;
    humidFilled = true;
  }

  float humidAvg = 0;
  if (humidFilled) {
    for (int i = 0; i < HUMIDSAMPLES; i++) {
      humidAvg += humidBuffer[i];
    }
    humidAvg /= HUMIDSAMPLES;
  }

  tempBuffer[tempIndex++] = temperature;
  if (tempIndex >= TEMPSAMPLES) {
    tempIndex = 0;
    tempFilled = true;
  }

  float tempAvg = 0;
  if (tempFilled) {
    for (int i = 0; i < TEMPSAMPLES; i++) {
      tempAvg += tempBuffer[i];
    }
    tempAvg /= TEMPSAMPLES;
  }

  float mx, my, mz;

  if (IMU.magneticFieldAvailable()) {
    IMU.readMagneticField(mx, my, mz);
    float mag = abs(mx) + abs(my) + abs(mz);

    magBuffer[magIndex++] = mag;

    if (magIndex >= MAGSAMPLES) {
      magIndex = 0;
      magFilled = true;
    }
  }

  float magAvg = 0;
  if (magFilled) {
    for (int i = 0; i < MAGSAMPLES; i++) {
      magAvg += magBuffer[i];
    }
    magAvg /= MAGSAMPLES;
  }

  if (APDS.colorAvailable()) {
    APDS.readColor(r, g, b, clear);

    lightBuffer[lightIndex++] = clear;

    if (lightIndex >= LIGHTSAMPLES) {
      lightIndex = 0;
      lightFilled = true;
    }
  }

  float lightAvg = 0;
  if (lightFilled) {
    for (int i = 0; i < LIGHTSAMPLES; i++) {
      lightAvg += lightBuffer[i];
    }
    lightAvg /= LIGHTSAMPLES;
  }

  static float lastHumid = 0;
  static float lastTemp = 0;
  static float lastMag = 0;
  static float lastLight = 0;

  float humidDelta = abs(humidAvg - lastHumid);
  float tempDelta  = tempAvg - lastTemp;
  float magDelta   = abs(magAvg - lastMag);
  float lightDelta = abs(lightAvg - lastLight);

  lastHumid = humidAvg;
  lastTemp  = tempAvg;
  lastMag   = magAvg;
  lastLight = lightAvg;

  int humid_jump = (humidDelta > HUMID_JUMP_THR);
  int temp_rise  = (tempDelta > TEMP_RISE_THR);
  int mag_shift  = (magDelta > MAG_SHIFT_THR);
  int light_change = (lightDelta > LIGHT_CHANGE_THR);

  String event = "BASELINE_NORMAL";

  unsigned long now = millis();
  bool inCooldown = (now - lastEventTime < 2000);

  if (!inCooldown) {
    if (mag_shift) {
      event =  "MAGNETIC_DISTURBANCE_EVENT";
    }
    else if (light_change) {
      event = "LIGHT_OR_COLOR_CHANGE_EVENT";
    }
    else if (temp_rise) {
      event = "BREATH_OR_WARM_AIR_EVENT";
    }
    else if (humid_jump) {
      event = "BREATH_OR_WARM_AIR_EVENT";
    }

    // If any event triggered → start cooldown
    if (event != "none") {
      lastEventTime = now;
    }
  }
  // ================== REQUIRED OUTPUT ==================

  // 1. RAW
  Serial.print("raw,");
  Serial.print("rh=");
  Serial.print(humidAvg);
  Serial.print(",temp=");
  Serial.print(tempAvg);
  Serial.print(",mag=");
  Serial.print(magAvg);
  Serial.print(",r=");
  Serial.print(r);
  Serial.print(",g=");
  Serial.print(g);
  Serial.print(",b=");
  Serial.print(b);
  Serial.print(",clear=");
  Serial.println(clear);

  // 2. FLAGS
  Serial.print("flags,");
  Serial.print("humid_jump=");
  Serial.print(humid_jump);
  Serial.print(",temp_rise=");
  Serial.print(temp_rise);
  Serial.print(",mag_shift=");
  Serial.print(mag_shift);
  Serial.print(",light_or_color_change=");
  Serial.println(light_change);

  // 3. EVENT
  Serial.print("event,");
  Serial.println(event);

  delay(200);
}
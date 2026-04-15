#include <Arduino_BMI270_BMM150.h>
#include <Arduino_APDS9960.h>
#include <PDM.h>

#define MOTIONSAMPLES 5
#define LOUDLVL 122
#define DARKLVL 5

short sampleBuffer[256];

volatile int samplesRead = 0;

float motionBuffer[MOTIONSAMPLES];
int motionIndex = 0;
bool motionFilled = false;



void onPDMdata() {
  int bytesAvailable = PDM.available();
  PDM.read(sampleBuffer, bytesAvailable);
  samplesRead = bytesAvailable / 2;
}

void setup() {
  Serial.begin(115200);
  delay(1500);

  if (!APDS.begin()) {
    Serial.println("Failed to initialize APDS9960 sensor.");
    while (1);
  }
  if (!IMU.begin()) {
    Serial.println("Failed to initialize IMU.");
    while (1);
  }

  PDM.onReceive(onPDMdata);
  if (!PDM.begin(1, 16000)){
    Serial.println("failed to start microphone");
    while(1);
  }
}

void loop() {
  int r, g, b, clear, mic;
  float ax, ay, az;
  static int prox;

  // Flags
  //bool sound, dark, moving, near;

  // Mic
  if (samplesRead) {
    long sum = 0;
    for (int i = 0; i < samplesRead; i++) {
      sum += abs(sampleBuffer[i]);
    }
    mic = sum / samplesRead;
    samplesRead = 0;
  }

  bool sound = (mic > LOUDLVL);
  float motion = 0;
  // Accel
  if (IMU.accelerationAvailable()) {
    IMU.readAcceleration(ax, ay, az);

    motion = abs(ax) + abs(ay) + abs(az);

    motionBuffer[motionIndex++] = motion;

    if (motionIndex >= MOTIONSAMPLES) {
      motionIndex = 0;
      motionFilled = true;
    }
  }

  float motionAvg = 0;

  if (motionFilled) {
    for (int i = 0; i < MOTIONSAMPLES; i++) {
      motionAvg += motionBuffer[i];
    }
    motionAvg /= MOTIONSAMPLES;
  }
  

  bool moving = (abs(motion - motionAvg) > 0.05);
  
  // Light
  if (APDS.colorAvailable()) {
    APDS.readColor(r, g, b, clear);
  }

  bool dark = (clear < DARKLVL);

   

  // Proximity
  Serial.println(APDS.proximityAvailable());
  if (APDS.proximityAvailable()) {
    prox = APDS.readProximity();
    
  }

  bool near = (prox < 122);

  Serial.print("raw,");
  Serial.print("mic=");
  Serial.print(mic);
  Serial.print(",clear=");
  Serial.print(clear);
  Serial.print(",motion=");
  Serial.print(motion);
  Serial.print(",prox=");
  Serial.println(prox);

  Serial.print("flags,");
  Serial.print("sound=");
  Serial.print(sound);
  Serial.print(",dark=");
  Serial.print(dark);
  Serial.print(",moving=");
  Serial.print(moving);
  Serial.print(",near=");
  Serial.println(near);

  String state = "";
  if (sound) {
    state += "NOISY";
  } else {
    state += "QUIET";
  }
  if (dark) {
    state += "_DARK";
  } else {
    state += "_BRIGHT";
  }
  if (moving) {
    state += "_MOVING";
  } else {
    state += "_STEADY";
  }
  if (near) {
    state += "_NEAR";
  } else {
    state += "_FAR";
  }
  Serial.print("state,");
  Serial.println(state);


  delay(2000);
}
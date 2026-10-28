
/*
  BalanceX V3.1 (V2 Fixed & Optimized + Obstacle Guard + WiFi Control Deck
                + smooth-balance / drive retune)
  ESP32 DevKit / ESP32-WROOM-32 (Arduino-ESP32 core 3.x)

  Required libraries:
    Adafruit MPU6050
    Adafruit Unified Sensor
    Adafruit BusIO
  (WiFi + web server use only the libraries built into the ESP32 core.)

  Preserved from your working sketch:
    IMU axes and signs
    Motor directions
    Encoder directions
    1325.1 encoder counts/revolution
    CHANGE interrupts on encoder A channels
    Standard I2C pull-up configuration & proven PWM deadband/compensation
    Moving position-target outer loop for smooth forward tracking

  Fixes over original V2:
    1. Removed digitalWrite(SDA/SCL, LOW) which disabled internal I2C pull-ups
       and hung the bus when using standard MPU6050 modules.
    2. Removed strict undocumented register readbacks in configureImu() and
       per-tick 200Hz WHO_AM_I register reads in readImu() that caused
       Fault 1 (IMU_ERROR) and Fault 3 (TIMING_ERROR).
    3. Relaxed overly brittle calibration variance/sample-count gates that
       tripped Fault 2 (CAL_MOVED) from normal hand-holding tremor.
    4. Replaced aggressive 400ms encoder stall fault with safe, non-latching
       integral anti-windup so temporary floor friction doesn't kill balancing.
    5. Restored proven motor minimum-PWM mapping (starting at 80 / 100 above
       2.0 deg deadband) so the motors have immediate torque near 0 degrees.

  V3 add-ons:
    1. HC-SR04 ultrasonic obstacle guard + buzzer
         object < 2 m : buzzer ON
         object < 1 m : OBSTACLE LOCKOUT (Fault 10) -> motors disarmed + latched.
                        Auto-arming is also blocked while an object is < 1 m.
         Clear the lockout with 'r' (serial) or RESET on the dashboard.
    2. WiFi access point + BalanceX control deck
         Join WiFi "BalanceX" (password "balancex"), open http://192.168.4.1
         Live tilt, wheel RPM, obstacle distance, STABLE / FORWARD / REVERSE
         modes, E-STOP and RESET. The web server runs on core 0; the balance
         loop keeps core 1 to itself and never waits for the network.

  V3.1 retune (every changed value is marked "V3.1 (was ...)" below):
    1. Less vibration: the motor PWM no longer jumps 0 -> 80/100 on tiny
       corrections (soft entry), and the speed loop is calmer (0.12 deg/RPM,
       100 ms speed filter, ground-speed feedback, slewed lean target). Before,
       normal balancing wheel swings drove the speed loop into its +/-1.5 deg
       limit and back, which rocked the robot.
    2. FORWARD / REVERSE really move: 12 RPM cruise (was 1 RPM), 8 RPM/s ramp.
       Software fix: the drive request used to be cancelled whenever the gyro
       rate exceeded 20 deg/s, i.e. on every vibration. Mode changes now
       re-anchor the position target, so the robot stops where you switch.
    3. Faster start: 3 s calibration (was 5 s), 0.3 s upright hold (was 0.8 s),
       arming tolerates 25 deg/s hand tremor (was 15).
    4. Starts in STABLE (AUTO_FORWARD = false); WiFi TX power 8.5 dBm.
    Balance gains KP / KD, filters, signs, PWM limits and the encoder,
    IMU and pin configuration are unchanged.

  Add-on wiring:
    HC-SR04 VCC -> 5V    GND -> GND    TRIG -> GPIO23
    HC-SR04 ECHO -> GPIO13 through a divider (1k from ECHO, 2k to GND),
                   because ECHO is a 5 V signal.
    Buzzer (+) -> GPIO4, (-) -> GND (active buzzer; drive it through an NPN
                   transistor if it draws more than ~20 mA).

  Startup:
    1. 3-second calibration with motors OFF (hold upright and still).
    2. Automatic arming after holding upright within +/-5 deg for 0.3 s.
    3. Balances in place (STABLE) until FORWARD / REVERSE is selected.

  Serial commands (115200 baud):
    s / x : motors OFF, latched stop (Fault 9: USER_STOP)
    h     : hold position (0 RPM target), continue balancing   (STABLE)
    f     : enable forward movement (12 RPM target)             (FORWARD)
    b     : enable reverse movement (same speed, backwards)     (REVERSE)
    r     : recalibrate and restart while disarmed/stopped

  ESP32 hardware timer is reserved for the same 20 ms motor deadline guard.
  On a watchdog trip, ENA and ENB are disconnected from LEDC and driven LOW.
  This ESP32 port uses the previously documented BalanceX ESP32 GPIO map.
  Balance gains, filters, fall limit and commands are preserved from the
  working Uno sketch; V3.1 changes are marked where they occur.
*/

#include <Arduino.h>
#include <esp_arduino_version.h>
#include <Wire.h>
#include <Adafruit_MPU6050.h>
#include <Adafruit_Sensor.h>
#include <freertos/FreeRTOS.h>
#include <freertos/portmacro.h>
#include <freertos/queue.h>
#include <esp_rom_gpio.h>
#include <soc/gpio_reg.h>
#include <soc/gpio_sig_map.h>
#include <soc/soc.h>
#include <WiFi.h>
#include <esp_http_server.h>
#include <math.h>
#include <stdio.h>
#include <string.h>

#include "dashboard.h"

#if !defined(ARDUINO_ARCH_ESP32) || !defined(CONFIG_IDF_TARGET_ESP32)
#error "Select ESP32 Dev Module / classic ESP32-WROOM-32, Arduino-ESP32 core 3.x."
#endif

#if ESP_ARDUINO_VERSION_MAJOR < 3
#error "Update the Espressif esp32 boards package to Arduino-ESP32 core 3.x."
#endif

#ifndef LED_BUILTIN
#define LED_BUILTIN 2
#endif

// ============================================================
// USER SETTINGS
// ============================================================

const bool ENABLE_MOTORS = true;
// Set false for a sensor-only check: motors remain OFF.

const bool AUTO_FORWARD = false; // V3.1 (was true)
// false: start in STABLE (hold position); pick FORWARD / REVERSE on the
// dashboard or with 'f' / 'b'. Set true to drive forward automatically.

const uint8_t MPU_ADDRESS = 0x68;
const uint32_t I2C_CLOCK_HZ = 400000UL; // 400 kHz Fast I2C keeps loop < 1.5 ms

// Existing motor and encoder direction settings preserved.
const int8_t LEFT_MOTOR_SIGN  = +1;
const int8_t RIGHT_MOTOR_SIGN = -1;

const int8_t LEFT_ENCODER_SIGN  = +1;
const int8_t RIGHT_ENCODER_SIGN = -1;

const float LEFT_COUNTS_PER_WHEEL_REV  = 1325.1f;
const float RIGHT_COUNTS_PER_WHEEL_REV = 1325.1f;

// Existing IMU sign settings preserved.
const float ANGLE_SIGN   = +1.0f;
const float GYRO_SIGN    = -1.0f;
const float BALANCE_SIGN = -1.0f;

// +1.0f matches your working second sketch: positive targetRPM -> positive targetAngleDeg.
const float FORWARD_DIRECTION_SIGN = +1.0f;

// Balance PD controller gains preserved.
const float KP = 11.0f;
const float KD = 0.75f;

// Complementary filter and gyro-rate smoothing preserved from working sketch.
const float ANGLE_FILTER = 0.985f;
const float RATE_FILTER  = 0.40f;

// Motor PWM compensation values preserved from working sketch.
const int LEFT_MIN_PWM  = 80;
const int RIGHT_MIN_PWM = 100;
const int MAX_PWM       = 180;

const float OUTPUT_DEADBAND   = 2.0f;
const float OUTPUT_FULL_SCALE = 40.0f;

// V3.1 anti-vibration: the motor PWM no longer jumps straight from 0 to
// 80 / 100 at the deadband edge. It starts at MIN_PWM_ENTRY_FRACTION of the
// minimum PWM and blends into the original curve over MIN_PWM_SOFT_BAND
// output units. Set MIN_PWM_ENTRY_FRACTION = 1.0 for the old hard step.
const float MIN_PWM_ENTRY_FRACTION = 0.70f;
const float MIN_PWM_SOFT_BAND      = 3.0f;

// Outer speed & position loop settings.
const float FORWARD_TARGET_RPM         = 12.0f;  // V3.1 (was 1.0):  ~4 cm/s on 65 mm wheels
const float SPEED_RAMP_RPM_PER_S       = 8.0f;   // V3.1 (was 0.5):  cruise reached in ~1.5 s
const float POSITION_KP_RPM_PER_REV    = 1.0f;
const float SPEED_KP_DEG_PER_RPM       = 0.12f;  // V3.1 (was 0.20): 0.20 kept the loop slammed at
                                                 //   its limit by balancing wheel swings -> rocking
const float SPEED_KI_DEG_PER_RPM_SEC   = 0.02f;  // V3.1 (was 0.0):  learns calibration offset / friction
const float MAX_TARGET_SPEED_RPM       = 20.0f;  // V3.1 (was 5.0):  must exceed the cruise speed
const float MAX_OUTER_TARGET_ANGLE_DEG = 4.0f;   // V3.1 (was 1.5):  braking authority after a push
const float SPEED_FILTER_TAU_S         = 0.100f; // V3.1 (was 0.040): less encoder-quantization noise

// V3.1: the outer loop's lean target is slewed at 200 Hz instead of stepping
// every 40 ms, and the drive gate no longer drops on every vibration spike.
const float TARGET_ANGLE_SLEW_DEG_PER_S = 20.0f;
const float SPEED_TILT_COMPENSATION     = 1.0f;  // 1 = ground speed, 0 = old wheel-only speed
const uint32_t DRIVE_SETTLE_US          = 300000UL; // settled this long before driving
const float DRIVE_SETTLE_ANGLE_DEG      = 4.5f;
const float DRIVE_SETTLE_RATE_DPS       = 45.0f;
const float DRIVE_RELEASE_ANGLE_DEG     = 10.0f;    // only a real upset pauses driving

// Wheel synchronization trim.
const float WHEEL_SYNC_KP_PWM_PER_RPM = 0.20f;
const float MAX_SYNC_TRIM_PWM         = 8.0f;

// ============================================================
// TIMING AND SAFETY LIMITS
// ============================================================

const uint32_t CONTROL_US      = 5000UL;    // 200 Hz inner balance loop
const uint32_t OUTER_US        = 40000UL;   // 25 Hz outer speed/position loop
const uint32_t CALIBRATION_US  = 3000000UL; // V3.1 (was 5 s): 3-second calibration window
const uint32_t UPRIGHT_HOLD_US = 300000UL;  // V3.1 (was 0.8 s): 0.3-second upright hold to auto-arm

const float START_ANGLE_DEG = 5.0f;
const float START_RATE_DPS  = 25.0f;        // V3.1 (was 15): hand tremor no longer resets arming
const float FALL_ANGLE_DEG  = 25.0f;

const float RAD_TO_DEG_F = 57.2957795f;

// ============================================================
// PIN CONNECTIONS (Matches prior ESP32 BalanceX connections)
// ============================================================

const uint8_t LEFT_EN  = 25; // L298N ENA (LEDC PWM)
const uint8_t LEFT_IN1 = 26; // L298N IN1
const uint8_t LEFT_IN2 = 27; // L298N IN2

const uint8_t RIGHT_EN  = 14; // L298N ENB (LEDC PWM)
const uint8_t RIGHT_IN1 = 18; // L298N IN3
const uint8_t RIGHT_IN2 = 19; // L298N IN4

const uint8_t LEFT_ENC_A  = 34; // Interrupt, input-only GPIO
const uint8_t LEFT_ENC_B  = 35; // Input-only GPIO; external pull-up needed
const uint8_t RIGHT_ENC_A = 32; // Interrupt
const uint8_t RIGHT_ENC_B = 33;

const uint8_t MPU_SDA = 21;
const uint8_t MPU_SCL = 22;

// Hardware PWM replacement for Uno analogWrite(D5/D6), ~976.56 Hz, 8 bits.
// These are PWM-hardware settings only, NOT PID or control-loop changes.
const uint32_t MOTOR_PWM_HZ = 977;
const uint8_t MOTOR_PWM_BITS = 8;

// ============================================================
// ADD-ON: OBSTACLE GUARD (HC-SR04 + BUZZER) SETTINGS
// ============================================================

const uint8_t US_TRIG_PIN = 23; // HC-SR04 TRIG (3.3 V output is enough)
const uint8_t US_ECHO_PIN = 13; // HC-SR04 ECHO (through 5 V -> 3.3 V divider)
const uint8_t BUZZER_PIN  = 4;  // Buzzer driver input

const float BUZZER_DISTANCE_CM  = 10.0f; // object < 10cm -> buzzer ON
const float LOCKOUT_DISTANCE_CM = 5.0f; // object < 5cm -> lockout, motors disarmed

// Active buzzer (beeps on DC) = default. Set BUZZER_ACTIVE_HIGH false for
// "low level trigger" buzzer modules. Set BUZZER_IS_PASSIVE true only for a
// passive buzzer (needs a tone); it then gets its own LEDC channel/timer.
const bool     BUZZER_ACTIVE_HIGH  = true;
const bool     BUZZER_IS_PASSIVE   = false;
const uint32_t BUZZER_TONE_HZ      = 2700;
const uint8_t  BUZZER_LEDC_CHANNEL = 4;   // never shares the motor PWM timer

const uint32_t US_PING_PERIOD_US    = 60000UL; // HC-SR04 measurement cycle
const uint32_t US_ECHO_TIMEOUT_US   = 50000UL; // no complete echo by then -> resolve ping
const float    US_MICROS_PER_CM     = 58.3f;   // round-trip time per cm @ 20 C
const float    US_MIN_CM            = 2.0f;
const float    US_MAX_CM            = 400.0f;  // beyond this = path clear
const uint8_t  US_LOCKOUT_CONFIRM   = 2;       // consecutive filtered readings < 1 m
const uint8_t  US_OFFLINE_MISSES    = 8;       // ~0.5 s without echoes -> sensor offline

// Reverse mode uses the same slow speed as forward, in the opposite direction.
const float REVERSE_TARGET_RPM = FORWARD_TARGET_RPM;

// ============================================================
// ADD-ON: WIFI CONTROL DECK SETTINGS
// ============================================================

const char *const WIFI_AP_SSID     = "BalanceX";
const char *const WIFI_AP_PASSWORD = "balancex"; // 8+ characters (WPA2)
const uint8_t     WIFI_AP_CHANNEL     = 6;
const uint8_t     WIFI_AP_MAX_CLIENTS = 4;
const wifi_power_t WIFI_AP_TX_POWER   = WIFI_POWER_8_5dBm; // V3.1 (was 15 dBm): less supply noise, ~10-20 m range

const uint32_t TELEMETRY_PERIOD_MS = 50;    // dashboard snapshot rate (20 Hz)
const uint32_t IDLE_IMU_PERIOD_MS  = 40;    // live tilt while STOPPED (display only)
const float    DISPLAY_RPM_TAU_S   = 0.15f; // dashboard wheel-speed smoothing

// ============================================================
// STATES AND FAULT CODES
// ============================================================

enum RobotState : uint8_t {
  CALIBRATING = 0,
  READY       = 1,
  BALANCING   = 2,
  STOPPED     = 3
};

enum FaultCode : uint8_t {
  NO_FAULT         = 0,
  IMU_ERROR        = 1,
  CAL_MOVED        = 2,
  TIMING_ERROR     = 3,
  FALLEN           = 4,
  EXCESS_RATE      = 5,
  OVERSPEED        = 6,
  ENCODER_OR_STALL = 7,
  MOTOR_DEADLINE   = 8,
  USER_STOP        = 9,
  OBSTACLE_LOCKOUT = 10
};

enum DriveMode : uint8_t {
  MODE_STABLE  = 0, // hold position (0 RPM target)
  MODE_FORWARD = 1, // slow forward while balancing
  MODE_REVERSE = 2  // slow reverse while balancing
};

// ============================================================
// DATA STRUCTURES
// ============================================================

struct Statistics {
  uint16_t n = 0;
  float mean = 0.0f;
  float m2 = 0.0f;

  void clear() {
    n = 0;
    mean = 0.0f;
    m2 = 0.0f;
  }

  void add(float x) {
    ++n;
    float difference = x - mean;
    mean += difference / n;
    m2 += difference * (x - mean);
  }

  float deviation() const {
    if (n <= 1) {
      return 0.0f;
    }
    return sqrtf(fmaxf(0.0f, m2 / (n - 1)));
  }
};

struct ImuSample {
  float pitch;
  float gy;
};

// Snapshot handed from the balance loop (core 1) to the web server (core 0).
struct Telemetry {
  uint32_t uptimeMs;
  float    tiltDeg;
  float    rateDPS;
  float    targetAngleDeg;
  float    leftRPM;
  float    rightRPM;
  float    targetRPM;
  float    commandRPM;
  float    positionRev;
  float    distanceCm;
  int16_t  leftPWM;
  int16_t  rightPWM;
  uint16_t calMs;
  uint16_t workUs;
  uint8_t  state;
  uint8_t  fault;
  uint8_t  mode;
  uint8_t  armPercent;
  uint8_t  armBlocked;
  uint8_t  sonarOnline;
  uint8_t  zone;
  uint8_t  buzzer;
};

// ============================================================
// GLOBAL VARIABLES
// ============================================================

Adafruit_MPU6050 mpu;

RobotState state = STOPPED;
FaultCode fault  = NO_FAULT;

Statistics calPitch;
Statistics calGY;

DriveMode driveMode = AUTO_FORWARD ? MODE_FORWARD : MODE_STABLE;

float angleOffsetDeg = 0.0f;
float gyroBiasYRadS  = 0.0f;

float angleDeg = 0.0f;
float rateDPS  = 0.0f;

float leftRPM  = 0.0f;
float rightRPM = 0.0f;
float averagePositionRev = 0.0f;

float commandRPM        = 0.0f;
float positionTargetRev = 0.0f;
float positionErrorRev  = 0.0f;
float targetRPM         = 0.0f;
float targetAngleDeg    = 0.0f;
float outerTargetAngleDeg = 0.0f; // V3.1: 25 Hz outer-loop output, slewed into targetAngleDeg
float groundRPM            = 0.0f; // V3.1: filtered ground speed used by the speed loop
float previousOuterAngleDeg = 0.0f;
float speedIntegral     = 0.0f;

int leftPWM  = 0;
int rightPWM = 0;

volatile long leftRawCount  = 0;
volatile long rightRawCount = 0;

long previousLeftNormalized  = 0;
long previousRightNormalized = 0;

uint32_t nextControl = 0;
uint32_t lastControl = 0;
uint32_t lastOuter   = 0;
uint32_t calStart    = 0;

uint32_t uprightUs       = 0;
uint32_t forwardStableUs = 0;
uint32_t lastReportMs    = 0;

uint16_t calMs        = 0;
uint16_t workUs       = 0;
uint8_t  imuFailCount = 0;

volatile uint8_t motorAgeMs      = 0;
volatile bool    motorGuard      = false;
volatile bool    deadlineTripped = false;

// ESP32 is dual-core: these locks replace Uno ATOMIC_BLOCK for shared state.
portMUX_TYPE encoderMux = portMUX_INITIALIZER_UNLOCKED;
portMUX_TYPE motorMux   = portMUX_INITIALIZER_UNLOCKED;
hw_timer_t *motorWatchdogTimer = nullptr;

char    reportBuffer[160];
uint8_t reportLength = 0;
uint8_t reportOffset = 0;

// ---------- Add-on: obstacle guard ----------
portMUX_TYPE sonarMux = portMUX_INITIALIZER_UNLOCKED;

volatile uint32_t echoRiseUs      = 0;
volatile bool     echoRiseSeen    = false;
volatile uint32_t echoPulseUs     = 0;
volatile uint32_t echoPulseRiseUs = 0;
volatile bool     echoPulseReady  = false;

uint32_t sonarPingUs      = 0;
bool     sonarPingPending = false;
uint8_t  sonarMisses      = 0;
bool     sonarOnline      = false;

float   sonarHistory[3] = {US_MAX_CM, US_MAX_CM, US_MAX_CM};
uint8_t sonarHistoryIndex = 0;
float   distanceCm        = US_MAX_CM; // median-of-3 filtered distance

bool    obstacleWarning = false; // < 2 m
bool    obstacleNear    = false; // < 1 m
uint8_t nearCount       = 0;
bool    buzzerOn        = false;

// ---------- Add-on: WiFi control deck ----------
portMUX_TYPE   telemetryMux = portMUX_INITIALIZER_UNLOCKED;
Telemetry      telemetry    = {};
QueueHandle_t  webCommandQueue   = nullptr;
httpd_handle_t controlDeckServer = nullptr;
bool           controlDeckOnline = false;

uint32_t lastTelemetryMs      = 0;
long     telemetryLeftCounts  = 0;
long     telemetryRightCounts = 0;
float    displayLeftRPM       = 0.0f;
float    displayRightRPM      = 0.0f;

float    lastPitchDeg  = 0.0f; // most recent raw IMU pitch (display only)
bool     idleImuValid  = false;
float    idleTiltDeg   = 0.0f;
float    idleRateDPS   = 0.0f;
uint32_t lastIdleImuMs = 0;

// ============================================================
// SMALL HELPERS
// ============================================================

float clip(float x, float low, float high) {
  return fminf(high, fmaxf(low, x));
}

int clipInt(int x, int low, int high) {
  if (x < low) return low;
  if (x > high) return high;
  return x;
}

float approach(float current, float target, float step) {
  return current + clip(target - current, -step, step);
}

const __FlashStringHelper *faultName(FaultCode code) {
  switch (code) {
    case NO_FAULT:         return F("NO_FAULT");
    case IMU_ERROR:        return F("IMU_ERROR");
    case CAL_MOVED:        return F("CAL_MOVED");
    case TIMING_ERROR:     return F("TIMING_ERROR");
    case FALLEN:           return F("FALLEN");
    case EXCESS_RATE:      return F("EXCESS_RATE");
    case OVERSPEED:        return F("OVERSPEED");
    case ENCODER_OR_STALL: return F("ENCODER_OR_STALL");
    case MOTOR_DEADLINE:   return F("MOTOR_DEADLINE");
    case USER_STOP:        return F("USER_STOP");
    case OBSTACLE_LOCKOUT: return F("OBSTACLE_LOCKOUT");
    default:               return F("UNKNOWN");
  }
}

// ============================================================
// ENCODERS (same A-channel CHANGE edge and A==B direction logic)
// ============================================================

void IRAM_ATTR leftEncoderISR() {
  bool same = bool(digitalRead(LEFT_ENC_A)) == bool(digitalRead(LEFT_ENC_B));
  portENTER_CRITICAL_ISR(&encoderMux);
  if (same) {
    ++leftRawCount;
  } else {
    --leftRawCount;
  }
  portEXIT_CRITICAL_ISR(&encoderMux);
}

void IRAM_ATTR rightEncoderISR() {
  bool same = bool(digitalRead(RIGHT_ENC_A)) == bool(digitalRead(RIGHT_ENC_B));
  portENTER_CRITICAL_ISR(&encoderMux);
  if (same) {
    ++rightRawCount;
  } else {
    --rightRawCount;
  }
  portEXIT_CRITICAL_ISR(&encoderMux);
}

void readNormalizedCounts(long &leftNorm, long &rightNorm) {
  long rawL, rawR;
  portENTER_CRITICAL(&encoderMux);
  rawL = leftRawCount;
  rawR = rightRawCount;
  portEXIT_CRITICAL(&encoderMux);
  leftNorm  = rawL * LEFT_ENCODER_SIGN;
  rightNorm = rawR * RIGHT_ENCODER_SIGN;
}

// ============================================================
// MOTOR DEADLINE GUARD (ESP32 hardware timer @ 1 kHz)
// ============================================================

// Exactly as on Uno: if motor commands age >=20 ms while balancing, latch
// the fault and force BOTH enable pins LOW without waiting for loop().
// Unlike a simple LEDC duty=0 call from an ISR, this detaches the PWM GPIO
// output-matrix signals, then routes output through an already-low GPIO latch.
// The trip is latched until a stopped robot receives 'r' (recalibration).
void IRAM_ATTR motorDeadlineISR() {
  portENTER_CRITICAL_ISR(&motorMux);
  if (motorGuard && ++motorAgeMs >= 20) {
    // Keep GPIO latch low while PWM still owns the output matrix.
    REG_WRITE(GPIO_OUT_W1TC_REG, (1UL << LEFT_EN) | (1UL << RIGHT_EN));
    // Reconnect both pads to GPIO output (not to the PWM peripheral).
    // Both EN pins are on GPIO 0..31 on the classic ESP32.
    esp_rom_gpio_connect_out_signal(LEFT_EN, SIG_GPIO_OUT_IDX, false, false);
    esp_rom_gpio_connect_out_signal(RIGHT_EN, SIG_GPIO_OUT_IDX, false, false);
    motorGuard = false;
    deadlineTripped = true;
  }
  portEXIT_CRITICAL_ISR(&motorMux);
}

// ============================================================
// MOTOR CONTROL
// ============================================================

void stopMotors() {
  portENTER_CRITICAL(&motorMux);
  motorGuard = false;
  portEXIT_CRITICAL(&motorMux);

  ledcWrite(LEFT_EN, 0);
  ledcWrite(RIGHT_EN, 0);

  digitalWrite(LEFT_IN1, LOW);
  digitalWrite(LEFT_IN2, LOW);
  digitalWrite(RIGHT_IN1, LOW);
  digitalWrite(RIGHT_IN2, LOW);

  leftPWM  = 0;
  rightPWM = 0;
}

void latchFault(FaultCode reason) {
  stopMotors();

  if (fault == NO_FAULT) {
    fault = reason;
  }

  state = STOPPED;

  commandRPM     = 0.0f;
  targetRPM      = 0.0f;
  targetAngleDeg = 0.0f;
  outerTargetAngleDeg = 0.0f;
  speedIntegral  = 0.0f;

  uprightUs       = 0;
  forwardStableUs = 0;

  Serial.print(F("FAULT LATCHED ["));
  Serial.print(unsigned(fault));
  Serial.print(F("]: "));
  Serial.println(faultName(fault));
}

void driveOneMotor(
    uint8_t enPin,
    uint8_t in1Pin,
    uint8_t in2Pin,
    int logicalPWM,
    int8_t motorSign
) {
  logicalPWM = clipInt(logicalPWM, -MAX_PWM, MAX_PWM);
  int electricalPWM = logicalPWM * motorSign;

  if (electricalPWM > 0) {
    digitalWrite(in1Pin, HIGH);
    digitalWrite(in2Pin, LOW);
  } else if (electricalPWM < 0) {
    digitalWrite(in1Pin, LOW);
    digitalWrite(in2Pin, HIGH);
  } else {
    digitalWrite(in1Pin, LOW);
    digitalWrite(in2Pin, LOW);
  }

  ledcWrite(enPin, abs(electricalPWM));
}

void applyMotors(int left, int right) {
  left  = clipInt(left, -MAX_PWM, MAX_PWM);
  right = clipInt(right, -MAX_PWM, MAX_PWM);

  if (!deadlineTripped && state == BALANCING && ENABLE_MOTORS) {
    driveOneMotor(LEFT_EN, LEFT_IN1, LEFT_IN2, left, LEFT_MOTOR_SIGN);
    driveOneMotor(RIGHT_EN, RIGHT_IN1, RIGHT_IN2, right, RIGHT_MOTOR_SIGN);

    portENTER_CRITICAL(&motorMux);
    leftPWM    = left;
    rightPWM   = right;
    motorAgeMs = 0;
    if (!deadlineTripped) {
      motorGuard = true;
    }
    portEXIT_CRITICAL(&motorMux);
  } else {
    stopMotors();
  }
}

// Proven minimum-PWM mapping from your working sketch.
// Ensures motors overcome static friction immediately outside the deadband.
int outputToPWM(float output, int minimumPWM) {
  float magnitude = fabsf(output);

  if (magnitude <= OUTPUT_DEADBAND) {
    return 0;
  }

  float denominator = OUTPUT_FULL_SCALE - OUTPUT_DEADBAND;
  if (denominator <= 0.0f) {
    return 0;
  }

  float ratio = (magnitude - OUTPUT_DEADBAND) / denominator;
  ratio = clip(ratio, 0.0f, 1.0f);

  float pwm = minimumPWM + ratio * (MAX_PWM - minimumPWM);

  // V3.1 soft entry: blend from a fraction of the minimum PWM up to the
  // original curve, so tiny corrections no longer kick the motors (chatter).
  if (MIN_PWM_SOFT_BAND > 0.0f && MIN_PWM_ENTRY_FRACTION < 1.0f) {
    float entry = clip((magnitude - OUTPUT_DEADBAND) / MIN_PWM_SOFT_BAND, 0.0f, 1.0f);
    pwm *= MIN_PWM_ENTRY_FRACTION + (1.0f - MIN_PWM_ENTRY_FRACTION) * entry;
  }

  int result = int(lroundf(pwm));

  if (output < 0.0f) {
    result = -result;
  }

  return clipInt(result, -MAX_PWM, MAX_PWM);
}

// ============================================================
// MPU6050
// ============================================================

bool configureImu() {
  Wire.begin(MPU_SDA, MPU_SCL);
  Wire.setClock(I2C_CLOCK_HZ);
  Wire.setTimeOut(3); // ESP32 timeout unit is milliseconds; Uno used 3000 us.

  if (!mpu.begin(MPU_ADDRESS, &Wire)) {
    return false;
  }

  // Restore clock and timeout settings after Adafruit mpu.begin()
  Wire.setClock(I2C_CLOCK_HZ);
  Wire.setTimeOut(3);

  mpu.setAccelerometerRange(MPU6050_RANGE_4_G);
  mpu.setGyroRange(MPU6050_RANGE_500_DEG);
  mpu.setFilterBandwidth(MPU6050_BAND_44_HZ);

  delay(50);
  return true; // MPU6050 communication success is checked via mpu.begin/getEvent.
}

bool readImu(ImuSample &sample) {
  sensors_event_t accel;
  sensors_event_t gyro;
  sensors_event_t temp;

  bool ok = mpu.getEvent(&accel, &gyro, &temp);
  if (!ok) {
    return false;
  }

  float ax = accel.acceleration.x;
  float ay = accel.acceleration.y;
  float az = accel.acceleration.z;

  if (
      !isfinite(ax) ||
      !isfinite(ay) ||
      !isfinite(az) ||
      !isfinite(gyro.gyro.y)
  ) {
    return false;
  }

  // Guard against degenerate atan2(0, 0) if sensor returns all zeros
  if (ax * ax + az * az < 0.001f) {
    return false;
  }

  sample.pitch = ANGLE_SIGN * atan2f(ax, az) * RAD_TO_DEG_F;
  sample.gy    = gyro.gyro.y;

  return true;
}

// ============================================================
// CALIBRATION
// ============================================================

void resetSchedule() {
  lastControl = micros();
  nextControl = lastControl + CONTROL_US;
  lastOuter   = lastControl;
}

void startCalibration() {
  stopMotors();

  state = STOPPED;
  fault = NO_FAULT;

  // A watchdog trip physically detaches LEDC from the two enable pins.
  // Recalibration while stopped must reconnect it (with duty held at zero).
  if (deadlineTripped) {
    ledcDetach(LEFT_EN);
    ledcDetach(RIGHT_EN);
    if (!ledcAttach(LEFT_EN, MOTOR_PWM_HZ, MOTOR_PWM_BITS) ||
        !ledcAttach(RIGHT_EN, MOTOR_PWM_HZ, MOTOR_PWM_BITS)) {
      latchFault(MOTOR_DEADLINE);
      return;
    }
    ledcWrite(LEFT_EN, 0);
    ledcWrite(RIGHT_EN, 0);
  }

  portENTER_CRITICAL(&motorMux);
  deadlineTripped = false;
  motorAgeMs      = 0;
  portEXIT_CRITICAL(&motorMux);

  reportLength = 0;
  reportOffset = 0;

  angleDeg = 0.0f;
  rateDPS  = 0.0f;

  leftRPM            = 0.0f;
  rightRPM           = 0.0f;
  averagePositionRev = 0.0f;

  commandRPM        = 0.0f;
  positionTargetRev = 0.0f;
  positionErrorRev  = 0.0f;
  targetRPM         = 0.0f;
  targetAngleDeg    = 0.0f;
  outerTargetAngleDeg = 0.0f;
  speedIntegral     = 0.0f;

  uprightUs       = 0;
  forwardStableUs = 0;
  imuFailCount    = 0;

  driveMode = AUTO_FORWARD ? MODE_FORWARD : MODE_STABLE;

  calPitch.clear();
  calGY.clear();
  calMs = 0;

  if (!configureImu()) {
    latchFault(IMU_ERROR);
    resetSchedule();
    return;
  }

  Serial.println(
      F("CAL: Hold robot upright and still for 3 s (motors OFF)...")
  );
  Serial.flush();

  readNormalizedCounts(previousLeftNormalized, previousRightNormalized);

  calStart = micros();
  state    = CALIBRATING;

  resetSchedule();
}

void updateCalibration(
    const ImuSample &sample,
    uint32_t now,
    uint32_t sampleUs
) {
  calPitch.add(sample.pitch);
  calGY.add(sample.gy);

  // Track upright stability during calibration so it can arm quickly once done
  bool upright =
      fabsf(sample.pitch - calPitch.mean) <= START_ANGLE_DEG &&
      fabsf((sample.gy - calGY.mean) * RAD_TO_DEG_F) <= START_RATE_DPS;

  if (!upright) {
    uprightUs = 0;
  } else if (uprightUs < UPRIGHT_HOLD_US) {
    uprightUs += sampleUs;
  }

  uint32_t elapsed = now - calStart;
  calMs = uint16_t(
      elapsed / 1000UL > CALIBRATION_US / 1000UL ? CALIBRATION_US / 1000UL : elapsed / 1000UL
  );

  if (elapsed < CALIBRATION_US) {
    return;
  }

  float pitchSD = calPitch.deviation();
  float gyroSD  = calGY.deviation() * RAD_TO_DEG_F;

  Serial.print(F("CAL samples="));
  Serial.print(calPitch.n);
  Serial.print(F(" pitchSD_deg="));
  Serial.print(pitchSD, 3);
  Serial.print(F(" gyroSD_dps="));
  Serial.println(gyroSD, 3);

  // Realistic sanity check: only reject if robot was dropped or swung wildly
  bool good =
      calPitch.n >= 200 &&
      pitchSD <= 15.0f &&
      gyroSD <= 25.0f;

  if (!good) {
    latchFault(CAL_MOVED);
    resetSchedule();
    return;
  }

  angleOffsetDeg = calPitch.mean;
  gyroBiasYRadS  = calGY.mean;

  angleDeg = 0.0f;
  rateDPS  = 0.0f;

  Serial.print(F("CAL OK: offset_deg="));
  Serial.print(angleOffsetDeg, 4);
  Serial.print(F(" gyroY_bias_rad_s="));
  Serial.println(gyroBiasYRadS, 6);
  Serial.println(F("READY: Hold upright and still to auto-arm."));
  Serial.flush();

  state = READY;
  resetSchedule();
}

// ============================================================
// OBSTACLE GUARD (HC-SR04 ultrasonic + buzzer) - add-on
// ============================================================

// Non-blocking ultrasonic ranging: the ECHO pulse is timed by this interrupt,
// so the 200 Hz balance loop never waits for sound (no pulseIn()).
void IRAM_ATTR sonarEchoISR() {
  uint32_t t = micros();
  bool high = digitalRead(US_ECHO_PIN);
  portENTER_CRITICAL_ISR(&sonarMux);
  if (high) {
    echoRiseUs   = t;
    echoRiseSeen = true;
  } else if (echoRiseSeen) {
    echoPulseUs     = t - echoRiseUs;
    echoPulseRiseUs = echoRiseUs;
    echoPulseReady  = true;
    echoRiseSeen    = false;
  }
  portEXIT_CRITICAL_ISR(&sonarMux);
}

float median3(float a, float b, float c) {
  return fmaxf(fminf(a, b), fminf(fmaxf(a, b), c));
}

void resetSonarFilter() {
  for (uint8_t i = 0; i < 3; ++i) {
    sonarHistory[i] = US_MAX_CM;
  }
  sonarHistoryIndex = 0;
  distanceCm        = US_MAX_CM;
  obstacleWarning   = false;
  obstacleNear      = false;
  nearCount         = 0;
}

void addSonarReading(float cm) {
  sonarMisses = 0;
  sonarOnline = true;

  sonarHistory[sonarHistoryIndex] = clip(cm, US_MIN_CM, US_MAX_CM);
  sonarHistoryIndex = uint8_t((sonarHistoryIndex + 1) % 3);

  // Median of the last 3 readings rejects single-ping spikes.
  distanceCm = median3(sonarHistory[0], sonarHistory[1], sonarHistory[2]);

  obstacleWarning = distanceCm < BUZZER_DISTANCE_CM;
  obstacleNear    = distanceCm < LOCKOUT_DISTANCE_CM;

  if (!obstacleNear) {
    nearCount = 0;
  } else if (nearCount < 255) {
    ++nearCount;
  }
}

void sonarMiss() {
  if (sonarMisses < 255) {
    ++sonarMisses;
  }
  if (sonarMisses >= US_OFFLINE_MISSES && sonarOnline) {
    sonarOnline = false;
    resetSonarFilter();
  }
}

void serviceSonar() {
  uint32_t nowUs = micros();

  if (sonarPingPending) {
    bool     ready, riseSeen;
    uint32_t pulseUs, pulseRiseUs, riseUs;

    portENTER_CRITICAL(&sonarMux);
    ready          = echoPulseReady;
    pulseUs        = echoPulseUs;
    pulseRiseUs    = echoPulseRiseUs;
    riseSeen       = echoRiseSeen;
    riseUs         = echoRiseUs;
    echoPulseReady = false;
    portEXIT_CRITICAL(&sonarMux);

    if (ready && int32_t(pulseRiseUs - sonarPingUs) >= 0) {
      // Echo belonging to this ping. Over-range pulses mean "nothing in range".
      sonarPingPending = false;
      addSonarReading(float(pulseUs) / US_MICROS_PER_CM);
    } else if (uint32_t(nowUs - sonarPingUs) >= US_ECHO_TIMEOUT_US) {
      sonarPingPending = false;
      if (riseSeen && int32_t(riseUs - sonarPingUs) >= 0) {
        addSonarReading(US_MAX_CM); // echo still high: no object within range
      } else {
        sonarMiss();                // sensor did not answer
      }
    }
  }

  if (!sonarPingPending && uint32_t(nowUs - sonarPingUs) >= US_PING_PERIOD_US) {
    if (digitalRead(US_ECHO_PIN) == HIGH) {
      // Module still busy (or ECHO stuck high): wait, and count it as a miss.
      if (uint32_t(nowUs - sonarPingUs) >= 2UL * US_PING_PERIOD_US) {
        sonarMiss();
        sonarPingUs = nowUs;
      }
      return;
    }

    portENTER_CRITICAL(&sonarMux);
    echoPulseReady = false;
    portEXIT_CRITICAL(&sonarMux);

    // Timestamp first: every echo of this ping rises after this instant.
    sonarPingUs      = micros();
    sonarPingPending = true;

    digitalWrite(US_TRIG_PIN, HIGH);
    delayMicroseconds(10);
    digitalWrite(US_TRIG_PIN, LOW);
  }
}

void setBuzzer(bool on) {
  if (on == buzzerOn) {
    return;
  }
  buzzerOn = on;

  if (BUZZER_IS_PASSIVE) {
    ledcWrite(BUZZER_PIN, on ? 128 : 0); // 50% duty square wave
  } else {
    digitalWrite(BUZZER_PIN, (on == BUZZER_ACTIVE_HIGH) ? HIGH : LOW);
  }
}

// Auto-arming waits while an object is inside the lockout distance.
bool armingBlocked() {
  return sonarOnline && obstacleNear;
}

void serviceObstacleGuard() {
  serviceSonar();

  // Object < 2 m -> buzzer ON (in every state).
  setBuzzer(sonarOnline && obstacleWarning);

  // Object < 1 m while armed -> latched lockout, motors disarmed.
  if (
      state == BALANCING &&
      sonarOnline &&
      nearCount >= US_LOCKOUT_CONFIRM
  ) {
    latchFault(OBSTACLE_LOCKOUT);
    Serial.print(F("OBSTACLE LOCKOUT: object at "));
    Serial.print(distanceCm, 1);
    Serial.println(F(" cm. Motors disarmed. Send 'r' or press RESET."));
  }
}

// ============================================================
// ARMING
// ============================================================

void armRobot(uint32_t now) {
  long leftNorm, rightNorm;
  readNormalizedCounts(leftNorm, rightNorm);

  previousLeftNormalized  = leftNorm;
  previousRightNormalized = rightNorm;

  float leftRev  = float(leftNorm) / LEFT_COUNTS_PER_WHEEL_REV;
  float rightRev = float(rightNorm) / RIGHT_COUNTS_PER_WHEEL_REV;
  averagePositionRev = 0.5f * (leftRev + rightRev);

  positionTargetRev = averagePositionRev;
  positionErrorRev  = 0.0f;

  commandRPM     = 0.0f;
  targetRPM      = 0.0f;
  targetAngleDeg = 0.0f;
  outerTargetAngleDeg = 0.0f;
  speedIntegral  = 0.0f;

  leftRPM  = 0.0f;
  rightRPM = 0.0f;
  groundRPM = 0.0f;
  previousOuterAngleDeg = angleDeg;

  forwardStableUs = 0;

  lastOuter = now;
  state     = BALANCING;
}

// ============================================================
// OUTER SPEED & POSITION CONTROLLER (25 Hz)
// ============================================================

void updateSpeed(uint32_t now) {
  uint32_t elapsed = now - lastOuter;
  if (elapsed < OUTER_US) {
    return;
  }

  lastOuter = now;
  float dt = elapsed * 0.000001f;
  if (dt <= 0.0f) {
    return;
  }

  long leftNorm, rightNorm;
  readNormalizedCounts(leftNorm, rightNorm);

  float leftPositionRev  = float(leftNorm) / LEFT_COUNTS_PER_WHEEL_REV;
  float rightPositionRev = float(rightNorm) / RIGHT_COUNTS_PER_WHEEL_REV;
  averagePositionRev     = 0.5f * (leftPositionRev + rightPositionRev);

  long deltaLeft  = leftNorm - previousLeftNormalized;
  long deltaRight = rightNorm - previousRightNormalized;

  previousLeftNormalized  = leftNorm;
  previousRightNormalized = rightNorm;

  float rawLeftRPM  = (float(deltaLeft) / LEFT_COUNTS_PER_WHEEL_REV) * 60.0f / dt;
  float rawRightRPM = (float(deltaRight) / RIGHT_COUNTS_PER_WHEEL_REV) * 60.0f / dt;

  float alpha = dt / (SPEED_FILTER_TAU_S + dt);
  leftRPM  += alpha * (rawLeftRPM - leftRPM);
  rightRPM += alpha * (rawRightRPM - rightRPM);

  // V3.1: the speed loop uses the robot's ground speed = wheel speed relative
  // to the body + the body's own pitch rate over the same 40 ms window, so
  // rocking back and forth is no longer mistaken for driving.
  float tiltRPM = (angleDeg - previousOuterAngleDeg) / 360.0f * 60.0f / dt;
  previousOuterAngleDeg = angleDeg;
  float rawGroundRPM =
      0.5f * (rawLeftRPM + rawRightRPM) + SPEED_TILT_COMPENSATION * tiltRPM;
  groundRPM += alpha * (rawGroundRPM - groundRPM);

  float averageRPM = groundRPM;

  // Smoothly ramp the desired cruise speed once upright and stable.
  // FORWARD and REVERSE use the same slow speed and ramp, opposite signs.
  float requestedRPM = 0.0f;
  if (driveMode == MODE_FORWARD && forwardStableUs >= DRIVE_SETTLE_US) {
    requestedRPM = FORWARD_TARGET_RPM;
  } else if (driveMode == MODE_REVERSE && forwardStableUs >= DRIVE_SETTLE_US) {
    requestedRPM = -REVERSE_TARGET_RPM;
  }

  commandRPM = approach(
      commandRPM,
      requestedRPM,
      SPEED_RAMP_RPM_PER_S * dt
  );

  // Advance moving position reference at the ramped command RPM
  positionTargetRev +=
      FORWARD_DIRECTION_SIGN * commandRPM * dt / 60.0f;

  positionErrorRev = positionTargetRev - averagePositionRev;

  // Prevent unbounded position windup if the robot is held back by hand
  positionErrorRev = clip(positionErrorRev, -2.0f, 2.0f);

  targetRPM =
      FORWARD_DIRECTION_SIGN * commandRPM +
      POSITION_KP_RPM_PER_REV * positionErrorRev;

  targetRPM = clip(
      targetRPM,
      -MAX_TARGET_SPEED_RPM,
      MAX_TARGET_SPEED_RPM
  );

  float speedError = targetRPM - averageRPM;

  bool saturated =
      abs(leftPWM) >= MAX_PWM - 1 ||
      abs(rightPWM) >= MAX_PWM - 1;

  if (!saturated) {
    speedIntegral = clip(
        speedIntegral + speedError * dt,
        -10.0f,
        10.0f
    );
  }

  outerTargetAngleDeg = clip(
      SPEED_KP_DEG_PER_RPM * speedError +
      SPEED_KI_DEG_PER_RPM_SEC * speedIntegral,
      -MAX_OUTER_TARGET_ANGLE_DEG,
      MAX_OUTER_TARGET_ANGLE_DEG
  );
}

// ============================================================
// MAIN BALANCE CONTROLLER (200 Hz)
// ============================================================

void controlTick(uint32_t now, uint32_t elapsed) {
  ImuSample sample;

  // Tolerate rare single-sample I2C hiccups; latch fault on 5 consecutive fails
  if (!readImu(sample)) {
    if (++imuFailCount >= 5) {
      latchFault(IMU_ERROR);
    }
    return;
  }
  imuFailCount = 0;
  lastPitchDeg = sample.pitch; // dashboard display only

  if (state == CALIBRATING) {
    updateCalibration(sample, now, elapsed);
    return;
  }

  float dt = elapsed * 0.000001f;

  float correctedAccelAngle = sample.pitch - angleOffsetDeg;

  float gyroRateDegS =
      (sample.gy - gyroBiasYRadS) *
      RAD_TO_DEG_F *
      GYRO_SIGN;

  // Proven gyro rate filter and complementary angle filter
  rateDPS =
      RATE_FILTER * gyroRateDegS +
      (1.0f - RATE_FILTER) * rateDPS;

  angleDeg =
      ANGLE_FILTER * (angleDeg + rateDPS * dt) +
      (1.0f - ANGLE_FILTER) * correctedAccelAngle;

  if (state == READY) {
    bool upright =
        fabsf(angleDeg) <= START_ANGLE_DEG &&
        fabsf(rateDPS) <= START_RATE_DPS;

    if (!upright) {
      uprightUs = 0;
    } else if (uprightUs < UPRIGHT_HOLD_US) {
      uprightUs += elapsed;
    }

    // Add-on: do not arm while an object is inside the 1 m lockout distance.
    if (ENABLE_MOTORS && uprightUs >= UPRIGHT_HOLD_US && !armingBlocked()) {
      armRobot(now);
    } else {
      return;
    }
  }

  if (state != BALANCING) {
    return;
  }

  // Fall protection
  if (fabsf(angleDeg) >= FALL_ANGLE_DEG) {
    latchFault(FALLEN);
    return;
  }

  // V3.1 drive gate: settle once for DRIVE_SETTLE_US after arming, then stay
  // enabled; only a real upset (> DRIVE_RELEASE_ANGLE_DEG) pauses driving.
  // (Before, every vibration spike above 20 deg/s cancelled FORWARD/REVERSE.)
  bool settled =
      fabsf(angleDeg) < DRIVE_SETTLE_ANGLE_DEG &&
      fabsf(rateDPS) < DRIVE_SETTLE_RATE_DPS;

  if (forwardStableUs >= DRIVE_SETTLE_US) {
    if (fabsf(angleDeg) > DRIVE_RELEASE_ANGLE_DEG) {
      forwardStableUs = 0;
    }
  } else if (!settled) {
    forwardStableUs = 0;
  } else {
    forwardStableUs += elapsed;
  }

  updateSpeed(now);

  // V3.1: slew the lean target smoothly at 200 Hz (no 25 Hz step kicks).
  targetAngleDeg = approach(
      targetAngleDeg,
      outerTargetAngleDeg,
      TARGET_ANGLE_SLEW_DEG_PER_S * dt
  );

  // Inner PD balance controller
  float angleError = angleDeg - targetAngleDeg;
  float balanceOutput =
      BALANCE_SIGN * (KP * angleError + KD * rateDPS);

  int leftBasePWM  = outputToPWM(balanceOutput, LEFT_MIN_PWM);
  int rightBasePWM = outputToPWM(balanceOutput, RIGHT_MIN_PWM);

  // Wheel synchronization trim
  float trim = 0.0f;
  if (leftBasePWM != 0 || rightBasePWM != 0) {
    trim = WHEEL_SYNC_KP_PWM_PER_RPM * (leftRPM - rightRPM);
    trim = clip(trim, -MAX_SYNC_TRIM_PWM, MAX_SYNC_TRIM_PWM);
  }

  int leftCmd  = clipInt(leftBasePWM - int(lroundf(trim)), -MAX_PWM, MAX_PWM);
  int rightCmd = clipInt(rightBasePWM + int(lroundf(trim)), -MAX_PWM, MAX_PWM);

  applyMotors(leftCmd, rightCmd);
}

// ============================================================
// SERIAL COMMANDS
// ============================================================

// V3.1: on a mode change the moving position reference restarts from where
// the robot is now, so it never "catches up" distance it lagged behind
// (e.g. rolling on after FORWARD -> STABLE).
void setDriveMode(DriveMode mode) {
  if (mode == driveMode) {
    return;
  }
  driveMode = mode;
  positionTargetRev = averagePositionRev;
  positionErrorRev  = 0.0f;
}

void executeCommand(char command) {
  if (
      command == 's' ||
      command == 'S' ||
      command == 'x' ||
      command == 'X'
  ) {
    latchFault(USER_STOP);
  } else if (command == 'h' || command == 'H') {
    setDriveMode(MODE_STABLE);
  } else if (command == 'f' || command == 'F') {
    setDriveMode(MODE_FORWARD);
  } else if (command == 'b' || command == 'B') {
    setDriveMode(MODE_REVERSE);
  } else if (
      (command == 'r' || command == 'R') &&
      state != BALANCING
  ) {
    startCalibration();
  }
}

void handleCommands() {
  for (uint8_t n = 0; n < 8 && Serial.available(); ++n) {
    executeCommand(char(Serial.read()));
  }
}

// Commands queued by the dashboard (core 0) are executed here, on the balance
// loop's own core, exactly like serial commands.
void handleWebCommands() {
  if (webCommandQueue == nullptr) {
    return;
  }

  char command;
  for (uint8_t n = 0; n < 4 && xQueueReceive(webCommandQueue, &command, 0) == pdTRUE; ++n) {
    if (command == 'r') {
      // Dashboard RESET = serial 'r', but keeps the mode chosen on the dashboard.
      if (state != BALANCING) {
        DriveMode selectedMode = driveMode;
        startCalibration();
        driveMode = selectedMode;
      }
    } else {
      executeCommand(command);
    }
  }
}

// ============================================================
// NONBLOCKING TELEMETRY (10 Hz CSV)
// ============================================================

void serviceReport(bool mayFormat) {
  if (
      mayFormat &&
      reportOffset == reportLength &&
      uint32_t(millis() - lastReportMs) >= 100
  ) {
    lastReportMs = millis();

    int length = snprintf(
        reportBuffer,
        sizeof(reportBuffer),
        "%lu,%u,%u,%u,%d,%d,%d,%d,%d,%d,%d,%d,%u\n",
        (unsigned long)millis(),
        unsigned(state),
        unsigned(fault),
        unsigned(calMs),
        int(angleDeg * 100),
        int(rateDPS * 10),
        int(targetAngleDeg * 100),
        int(leftRPM * 100),
        int(rightRPM * 100),
        int(targetRPM * 100),
        leftPWM,
        rightPWM,
        unsigned(workUs)
    );

    reportLength =
        (length > 0 && length < int(sizeof(reportBuffer)))
            ? uint8_t(length)
            : 0;
    reportOffset = 0;
  }

  int availableSpace = Serial.availableForWrite();
  uint8_t remaining  = reportLength - reportOffset;
  uint8_t count      = availableSpace < remaining
                           ? uint8_t(availableSpace)
                           : remaining;

  if (count > 0) {
    Serial.write(
        (const uint8_t *)reportBuffer + reportOffset,
        count
    );
    reportOffset += count;
  }
}

// ============================================================
// DASHBOARD TELEMETRY (display only - never feeds the controller)
// ============================================================

// While STOPPED the control loop is idle, so read the IMU slowly to keep the
// dashboard tilt live (e.g. robot lying on the floor after a lockout).
void serviceIdleImu() {
  if (state != STOPPED) {
    idleImuValid = false;
    return;
  }
  if (fault == IMU_ERROR) {
    return;
  }

  uint32_t nowMs = millis();
  if (uint32_t(nowMs - lastIdleImuMs) < IDLE_IMU_PERIOD_MS) {
    return;
  }
  lastIdleImuMs = nowMs;

  ImuSample sample;
  if (!readImu(sample)) {
    return;
  }

  float tilt = sample.pitch - angleOffsetDeg;
  float rate = (sample.gy - gyroBiasYRadS) * RAD_TO_DEG_F * GYRO_SIGN;

  if (!idleImuValid) {
    idleTiltDeg  = tilt;
    idleRateDPS  = rate;
    idleImuValid = true;
  } else {
    idleTiltDeg += 0.35f * (tilt - idleTiltDeg);
    idleRateDPS += 0.35f * (rate - idleRateDPS);
  }
}

void publishTelemetry() {
  uint32_t nowMs     = millis();
  uint32_t elapsedMs = nowMs - lastTelemetryMs;
  if (elapsedMs < TELEMETRY_PERIOD_MS) {
    return;
  }
  lastTelemetryMs = nowMs;

  // Independent wheel-speed measurement for the dashboard (all states).
  long leftNorm, rightNorm;
  readNormalizedCounts(leftNorm, rightNorm);

  if (elapsedMs < 500) {
    float dt       = elapsedMs * 0.001f;
    float rawLeft  = (float(leftNorm - telemetryLeftCounts) / LEFT_COUNTS_PER_WHEEL_REV) * 60.0f / dt;
    float rawRight = (float(rightNorm - telemetryRightCounts) / RIGHT_COUNTS_PER_WHEEL_REV) * 60.0f / dt;
    float alpha    = dt / (DISPLAY_RPM_TAU_S + dt);
    displayLeftRPM  += alpha * (rawLeft - displayLeftRPM);
    displayRightRPM += alpha * (rawRight - displayRightRPM);
  }
  telemetryLeftCounts  = leftNorm;
  telemetryRightCounts = rightNorm;

  Telemetry t = {};
  t.uptimeMs = nowMs;

  if (state == CALIBRATING) {
    t.tiltDeg = calPitch.n > 0 ? lastPitchDeg - calPitch.mean : 0.0f;
    t.rateDPS = 0.0f;
  } else if (state == STOPPED) {
    t.tiltDeg = idleImuValid ? idleTiltDeg : angleDeg;
    t.rateDPS = idleImuValid ? idleRateDPS : 0.0f;
  } else {
    t.tiltDeg = angleDeg;
    t.rateDPS = rateDPS;
  }

  bool balancing = state == BALANCING;
  t.targetAngleDeg = balancing ? targetAngleDeg : 0.0f;
  t.targetRPM      = balancing ? targetRPM : 0.0f;
  t.commandRPM     = balancing ? commandRPM : 0.0f;
  t.leftRPM        = displayLeftRPM;
  t.rightRPM       = displayRightRPM;
  t.positionRev    = 0.5f * (float(leftNorm) / LEFT_COUNTS_PER_WHEEL_REV +
                             float(rightNorm) / RIGHT_COUNTS_PER_WHEEL_REV);
  t.distanceCm     = distanceCm;
  t.leftPWM        = int16_t(leftPWM);
  t.rightPWM       = int16_t(rightPWM);
  t.calMs          = calMs;
  t.workUs         = workUs;
  t.state          = uint8_t(state);
  t.fault          = uint8_t(fault);
  t.mode           = uint8_t(driveMode);

  if (balancing) {
    t.armPercent = 100;
  } else if (state == READY) {
    uint32_t pct = uprightUs * 100UL / UPRIGHT_HOLD_US;
    t.armPercent = uint8_t(pct > 100UL ? 100UL : pct);
  }
  t.armBlocked  = (state == READY && armingBlocked()) ? 1 : 0;
  t.sonarOnline = sonarOnline ? 1 : 0;
  t.zone        = !sonarOnline ? 0 : obstacleNear ? 2 : obstacleWarning ? 1 : 0;
  t.buzzer      = buzzerOn ? 1 : 0;

  portENTER_CRITICAL(&telemetryMux);
  telemetry = t;
  portEXIT_CRITICAL(&telemetryMux);
}

// ============================================================
// WIFI CONTROL DECK (esp_http_server task pinned to core 0)
// ============================================================

float jsonSafe(float x) {
  return isfinite(x) ? x : 0.0f;
}

esp_err_t controlDeckPageHandler(httpd_req_t *req) {
  httpd_resp_set_type(req, "text/html; charset=utf-8");
  httpd_resp_set_hdr(req, "Cache-Control", "no-cache");
  return httpd_resp_send(req, DASHBOARD_HTML, sizeof(DASHBOARD_HTML) - 1);
}

esp_err_t controlDeckStateHandler(httpd_req_t *req) {
  Telemetry t;
  portENTER_CRITICAL(&telemetryMux);
  t = telemetry;
  portEXIT_CRITICAL(&telemetryMux);

  char json[448];
  int length = snprintf(
      json,
      sizeof(json),
      "{\"st\":%u,\"ft\":%u,\"md\":%u,\"cal\":%u,\"calt\":%lu,\"arm\":%u,\"blk\":%u,"
      "\"tilt\":%.2f,\"rate\":%.1f,\"tgt\":%.2f,"
      "\"lr\":%.2f,\"rr\":%.2f,\"trpm\":%.2f,\"crpm\":%.2f,"
      "\"lp\":%d,\"rp\":%d,\"pos\":%.3f,"
      "\"dist\":%.1f,\"son\":%u,\"zone\":%u,\"buz\":%u,"
      "\"work\":%u,\"up\":%lu,\"cli\":%u}",
      unsigned(t.state),
      unsigned(t.fault),
      unsigned(t.mode),
      unsigned(t.calMs),
      (unsigned long)(CALIBRATION_US / 1000UL),
      unsigned(t.armPercent),
      unsigned(t.armBlocked),
      jsonSafe(t.tiltDeg),
      jsonSafe(t.rateDPS),
      jsonSafe(t.targetAngleDeg),
      jsonSafe(t.leftRPM),
      jsonSafe(t.rightRPM),
      jsonSafe(t.targetRPM),
      jsonSafe(t.commandRPM),
      int(t.leftPWM),
      int(t.rightPWM),
      jsonSafe(t.positionRev),
      jsonSafe(t.distanceCm),
      unsigned(t.sonarOnline),
      unsigned(t.zone),
      unsigned(t.buzzer),
      unsigned(t.workUs),
      (unsigned long)t.uptimeMs,
      unsigned(WiFi.softAPgetStationNum())
  );

  if (length <= 0 || length >= int(sizeof(json))) {
    return httpd_resp_send_500(req);
  }

  httpd_resp_set_type(req, "application/json");
  httpd_resp_set_hdr(req, "Cache-Control", "no-store");
  return httpd_resp_send(req, json, length);
}

esp_err_t controlDeckCommandHandler(httpd_req_t *req) {
  char query[48];
  char value[16];
  char command = 0;

  if (
      httpd_req_get_url_query_str(req, query, sizeof(query)) == ESP_OK &&
      httpd_query_key_value(query, "c", value, sizeof(value)) == ESP_OK
  ) {
    if (strcmp(value, "stable") == 0) {
      command = 'h';
    } else if (strcmp(value, "forward") == 0) {
      command = 'f';
    } else if (strcmp(value, "reverse") == 0) {
      command = 'b';
    } else if (strcmp(value, "estop") == 0) {
      command = 's';
    } else if (strcmp(value, "reset") == 0) {
      command = 'r';
    }
  }

  httpd_resp_set_type(req, "application/json");
  httpd_resp_set_hdr(req, "Cache-Control", "no-store");

  if (command == 0) {
    httpd_resp_set_status(req, "400 Bad Request");
    return httpd_resp_sendstr(req, "{\"ok\":false}");
  }

  if (webCommandQueue == nullptr ||
      xQueueSend(webCommandQueue, &command, 0) != pdTRUE) {
    httpd_resp_set_status(req, "503 Service Unavailable");
    return httpd_resp_sendstr(req, "{\"ok\":false}");
  }

  return httpd_resp_sendstr(req, "{\"ok\":true}");
}

esp_err_t controlDeckNoContentHandler(httpd_req_t *req) {
  httpd_resp_set_status(req, "204 No Content");
  return httpd_resp_send(req, nullptr, 0);
}

bool registerRoute(const char *uri, httpd_method_t method, esp_err_t (*handler)(httpd_req_t *)) {
  httpd_uri_t route = {};
  route.uri      = uri;
  route.method   = method;
  route.handler  = handler;
  route.user_ctx = nullptr;
  return httpd_register_uri_handler(controlDeckServer, &route) == ESP_OK;
}

bool startControlDeck() {
  webCommandQueue = xQueueCreate(8, sizeof(char));
  if (webCommandQueue == nullptr) {
    return false;
  }

  WiFi.persistent(false); // never write WiFi settings to flash at runtime
  if (!WiFi.softAP(
          WIFI_AP_SSID,
          WIFI_AP_PASSWORD,
          WIFI_AP_CHANNEL,
          0,
          WIFI_AP_MAX_CLIENTS
      )) {
    return false;
  }
  WiFi.setTxPower(WIFI_AP_TX_POWER);

  httpd_config_t config   = HTTPD_DEFAULT_CONFIG();
  config.server_port      = 80;
  config.core_id          = 0;                    // keep core 1 for balancing
  config.task_priority    = tskIDLE_PRIORITY + 3; // below WiFi / TCP-IP tasks
  config.stack_size       = 8192;
  config.max_open_sockets = 7;
  config.max_uri_handlers = 8;
  config.lru_purge_enable = true;

  if (httpd_start(&controlDeckServer, &config) != ESP_OK) {
    controlDeckServer = nullptr;
    return false;
  }

  return registerRoute("/", HTTP_GET, controlDeckPageHandler) &&
         registerRoute("/api/state", HTTP_GET, controlDeckStateHandler) &&
         registerRoute("/api/cmd", HTTP_POST, controlDeckCommandHandler) &&
         registerRoute("/favicon.ico", HTTP_GET, controlDeckNoContentHandler);
}

// ============================================================
// SETUP
// ============================================================

void setup() {
  const uint8_t outputPins[] = {
    LEFT_EN,
    RIGHT_EN,
    LEFT_IN1,
    LEFT_IN2,
    RIGHT_IN1,
    RIGHT_IN2,
    LED_BUILTIN
  };

  for (uint8_t i = 0; i < sizeof(outputPins); ++i) {
    digitalWrite(outputPins[i], LOW);
    pinMode(outputPins[i], OUTPUT);
  }

  // Add-on outputs: buzzer silent and TRIG low from the very first moment.
  digitalWrite(
      BUZZER_PIN,
      (BUZZER_IS_PASSIVE || BUZZER_ACTIVE_HIGH) ? LOW : HIGH
  );
  pinMode(BUZZER_PIN, OUTPUT);
  digitalWrite(US_TRIG_PIN, LOW);
  pinMode(US_TRIG_PIN, OUTPUT);

  Serial.begin(115200);

  // Arduino-ESP32 core 3.x LEDC replaces AVR analogWrite timers.
  // Keep the original 0..255 motor PWM scale (8-bit).
  if (!ledcAttach(LEFT_EN, MOTOR_PWM_HZ, MOTOR_PWM_BITS) ||
      !ledcAttach(RIGHT_EN, MOTOR_PWM_HZ, MOTOR_PWM_BITS)) {
    digitalWrite(LEFT_EN, LOW);
    digitalWrite(RIGHT_EN, LOW);
    Serial.println(F("ERROR: ESP32 motor PWM setup failed."));
    while (true) { delay(1000); }
  }
  stopMotors();

  // GPIO34/35 on ESP32 are input-only, WITHOUT internal pull-ups.
  // The encoder outputs must be level-shifted to <=3.3V and have
  // pull-ups on the ESP32 side of the shifter if needed.
  pinMode(LEFT_ENC_A, INPUT);
  pinMode(LEFT_ENC_B, INPUT);
  pinMode(RIGHT_ENC_A, INPUT_PULLUP);
  pinMode(RIGHT_ENC_B, INPUT_PULLUP);

  attachInterrupt(
      digitalPinToInterrupt(LEFT_ENC_A),
      leftEncoderISR,
      CHANGE
  );
  attachInterrupt(
      digitalPinToInterrupt(RIGHT_ENC_A),
      rightEncoderISR,
      CHANGE
  );

  // ESP32 hardware timer: 1 MHz counter, alarm each 1000 us (1 kHz).
  // Original 20-tick / 20 ms deadline is unchanged.
  motorWatchdogTimer = timerBegin(1000000);
  if (motorWatchdogTimer == nullptr) {
    stopMotors();
    Serial.println(F("ERROR: ESP32 motor watchdog timer unavailable."));
    while (true) { delay(1000); }
  }
  timerAttachInterrupt(motorWatchdogTimer, &motorDeadlineISR);
  timerAlarm(motorWatchdogTimer, 1000, true, 0);

  // ---------- Add-on: obstacle guard ----------
  pinMode(US_ECHO_PIN, INPUT_PULLDOWN); // stays LOW if the sensor is unplugged
  attachInterrupt(
      digitalPinToInterrupt(US_ECHO_PIN),
      sonarEchoISR,
      CHANGE
  );

  if (BUZZER_IS_PASSIVE) {
    // Passive buzzer: own channel (and timer), attached after the motor PWM.
    if (ledcAttachChannel(BUZZER_PIN, BUZZER_TONE_HZ, 8, BUZZER_LEDC_CHANNEL)) {
      ledcWrite(BUZZER_PIN, 0);
    } else {
      Serial.println(F("WARNING: passive buzzer PWM setup failed."));
    }
  }

  // ---------- Add-on: WiFi control deck ----------
  controlDeckOnline = startControlDeck();
  if (controlDeckOnline) {
    Serial.print(F("WiFi AP \""));
    Serial.print(WIFI_AP_SSID);
    Serial.print(F("\" (password \""));
    Serial.print(WIFI_AP_PASSWORD);
    Serial.print(F("\")  Control deck: http://"));
    Serial.println(WiFi.softAPIP());
  } else {
    Serial.println(F("WARNING: WiFi control deck unavailable (serial control still works)."));
  }

  Serial.println(
      F("BalanceX V3.1 | s/x=STOP h=hold f=forward b=reverse r=recalibrate")
  );
  Serial.println(
      F("ms,state,fault,cal_ms,angle_cdeg,rate_dps10,target_cdeg,Lrpm100,Rrpm100,cmdRPM100,Lpwm,Rpwm,work_us")
  );

  startCalibration();
}

// ============================================================
// LOOP
// ============================================================

void loop() {
  if (deadlineTripped && state != STOPPED) {
    latchFault(MOTOR_DEADLINE);
  }

  handleCommands();
  handleWebCommands();
  serviceObstacleGuard();

  uint32_t now = micros();

  if (state != STOPPED && int32_t(now - nextControl) >= 0) {
    uint32_t elapsed  = now - lastControl;
    uint32_t lateness = now - nextControl;

    lastControl = now;
    nextControl += CONTROL_US;

    if (lateness >= CONTROL_US) {
      nextControl = now + CONTROL_US;
    }

    // If a massive stall (>50 ms) occurs while balancing, reset filter safely
    if (state == BALANCING && elapsed > 50000UL) {
      latchFault(TIMING_ERROR);
    } else {
      controlTick(now, elapsed);
    }

    workUs = uint16_t(
        clip(float(uint32_t(micros() - now)), 0.0f, 65535.0f)
    );
  }

  bool ledOn;
  if (state == BALANCING) {
    ledOn = true;
  } else {
    uint32_t blinkPeriod = (state == STOPPED) ? 100UL : 350UL;
    ledOn = (millis() / blinkPeriod) & 1;
  }
  digitalWrite(LED_BUILTIN, ledOn ? HIGH : LOW);

  int32_t spareTime = int32_t(nextControl - micros());
  serviceReport(state == STOPPED || spareTime > 1000);

  // Dashboard housekeeping only in spare time: never delays a control tick.
  if (state == STOPPED || int32_t(nextControl - micros()) > 1000) {
    serviceIdleImu();
    publishTelemetry();
  }
}
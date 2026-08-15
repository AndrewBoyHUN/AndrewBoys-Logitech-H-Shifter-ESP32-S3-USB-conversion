#include <Arduino.h>
#include "USB.h"
#include "USBHIDGamepad.h"

#include <algorithm>
#include <array>

namespace {

constexpr uint8_t PIN_X = 1;
constexpr uint8_t PIN_Y = 2;
constexpr uint8_t PIN_REVERSE = 4;

constexpr size_t SAMPLE_COUNT = 21;
constexpr uint32_t SAMPLE_SPACING_US = 250;
constexpr uint32_t LOOP_DELAY_MS = 2;
constexpr uint32_t DEBOUNCE_MS = 25;

constexpr int X_LEFT_CENTER = 1430;
constexpr int X_CENTER_RIGHT = 2375;
constexpr int X_HYSTERESIS = 80;

constexpr int Y_BOTTOM_NEUTRAL = 1160;
constexpr int Y_NEUTRAL_TOP = 2945;
constexpr int Y_HYSTERESIS = 100;

USBHIDGamepad Gamepad;

enum class XColumn : uint8_t {
  Unknown,
  Left,
  Center,
  Right,
};

enum class YRow : uint8_t {
  Unknown,
  Bottom,
  Neutral,
  Top,
};

struct Reading {
  uint16_t x;
  uint16_t y;
  uint8_t reverse;
};

Reading readFiltered() {
  std::array<uint16_t, SAMPLE_COUNT> xSamples{};
  std::array<uint16_t, SAMPLE_COUNT> ySamples{};
  uint8_t reverseHighCount = 0;

  for (size_t i = 0; i < SAMPLE_COUNT; ++i) {
    xSamples[i] = static_cast<uint16_t>(analogRead(PIN_X));
    ySamples[i] = static_cast<uint16_t>(analogRead(PIN_Y));
    reverseHighCount += digitalRead(PIN_REVERSE) == HIGH ? 1 : 0;
    delayMicroseconds(SAMPLE_SPACING_US);
  }

  std::sort(xSamples.begin(), xSamples.end());
  std::sort(ySamples.begin(), ySamples.end());

  return {
      xSamples[SAMPLE_COUNT / 2],
      ySamples[SAMPLE_COUNT / 2],
      static_cast<uint8_t>(reverseHighCount > SAMPLE_COUNT / 2 ? 1 : 0),
  };
}

XColumn initialXColumn(uint16_t x) {
  if (x < X_LEFT_CENTER) {
    return XColumn::Left;
  }
  if (x <= X_CENTER_RIGHT) {
    return XColumn::Center;
  }
  return XColumn::Right;
}

YRow initialYRow(uint16_t y) {
  if (y < Y_BOTTOM_NEUTRAL) {
    return YRow::Bottom;
  }
  if (y <= Y_NEUTRAL_TOP) {
    return YRow::Neutral;
  }
  return YRow::Top;
}

XColumn updateXColumn(XColumn current, uint16_t x) {
  switch (current) {
    case XColumn::Left:
      return x >= X_LEFT_CENTER + X_HYSTERESIS ? initialXColumn(x)
                                               : XColumn::Left;
    case XColumn::Center:
      if (x < X_LEFT_CENTER - X_HYSTERESIS) {
        return XColumn::Left;
      }
      if (x > X_CENTER_RIGHT + X_HYSTERESIS) {
        return XColumn::Right;
      }
      return XColumn::Center;
    case XColumn::Right:
      return x <= X_CENTER_RIGHT - X_HYSTERESIS ? initialXColumn(x)
                                                : XColumn::Right;
    case XColumn::Unknown:
    default:
      return initialXColumn(x);
  }
}

YRow updateYRow(YRow current, uint16_t y) {
  switch (current) {
    case YRow::Bottom:
      return y >= Y_BOTTOM_NEUTRAL + Y_HYSTERESIS ? initialYRow(y)
                                                  : YRow::Bottom;
    case YRow::Neutral:
      if (y < Y_BOTTOM_NEUTRAL - Y_HYSTERESIS) {
        return YRow::Bottom;
      }
      if (y > Y_NEUTRAL_TOP + Y_HYSTERESIS) {
        return YRow::Top;
      }
      return YRow::Neutral;
    case YRow::Top:
      return y <= Y_NEUTRAL_TOP - Y_HYSTERESIS ? initialYRow(y) : YRow::Top;
    case YRow::Unknown:
    default:
      return initialYRow(y);
  }
}

uint8_t gearFromPosition(XColumn column, YRow row, uint8_t reverse) {
  if (row == YRow::Neutral) {
    return 0;
  }

  if (column == XColumn::Left && row == YRow::Top) {
    return 1;
  }
  if (column == XColumn::Left && row == YRow::Bottom) {
    return 2;
  }
  if (column == XColumn::Center && row == YRow::Top) {
    return 3;
  }
  if (column == XColumn::Center && row == YRow::Bottom) {
    return 4;
  }
  if (column == XColumn::Right && row == YRow::Top) {
    return 5;
  }
  if (column == XColumn::Right && row == YRow::Bottom) {
    return reverse ? 7 : 6;
  }

  return 0;
}

const char *gearName(uint8_t gear) {
  switch (gear) {
    case 1:
      return "1";
    case 2:
      return "2";
    case 3:
      return "3";
    case 4:
      return "4";
    case 5:
      return "5";
    case 6:
      return "6";
    case 7:
      return "R";
    default:
      return "N";
  }
}

void releaseGearButton(uint8_t gear) {
  if (gear >= 1 && gear <= 7) {
    Gamepad.releaseButton(gear - 1);
  }
}

void pressGearButton(uint8_t gear) {
  if (gear >= 1 && gear <= 7) {
    Gamepad.pressButton(gear - 1);
  }
}

void releaseAllGearButtons() {
  for (uint8_t gear = 1; gear <= 7; ++gear) {
    releaseGearButton(gear);
  }
}

void applyGear(uint8_t previousGear, uint8_t newGear) {
  if (previousGear == newGear) {
    return;
  }

  if (previousGear != 0) {
    releaseGearButton(previousGear);
    delay(1);
  } else {
    releaseAllGearButtons();
  }

  if (newGear != 0) {
    pressGearButton(newGear);
  }
}

void printGearChange(const Reading &reading, uint8_t gear) {
  Serial.printf("X=%04u Y=%04u REV=%u GEAR=%s\r\n", reading.x, reading.y,
                reading.reverse, gearName(gear));
}

}  // namespace

void setup() {
  Serial.begin(115200);
  delay(1200);

  pinMode(PIN_X, INPUT);
  pinMode(PIN_Y, INPUT);
  pinMode(PIN_REVERSE, INPUT_PULLUP);

  analogReadResolution(12);
  analogSetPinAttenuation(PIN_X, ADC_11db);
  analogSetPinAttenuation(PIN_Y, ADC_11db);

  USB.productName("Logitech H-Shifter");
  USB.manufacturerName("Codex");
  Gamepad.begin();
  USB.begin();

  releaseAllGearButtons();

  Serial.println();
  Serial.println("Logitech H-Shifter ESP32-S3 USB HID Gamepad");
  Serial.println("Buttons: 1..6 = gears 1..6, 7 = Reverse, Neutral = none");
  Serial.println("Debug prints only when debounced detected gear changes.");
}

void loop() {
  static XColumn xColumn = XColumn::Unknown;
  static YRow yRow = YRow::Unknown;
  static uint8_t candidateGear = 255;
  static uint8_t activeGear = 255;
  static uint32_t candidateSinceMs = 0;

  const Reading reading = readFiltered();
  xColumn = updateXColumn(xColumn, reading.x);
  yRow = updateYRow(yRow, reading.y);

  const uint8_t detectedGear =
      gearFromPosition(xColumn, yRow, reading.reverse);
  const uint32_t now = millis();

  if (detectedGear != candidateGear) {
    candidateGear = detectedGear;
    candidateSinceMs = now;
  }

  if (activeGear == 255 ||
      (candidateGear != activeGear && now - candidateSinceMs >= DEBOUNCE_MS)) {
    const uint8_t previousGear = activeGear == 255 ? 0 : activeGear;
    applyGear(previousGear, candidateGear);
    activeGear = candidateGear;
    printGearChange(reading, activeGear);
  }

  delay(LOOP_DELAY_MS);
}

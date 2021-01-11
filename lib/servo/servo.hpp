#ifndef SERVO_HPP
#define SERVO_HPP

#include <Arduino.h>

template <class T>
static constexpr T spow(T base, T exp) noexcept
{
  //static_assert(exp >= 0, "Exponent must not be negative");
  return exp <= 0 ? 1
                  : exp == 1 ? base
                             : base * spow(base, exp - 1);
}

class M5Servo
{
protected:
  constexpr static int LEDC_SERVO_FREQ = 50;
  constexpr static float LEDC_UNIT_TIME_MS = 1000 / LEDC_SERVO_FREQ;
  constexpr static int LEDC_TIMER_BIT = 16;
  constexpr static int LEDC_FULL_BIT = spow<int>(2, LEDC_TIMER_BIT);
  int _chennel;
  int _pin;
  int _angle;
  float MIN_WIDTH_MS;
  float MAX_WIDTH_MS;

  int count(int angle)
  {
    float ratio = (angle + 90) / 180.0;
    return (int)(LEDC_FULL_BIT * (MIN_WIDTH_MS + ratio * (MAX_WIDTH_MS - MIN_WIDTH_MS)) / LEDC_UNIT_TIME_MS);
  }

public:
  /**
   * constructor
   * @param channnel PWM chennel (0~15)
   * @param pin signal pin
   */
  M5Servo(int channel, int pin, float min_width_ms = 0.9, float max_width_ms = 2.0)
    : _chennel(channel)
    , _pin(pin)
    , MIN_WIDTH_MS(min_width_ms)
    , MAX_WIDTH_MS(max_width_ms)
  {
    pinMode(pin, OUTPUT);
    ledcSetup(channel, LEDC_SERVO_FREQ, LEDC_TIMER_BIT);
    ledcAttachPin(_pin, channel);
  }

  /**
   * send reference angle to servo
   * @param angle reference angle (-90~+90 [deg])
   */
  void write(int angle)
  {
    _angle = angle;
    ledcWrite(_chennel, count(angle));
  }

  int read()
  {
    return _angle;
  }

  void maintenance(int angle, int delay_ms = 1000)
  {
    this->write(angle);
    delay(delay_ms);
    this->write(-angle);
    delay(delay_ms);
    this->write(0);
  }
};

#endif
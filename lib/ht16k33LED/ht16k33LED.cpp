#include <M5Stack.h>
#include <Wire.h>
#include "ht16k33LED.hpp"

using namespace ht16k33LED;

// 複数定義しないようにstatic変数はここで実体を定義
std::map<uint8_t, bool> Led::_initialized;

Led::Led(uint8_t id, uint8_t address, bool do_wire_begin)
    : _address(address), _id(id)
{
  Serial.printf("Led generate id=%d\n", id);
  if (do_wire_begin)
  {
    Serial.println("wire.begin()");
    Wire.begin();
  }
}

void Led::init()
{
  Serial.printf("Led init() called id=%d, address=%x\n", this->_id, this->_address);
  if (_initialized.count(_address))
  {
    return;
  }
  Serial.printf("Led init() run id=%d, address=%x\n", this->_id, this->_address);
  // システムオシレータON
  Wire.beginTransmission(_address);
  Wire.write(0x21);
  Wire.endTransmission();
  // 点滅OFF
  Wire.beginTransmission(_address);
  Wire.write(0x81);
  Wire.endTransmission();
  // 明るさ1(0-15)
  Wire.beginTransmission(_address);
  Wire.write(0xE1);
  Wire.endTransmission();
  // 全消灯
  Wire.beginTransmission(_address);
  Wire.write(0x00);
  for (int i = 0; i < 8; i++)
  {
    Wire.write(0x00);
    Wire.write(0x00);
  }
  Wire.endTransmission();
  _initialized[_address] = true;
}

void Led::write_rgb(uint8_t r, uint8_t g, uint8_t b)
{
  uint16_t row = 0x0000 | (r << (_id * 3)) | (b << (_id * 3 + 1)) | (g << (_id * 3 + 2));
  Wire.beginTransmission(_address);
  Wire.write(_id * 2);
  Wire.write(static_cast<uint8_t>(row & 0x00FF));
  Wire.write((row >> 8));
  Wire.endTransmission();
}

void Led::write_row(uint8_t com, uint8_t row1, uint8_t row2)
{
  Wire.beginTransmission(_address);
  Wire.write(com);
  Wire.write(row1);
  Wire.write(row2);
  Wire.endTransmission();
}

void Led::write_color(Color color)
{
  std::array<uint8_t, 3> color_array = _color_array(color);
  this->write_rgb(color_array[0], color_array[1], color_array[2]);
}

void Led::clear()
{
  write_rgb(0, 0, 0);
}

std::array<uint8_t, 3> Led::_color_array(Color color) const
{
  std::map<Color, std::array<uint8_t, 3>> dict{
      {Color::clear, {0, 0, 0}},
      {Color::red, {1, 0, 0}},
      {Color::green, {0, 1, 0}},
      {Color::blue, {0, 0, 1}}};
  try
  {
    return dict.at(color);
  }
  catch (std::out_of_range &)
  {
  }
  return dict.at(Color::clear);
}

void Led::maintenance(int delay_ms)
{
  unsigned count = 0;
  for (int i = 0; i < 3; i++)
  {
    if (count % 3 == 0)
    {
      this->write_color(ht16k33LED::red);
    }
    else if (count % 3 == 1)
    {
      this->write_color(ht16k33LED::green);
    }
    else
    {
      this->write_color(ht16k33LED::blue);
    }
    count++;
    delay(delay_ms);
  }
  this->clear();
}

void Led::blink(Color color, int times, int delay_ms){
  for(int i = 0; i < times; i++){
    this->write_color(color);
    delay(delay_ms);
    this->clear();
    delay(delay_ms);
  }
}
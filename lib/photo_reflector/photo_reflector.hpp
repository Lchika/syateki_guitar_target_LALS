/**
 * @brief フォトリフレクタ用クラス
 */

#ifndef PHOTO_REFLECTOR_HPP
#define PHOTO_REFLECTOR_HPP

/**
 * @class PhotoReflector
 * @brief フォトリフレクタ用クラス
 */
class PhotoReflector
{
private:
  int _pin = 0;
  // この値より低い値が読み取れたら接近中と判断する
  int _close_ref_value = 3500;

public:
  PhotoReflector(int pin, int close_ref_value = 3500)
    :_pin(pin), _close_ref_value(close_ref_value)
  {
    pinMode(pin, INPUT);
  }
  ~PhotoReflector(){}
  int value(void)
  {
    return analogRead(_pin);
  }
  bool is_close(void)
  {
    return (analogRead(_pin) < _close_ref_value);
  }
};

#endif
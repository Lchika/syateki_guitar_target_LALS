/**
 * @brief モータ制御用クラス
 */

#ifndef MOTOR_HPP
#define MOTOR_HPP

enum Direction {
  UP = (0),
  DOWN,
  NOCHANGE
};

/**
 * @class Motor
 * @brief モータ制御用クラス
 */
class Motor
{
private:
  int _ref_pin = 0;
  int _in1_pin = 0;
  int _in2_pin = 0;
  int _power = 0;
  Direction _direction = Direction::UP;

public:
  Motor(int ref_pin, int in1_pin, int in2_pin)
    :_ref_pin(ref_pin), _in1_pin(in1_pin), _in2_pin(in2_pin)
  {
    // ピン設定初期化
    pinMode(in1_pin, OUTPUT);
    pinMode(in2_pin, OUTPUT);
    digitalWrite(in1_pin, LOW);
    digitalWrite(in2_pin, LOW);
    dacWrite(ref_pin, 0);
  }
  ~Motor() {}
  bool set_power(int power, Direction direction)
  {
    if(power < 0 || power > 255){
      return false;
    }
    if(power == _power &&
      (direction == Direction::NOCHANGE || direction == _direction)){
      // 変化がない時は処理負荷軽減のため何もせずreturn
      return true;
    }
    // TODO: ここらへんもうちょっときれいにできそう
    if(direction != Direction::NOCHANGE){
      _direction = direction;
    }
    dacWrite(_ref_pin, power);
    if(_direction == Direction::DOWN){
      digitalWrite(_in1_pin, HIGH);
      digitalWrite(_in2_pin, LOW);
    }else{
      digitalWrite(_in2_pin, HIGH);
      digitalWrite(_in1_pin, LOW);
    }
    _power = power;
    return true;
  }
  int get_power(void)
  {
    return _power;
  }
};

#endif
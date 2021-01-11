#include <memory>
#include <Arduino.h>
#include <M5Stack.h>
#include <LovyanGFX.hpp>
#include <motor.hpp>
#include <photo_reflector.hpp>
#include <servo.hpp>
#include <ht16k33LED.hpp>
#include "Targets.hpp"
#include "debug.h"

/*
struct RotationServoPhase
{
  unsigned long millis_phase_change = 0;
  bool is_reversing = false;
  unsigned long interval = 0;
  int current_angle = 0;
};
*/

static void on_init();
static void on_receive_ir(int target_id, bool is_alive);
static void on_not_receive_ir(int target_id, bool is_alive);
static void on_hit(int target_id, int gun_id);
static void maintenance();
static void init_lcd();
static void show_motor_value(int power);
static void update_motor(int motor_power);
static void show_reflector_values(int top, int bottom);
static void clear_leds();
static ht16k33LED::Color gun_id2color(int gun_id);
static void update_servos();
//static void update_rotation_servo(M5Servo &servo, RotationServoPhase &phase);
static long update_normal_servo(M5Servo &servo, long millis_angle_change);

// まとユニット番号、この番号によってIPアドレスが決まるため、他とかぶってはいけない
static constexpr int UNIT_ID = 2;
// このユニットに紐づく赤外線受光モジュールの数
static constexpr int TARGET_NUM = 9;

static constexpr int PIN_MOTOR_REF = 26;
static constexpr int PIN_MOTOR1 = 16;
static constexpr int PIN_MOTOR2 = 17;
static constexpr int PIN_BOTTOM_REFLECTOR = 35;
static constexpr int PIN_TOP_REFLECTOR = 36;
static constexpr int PIN_SERVO_PICK = 2;    // 起動設定に関係するピンなので注意
static constexpr int PIN_SERVO_VOLUMES = 5; // 起動設定に関係するピンなので注意

// Targetsクラスのインスタンスをglobalで定義する
static Targets targets(on_init, on_receive_ir, on_not_receive_ir, on_hit);
static int motor_power = 0;
static LGFX lcd;
static Motor motor(PIN_MOTOR_REF, PIN_MOTOR1, PIN_MOTOR2);
static PhotoReflector bottom_reflector(PIN_BOTTOM_REFLECTOR);
static PhotoReflector top_reflector(PIN_TOP_REFLECTOR);
static M5Servo servo_pick(0, PIN_SERVO_PICK, 0.5, 2.4);
static M5Servo servo_volumes(1, PIN_SERVO_VOLUMES, 0.5, 2.4);
static ht16k33LED::Led leds[TARGET_NUM] = {
    ht16k33LED::Led(0),
    ht16k33LED::Led(1),
    ht16k33LED::Led(2),
    ht16k33LED::Led(3),
    ht16k33LED::Led(4),
    ht16k33LED::Led(0, 0x71),
    ht16k33LED::Led(1, 0x71),
    ht16k33LED::Led(2, 0x71),
    ht16k33LED::Led(3, 0x71)};
//static RotationServoPhase servo_pick_phase{};
static long millis_nservo_angle_change[2] = {0, 0};

void setup()
{
  // M5Stack関係の初期化、ここらへんはお好みで
  M5.begin();
  dacWrite(25, 0);
  M5.Power.begin();

  // まと関係の初期化、M5.begin() or Serial.begin() の後に行う
  targets.begin(UNIT_ID, TARGET_NUM, on_hit);

  // 赤外線受光モジュールとの疎通確認が可能
  std::vector<int> error_target_ids = targets.get_error_targets();
  for (const auto &id : error_target_ids)
  {
    DebugPrint("<ERROR> failed to connect target[%d]", id);
  }

  for (auto &led : leds)
  {
    led.init();
  }

  // 動作の確認
  maintenance();

  init_lcd();
  show_motor_value(motor_power);
}

void loop()
{
  // M5Stack関係の更新処理、ボタンを使わないなら多分いらない
  M5.update();
  // まと関係の更新処理、ここでHTTPリクエストの処理をしたり、まとの演出処理をやっている
  targets.update();

  update_servos();

  int motor_power_diff = 10;
  if (M5.BtnA.isPressed())
  {
    if (motor_power - motor_power_diff >= 0)
    {
      motor_power -= motor_power_diff;
    }
  }
  if (M5.BtnC.isPressed())
  {
    if (motor_power + motor_power_diff <= 255)
    {
      motor_power += motor_power_diff;
    }
  }
  update_motor(motor_power);

  show_reflector_values(top_reflector.value(), bottom_reflector.value());

  delay(100);
}

// ゲーム開始毎の初期化処理、LED消したり、動きを元に戻したりを想定
static void on_init()
{
  DebugPrint("on_init() start");
  clear_leds();
}

// 赤外線を受光した時の処理、引数で対象のまと番号(赤外線受光モジュールのロータリースイッチの値)がとれるので、まと毎に違う処理もできる
// この関数はtargets.update()を呼ばれたタイミングで赤外線を受光していたら実行される
static void on_receive_ir(int target_id, bool is_alive)
{
  if(is_alive){
    leds[target_id].write_color(ht16k33LED::red);
  }
}

// 赤外線を受光していない時の処理、引数で対象のまと番号(赤外線受光モジュールのロータリースイッチの値)がとれるので、まと毎に違う処理もできる
// この関数はtargets.update()を呼ばれたタイミングで赤外線を受光していなかったら実行される
static void on_not_receive_ir(int target_id, bool is_alive)
{
  if(is_alive){
    leds[target_id].clear();
  }
}

// 弾が当たった時の処理、引数で対象のまと番号(赤外線受光モジュールのロータリースイッチの値)がとれるので、まと毎に違う処理もできる
static void on_hit(int target_id, int gun_id)
{
  DebugPrint("on_hit() target_id=%d", target_id);
  ht16k33LED::Color color = gun_id2color(gun_id);
  // LED点滅
  leds[target_id].blink(color, 3, 300);
  // 弾が一度当たったまとのLEDは点灯しっぱなしにしておく
  leds[target_id].write_color(color);
}

// 一通り動かしてみて動作確認する
static void maintenance()
{
  // フルカラーLEDのメンテナンス
  for (auto &led : leds)
  {
    led.maintenance(500);
  }

  // サーボのメンテナンス
  servo_pick.maintenance(10, 500);
  servo_volumes.maintenance(10, 500);
}

static void init_lcd()
{
  lcd.init();
  lcd.setBrightness(100);
  lcd.clear();
  return;
}

static void show_motor_value(int power)
{
  lcd.setCursor(10, 20);
  lcd.printf("motor: power=%4d", power);
  return;
}

static void update_motor(int motor_power)
{
  Direction direction = Direction::NOCHANGE;
  if (bottom_reflector.is_close())
  {
    direction = Direction::UP;
    if (top_reflector.is_close())
    {
      // 両方のフォトリフレクタが反応していたら念のためモータは止める
      motor_power = 0;
    }
  }
  else if (top_reflector.is_close())
  {
    direction = Direction::DOWN;
  }
  motor.set_power(motor_power, direction);
  show_motor_value(motor_power);
}

static void show_reflector_values(int top, int bottom)
{
  lcd.setCursor(10, 10);
  lcd.printf("photo reflector: top=%4d, bottom=%4d", top, bottom);
  return;
}

static void clear_leds()
{
  for (auto &led : leds)
  {
    led.clear();
  }
}

static ht16k33LED::Color gun_id2color(int gun_id)
{
  std::map<int, ht16k33LED::Color> dict{
      {1, ht16k33LED::Color::red},
      {2, ht16k33LED::Color::blue}};
  try
  {
    return dict.at(gun_id);
  }
  catch (std::out_of_range &)
  {
  }
  return ht16k33LED::Color::clear;
}

static void update_servos()
{
  //update_rotation_servo(servo_pick, servo_pick_phase);
  millis_nservo_angle_change[0] = update_normal_servo(servo_pick, millis_nservo_angle_change[0]);
  millis_nservo_angle_change[1] = update_normal_servo(servo_volumes, millis_nservo_angle_change[1]);
}

/*
static void update_rotation_servo(M5Servo &servo, RotationServoPhase &phase)
{
  unsigned long now = millis();
  if (now > phase.millis_phase_change)
  {
    if (phase.is_reversing)
    {
      long interval = random(1000, 3001);
      phase.millis_phase_change = now + interval;
      phase.interval = interval;
      int angle = static_cast<int>(random(-10, 11));
      servo.write(angle);
      DebugPrint("rotation servo angle: %d", angle);
      phase.current_angle = angle;
      phase.is_reversing = false;
    }
    else
    {
      phase.millis_phase_change = now + phase.interval;
      servo.write(-phase.current_angle);
      DebugPrint("rotation servo angle(reverse): %d", -phase.current_angle);
      phase.current_angle *= -1;
      phase.is_reversing = true;
    }
  }
}
*/

static long update_normal_servo(M5Servo &servo, long millis_angle_change)
{
  long next_millis_angle_change = millis_angle_change;
  unsigned long now = millis();
  if (now > millis_angle_change)
  {
    next_millis_angle_change = millis_angle_change + random(1000, 3001);
    int angle = static_cast<int>(random(-90, 91));
    servo.write(angle);
    DebugPrint("normal servo angle: %d", angle)
  }
  return next_millis_angle_change;
}
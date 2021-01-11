#ifndef TARGET_HPP
#define TARGET_HPP

#include <memory>
#include "IrReceiver.hpp"

class Target
{
public:
  // idは0始まり、赤外線受光モジュールのロータリースイッチの値と対応している。
  Target(int id) : _id(id)
  {
    _irReceiver.reset(new IrReceiver(id));
  }
  bool is_connected() const
  {
    return _irReceiver->is_connected();
  }
  bool is_recieve_ir() const
  {
    if(_irReceiver->read() > 0){
      return true;
    }
    return false;
  }
  int get_id() const
  {
    return _id;
  }
  byte get_gun_num() const
  {
    return _irReceiver->read();
  }
  bool is_alive = true;

private:
  int _id;
  std::unique_ptr<IrReceiver> _irReceiver;
};

#endif
#include "Pc98MouseReportParser.h"

class BskBw120aMParser : public Pc98MouseReportParser {
  
  public:
    BskBw120aMParser();
  protected:
    bool ParseMouseData(MOUSEINFO_EX &pmi,uint32_t len, uint8_t *buf);
};

//コンストラクタでパラメーターを設定する
BskBw120aMParser::BskBw120aMParser(){
  x_limmit = 999;
  y_limmit = 999;
}

bool BskBw120aMParser::ParseMouseData(MOUSEINFO_EX &pmi,uint32_t len, uint8_t *buf){
  //データ長 3byte
  if(len != 3) return false;

  //ボタン
  uint8_t btn = buf[0];
  pmi.bmLeftButton    = ((btn & 0x01) == 0x01 ) ? 1:0;
  pmi.bmRightButton   = ((btn & 0x02) == 0x02 ) ? 1:0;
  pmi.bmMiddleButton  = ((btn & 0x04) == 0x04 ) ? 1:0;

  //X方向の移動
  uint8_t x_val = buf[1];
  bool x_dire = (x_val <= 128 ) ? true : false;  //右向きならtrue;
  if(x_dire){
    pmi.dX = x_val;  
  }else{
    pmi.dX = - (256 - x_val);
  }

  //Y方向の移動
  uint8_t y_val = buf[2];
  bool y_dire = (y_val <= 128 ) ? true : false;  //下向きならtrue;
  if(y_dire){
    pmi.dY = y_val;  
  }else{
    pmi.dY = - (256 - y_val);
  }

  //USB_HID_PROTOCOL_MOUSEではホイールスクロール情報はない  
  //HidComposite(&Usb,true);としてREPORTER PROTOCOLを有効にすると
  //スクロール情報取得はできるがもっさりしすぎで実用に耐えない
  //このマウス固有の問題か？
  pmi.wheel = 0; 
 
  return true;
}
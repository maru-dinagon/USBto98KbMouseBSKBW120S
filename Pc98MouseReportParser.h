#define XA 27
#define XB 14
#define YA 32
#define YB 33
#define LB 25
#define RB 26

//#define MOUSE_DEBUG

struct MOUSEINFO_EX {
    uint8_t bmLeftButton = 0;
    uint8_t bmRightButton = 0;
    uint8_t bmMiddleButton = 0;
    int8_t wheel = 0; // 0:動きなし 1:UP -1:DOWN
    int8_t dX = 0;
    int8_t dY = 0;
};

class Pc98MouseReportParser : public HIDReportParser {

  //USBマウスの生データを出力するかどうか
  static const bool OUTPUT_MOUSE_DATA = false;
  
  private:
    MOUSEINFO_EX	prev_mInfo;

    int stateB = 0; //L,R,Midボタンの状態
    
    //98マウスの現在のパルスの状態を保持する
    int X_state = 0;
    int Y_state = 0;

    void updateMouseBtn();
    //98マウスのX方向への移動(1パルス分 di=trueで右方向)
    void MoveXPc98Mouse(bool di);
    //98マウスのY方向への移動(1パルス分 di=trueで下方向)
    void MoveYPc98Mouse(bool di);
  

  public:

    void Parse(USBHID *hid, bool is_rpt_id, uint8_t len, uint8_t *buf); //要HID_RPT_PROTOCOL
    void setUpBusMouse();

  protected:
    /*
    lmmitは最大移動量の設定加速限界を決定する(Defは最大加速)
    */
    //パラメータ(既定値)
    uint8_t x_limmit = 9999;
    uint8_t y_limmit = 9999;

    //各種マウスのデータ解析関数(要オーバーロード)
    virtual bool ParseMouseData(MOUSEINFO_EX &pmi,uint32_t len, uint8_t *buf){ return false; };

    //イベント
    void OnMouseMove(MOUSEINFO_EX *mi);
    void OnLeftButtonUp(MOUSEINFO_EX *mi);
    void OnLeftButtonDown(MOUSEINFO_EX *mi);
    void OnRightButtonUp(MOUSEINFO_EX *mi);
    void OnRightButtonDown(MOUSEINFO_EX *mi);
    void OnMiddleButtonUp(MOUSEINFO_EX *mi);
    void OnMiddleButtonDown(MOUSEINFO_EX *mi);
    void OnScrollDown(MOUSEINFO_EX *mi);
    void OnScrollUp(MOUSEINFO_EX *mi);
};

void Pc98MouseReportParser::Parse(USBHID *hid, bool is_rpt_id, uint8_t len, uint8_t *buf){
  
  /*
    ParseMouseData()をオーバーライドすることで各種マウスに対応する実装を行う
    オーバーライドしなければfalseを常に返し生データを吐き出す
    このままでは各種イベントは起きないので注意!
  */

  MOUSEINFO_EX pmi;
  bool f = ParseMouseData(pmi,len,buf);
  if(!f || OUTPUT_MOUSE_DATA){
    if (len && buf)  {
      Serial.print(F("MouseData"));
      Serial.print(F(": "));
      for (uint8_t i = 0; i < len; i++) {
          if (buf[i] < 16) Serial.print(F("0"));
          Serial.print(buf[i], HEX);
          Serial.print(F(" "));
      }
      Serial.println("");
    }
  }
  if(!f) return;

	if (prev_mInfo.bmLeftButton == 0 && pmi.bmLeftButton == 1)
		OnLeftButtonDown(&pmi);

	if (prev_mInfo.bmLeftButton == 1 && pmi.bmLeftButton == 0)
		OnLeftButtonUp(&pmi);

	if (prev_mInfo.bmRightButton == 0 && pmi.bmRightButton == 1)
		OnRightButtonDown(&pmi);

	if (prev_mInfo.bmRightButton == 1 && pmi.bmRightButton == 0)
		OnRightButtonUp(&pmi);

	if (prev_mInfo.bmMiddleButton == 0 && pmi.bmMiddleButton == 1)
		OnMiddleButtonDown(&pmi);

	if (prev_mInfo.bmMiddleButton == 1 && pmi.bmMiddleButton == 0)
		OnMiddleButtonUp(&pmi);

	if(pmi.wheel != 0){
    ( pmi.wheel > 0 ) ? OnScrollUp(&pmi) : OnScrollDown(&pmi); 
  }
  
  if (pmi.dX != 0 || pmi.dY != 0)
		OnMouseMove(&pmi);

  prev_mInfo = pmi;

}

//マウスボタンデータアップデート
void Pc98MouseReportParser::updateMouseBtn(){

#ifdef MOUSE_DEBUG  
  Serial.print("stateB = ");
  Serial.println(stateB,HEX);
#endif

// Button
    switch (stateB) {
      case 0x00:
        pinMode(LB, INPUT);
        pinMode(RB, INPUT);
        break;
      case 0x01:
        pinMode(LB, OUTPUT);
        pinMode(RB, INPUT);
        break;
      case 0x02:
        pinMode(LB, INPUT);
        pinMode(RB, OUTPUT);
        break;
      case 0x03:
        pinMode(LB, OUTPUT);
        pinMode(RB, OUTPUT);
        break;
    }
}

//98バスマウス通信線の初期化
void Pc98MouseReportParser::setUpBusMouse(){
  pinMode(XA, INPUT);
  pinMode(XB, INPUT);
  pinMode(YA, INPUT);
  pinMode(YB, INPUT);
  pinMode(LB, INPUT);
  pinMode(RB, INPUT);
  digitalWrite(XA, LOW);
  digitalWrite(XB, LOW);
  digitalWrite(YA, LOW);
  digitalWrite(YB, LOW);
  digitalWrite(LB, LOW);
  digitalWrite(RB, LOW);
}

void Pc98MouseReportParser::OnMouseMove(MOUSEINFO_EX *mi){
#ifdef MOUSE_DEBUG  
  Serial.print("dx=");
  Serial.print(mi->dX, DEC);
  Serial.print(" dy=");
  Serial.println(mi->dY, DEC);
#endif

  bool X_prev = (mi->dX >= 0); //現在カウントしているX軸方向(true:+ false:-)
  bool Y_prev = (mi->dY >= 0); //現在カウントしているY軸方向(true:+ false:-)

  uint8_t X_count = abs(mi->dX);  //X軸方向の移動量のカウンター
  uint8_t Y_count = abs(mi->dY);  //Y軸方向の移動量のカウンター

  // Limiter
  X_count = X_count * 1;
  Y_count = Y_count * 1;

  if (X_count > x_limmit) X_count = x_limmit;
  if (Y_count > y_limmit) Y_count = y_limmit;

#ifdef MOUSE_DEBUG  
  Serial.print("X_count = ");
  Serial.print(X_count);  
  Serial.print(" Y_count = ");
  Serial.println(Y_count);  
#endif

  for(int i = 0 ; i < X_count ; i++) MoveXPc98Mouse(X_prev);
  for(int i = 0 ; i < Y_count ; i++) MoveYPc98Mouse(Y_prev);

};

// Cursor
// state      0 1 3 2
//          ___     ___
// A pulse |   |___|   |___
//            ___     ___
// B pulse   |   |___|   |___
//
// declease <--        --> increase
//
// For XA,XB the increasing pulse move the cursor rightward. (Positive for PS/2)
// For YA,YB the increasing pulse move the cursor downward. (Negative for PS/2)

void Pc98MouseReportParser::MoveXPc98Mouse(bool di){

  if(di){
    //+方向(右方向) 0->1->3->2
    if(X_state == 0){
      X_state = 1;
      pinMode(XA, OUTPUT);
      pinMode(XB, INPUT);
    }else if(X_state == 1){
      X_state = 3;
      pinMode(XA, OUTPUT);
      pinMode(XB, OUTPUT);
    }else if(X_state == 3){
      X_state = 2;
      pinMode(XA, INPUT);
      pinMode(XB, OUTPUT);
    }else if(X_state == 2){
      X_state = 0;
      pinMode(XA, INPUT);
      pinMode(XB, INPUT);
    }else{
      X_state = 0;
    }
  }else{
    //-方向(左方向) 2->3->1->0
    if(X_state == 2){
      X_state = 3;
      pinMode(XA, OUTPUT);
      pinMode(XB, OUTPUT);
    }else if(X_state == 3){
      X_state = 1;
      pinMode(XA, OUTPUT);
      pinMode(XB, INPUT);
    }else if(X_state == 1){
      X_state = 0;
      pinMode(XA, INPUT);
      pinMode(XB, INPUT);
    }else if(X_state == 0){
      X_state = 2;
      pinMode(XA, INPUT);
      pinMode(XB, OUTPUT);
    }else{
      X_state = 0;
    }
  }
  delayMicroseconds(150);
}

void Pc98MouseReportParser::MoveYPc98Mouse(bool di){

  if(di){
    //+方向(下方向) 0->1->3->2
    if(Y_state == 0){
      Y_state = 1;
      pinMode(YA, OUTPUT);
      pinMode(YB, INPUT);
    }else if(Y_state == 1){
      Y_state = 3;
      pinMode(YA, OUTPUT);
      pinMode(YB, OUTPUT);
    }else if(Y_state == 3){
      Y_state = 2;
      pinMode(YA, INPUT);
      pinMode(YB, OUTPUT);
    }else if(Y_state == 2){
      Y_state = 0;
      pinMode(YA, INPUT);
      pinMode(YB, INPUT);
    }else{
      Y_state = 0;
    }
  }else{
    //-方向(上方向) 2->3->1->0
    if(Y_state == 2){
      Y_state = 3;
      pinMode(YA, OUTPUT);
      pinMode(YB, OUTPUT);
    }else if(Y_state == 3){
      Y_state = 1;
      pinMode(YA, OUTPUT);
      pinMode(YB, INPUT);
    }else if(Y_state == 1){
      Y_state = 0;
      pinMode(YA, INPUT);
      pinMode(YB, INPUT);
    }else if(Y_state == 0){
      Y_state = 2;
      pinMode(YA, INPUT);
      pinMode(YB, OUTPUT);
    }else{
      Y_state = 0;
    }
  }
  delayMicroseconds(150);
}

void Pc98MouseReportParser::OnLeftButtonUp  (MOUSEINFO_EX *mi){
#ifdef MOUSE_DEBUG  
  Serial.println("L Butt Up");
#endif
  stateB = stateB & 0xFE;
  updateMouseBtn();
};

void Pc98MouseReportParser::OnLeftButtonDown (MOUSEINFO_EX *mi){
#ifdef MOUSE_DEBUG  
  Serial.println("L Butt Dn");
#endif
  stateB = stateB | 0x01;
  updateMouseBtn();
};

void Pc98MouseReportParser::OnRightButtonUp  (MOUSEINFO_EX *mi){
#ifdef MOUSE_DEBUG  
  Serial.println("R Butt Up");
#endif
  stateB = stateB & 0xFD;
  updateMouseBtn();
};

void Pc98MouseReportParser::OnRightButtonDown  (MOUSEINFO_EX *mi){
#ifdef MOUSE_DEBUG  
  Serial.println("R Butt Dn");
#endif
  stateB = stateB | 0x02;
  updateMouseBtn();
};

void Pc98MouseReportParser::OnMiddleButtonUp (MOUSEINFO_EX *mi){
#ifdef MOUSE_DEBUG  
  Serial.println("M Butt Up");
#endif
};

void Pc98MouseReportParser::OnMiddleButtonDown (MOUSEINFO_EX *mi){
#ifdef MOUSE_DEBUG  
  Serial.println("M Butt Dn");
#endif
};

void Pc98MouseReportParser::OnScrollUp (MOUSEINFO_EX *mi){
#ifdef MOUSE_DEBUG  
  Serial.println("ScrollUp");
#endif
};

void Pc98MouseReportParser::OnScrollDown (MOUSEINFO_EX *mi){
#ifdef MOUSE_DEBUG  
  Serial.println("ScrollDown");
#endif
};

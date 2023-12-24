#include "Keyconst.h"

//98側(miniDin8)ピン定義
#define RST 4 //リセット要求
#define RDY 34 //送信可能
#define RXD 22                                                                                                                                                                                                            //データ送信
#define RTY 39 //再送要求

//ボタンアレイ接続ピン定義
#define BUTTON_ARRAY_ADC_PIN 35

//LED接続ピン定義
#define KANA_LED 12
#define CAPS_LED 13
#define NUM_LED  15

//キーボードに対して送るコマンド定義
#define ACK              0xFA
#define NACK             0xFC

//#define KEY_B_DEBUG

HardwareSerial mySerial(1);

class KbdRptParser : public KeyboardReportParser
{
   private:
    uint8_t prev  = 0x00; //直前の送信キーの保持
    int kana_f = 0, caps_f = 0,  num_f = 1;
    bool repeat_func = true; //リピートキー機能の有効・無効(NumLockしてない状態でF12で切り替え)
    int repeat_df = 0; // 0:リピート可 1:リピート不可 
    int repeat_delay_time = 500;    // リピートするまでの時間500msec
    int repeat_speed_time = 40;     // リピート間隔40msec
    int repeat_delay[4] = { 250, 500, 500, 1000,};
    int last_downkey = 0xFF; //最後に押された98キースキャンコード
    int downkey_c = 0; //同時に押されているキーの数
    unsigned long down_mi = 0; //最後に押されたキーの時間の保持
    unsigned long repeat_mi = 0; //リピート間隔を処理する時間の保持
    boot_mode bootmode = NORMAL; //ブートモードの指定
    
    void pc98key_send(uint8_t data);
    void pc98key_command(void);
    void repeatKey_proc();
    boot_mode getBootMode();
    void SetLed();

  public:
    void setUp98Keyboard();
    void task();

  protected:
    void OnControlKeysChanged(uint8_t before, uint8_t after);
    void OnKeyDown  (uint8_t mod, uint8_t key);
    void OnKeyUp  (uint8_t mod, uint8_t key);
};

boot_mode KbdRptParser::getBootMode(){
  //アナログボタンアレイが接続されている電圧を計測する(5回サンプリング平均)
  int sample = 5;
  int vol = 0;
  for(int i = 0 ; i < sample ; i++){
    vol += analogReadMilliVolts(BUTTON_ARRAY_ADC_PIN);
  }
  vol = vol / sample;

  //Serial.print(vol);
  //Serial.println(" mV");  

  if(vol > 3000){
    //押していない(実測3181mv)
    //Serial.println("No Push");
    return NORMAL;
  }else
  if(vol > 2500){
    //ボタン1(2880mv)
    //Serial.println("Button1 Pushed");
    return SETUP_MENU;
  }else
  if(vol > 2000){
    //ボタン2(2270mv)
    //Serial.println("Button2 Pushed");
    return CRT_24;
  }else
  if(vol > 1500){
    //ボタン3(1800mv)
    //Serial.println("Button3 Pushed");
    return CRT_31;
  }else
  if(vol > 1000){
    //ボタン4(1340mv)
    //Serial.println("Button4 Pushed");
    return BIOS_REV;
  }else
  if(vol > 500){
    //ボタン5(820mv)
    //Serial.println("Button5 Pushed");
    return CPU_MSG;
  }else
  if(vol > 200){
    //ボタン6(300mv)
    //Serial.println("Button6 Pushed");
    return NO_MEMCHK;
  }else{
    //ボタンアレイが接続されていないか接触不良(200mV以下)
    //Serial.println("Not Commect Button Array");
    return NORMAL;    
  }
  
}

void KbdRptParser::SetLed(){
  digitalWrite(KANA_LED, kana_f ? HIGH : LOW);
  digitalWrite(CAPS_LED, caps_f ? HIGH : LOW);
  digitalWrite(NUM_LED , num_f  ? HIGH : LOW);
}

void KbdRptParser::setUp98Keyboard(){

  //ブートモードの取得
  pinMode(BUTTON_ARRAY_ADC_PIN, ANALOG);  
  bootmode = getBootMode();

  Serial.print("bootmode = ");
  Serial.println(bootmode);
  

  //LEDの設定
  pinMode(KANA_LED,OUTPUT);
  digitalWrite(KANA_LED, LOW);
  pinMode(CAPS_LED,OUTPUT);
  digitalWrite(CAPS_LED, LOW);
  pinMode(NUM_LED,OUTPUT);
  digitalWrite(NUM_LED, LOW);

  setCodeArray();
  
  pinMode(RST, INPUT);
  pinMode(RDY, INPUT);
  //pinMode(RXD, OUTPUT);
  pinMode(RTY, INPUT);

  //ハードウェアシリアル初期化
  //19.2kbps 8ビット 奇数パリティ ストップビット1ビット
  mySerial.begin(19200, SERIAL_8O1,RST, RXD);
}

void KbdRptParser::task(){
  //受信コマンドの処理
  pc98key_command();
  //キーリピート処理
  if(repeat_func) repeatKey_proc();
  //ブートモードの更新
  bootmode = getBootMode();

  //LED更新
  SetLed(); 
}

void KbdRptParser::pc98key_send(uint8_t data){

  while(digitalRead(RDY) == HIGH) delayMicroseconds(1); //送信不可より待機
  
  //if(digitalRead(RST) == LOW) delayMicroseconds(30);  //リセット要求
  
  if(digitalRead(RTY) == LOW){ // RTYオンなら直前のキーを再送信する
      mySerial.write(prev);
  }else{           //  現在キーを直前のキーに保存
      mySerial.write(data);
      prev = data;

#ifdef KEY_B_DEBUG
      Serial.print("->Send : ");
      Serial.print(data,HEX);
      Serial.println("");
#endif    
  }

  delayMicroseconds(500);
}

void KbdRptParser::pc98key_command(void){

  uint8_t c,up_c; //cの上位4bit保持用

  while(mySerial.available()>0){

    c = mySerial.read();
    up_c = c & 0xf0; //上位4bit

#ifdef KEY_B_DEBUG
      Serial.print("Read : ");
      Serial.print(c,HEX);
      Serial.println("");
#endif

    //ブートモード・コマンドの実行
    if(c == 0xFC && bootmode != NORMAL){
      uint8_t *command;
      int c_size = 0;        
      switch(bootmode){
        case SETUP_MENU:
          command = BOOTMODE_SETUP_MENU;
          c_size  = sizeof(BOOTMODE_SETUP_MENU) / sizeof(BOOTMODE_SETUP_MENU[0]);
          break;

        case CRT_24:
          command = BOOTMODE_CRT_24;
          c_size  = sizeof(BOOTMODE_CRT_24) / sizeof(BOOTMODE_CRT_24[0]);
          break;
          
        case CRT_31:
          command = BOOTMODE_CRT_31;
          c_size  = sizeof(BOOTMODE_CRT_31) / sizeof(BOOTMODE_CRT_31[0]);
          break;

        case BIOS_REV:
          command = BOOTMODE_BIOS_REV;
          c_size  = sizeof(BOOTMODE_BIOS_REV) / sizeof(BOOTMODE_BIOS_REV[0]);
          break;
        
        case CPU_MSG:
          command = BOOTMODE_CPU_MSG;
          c_size  = sizeof(BOOTMODE_CPU_MSG) / sizeof(BOOTMODE_CPU_MSG[0]);
          break;

        case NO_MEMCHK:
          command = BOOTMODE_NO_MEMCHK;
          c_size  = sizeof(BOOTMODE_NO_MEMCHK) / sizeof(BOOTMODE_NO_MEMCHK[0]);
          break;

      }
      //起動時コマンドの送信
      for(int i = 0 ; i < c_size ; i++){
        pc98key_send(command[i]);
        delayMicroseconds(500);
      }
      continue; 
     }

    //コマンドが0x9_以外はリセットかける。 ログでは0xFF,0x7_などが起動時に流れているが委細不明
    if(up_c != 0x90){
      kana_f = 0;
      caps_f = 0;
      //num_f = 0;

      repeat_df = 0;
      repeat_delay_time = 500;
      repeat_speed_time = 40;
      continue;   
    }

    // コマンド処理
    switch(c){
    case 0x95: // Windows,Appliキー制御
        pc98key_send(ACK);

        while(!mySerial.available());
        c = mySerial.read();
        // c=0x00 通常
        // c=0x03 Windowsキー,Applicationキー有効
        pc98key_send(ACK);
        break;
        
    case 0x96: // モード識別　取り敢えず通常モードを送信しておく.
        pc98key_send(ACK);
        delayMicroseconds(500);
        pc98key_send(0xA0);
        delayMicroseconds(500);
        pc98key_send(0x85);   // 0x85=通常変換モード, 0x86=自動変換モード
        break;
        
    case 0x9C:{ // キーボードリピート制御
        pc98key_send(ACK);

        while(!mySerial.available());
        c = mySerial.read();

        //上位4bitと下位4bitを入れ替え
        uint8_t t1 = c;
        uint8_t t2 = c;
        t1 <<= 4;
        t2 >>= 4;
        uint8_t d = t1 | t2;

        //リピートスピード
        uint8_t r_s = (d & 0b11111000) >> 3;
        if(r_s == 0){
          repeat_df = 1;
          repeat_delay_time = 0;
          repeat_speed_time = 0;
        }else{
           repeat_df = 0;
           repeat_delay_time = repeat_delay[(d & 0b00000110) >> 1];
           repeat_speed_time = 20 * r_s;           
        }
        
        /*
        if((c & 0b11111)==0){ // キーリピート禁止
           repeat_df=1;
        }else{
           repeat_df=0;
           repeat_delay_time = repeat_delay[(c & 0b01100000) >> 5];
           repeat_speed_time = 60 * (c&0b11111);           
        }
        */

#ifdef KEY_B_DEBUG
      Serial.print("9C-DATA : ");
      Serial.print(c,HEX);
      Serial.println("");
      Serial.print("repeat_df = ");
      Serial.println(repeat_df);
      Serial.print("repeat_delay_time  = ");
      Serial.println(repeat_delay_time );
      Serial.print("repeat_speed_time  = ");
      Serial.println(repeat_speed_time );
#endif    
        
        pc98key_send(ACK);
        break;
    }

    case 0x9D:   // LED 制御
        pc98key_send(ACK);
        while(!mySerial.available());
        c = mySerial.read();
        up_c = c & 0xf0; //上位4bit

#ifdef KEY_B_DEBUG
        Serial.print("9D-DATA : ");
        Serial.print(c,HEX);
        Serial.println("");
        Serial.print("9D-UP_DATA : ");
        Serial.print(up_c,HEX);
        Serial.println("");
#endif  

        if(up_c == 0x70){ //LED状態の通知
            //状態の記録
            //num_f  = ((c & 0x01) == 0x01);
            caps_f = ((c & 0x04) == 0x04);
            kana_f = ((c & 0x08) == 0x08);
            
            pc98key_send(ACK);
        
        }else if(up_c == 0x60){ // LED状態読み出し
            c = 0;
            //if(num_f)  c |= 0x01;
            if(caps_f) c |= 0x04;
            if(kana_f) c |= 0x08;
            pc98key_send(c);
        
        }else{
            pc98key_send(NACK);
        }
        break;
        
    case 0x9F:  // check keyboard type
        pc98key_send(ACK);
        delayMicroseconds(500);
        pc98key_send(0xA0);
        delayMicroseconds(500);
        pc98key_send(0x80);  // 新キーボードを送信
        break;

    default:
        // 不明コマンドは NACK で応答
        pc98key_send(NACK);
        break;            
    }    
    return;
  }
}

void KbdRptParser::repeatKey_proc(){

  if(last_downkey == 0xFF || repeat_df) {
    repeat_mi = 0;
    last_downkey = 0xFF;
    return;
  }

  if(millis() - down_mi >= repeat_delay_time){
    if(millis() - repeat_mi >= repeat_speed_time){

#ifdef KEY_B_DEBUG
      Serial.print("Repeat key :");
      Serial.print(last_downkey,HEX);
      Serial.println("");
#endif
      pc98key_send(last_downkey);
      repeat_mi = millis();
    }
  }
}

void KbdRptParser::OnKeyDown(uint8_t mod, uint8_t key)
{
#ifdef KEY_B_DEBUG
      Serial.print("OnKeyDown : ");
      Serial.print(key,HEX);
      Serial.println("");
#endif   

  //NumLock処理(98にはNumLock機能がないのでここでキー変換処理)
  if(key == USB_NUMLOCK){
    if(!num_f){
      //NumLockがONの時はテンキーの数字をそのまま出力
      num_f = true;
    }else{
      //NumLockがOFFの時は出力
      num_f = false;
    }
    return;
  }

  //半角・全角キーはIMEオンオフ
  if(key == USB_HANKAKUZENKAKU){
    pc98key_send(0x74);
    pc98key_send(0x35);
    pc98key_send(0x35 | 0x80);
    pc98key_send(0x74 | 0x80);
    return;
  }

  //NumLock OFFでF12キーが押された場合、キーリピート切り替え
  if(!num_f && key == USB_F12) repeat_func = !repeat_func;

  
  uint8_t c = get98Code(key,num_f);
  if(c == 0xFF) return;

  //CapsLock処理
  if(c == KEY98_CPSLK){
    if(!caps_f){
      caps_f = true;    
    }else{
      c = KEY98_CPSLK | R_CODE;
      caps_f = false;
    }
  }

  //かなキー処理
  if(c == KEY98_KANA){
    if(!kana_f){
      kana_f = true;    
    }else{
      c = KEY98_KANA | R_CODE;
      kana_f = false;
    }
  }
  
  pc98key_send(c);

    
  //キーリピート処理
  downkey_c ++;
  last_downkey = c;
  down_mi = millis();

}

void KbdRptParser::OnKeyUp(uint8_t mod, uint8_t key)
{
#ifdef KEY_B_DEBUG
      Serial.print("OnKeyUp : ");
      Serial.print(key,HEX);
      Serial.println("");
#endif   

  uint8_t c = get98Code(key,num_f);
  if(c == 0xFF) return;

  //キーリピート処理
  downkey_c --;
  if(downkey_c <= 0){
    last_downkey = 0xFF; 
    downkey_c = 0;
  }else{
    down_mi = millis();
  }
  
  //CapsLock,かなキーはOnKeyUpでコマンド送らない
  if(c == KEY98_CPSLK || c == KEY98_KANA) return;
  
  pc98key_send(c | R_CODE);

}


void KbdRptParser::OnControlKeysChanged(uint8_t before, uint8_t after) {

  MODIFIERKEYS beforeMod;
  *((uint8_t*)&beforeMod) = before;

  MODIFIERKEYS afterMod;
  *((uint8_t*)&afterMod) = after;

  //左Ctrl
  if (beforeMod.bmLeftCtrl != afterMod.bmLeftCtrl) {
    last_downkey = 0xFF;
    if(afterMod.bmLeftCtrl){
      Serial.println("LeftCtrl push");
      pc98key_send(KEY98_CTRL);  
    }else{
      Serial.println("LeftCtrl relase");
      pc98key_send(KEY98_CTRL | R_CODE);
    }
  }

  //左シフト
  if (beforeMod.bmLeftShift != afterMod.bmLeftShift) {
    if(afterMod.bmLeftShift){
#ifdef KEY_B_DEBUG
      Serial.println("LeftShift push");
#endif
      pc98key_send(KEY98_SHIFT);  
      last_downkey = 0xFF;
    }else{
#ifdef KEY_B_DEBUG
      Serial.println("LeftShift relase");
#endif
      pc98key_send(KEY98_SHIFT | R_CODE);
      down_mi = millis();
    }
  }

  //左ALT -> GRPH
  if (beforeMod.bmLeftAlt != afterMod.bmLeftAlt) {
    last_downkey = 0xFF;
    if(afterMod.bmLeftAlt){
#ifdef KEY_B_DEBUG
      Serial.println("LeftAlt push");
#endif
      pc98key_send(KEY98_GRPH);  
    }else{
#ifdef KEY_B_DEBUG
      Serial.println("LeftAlt relase");
#endif
      pc98key_send(KEY98_GRPH | R_CODE);
    }
  }
  
  //左Windowsボタン
  if (beforeMod.bmLeftGUI != afterMod.bmLeftGUI) {
    last_downkey = 0xFF;
    if(afterMod.bmLeftGUI){
#ifdef KEY_B_DEBUG
      Serial.println("LeftGUI push");
#endif
      pc98key_send(KEY98_WIN);  
    }else{
#ifdef KEY_B_DEBUG
      Serial.println("LeftGUI relase");
#endif
      pc98key_send(KEY98_WIN | R_CODE);
    }
  }

  //右Ctrl
  if (beforeMod.bmRightCtrl != afterMod.bmRightCtrl) {
    last_downkey = 0xFF;
    if(afterMod.bmRightCtrl){
#ifdef KEY_B_DEBUG
      Serial.println("RightCtrl push");
#endif
      pc98key_send(KEY98_CTRL);  
    }else{
#ifdef KEY_B_DEBUG
      Serial.println("RightCtrl relase");
#endif
      pc98key_send(KEY98_CTRL | R_CODE);
    }
  }
  
  //右シフト
  if (beforeMod.bmRightShift != afterMod.bmRightShift) {
    if(afterMod.bmRightShift){
#ifdef KEY_B_DEBUG
      Serial.println("RightShift push");
#endif
      pc98key_send(KEY98_SHIFT);  
      last_downkey = 0xFF;
    }else{
      Serial.println("RightShift relase");
      pc98key_send(KEY98_SHIFT | R_CODE);
      down_mi = millis();  
    }
  }

  //右ALT -> GRPH
  if (beforeMod.bmRightAlt != afterMod.bmRightAlt) {
    last_downkey = 0xFF;
    if(afterMod.bmRightAlt){
#ifdef KEY_B_DEBUG
      Serial.println("RightAlt push");
#endif
      pc98key_send(KEY98_GRPH);  
    }else{
#ifdef KEY_B_DEBUG
      Serial.println("RightAlt relase");
#endif
      pc98key_send(KEY98_GRPH | R_CODE);
    }
  }

  //右Windowsボタン
  if (beforeMod.bmRightGUI != afterMod.bmRightGUI) {
    last_downkey = 0xFF;
    if(afterMod.bmRightGUI){
#ifdef KEY_B_DEBUG
      Serial.println("RightGUI push");
#endif
      pc98key_send(KEY98_WIN);  
    }else{
#ifdef KEY_B_DEBUG      
      Serial.println("RightGUI relase");
#endif
      pc98key_send(KEY98_WIN | R_CODE);
    }
  }

}

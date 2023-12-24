#define NA_CODE 0xFF //98キーボードスキャンコードの未定義初期値

#define R_CODE 0x80 // 98キーコードのリリースの値

//98スキャンコード定義(必要なものだけ)
#define KEY98_SHIFT 0x70 //シフトキー
#define KEY98_CPSLK 0x71 //CapsLock
#define KEY98_KANA  0x72 //かなキー
#define KEY98_WIN   0x77 //Winキー
#define KEY98_GRPH  0x73 //GRPHキー
#define KEY98_CTRL  0x74 //CTRLキー

//USBキーボードスキャンコード定義(必要なものだけ)
#define USB_NUMLOCK        0x53 //NumLockキー
#define USB_HANKAKUZENKAKU 0x35 //半角全角漢字キー
#define USB_F12            0x45 //F12

uint8_t codeArray[0xFF];         //NumLock時のコード変換テーブル
uint8_t codeArrayNotLk[0xFF];    //NumLock解除時のコード変換テーブル

//ブートモード定義
enum boot_mode {
  NORMAL,     //通常起動(何も押さない)
  SETUP_MENU, //「HELP」セットアップメニュー起動
  CRT_24,     //「GRPH」+「1(ヌ)」 DOSのCRT出力・水平周波数を24.8kHzにする
  CRT_31,     //「GRPH」+「2(フ)」 DOSのCRT出力を31kHzにする
  BIOS_REV,   //「ESC」+「HELP」+「1」ＢＩＯＳリビジョン表示
  CPU_MSG,    // [CTRL] + [CAPS] + [カナ] + [GRPH]イニシャルテストファームウェアメッセージ（CPU情報表示）
  NO_MEMCHK   // [STOP] メモリチェックなしで起動する
};

static uint8_t BOOTMODE_SETUP_MENU[2] = {0x3F,0xBF};
static uint8_t BOOTMODE_CRT_24[4]     = {0x01,0x73,0x81,0x01};
static uint8_t BOOTMODE_CRT_31[4]     = {0x02,0x73,0x82,0x02};
static uint8_t BOOTMODE_BIOS_REV[5]   = {0x01,0x3F,0x00,0x80,0x00};
static uint8_t BOOTMODE_CPU_MSG[5]    = {0x73,0x74,0x72,0x71,0xF4};
static uint8_t BOOTMODE_NO_MEMCHK[3]  = {0x60,0xE0,0x60};


//USBスキャンコードから98スキャンコードに変換(NumLock状態でテーブル切り替え)
uint8_t get98Code(uint8_t key , bool num_f){
  if(num_f){
    return(codeArray[key]);
  }else{
    return(codeArrayNotLk[key]);
  }
}

//テーブル初期化
void setCodeArray()
{
  for(int i = 0 ; i < 0xFF ; i++){
    codeArray[i]      = NA_CODE;
    codeArrayNotLk[i] = NA_CODE;
  }

codeArray[0x04] = 0x1D;    // A   
codeArray[0x05] = 0x2D;    // B   
codeArray[0x06] = 0x2B;    // C  
codeArray[0x07] = 0x1F;    // D  
codeArray[0x08] = 0x12;    // E  
codeArray[0x09] = 0x20;    // F  
codeArray[0x0A] = 0x21;    // G  
codeArray[0x0B] = 0x22;    // H  
codeArray[0x0C] = 0x17;    // I  
codeArray[0x0D] = 0x23;    // J  
codeArray[0x0E] = 0x24;    // K  
codeArray[0x0F] = 0x25;    // L  
codeArray[0x10] = 0x2F;    // M  
codeArray[0x11] = 0x2E;    // N  
codeArray[0x12] = 0x18;    // O  
codeArray[0x13] = 0x19;    // P  
codeArray[0x14] = 0x10;    // Q  
codeArray[0x15] = 0x13;    // R  
codeArray[0x16] = 0x1E;    // S  
codeArray[0x17] = 0x14;    // T  
codeArray[0x18] = 0x16;    // U  
codeArray[0x19] = 0x2C;    // V  
codeArray[0x1A] = 0x11;    // W  
codeArray[0x1B] = 0x2A;    // X  
codeArray[0x1C] = 0x15;    // Y  
codeArray[0x1D] = 0x29;    // Z  
codeArray[0x1E] = 0x01;    // 1  
codeArray[0x1F] = 0x02;    // 2  
codeArray[0x20] = 0x03;    // 3  
codeArray[0x21] = 0x04;    // 4  
codeArray[0x22] = 0x05;    // 5   
codeArray[0x23] = 0x06;    // 6  
codeArray[0x24] = 0x07;    // 7  
codeArray[0x25] = 0x08;    // 8  
codeArray[0x26] = 0x09;    // 9  
codeArray[0x27] = 0x0A;    // 0  
codeArray[0x28] = 0x1C;    // CR -> ENTER  
codeArray[0x29] = 0x00;    // ESC  
codeArray[0x2A] = 0x0E;    // BS  
codeArray[0x2B] = 0x0F;    // TAB  
codeArray[0x2C] = 0x34;    // SPC  
codeArray[0x2D] = 0x0B;    // -  
codeArray[0x2E] = 0x0C;    // ^  
codeArray[0x2F] = 0x1A;    // @  
codeArray[0x30] = 0x1B;    // [  
codeArray[0x31] = 0xFF;    // ??  
codeArray[0x32] = 0x28;    // ]  
codeArray[0x33] = 0x26;    // ;  
codeArray[0x34] = 0x27;    // :  
codeArray[0x35] = 0xFF;    // KNJ -> CTRL+XFERを出力する（漢字入力モード)
codeArray[0x36] = 0x30;    // ,  
codeArray[0x37] = 0x31;    // .  
codeArray[0x38] = 0x32;    // /  
codeArray[0x39] = 0x71;    // CpsLk  
codeArray[0x3A] = 0x62;    // F1  
codeArray[0x3B] = 0x63;    // F2  
codeArray[0x3C] = 0x64;    // F3  
codeArray[0x3D] = 0x65;    // F4  
codeArray[0x3E] = 0x66;    // F5  
codeArray[0x3F] = 0x67;    // F6  
codeArray[0x40] = 0x68;    // F7   
codeArray[0x41] = 0x69;    // F8 
codeArray[0x42] = 0x6A;    // F9  
codeArray[0x43] = 0x6B;    // F10  
codeArray[0x44] = 0x52;    // F11 -> VF1  
codeArray[0x45] = 0x53;    // F12 -> VF2 
codeArray[0x46] = 0x61;    // PrtScr -> COPY  
codeArray[0x47] = 0xFF;    // ScrL  
codeArray[0x48] = 0x60;    // BRK -> STOP 
codeArray[0x49] = 0x38;    // INS  
codeArray[0x4A] = 0x3E;    // HOME -> HOMECLR 
codeArray[0x4B] = 0x36;    // PgUP  
codeArray[0x4C] = 0x39;    // DEL  
codeArray[0x4D] = 0x3F;    // END -> HELP 
codeArray[0x4E] = 0x37;    // PgDN  
codeArray[0x4F] = 0x3C;    // RIGHT  
codeArray[0x50] = 0x3B;    // LEFT  
codeArray[0x51] = 0x3D;    // DOWN  
codeArray[0x52] = 0x3A;    // UP  
codeArray[0x53] = 0xFF;    // NmLK  NumLKでパッドのデータを変換する
codeArray[0x54] = 0x41;    // /  
codeArray[0x55] = 0x45;    // *  
codeArray[0x56] = 0x40;    // -  
codeArray[0x57] = 0x49;    // +  
codeArray[0x58] = 0x1C;    // CR  
codeArray[0x59] = 0x4A;    // 1  
codeArray[0x5A] = 0x4B;    // 2  
codeArray[0x5B] = 0x4C;    // 3  
codeArray[0x5C] = 0x46;    // 4  
codeArray[0x5D] = 0x47;    // 5  
codeArray[0x5E] = 0x48;    // 6   
codeArray[0x5F] = 0x42;    // 7  
codeArray[0x60] = 0x43;    // 8  
codeArray[0x61] = 0x44;    // 9  
codeArray[0x62] = 0x4E;    // 0  
codeArray[0x63] = 0x50;    // .  
codeArray[0x64] = 0xFF;    //   
codeArray[0x65] = 0x79;    // Apl  
codeArray[0x87] = 0x33;    // ﾛ  
codeArray[0x88] = 0x72;    // KANA  
codeArray[0x89] = 0x0D;    // YEN  
codeArray[0x8A] = 0x35;    // CONV(変換) -> XFER  
codeArray[0x8B] = 0x51;    // NCONV(無変換) -> NFER  
codeArray[0xE0] = 0x74;    // LCTRL  
codeArray[0xE1] = 0x70;    // LSHIFT  
codeArray[0xE2] = 0x73;    // LALT -> GRAH 
codeArray[0xE3] = 0x77;    // LWIN  
codeArray[0xE4] = 0x74;    // RCTRL  
codeArray[0xE5] = 0x70;    // RSHFT  
codeArray[0xE6] = 0x73;    // RALT -> GRPH 
codeArray[0xE7] = 0x78;    //  RWIN 
codeArray[0xE8] = 0x52;    //  コンシューマコントロール Mute        -> VF1 
codeArray[0xE9] = 0x53;    //  コンシューマコントロール Volume Up   -> VF2 
codeArray[0xEA] = 0x54;    //  コンシューマコントロール Volume Down -> VF3 
codeArray[0xEB] = 0x55;    //  コンシューマコントロール Next Track  -> VF4 
codeArray[0xEC] = 0x56;    //  コンシューマコントロール Prev Track  -> VF5 
    
//---------------------------------------------
// NumLock解除時の変換キーコード
memcpy(codeArrayNotLk, codeArray, sizeof(codeArray));

codeArrayNotLk[0x59] = 0x3F;    // 1 -> HELP
codeArrayNotLk[0x5A] = 0x3D;    // 2 -> DOWN 
codeArrayNotLk[0x5B] = 0x37;    // 3 -> PgDn  
codeArrayNotLk[0x5C] = 0x3B;    // 4 -> LEFT 
codeArrayNotLk[0x5D] = 0xFF;    // 5 -> N/A  
codeArrayNotLk[0x5E] = 0x3C;    // 6 -> RIGHT 
codeArrayNotLk[0x5F] = 0x3E;    // 7 -> HOMECLR  
codeArrayNotLk[0x60] = 0x3A;    // 8 -> UP 
codeArrayNotLk[0x61] = 0x36;    // 9 -> PgUP   
codeArrayNotLk[0x62] = 0x38;    // 0 -> INS 
codeArrayNotLk[0x63] = 0x39;    // . -> DEL 

codeArrayNotLk[0x3A] = 0x52;    // F1 -> Vf1 
codeArrayNotLk[0x3B] = 0x53;    // F2 -> Vf2 
codeArrayNotLk[0x3C] = 0x54;    // F3 -> Vf3 
codeArrayNotLk[0x3D] = 0x55;    // F4 -> Vf4 
codeArrayNotLk[0x3E] = 0x56;    // F5 -> Vf5 



/*
codeArrayNumLk[0x] = 0x;    //   
codeArray[0x0] = 0x;    //   
codeArray[0x0] = 0x;    //   
*/
}

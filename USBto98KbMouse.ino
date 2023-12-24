#include <hidboot.h>

#include <usbhub.h>
#include <hiduniversal.h>
#include "BskBw120aMParser.h"
#include "KbdRptParser.h"

// Satisfy IDE, which only needs to see the include statment in the ino.
#ifdef dobogusinclude
#include <spi4teensy3.h>
#endif
#include <SPI.h>

USB         Usb;
USBHub     Hub(&Usb);

HIDBoot < USB_HID_PROTOCOL_KEYBOARD | USB_HID_PROTOCOL_MOUSE > HidComposite(&Usb,false);

KbdRptParser KbdPrs;
BskBw120aMParser MousePrs;

TaskHandle_t thp[1];

void setup()
{  
  Serial.begin( 115200 );
  while (!Serial); // Wait for serial port to connect - used on Leonardo, Teensy and other boards with built-in USB CDC serial connection
  Serial.println("Start");

  
  if (Usb.Init() == -1) Serial.println("OSC did not start.");
  delay( 200 );

  //マウス・キーボートパーサーの初期化
  KbdPrs.setUp98Keyboard();
  //キーボート通信処理用タスク(loopだとUSB処理が重く起動時にパケットロスを起こすのでCore0でマルチタスク化)
  xTaskCreatePinnedToCore(Core0_KbdTask, "Core0_KbdTask", 4096, NULL, 24, &thp[0], 0); 

  MousePrs.setUpBusMouse();

  //複合デバイスでマウスが動く場合(Buffalo BSKBW120Sシリーズなど)
  //キーボードパーサーセット・マウスパーサーセット
  HidComposite.SetReportParser(0, &KbdPrs);
  HidComposite.SetReportParser(1, &MousePrs);
}


void loop()
{
  Usb.Task();
  //KbdPrs.task();
}

void Core0_KbdTask(void *args) {
  while (1) {
    delay(1);
    KbdPrs.task();
  }
}

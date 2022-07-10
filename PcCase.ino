#define isDebug 1 // Serial出力をするかしないか

//　Adafruit NeoPixel
#include <Adafruit_NeoPixel.h>
#ifdef __AVR__
#include <avr/power.h> // Required for 16 MHz Adafruit Trinket
#endif
// Which pin on the Arduino is connected to the NeoPixels?
#define LED_PIN    2
// How many NeoPixels are attached to the Arduino?
#define LED_COUNT 16
// Declare our NeoPixel strip object:
Adafruit_NeoPixel strip(LED_COUNT, LED_PIN, NEO_GRB + NEO_KHZ800);

// 入出力用PINの定義
//#define RST_LED  3
//#define RST_BTN  4
#define PWR_LED  5
#define PWR_BTN  4 // 6
#define HDD_LED  6 //7
#define CAP_OUT  8 // 静電容量スイッチの出力側
#define CAP_IN   10 // 静電容量スイッチの入力側
#define BDY_LED 13 //Arduino本体のLED

int maxBrightness = 55;    // 最大の明るさ
int minBrightness = 10;     // 最小の明るさ（Fade時にこれで折り返す）
float colorInterval = 0.05;// Fade時にこの単位で加減さん算が行われる
int flgUpDown = 1;         // Fadeの方向を管理するフラグ 1 = Up
float R = minBrightness;   //Globalで管理する現在の色
float G = minBrightness;   //Globalで管理する現在の色
float B = minBrightness;   //Globalで管理する現在の色
int prevtCap = 0;          //ローパスフィルタ用の前回値（静電容量）
int tCap = 0;              //パルス立ち上がりまでの時間（静電容量）
int thresCap = 20;          //静電容量スイッチのオン判断用しきい値
#define onJudge  5  // 静電容量スイッチの判定をするための判断数
int onFlgs[onJudge];     // 静電容量スイッチの判定をするためのハコ

void setup() {
#if defined(__AVR_ATtiny85__) && (F_CPU == 16000000)
  clock_prescale_set(clock_div_1);
#endif
  // Arduino本体のLED
  pinMode(BDY_LED, OUTPUT);


  // NeoPixel用のオブジェクト
  strip.begin();           // INITIALIZE NeoPixel strip object (REQUIRED)
  strip.show();            // Turn OFF all pixels ASAP

  // Serial
  Serial.begin(115200);

  // inputPin
  pinMode(PWR_LED, INPUT_PULLUP);
  pinMode(HDD_LED, INPUT_PULLUP);

  // 静電容量スイッチ
  pinMode(CAP_OUT, OUTPUT);
  pinMode(CAP_IN, INPUT);

  // 電源ボタン
  pinMode(PWR_BTN, OUTPUT);

}

// PWR　LED の状態を確認。通電してたらFadeさせる
void fadePwrLed() {
  int retVal = digitalRead(PWR_LED);
  if ( isDebug == 1)  {
    Serial.print(" PWR_LED: ");
    Serial.print(retVal);
  }
  if (retVal != 1) {
    fadeAllLEDs();
  } else {
    offAllLEDs();
  }
}

// HDD LED の状態を確認し状況によりOnOffさせる
void  checkHddLED() {
  int retVal = digitalRead(HDD_LED);
  if ( isDebug == 1)  {
    Serial.print(" HDD_LED: ");
    Serial.print(retVal);
  }
  if (retVal != 1) {
    turnOnLED(strip.Color(255, 0, 0), 11);
    if (isDebug == 1)  digitalWrite(BDY_LED, HIGH);
  } else {
    // 何も点けない
    if (isDebug == 1)  digitalWrite(BDY_LED, LOW);
  }
}
void loop() {
  // PWR　LED の状態を確認。通電してたらFadeさせる
  fadePwrLed();

  // HDD LED の状態を確認し状況によりOnOffさせる
  checkHddLED();


  // 静電容量スイッチの状態チェック
  checkCapacitiveSensor();

  // onFlgをチェックして、全部Onなら電源ボタンに通電
  onFlgsCheck();

  if ( isDebug == 1) {
    Serial.println();
  }
}

// 静電容量スイッチの入力チェック
void checkCapacitiveSensor() {
  int retVal = 0;
  tCap = 0;

  // パルスの立ち上げ
  digitalWrite(CAP_OUT, HIGH);

  // 立ち上がりまでの時間計測
  while (digitalRead(CAP_IN) != HIGH) tCap++;

  // 放電するまで待つ
  digitalWrite(CAP_OUT, LOW);
  delay(1);

  if ( isDebug == 1) {
    Serial.print("  tCap:");
    Serial.print(tCap);
  }
  // ローパスフィルタ
  tCap = 0.8 * prevtCap + 0.2 * tCap;
  prevtCap = tCap;

  if ( isDebug == 1) {
    Serial.print("  tCapLowPass:");
    Serial.print(tCap);
  }


  // LED点灯
  if ( tCap > thresCap ) {
    digitalWrite(BDY_LED, HIGH);
    onFlgsUpdater(1);
    retVal = 1;
  } else {
    digitalWrite(BDY_LED, LOW);
    onFlgsUpdater(0);
    retVal = 0;
  }
  if ( isDebug == 1) {
    Serial.print(" 静電容量: ");
    Serial.print(retVal);
  }
  //  return retVal;
}

// すべてのLEDを消灯
void offAllLEDs() {
  // すべてのLEDに0をセット
  for (int i = 0; i < strip.numPixels(); i++) {
    strip.setPixelColor(i, strip.Color(0, 0, 0));
  }
  // LEDの点灯
  strip.show();

}

//　引数で渡されたLEDに指定の色をつける
void turnOnLED(uint32_t color, int numLed) {
  strip.setPixelColor(numLed, color);
  strip.show();
}

// すべてのLEDでフェードさせる
void fadeAllLEDs() {
  if ( isDebug == 1) {
    Serial.print(" R: ");  Serial.print(R);
    Serial.print(" G: ");  Serial.print(G);
    Serial.print(" B: ");  Serial.print(B);
  }
  if (flgUpDown == 1) {
    // LEDの色の決定
    R = R + colorInterval;
    G = R - 10;
    B = 0;
  } else {
    R = R - colorInterval;
    G = R - 10;
    B = 0;
  }
  if (G < minBrightness) {
    G = minBrightness; // Gがマイナス方向に行くを防止
  }

  // Fadeの方向決定
  if (R > maxBrightness) {
    flgUpDown = 0;
  } else if (R < minBrightness) {
    flgUpDown = 1;
  }

  // すべてのLEDに色をセット
  for (int i = 0; i < strip.numPixels(); i++) {
    strip.setPixelColor(i, strip.Color(R, G, B));
  }
  // LEDの点灯
  strip.show();

}

// オンフラグが全て１なら電源PinをHIGHに
void onFlgsCheck() {
  int HIGHLOW = 0;
  int sumFlgs = 0;
  for (int i = 0; i < onJudge; i++) {
    sumFlgs += onFlgs[i];
  }

  if ( isDebug == 1) {
    Serial.print(" sumFlgs:");
      Serial.print(sumFlgs);
  }
  if ( sumFlgs == onJudge){
    digitalWrite(PWR_BTN,HIGH);
  }else{
    digitalWrite(PWR_BTN,LOW);
  }
}

// 引数で渡された値を
void onFlgsUpdater(int valCap) {
  // onFlgsを一つずつ後ろにシフト
  for (int i = onJudge - 1; i >= 0; i--) {
    onFlgs[i] = onFlgs[i - 1];
  }

  onFlgs[0] = valCap;
  if ( isDebug == 1) {
    Serial.print(" onFlgs:");
    for (int i = 0; i < onJudge; i++) {
      Serial.print(onFlgs[i]);
    }
  }
}

#include <Servo.h>
#include <PinChangeInterrupt.h>
#include <LiquidCrystal.h>        // LiquidCrystal.h 라이브러리를 포함한다.

LiquidCrystal lcd(48, 49, 42, 43, 44, 45);       // LCD가 연결된 핀을 설정한다.
Servo myservo;

//display flag 설정
boolean flag_20 = false;
boolean flag_10 = false;
boolean flag_5 = false;
int hintDisplayList[4] = {};

boolean DigitOn = LOW;
boolean DigitOff = HIGH;
boolean SegOn = HIGH;
boolean SegOff = LOW;

//7segment 핀설정
int DigitPins[] = {22, 3, 4, 5};  //숫자 자리수 설정, {D1,D2,D3,D4}
int SegmentPins[] = {6, 7, 8, 9, 24, 25, 26, 27}; //숫자의 부분 설정{a:g,dp}

//게임모드 핀 설정
int mode = 0;
int setTimerMode2 = 2;
boolean countstate;

//Buzzer 핀 설정
int buzzer = 40; //define the digital 2
unsigned long pre = 0;
long inter;

//난수 설정
int nansu;


// 인터럽트 핀 설정
const int interruptPin = 2;//인터럽트핀
const int interruptPin2 = 51;

const int interruptLinecut1 = 10;
const int interruptLinecut2 = 11;
const int interruptLinecut3 = 12;
const int interruptLinecut4 = 13;
const int interruptLinecut5 = 50;

//lineCut flag 설정
int flag_1pin = LOW;
int flag_2pin = LOW;
int flag_3pin = LOW;
int flag_4pin = LOW;
int flag_5pin = LOW;

// 카운트 핀 설정
unsigned long timeVal = 0; //이전시간
unsigned long resetime = 0; //이전시간
unsigned long millisTime = 0; //현재시간
unsigned long millisTime2 = 0; //현재시간
unsigned long countTime = 0; //카운트시작시간
int d1, d2, d3, d4, d5, d6, d7, d8;//자리 숫자
boolean state = false;//타이머 동작 제어

//looks terrible, but I didn't find a way to copy Arrays or merge them from parts
//N is for numbers and NxP is a number with a decimal point behind
int BLANK[] = {LOW, LOW, LOW, LOW, LOW, LOW, LOW, LOW};
int N0[]    = {HIGH, HIGH, HIGH, HIGH, HIGH, HIGH, LOW, LOW};
int N0P[]   = {HIGH, HIGH, HIGH, HIGH, HIGH, HIGH, LOW, HIGH};
int N1[]    = {LOW, HIGH, HIGH, LOW, LOW, LOW, LOW, LOW};
int N1P[]   = {LOW, HIGH, HIGH, LOW, LOW, LOW, LOW, HIGH};
int N2[]    = {HIGH, HIGH, LOW, HIGH, HIGH, LOW, HIGH, LOW};
int N2P[]   = {HIGH, HIGH, LOW, HIGH, HIGH, LOW, HIGH, HIGH};
int N3[]    = {HIGH, HIGH, HIGH, HIGH, LOW, LOW, HIGH, LOW};
int N3P[]   = {HIGH, HIGH, HIGH, HIGH, LOW, LOW, HIGH, HIGH};
int N4[]    = {LOW, HIGH, HIGH, LOW, LOW, HIGH, HIGH, LOW};
int N4P[]   = {LOW, HIGH, HIGH, LOW, LOW, HIGH, HIGH, HIGH};
int N5[]    = {HIGH, LOW, HIGH, HIGH, LOW, HIGH, HIGH, LOW};
int N5P[]   = {HIGH, LOW, HIGH, HIGH, LOW, HIGH, HIGH, HIGH};
int N6[]    = {HIGH, LOW, HIGH, HIGH, HIGH, HIGH, HIGH, LOW};
int N6P[]   = {HIGH, LOW, HIGH, HIGH, HIGH, HIGH, HIGH, HIGH};
int N7[]    = {HIGH, HIGH, HIGH, LOW, LOW, LOW, LOW, LOW};
int N7P[]   = {HIGH, HIGH, HIGH, LOW, LOW, LOW, LOW, HIGH};
int N8[]    = {HIGH, HIGH, HIGH, HIGH, HIGH, HIGH, HIGH, LOW};
int N8P[]   = {HIGH, HIGH, HIGH, HIGH, HIGH, HIGH, HIGH, HIGH};
int N9[]    = {HIGH, HIGH, HIGH, HIGH, LOW, HIGH, HIGH, LOW};
int N9P[]   = {HIGH, HIGH, HIGH, HIGH, LOW, HIGH, HIGH, HIGH};
int MIN[]   = {LOW, LOW, LOW, LOW, LOW, LOW, HIGH, LOW};


//Array of pointers for the 4 digits
int* lights[4];

//char array coming from the serial interface
//4 numbers or chars, 4 optional decimal points, 1 end-of-line char
char incoming[9] = {};



//############# SetUp #################
void setup() {
  Serial.begin(115200);

  myservo.attach(30);
  myservo.write(90);

  pinMode(buzzer, OUTPUT); //set buzzer as output

  lcd.begin(16, 2);
  lcd.display();                 
  //lcd.setCursor(0,0);// 커서 위치 첫째줄 왼쪽 첫번째 칸
  lcd.print(" Ready! ");
  
  for (byte digit = 0; digit < 4; digit++) {
    pinMode(DigitPins[digit], OUTPUT);
  }
  for (byte seg = 0; seg < 8; seg++) {
    pinMode(SegmentPins[seg], OUTPUT);
  }
  //initialize display with 1.234
  lights[0] = N3;
  lights[1] = N0P;
  lights[2] = N0;
  lights[3] = N0;

  pinMode(interruptPin, INPUT_PULLUP);
  pinMode(interruptPin2, INPUT_PULLUP);

  pinMode(interruptLinecut1, INPUT);
  pinMode(interruptLinecut2, INPUT);
  pinMode(interruptLinecut3, INPUT);
  pinMode(interruptLinecut4, INPUT);
  pinMode(interruptLinecut5, INPUT);

  attachInterrupt(digitalPinToInterrupt(interruptPin), switchFn, FALLING);
  attachPCINT(digitalPinToPCINT(interruptPin2), switchFn2, FALLING);


  timeVal = 0;

  //#### Temp ####
  mode = 1;
}

void STOP() {
  Serial.print("STOP");
  state = LOW;
  delay(200);
}

void BOOM() {
  nansu = -1;
  lcd.clear();
  lcd.print("BOOM!");
  Serial.println("BOOM!");
  tone(buzzer, 760, 3000);
  state = LOW;
  d5 = 0;
  d6 = 0;
  d7 = 0;
  d8 = 0;
  myservo.write(179);
  delay(2000);
  myservo.write(90);
  nansu = random(4);
}

void RESET() {
  lcd.clear();
  Serial.println("RESET!!");
  randomSeed(analogRead(A0));
  nansu = random(4);
  makeDisplayHintList(nansu);
  state = HIGH;
  flag_20 = false;
  flag_10 = false;
  flag_5 = false;

  flag_1pin = LOW;
  flag_2pin = LOW;
  flag_3pin = LOW;
  flag_4pin = LOW;
  flag_5pin = LOW;
}



void makeDisplayHintList(int ign_Line){
  int temp;
  int rn;
  for (int i = 0; i < 4; i++){
    if(i == ign_Line) continue;
    if(i > ign_Line){
      hintDisplayList[i-1] = i;
      continue;
    } hintDisplayList[i] = i;
  }
  for (int j = 0; j < 3; j++){
    rn = random(3);
    temp = hintDisplayList[j];
    hintDisplayList[j] = hintDisplayList[rn];
    hintDisplayList[rn] = temp; 
  }
}

void displayHint(int hint){
  if(hint == 0){
    lcd.clear();
    lcd.print("Pupple is safe!");
  } if(hint == 1){
    lcd.clear();
    lcd.print("Blue is safe!");
  } if(hint == 2){
    lcd.clear();
    lcd.print("Green is safe!");
  } if(hint == 3){
    lcd.clear();
    lcd.print("Orange is safe!");
  } if(hint == 4){
    lcd.clear();
    lcd.print("Yellow is safe!");
  } 
}
void loop() {
  //Serial.println(digitalRead(interruptPin));
  //read the numbers and / or chars from the serial interface
  //########## COUNT NUMBER #############
  if (state == true) { //카운트 시작
    if (millis() - timeVal >= 10) { //1초단위로 출력
      timeVal = millis();
      millisTime = (millis() - countTime) / 10;
      //CountUp

      d1 = millisTime % 10; //1의 자리
      d2 = (millisTime / 10) % 10; //10의 자리
      d3 = (millisTime / 100) % 10; //100의 자리
      d4 = (millisTime / 1000) % 10; //1000의 자리

      //Countdown
      if (mode == 1) {
        d5 = 9 - d1;
        d6 = 9 - d2;
        d7 = 9 - d3;
        d8 = 2 - d4;
      }
      if (mode == 2) {
        d5 = 9 - d1;
        d6 = 9 - d2;
        d7 = 9 - d3;
        //setTimerMode2 는 1 ~ 9까지의 값만 입력받을 수 있음
        d8 = (setTimerMode2 - 1) - d4;
      }

      //30초 끝
      if (d4 == 3 && mode == 1) {
        state = LOW;
        BOOM(); 
      }
      if (d4 == setTimerMode2 && mode == 2) {
        Serial.println("test! timer end");
        state = LOW;
        BOOM();
      }

      //Buzzer 울리는 간격 설정
      inter = 1000;
      if (d8 == 1) inter = 500;
      else if (d8 == 0) inter = 250;
      if (d8 == 0 && d7 <= 5) inter = 125;

      // Buzzer Activate function and Read LineCut
      unsigned long cur = millis();
      
      int linecutstate1 = digitalRead(interruptLinecut1);
      int linecutstate2 = digitalRead(interruptLinecut2);
      int linecutstate3 = digitalRead(interruptLinecut3);
      int linecutstate4 = digitalRead(interruptLinecut4);
      int linecutstate5 = digitalRead(interruptLinecut5);
      
      //success
       if (nansu == 0 && linecutstate2 == 0 && linecutstate3 == 0 && linecutstate4 == 0 && linecutstate5 == 0) {
          Serial.println("Success");
          STOP();
        } if (nansu == 1 && linecutstate1 == 0 && linecutstate3 == 0 && linecutstate4 == 0 && linecutstate5 == 0) {
          Serial.println("Success");
          STOP();
        } if (nansu == 2 && linecutstate1 == 0 && linecutstate2 == 0 && linecutstate4 == 0 && linecutstate5 == 0) {
          Serial.println("Success");
          STOP();
        } if (nansu == 3 && linecutstate1 == 0 && linecutstate2 == 0 && linecutstate3 == 0 && linecutstate5 == 0) {
          Serial.println("Success");
          STOP();
        } if (nansu == 4 && linecutstate1 == 0 && linecutstate2 == 0 && linecutstate3 == 0 && linecutstate4 == 0) {
          Serial.println("Success");
          STOP();
        }
        
      //fail
      if(linecutstate1 == LOW && flag_1pin == LOW){
        Serial.println("Line 1 is cutting");
        if (nansu == 0) {
          Serial.println("Bad!!");
          BOOM();
        } else if (mode == 2) {
          //RESET();
          Serial.println("counterReset");
          countTime = millis();
        }
        flag_1pin = HIGH;
      } 
      
      if(linecutstate2 == LOW && flag_2pin == LOW){
        Serial.println("Line 2 is cutting");
        if (nansu == 1) {
          Serial.println("Bad!!");
          BOOM();
        } else if (mode == 2) {
          //RESET();
          Serial.println("counterReset");
          countTime = millis();
        }
        flag_2pin = HIGH;
      } 

      if(linecutstate3 == LOW && flag_3pin == LOW){
        Serial.println("Line 3 is cutting");
        if (nansu == 2) {
          Serial.println("Bad!!");
          BOOM();
        } else if (mode == 2) {
          //RESET();
          Serial.println("counterReset");
          countTime = millis();
        }
        flag_3pin = HIGH;
      } 

      if(linecutstate4 == LOW && flag_4pin == LOW){
        Serial.println("Line 4 is cutting");
        if (nansu == 3) {
          Serial.println("Bad!!");
          BOOM();
        } else if (mode == 2) {
          //RESET();
          Serial.println("counterReset");
          countTime = millis();
        }
        flag_4pin = HIGH;
      } 

      if(linecutstate5 == LOW && flag_5pin == LOW){
        Serial.println("Line 5 is cutting");
        if (nansu == 4) {
          Serial.println("Bad!!");
          BOOM();
        } else if (mode == 2) {
          //RESET();
          Serial.println("counterReset");
          countTime = millis();
        }
        flag_5pin = HIGH;
      } 
      
      if (cur - pre >= inter) {
        pre = cur ;       
        noTone(10);
        tone(buzzer, 760, 100);  //turn on buzzer
        Serial.print(linecutstate1);
        Serial.print(linecutstate2);
        Serial.print(linecutstate3);
        Serial.print(linecutstate4);
        Serial.println(linecutstate5);

        Serial.print("nansu = ");
        Serial.println(nansu);

      }
    }
  }

  //######### LCD Display ##########
  if(d8 == 2 && d7 == 0 && flag_20 == false ){
    displayHint(hintDisplayList[0]);
    flag_20 = true;
  } if(d8 == 1 && d7 == 0 && flag_10 == false){
    displayHint(hintDisplayList[1]);
    flag_10 = true;
  } if(d8 == 0 && d7 == 5 && flag_5 == false){
    displayHint(hintDisplayList[2]);
    flag_5 = true;
  }



  //########## 7SEGMENT DISPLAY ###########
  // 10의 자리
  if (d8 == 0) {
    lights[0] = N0;
  }
  if (d8 == 1) {
    lights[0] = N1;
  }
  if (d8 == 2) {
    lights[0] = N2;
  }
  if (d8 == 3) {
    lights[0] = N3;
  }
  if (d8 == 4) {
    lights[0] = N4;
  }
  if (d8 == 5) {
    lights[0] = N5;
  }
  if (d8 == 6) {
    lights[0] = N6;
  }
  if (d8 == 7) {
    lights[0] = N7;
  }
  if (d8 == 8) {
    lights[0] = N8;
  }
  if (d8 == 9) {
    lights[0] = N9;
  }

  // 1의 자리
  if (d7 == 0) {
    lights[1] = N0P;
  }
  if (d7 == 1) {
    lights[1] = N1P;
  }
  if (d7 == 2) {
    lights[1] = N2P;
  }
  if (d7 == 3) {
    lights[1] = N3P;
  }
  if (d7 == 4) {
    lights[1] = N4P;
  }
  if (d7 == 5) {
    lights[1] = N5P;
  }
  if (d7 == 6) {
    lights[1] = N6P;
  }
  if (d7 == 7) {
    lights[1] = N7P;
  }
  if (d7 == 8) {
    lights[1] = N8P;
  }
  if (d7 == 9) {
    lights[1] = N9P;
  }

  // 0.1
  if (d6 == 0) {
    lights[2] = N0;
  }
  if (d6 == 1) {
    lights[2] = N1;
  }
  if (d6 == 2) {
    lights[2] = N2;
  }
  if (d6 == 3) {
    lights[2] = N3;
  }
  if (d6 == 4) {
    lights[2] = N4;
  }
  if (d6 == 5) {
    lights[2] = N5;
  }
  if (d6 == 6) {
    lights[2] = N6;
  }
  if (d6 == 7) {
    lights[2] = N7;
  }
  if (d6 == 8) {
    lights[2] = N8;
  }
  if (d6 == 9) {
    lights[2] = N9;
  }

  //0.01
  if (d5 == 0) {
    lights[3] = N0;
  }
  if (d5 == 1) {
    lights[3] = N1;
  }
  if (d5 == 2) {
    lights[3] = N2;
  }
  if (d5 == 3) {
    lights[3] = N3;
  }
  if (d5 == 4) {
    lights[3] = N4;
  }
  if (d5 == 5) {
    lights[3] = N5;
  }
  if (d5 == 6) {
    lights[3] = N6;
  }
  if (d5 == 7) {
    lights[3] = N7;
  }
  if (d5 == 8) {
    lights[3] = N8;
  }
  if (d5 == 9) {
    lights[3] = N9;
  }


  //This part of the code is from the library SevSeg by Dean Reading
  for (byte seg = 0; seg < 8; seg++) {
    //Turn the relevant segment on
    digitalWrite(SegmentPins[seg], SegOn);

    //For each digit, turn relevant digits on
    for (byte digit = 0; digit < 4; digit++) {
      if (lights[digit][seg] == 1) {
        digitalWrite(DigitPins[digit], DigitOn);
      }
      //delay(200); //Uncomment this to see it in slow motion
    }
    //Turn all digits off
    for (byte digit = 0; digit < 4; digit++) {
      digitalWrite(DigitPins[digit], DigitOff);
    }

    //Turn the relevant segment off
    digitalWrite(SegmentPins[seg], SegOff);
  } //end of for
}

// mode 선택
void switchFn() {
  mode = 1;
  state = !state;
  countTime = millis();
  timeVal = countTime;
  delay(100);
  RESET();
} 
void switchFn2() {
  mode = 2;
  state = !state;
  countTime = millis();
  timeVal = countTime;
  delay(100);
  RESET();
}

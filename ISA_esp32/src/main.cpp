#include <Arduino.h>
#include <BluetoothSerial.h>
String device_name = "ESP32-BT-Slave";

BluetoothSerial SerialBT;

int vel = 100, modo = 0;
float turn = 1, Kp = 10, Ki = 1, err = 0, err_ant = 0, err_int = 0;
bool once = 0;
char command = 'A';
unsigned long int tempo = millis();

//*************************************************************************

//ultrassom
#define trigger 19
#define echo 23

float ultrassonico() {
  digitalWrite(trigger, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigger, LOW);
  long tempo = pulseIn(echo, HIGH);
  float distancia = tempo * 0.01715;
  return distancia;
}

//*************************************************************************

//IR direita
#define IRD 34

//IR esquerda
#define IRE 35

//IR anti-queda
#define IRB 2

//************************************************************************

//motor A
#define MA1 27
#define MA2 26
//motor B
#define MB1 25
#define MB2 33

//PWM
#define ENA 14
#define ENB 32

#define M 4

//*************************************************************************

#define SPEAKER 13

void MOTOR(int vel, float turn) {
  vel = map(vel, -100, 100, -255, 255);
  if (vel > 0 && digitalRead(IRB) == LOW) {
    if (turn <= 1) {
      digitalWrite(MA1, LOW);
      digitalWrite(MA2, HIGH);
      digitalWrite(MB1, LOW);
      digitalWrite(MB2, HIGH);
      analogWrite(ENA, vel);
      analogWrite(ENB, vel * turn);
    }

    if (turn > 1) {
      digitalWrite(MA1, LOW);
      digitalWrite(MA2, HIGH);
      digitalWrite(MB1, LOW);
      digitalWrite(MB2, HIGH);
      analogWrite(ENA, vel * (2 - turn));
      analogWrite(ENB, vel);
    }
  }

  else if (vel < 0) {
    digitalWrite(MA1, HIGH);
    digitalWrite(MA2, LOW);
    digitalWrite(MB1, HIGH);
    digitalWrite(MB2, LOW);
    analogWrite(ENA, -vel);
    analogWrite(ENB, -vel);
  }

}

void VIRAR(int vel) {
  vel = map(vel, -100, 100, -255, 255);
  //direita > 0
  if (vel > 0) {
    digitalWrite(MA1, LOW);
    digitalWrite(MA2, HIGH);
    digitalWrite(MB1, HIGH);
    digitalWrite(MB2, LOW);
    analogWrite(ENA, vel);
    analogWrite(ENB, vel);

    //esquerda < 0
  } else if (vel < 0) {
    digitalWrite(MA1, HIGH);
    digitalWrite(MA2, LOW);
    digitalWrite(MB1, LOW);
    digitalWrite(MB2, HIGH);
    analogWrite(ENA, -vel);
    analogWrite(ENB, -vel);
  }
}

void BREAK() {
  digitalWrite(MA1, LOW);
  digitalWrite(MA2, LOW);
  digitalWrite(MB1, LOW);
  digitalWrite(MB2, LOW);
}

void executeCommand(char command) {
  switch (command) {
    case 'F':
      if (once == 0) {
        for (int i = 65; i < vel; i += 1) {
          MOTOR(i, turn);
          delay(5);
        }
        once = 1;
      }
      MOTOR(vel, turn);
      break;

    case 'R':
      VIRAR(vel);
      break;

    case 'L':
      VIRAR(-vel);
      break;

    case 'B':
      MOTOR(-vel, turn);
      break;

    case 'S':
      if (turn > 0.8) {
        turn = turn - 0.2;
      }
      break;

    case 'C':
      if (turn < 1.2) {
        turn = turn + 0.2;
      }
      break;

    case 'T':
      vel = 100;
      break;

    case 'X':
      vel = 65;
      break;

    case '0':
      once = 0;
      BREAK();
  }
}

void setup() {

  //IR controle de dedo
  pinMode(IRD, INPUT);
  pinMode(IRE, INPUT);
  pinMode(IRB, INPUT);

  //setup motor
  pinMode(MA1, OUTPUT);
  pinMode(MA2, OUTPUT);
  pinMode(MB1, OUTPUT);
  pinMode(MB2, OUTPUT);
  pinMode(ENA, OUTPUT);
  pinMode(ENB, OUTPUT);

  //ultrassom setup
  pinMode(trigger, OUTPUT);
  pinMode(echo, INPUT);

  //setup mudanca de modo
  pinMode(M, INPUT_PULLDOWN);
  pinMode(SPEAKER, OUTPUT);

  Serial.begin(115200);
  SerialBT.begin(device_name);  //Bluetooth device name
}

void loop() {
  //mudanca de modo
  if (digitalRead(M) == HIGH) {
    BREAK();
    command = 'A';
    digitalWrite(SPEAKER, HIGH);
    modo++;
    delay(500);
    digitalWrite(SPEAKER, LOW);
  }
  if (modo == 2) {
    modo = 0;
  }

  //ultrassom
  float distancia = ultrassonico();

  switch (modo) {
    case 0:
      if (SerialBT.available()) {
        command = SerialBT.read();
        executeCommand(command);
        Serial.println(command);
      }
      break;

    case 1:
      if (distancia < 30) {
        unsigned long dt = millis() - tempo;
        err = distancia - 10;
        err_int = (err_ant += err)*dt/2;
        err_ant = err;
        int u = Kp*(err) + Ki*(err_int);
        tempo = millis();
        if(u > 100){
          err_int = 0;
        }
        MOTOR(u, 1);
      }

      else if (digitalRead(IRD) == LOW && digitalRead(IRE) == HIGH) {
        VIRAR(70);
        delay(50);
      }

      else if (digitalRead(IRE) == LOW && digitalRead(IRD) == HIGH) {
        VIRAR(-70);
        delay(50);
      }

      else {
        BREAK();
      }
      break;
  }
}
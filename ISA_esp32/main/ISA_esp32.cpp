#include "sdkconfig.h"
#include <Arduino.h>
#include <Bluepad32.h>

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

int gas = 0;

//*************************************************************************

#define SPEAKER 13

void MOTOR(int vel, float turn) {
  if (vel > 0 && digitalRead(IRB) == LOW) {
    vel = 255*vel/1020;
    if (turn < 0) {
      digitalWrite(MA1, LOW);
      digitalWrite(MA2, HIGH);
      digitalWrite(MB1, LOW);
      digitalWrite(MB2, HIGH);
      analogWrite(ENA, vel * (turn + 512)/512);
      analogWrite(ENB, vel);
    }

    else if (turn > 0) {
      digitalWrite(MA1, LOW);
      digitalWrite(MA2, HIGH);
      digitalWrite(MB1, LOW);
      digitalWrite(MB2, HIGH);
      analogWrite(ENA, vel);
      analogWrite(ENB, vel * (turn - 508)/-508);
    }

    else if (turn == 0) {
      digitalWrite(MA1, LOW);
      digitalWrite(MA2, HIGH);
      digitalWrite(MB1, LOW);
      digitalWrite(MB2, HIGH);
      analogWrite(ENA, vel);
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

ControllerPtr myControllers[BP32_MAX_GAMEPADS];

void onConnectedController(ControllerPtr ctl) {
    bool foundEmptySlot = false;
    for (int i = 0; i < BP32_MAX_GAMEPADS; i++) {
        if (myControllers[i] == nullptr) {
            Console.printf("CALLBACK: Controller is connected, index=%d\n", i);
            ControllerProperties properties = ctl->getProperties();
            Console.printf("Controller model: %s, VID=0x%04x, PID=0x%04x\n", ctl->getModelName(), properties.vendor_id,
                           properties.product_id);
            myControllers[i] = ctl;
            foundEmptySlot = true;
            break;
        }
    }
    if (!foundEmptySlot) {
        Console.println("CALLBACK: Controller connected, but could not found empty slot");
    }
}

void onDisconnectedController(ControllerPtr ctl) {
    bool foundController = false;

    for (int i = 0; i < BP32_MAX_GAMEPADS; i++) {
        if (myControllers[i] == ctl) {
            Console.printf("CALLBACK: Controller disconnected from index=%d\n", i);
            myControllers[i] = nullptr;
            foundController = true;
            break;
        }
    }

    if (!foundController) {
        Console.println("CALLBACK: Controller disconnected, but not found in myControllers");
    }
}

void dumpGamepad(ControllerPtr ctl) {
    Console.printf(
        "idx=%d, dpad: 0x%02x, buttons: 0x%04x, axis L: %4d, %4d, axis R: %4d, %4d, brake: %4d, throttle: %4d, misc: 0x%02x\n",
        ctl->index(),        // Controller Index
        ctl->dpad(),         // D-pad
        ctl->buttons(),      // bitmask of pressed buttons
        ctl->axisX(),        // (-511 - 512) left X Axis
        ctl->axisY(),        // (-511 - 512) left Y axis
        ctl->axisRX(),       // (-511 - 512) right X axis
        ctl->axisRY(),       // (-511 - 512) right Y axis
        ctl->brake(),        // (0 - 1023): brake button
        ctl->throttle(),     // (0 - 1023): throttle (AKA gas) button
        ctl->miscButtons()  // bitmask of pressed "misc" buttons
    );
}

void processGamepad(ControllerPtr ctl) {
    if(ctl->throttle() > 200){
        gas = ctl->throttle();
    }

    if(ctl->brake() > 200){
        gas = -ctl->brake();
    }

    if(ctl->x()) {
      digitalWrite(SPEAKER, HIGH);
    }

    if(!ctl->x()) {
      digitalWrite(SPEAKER, LOW);
    }
    else if(ctl->brake() < 200 && ctl->throttle() < 200){
        gas = 0;
        turn = 0;
        BREAK();
    }

    turn = ctl->axisX();
    MOTOR(gas, turn);
    dumpGamepad(ctl);
}

void processControllers() {
    for (auto myController : myControllers) {
        if (myController && myController->isConnected() && myController->hasData()) {
            if (myController->isGamepad()) {
                processGamepad(myController);
            } else {
                Console.printf("Unsupported controller\n");
            }
        }
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


    Console.printf("Firmware: %s\n", BP32.firmwareVersion());
    const uint8_t* addr = BP32.localBdAddress();
    Console.printf("BD Addr: %2X:%2X:%2X:%2X:%2X:%2X\n", addr[0], addr[1], addr[2], addr[3], addr[4], addr[5]);
    BP32.setup(&onConnectedController, &onDisconnectedController);
    BP32.forgetBluetoothKeys();
    BP32.enableVirtualDevice(false);
    BP32.enableBLEService(false);
}

void loop() {
bool dataUpdated = BP32.update();
    if (dataUpdated)
        processControllers();

    vTaskDelay(1);
    delay(50);
}

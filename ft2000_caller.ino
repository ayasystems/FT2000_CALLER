#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <EEPROM.h>

//DEFINE PINS
#define ledTX   13  //led indicator TX
#define rigTX   3  //señal TX de la emisora
#define pttIN   4  //entrada pedal TX
#define pttOUT  5  //salida disparar PTT emisora
#define mss1    6  //emular mensaje 1 
#define mss2    7  //emular mensaje 2 
#define mss3    8  //emular mensaje 3 
#define mss4    9  //emular mensaje 4 
#define bt1     10 //botón 1 cambia mensaje 1,2,3,4
#define bt2     11 //botón 2 cambia intervalo beacon
#define bt3     12 //botón 3 beacon on/off
//END DEFINE PINS

#define RX       0 //en reposo RX
#define PEDAL    1 //PTT pedal
#define BEACON   2 //TX POR BEACON
#define TX       3 //TX manual emisora
#define RXCHAR     "  RX  "
#define TXCHAR     "  TX  "
#define AUTOCHARON1   "AUTO1"
#define AUTOCHARON2   "AUTO2"
#define AUTOCHAROFF   "      "
#define BEACONCHAR "BEACON"
#define intervalSum  5//intervalos a sumar al tiempo de baliza (en segundos)
#define limitInterval  60//maximo cada 60 segundos
#define ON  1
#define OFF 0
#define AUTO1 3
#define AUTO2 4

#define BUTTON_PIN        2  // Button

#define LONGPRESS_LEN    25  // Min nr of loops for a long press
#define SHORTPRESS_LEN   2  // Min nr of loops for a long press
#define DELAY            20  // Delay per loop in ms

enum { EV_NONE=0, EV_SHORTPRESS, EV_LONGPRESS };

boolean button_was_pressed1; // previous state
boolean button_was_pressed2; // previous state
boolean button_was_pressed3; // previous state

int button_pressed_counter1; // press running duration
int button_pressed_counter2; // press running duration
int button_pressed_counter3; // press running duration
int blinkTX = 0;

uint8_t bell[8]  =    {0x4,0xe,0xe,0xe,0x1f,0x0,0x4};
uint8_t arena[9]   =  {0x1f,0x11,0xa,0x4,0xa,0x11,0x1f};
uint8_t arenaA[10]   = {0x1f,0x1f,0xe,0x4,0xa,0x11,0x1f};
uint8_t arenaB[11]   = {0x1f,0x11,0xa,0x4,0xe,0x1f,0x1f};
LiquidCrystal_I2C lcd(0x27,16,2);  // set the LCD address to 0x27 for a 16 chars and 2 line display

#if defined(ARDUINO) && ARDUINO >= 100
#define printByte(args)  write(args);
#else
#define printByte(args)  print(args,BYTE);
#endif


int mssValue      = 1;//Por defecto 1
int mssValueOld   = 99;//Esto fuerza que siempre pinte el valor en display la primera vez
int estMss        = 0;
int estMssOld     = 99;//Esto fuerza que siempre pinte el valor en display la primera vez
//int estBeacon     = 0; //
//int estBeaconOld  = 99;//Esto fuerza que siempre pinte el valor en display la primera vez
int beaconTime    = 5;//
int beaconTimeOld = 99;//Esto fuerza que siempre pinte el valor en display la primera vez
int stateBt1      = HIGH;

int stateBt2      = HIGH;
int stateBt3      = HIGH;
int statePTTin    = LOW;
int statePTTinOld = LOW;
int stateRigTX    = LOW;
int beaconMode    = 0;
int beaconModeOld = 99;//Esto fuerza que siempre pinte el valor en display la primera vez
long lastTXms  = 0;  //ultimo momento en TX. Para contar tiempo para baliza
unsigned long previousMillis = 0; 
const long interval = 1000;
long currentMillis;
int lanzar = 0;
int contador;
int clock = 0;
int backLight = 1; //por defecto luz encendida
//pendiente de hacer que muestre la cuenta atras
//que mire el estado de TX del equipo solo cada 500ms (por ejemplo, habra que ajustarlo)

void defineOutput(int pinN){
  pinMode(pinN,OUTPUT);
  digitalWrite(pinN,LOW);
}

void defineInput(int pinN,int value){
  pinMode(pinN,INPUT);
  digitalWrite(pinN, value);       // activa la resistencia pullup
}

void setup() {
  Wire.begin();  
  Serial.begin(9600);
  lcd.init();                      // initialize the lcd 
  lcd.backlight();
  lcd.home();
 
  lcd.print("EA4GKQ AUTO CALL");
  lcd.setCursor(0, 1);
  lcd.print("V 1.0 LOADING...");  
  delay(2000);
  lcd.createChar(0, bell);
  lcd.createChar(9, arena);  
  lcd.createChar(10, arenaA);  
  lcd.createChar(11, arenaB);
  // put your setup code here, to run once:
  defineOutput(ledTX);
  defineOutput(pttOUT);
  defineOutput(mss1);
  defineOutput(mss2);
  defineOutput(mss3);
  defineOutput(mss4);
  defineInput(pttIN,LOW);
  defineInput(rigTX,HIGH);
  
  defineInput(bt1,HIGH);
  defineInput(bt2,HIGH);
  defineInput(bt3,HIGH);
//leemos la configuracion antes de apagar  
  readMssValue();   
  readBeaconTime(); 
  readBLight();
//inicializamos el display  

  lcd.clear();
  lcd.setCursor(0,0);
  //       //0123456789012345
  lcd.print("MSS X        yys");
  
  lcd.setCursor(0,1);
           //0123456789012345 
  lcd.print("----         xx");
  lcd.printByte(0);
  clock = 1;  
  printClock();

  delay(1000);
  contador = beaconTime;
}



void loop() {
  currentMillis = millis();
  Countdown();   // Contador 
  checkButtons();
  // put your main code here, to run repeatedly:
  printX();         //mensaje a emular en yaesu por medio del teclado externo conectado al jack
  printYY();         //segundos intervalo baliza
  printBeaconMode(); //AUTO / 
  printStatus();     //  TX / RX / BEACON

  checkRigTX();
  printXX();//tiempo que queda para lanzar la baliza  
  Countdown();   // Contador 
  checkSendBeacon();
  
}

void printClock(){
  if(clock==3){
   clock = 1;
  }else{
   clock = clock + 1;
  }
  lcd.setCursor(9,0);
  switch(clock) {
    case 1:
      lcd.printByte(9);
      break;
    case 2:
      lcd.printByte(10);
      break;
    case 3:
      lcd.printByte(11);
      break;  
    }
}
void checkButtons(){

  checkBt1();
  checkBt2();
  checkBt3();  
}
void Countdown(){
  if(currentMillis - previousMillis >= interval) {
    if(beaconMode==AUTO1||beaconMode==AUTO2){
      contador = contador -1;    
    }
    previousMillis = currentMillis;
    debug();
    printClock();
    checkBLight();
    //Serial.print(digitalRead(bt1));
    //Serial.print(digitalRead(bt2));
    //Serial.println(digitalRead(bt3));

  }    
}

 
void checkBLight(){
    if(stateRigTX==HIGH){
     estMss = RX;
     digitalWrite(ledTX,LOW);
     if(backLight==OFF){
       lcd.noBacklight();        
     }else{
       lcd.backlight();        
     }  
     blinkTX=0;
    }
} 
void debug(){
Serial.print("Bacon: ");
    switch(beaconMode) {
    case AUTO1:
      Serial.print(AUTOCHARON1);
      break;
    case AUTO2:
      Serial.print(AUTOCHARON2);
      break;
    case OFF:
      Serial.print(AUTOCHAROFF);
      break;  
    }
    Serial.print(" ");
    Serial.print(beaconMode);
    Serial.print(" ");
    Serial.print(" ");
    Serial.print("estBeacon: ");
    
//    Serial.println(estBeacon);
Serial.print("Estatus: ");
    switch (estMss) {
    case RX:
      Serial.println(RXCHAR);
      break;
    case PEDAL:
      Serial.println(TXCHAR);
      break;
    case BEACON:
      Serial.println(BEACONCHAR);
      break;
    default:
      Serial.println("ERROR!");
      break;
    }
    
  Serial.print("Lanzar: ");
  Serial.println(lanzar);
  
  Serial.print("contador: ");
  Serial.print(contador);
  
  Serial.print(" mssValue: ");
  Serial.print(mssValue);
  
  Serial.print(" RigTX: ");
  Serial.println(digitalRead(rigTX));
  
  Serial.print(" blinkTX: ");
  Serial.println(blinkTX);
}
void checkSendBeacon(){
  int mssPIN;
    if(contador<1&&(beaconMode==AUTO1||beaconMode==AUTO2)){
    contador = beaconTime;  
    lanzar = 1;
  }
  //Si estamos en automatico (BEACON ON), en modo RX, y ha transcurrido el tiempo de lanzamiento de baliza
  if((beaconMode==AUTO1||beaconMode==AUTO2)&&estMss==RX&&lanzar==1){
    //lanzamos baliza
     if(mssValue>0&&mssValue<5){  
      if(mssValue==1)  mssPIN = mss1;
      if(mssValue==2)  mssPIN = mss2;
      if(mssValue==3)  mssPIN = mss3;
      if(mssValue==4)  mssPIN = mss4;      
      lcd.noBacklight();
      digitalWrite(mssPIN,HIGH);
      digitalWrite(ledTX,HIGH);
      estMss = BEACON;
      printStatus();
      lcd.backlight(); 
      Serial.print("Lanzamos pin");
      Serial.println(mssPIN);
      lcd.noBacklight();
      delay(200);
      lcd.backlight();    
     }
      digitalWrite(mss1,LOW);
      digitalWrite(mss2,LOW);
      digitalWrite(mss3,LOW);
      digitalWrite(mss4,LOW);
      digitalWrite(ledTX,LOW);
      
      lanzar = 0;
      contador = beaconTime;     
  }
  
}
void printXX(){
  lcd.setCursor(13,1);
  

  if(contador<10){
    lcd.print(" "); 
  }    
    lcd.print(contador);//% 10);
    lcd.setCursor(15,1);
    lcd.printByte(0);//campana
}

void printX(){
  if(mssValue!=mssValueOld){
    lcd.setCursor(0,0);
    lcd.print("MSS     ");
    //XX numero de mensaje que lanzará la baliza
    lcd.setCursor(4,0);
    lcd.print(mssValue);
    mssValueOld = mssValue;
  }
}
void printYY(){
  if(beaconTime!=beaconTimeOld){
    //YY intervalo actual para lanzar baliza
    lcd.setCursor(13,0);
    lcd.print("  s");
    if(beaconTime<10){
      lcd.setCursor(14,0);
    }else{
      lcd.setCursor(13,0);
    }
    lcd.print(beaconTime);
    beaconTimeOld = beaconTime;
  }
}

void printBeaconMode(){
  if(beaconMode!=beaconModeOld){
      lcd.setCursor(7,1);
      switch(beaconMode) {
        case AUTO1:
          lcd.print(AUTOCHARON1);
          break;
        case AUTO2:
          lcd.print(AUTOCHARON2);
          break;      
        case OFF:
          lcd.print(AUTOCHAROFF);
          break;  
      }
    beaconModeOld = beaconMode;
  }
}





void readMssValue(){
  int readMssVal = EEPROM.read(1);

  if(readMssVal>0||readMssVal<5){
   mssValue = readMssVal;
  }
}

void readBeaconTime(){
  int valorLeido = EEPROM.read(2);
  if(valorLeido>0){
   beaconTime = valorLeido;
  }
}
void readBLight(){
  int valorLeido = EEPROM.read(3);
  backLight = valorLeido;
}
void saveMssValue(int valor){
  EEPROM.write(1, valor);//en la dirección 1 guardamos el valor del mensaje que se está utilizando
}
void saveBeaconTime(int valor){
  EEPROM.write(2, valor);//en la dirección 2 guardamos el valor del intervalo de la baliza
}
void saveBLight(int valor){
  EEPROM.write(3, valor);//en la dirección 3 guardamos el estado de la luz de fondo del display
}


void printStatus(){
//solo modificamos el display cuando haya un cambio de estatus
  if(estMss!=estMssOld){
    switch (estMss) {
    case RX:
      lcd.setCursor(0,1);
      lcd.print(RXCHAR);
      break;
    case PEDAL:
      lcd.setCursor(0,1);
      lcd.print(TXCHAR);
      break;
    case BEACON:
      lcd.setCursor(0,1);
      lcd.print(BEACONCHAR);
      break;
    case TX:
      lcd.setCursor(0,1);
      lcd.print(TXCHAR);
      break;    
    default:
      lcd.setCursor(0,1);
      lcd.print("ERROR!");
      break;
    }
    estMssOld = estMss;
  }  
  

}



/*
void checkBt1(){

  stateBt1 = digitalRead(bt1);
  // Si el pin de entrada a pasado de nivel bajo y ha pasado suficiente tiempo desde 
  // el último cambio para ignorar ruido, cambiamos el estado de la salida
  // y almacenamos el tiempo
  if ((stateBt1 == LOW && stateBt1Old == HIGH)) {
     if((millis() - time > debounce ) && ((millis() - time) < (debounce + 800))) {
         Serial.print("button1 ");
         Serial.print(stateBt1);
         Serial.print(" - ");         
         Serial.println(stateBt1Old);
       mssValue = mssValue + 1;
       if(mssValue>4){
        mssValue = 1;
       }
        saveMssValue(mssValue);
        //time = millis();    
        contador = beaconTime; 
     }
  }     
   
  if(((stateBt1 == LOW && stateBt1Old == LOW))&&((millis() - time) > (debounce + 1200))){
         Serial.print("button1 ");
         Serial.print(stateBt1);
         Serial.print(" - ");         
         Serial.println(stateBt1Old);
        if(backLight==1){
           backLight=0;
           lcd.noBacklight();
         }else{
           lcd.backlight();
           backLight=1;
         }           
     //time = millis();  
  }
 stateBt1Old = stateBt1;
}
*/

void checkBt1()
{
  int event;
  int button_now_pressed = !digitalRead(bt1); // pin low -> pressed

  if (!button_now_pressed && button_was_pressed1) {
    if ((button_pressed_counter1 > SHORTPRESS_LEN) && (button_pressed_counter1 < LONGPRESS_LEN)){
      event = EV_SHORTPRESS;
      Serial.println("short1");
       mssValue = mssValue + 1;
       if(mssValue>4){
        mssValue = 1;
       }          
    }else if(button_pressed_counter1 >= LONGPRESS_LEN){
      event = EV_LONGPRESS;
      Serial.println("long1");    
        if(backLight==ON){
           backLight=OFF;
           lcd.noBacklight();
         }else{
           lcd.backlight();
           backLight=ON;
         }    
      saveBLight(backLight);     
    }  
  }
  else
    event = EV_NONE;

  if (button_now_pressed)
    ++button_pressed_counter1;
  else
    button_pressed_counter1 = 0;

  button_was_pressed1 = button_now_pressed;
  //return event;
}

void checkBt2()
{
  int event;
  int button_now_pressed = !digitalRead(bt2); // pin low -> pressed

  if (!button_now_pressed && button_was_pressed2) {
    if ((button_pressed_counter2 > SHORTPRESS_LEN) && (button_pressed_counter2 < LONGPRESS_LEN)){
      event = EV_SHORTPRESS;
      Serial.println("short2");
      beaconTime = beaconTime + intervalSum;
      if(beaconTime>limitInterval){
       beaconTime = 5;
      }
      saveBeaconTime(beaconTime);
      contador = beaconTime;  
        
    }else if(button_pressed_counter2 >= LONGPRESS_LEN){
        event = EV_LONGPRESS;
        Serial.println("long2");    
    }  
  }
  else
    event = EV_NONE;

  if (button_now_pressed)
    ++button_pressed_counter2;
  else
    button_pressed_counter2 = 0;

  button_was_pressed2 = button_now_pressed;
  //return event;
}


void checkBt3()
{
  int event;
  int button_now_pressed = !digitalRead(bt3); // pin low -> pressed

  if (!button_now_pressed && button_was_pressed3) {
    if ((button_pressed_counter3 > SHORTPRESS_LEN) && (button_pressed_counter3 < LONGPRESS_LEN)){
      event = EV_SHORTPRESS;
      Serial.println("short3");
      if(beaconMode==AUTO1){
       beaconMode = AUTO2;
      }else if(beaconMode==AUTO2){
       beaconMode = OFF;
      }else if(beaconMode==OFF){
       beaconMode = AUTO1;     
      }
      contador = beaconTime; 
        
    }else if(button_pressed_counter3 >= LONGPRESS_LEN){
      event = EV_LONGPRESS;
      Serial.println("long3");    

    }  
  }
  else
    event = EV_NONE;

  if (button_now_pressed)
    ++button_pressed_counter3;
  else
    button_pressed_counter3 = 0;

  button_was_pressed3 = button_now_pressed;
  //return event;
}



void checkRigTX(){
    stateRigTX = digitalRead(rigTX);
    if(stateRigTX==LOW&&estMss == BEACON){
      //no hacemos nada, está mandando la baliza
      lanzar = 0;
      contador = beaconTime;
    }
    
    if(stateRigTX==LOW&&estMss != BEACON){
      lastTXms = millis();//
      estMss = TX;
      contador = beaconTime;
      digitalWrite(ledTX,HIGH);
      if(beaconMode==AUTO1){
       beaconMode = OFF;  
      }   
    }
    
    if(stateRigTX==LOW){
      if(blinkTX<50){
        lcd.backlight();               
      }else if(blinkTX>70){
        blinkTX=0;
      }else{
        lcd.noBacklight(); 
      }
      blinkTX++;
    }
    if(stateRigTX==HIGH){
     estMss = RX;
    }
}

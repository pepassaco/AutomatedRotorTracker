/*
 * 
 * Controlador de rotores analógicos, por Pablo Álvarez (EA4HFV, del EA4RCT)
 * Diseñado para funcionar con el protocolo EasyComm II en rotores con salida analogica de 
 * elevación y acimut.
 * 
 * Fuentes: 
 * 
 * SatNOGS
 * https://gitlab.com/librespacefoundation/satnogs/satnogs-rotator-firmware/tree/master/stepper_motor_controller
 * 
 * paulschow
 * https://github.com/paulschow/CheapSatTrack
 */
 
#include <string.h>
#include <stdlib.h>
#include <math.h>


#define BufferSize 256
#define BaudRate 19200

/*Pines conectados a los reles*/
const int pinElMas = 3;
const int pinElMenos = 4;
const int pinAzMas = 5;
const int pinAzMenos = 6;

const int pinInterruptor = 7;

//Pines de los lectores de acimut y elevacion conectados a A0 y A1

int estadoAz = 0; // -1 bajar, 0 nada, 1 subir
int estadoEl = 0; // -1 bajar, 0 nada, 1 subir


/*Global Variables*/
//unsigned long t_DIS = 0; /*time to disable the Motors*/

void setup(){  
  
  /*Salida de serie*/
  Serial.begin(BaudRate);
  
  /*definifión de pines*/
  pinMode(pinElMas, INPUT);
  pinMode(pinElMenos, INPUT);
  pinMode(pinAzMas, INPUT);
  pinMode(pinAzMenos, INPUT);

  digitalWrite(pinElMas, HIGH);
  digitalWrite(pinElMenos, HIGH);
  digitalWrite(pinAzMas, HIGH);
  digitalWrite(pinAzMenos, HIGH);
  

  pinMode(pinInterruptor, OUTPUT);
  
}

void loop() {
   
  /*Valores recibidos*/
  static int AZnuevo = 0;
  static int ELnuevo = 0;
  
  /*Time Check*/
/*
  if (t_DIS == 0)
    t_DIS = millis();
*/

    
  /*Lector de comandos*/
  cmd_proc(AZnuevo, ELnuevo);
  /*Comprueba si sube o baja*/
  comprueba(AZnuevo, ELnuevo);
  /*Mueve el rotor*/
  mueve(AZnuevo, ELnuevo);

}


 
/*EasyComm 2 Protocol & Calculate the steps*/
void cmd_proc(int &stepAz, int &stepEl)
{
  /*Serial*/
  char buffer[BufferSize];
  char incomingByte;
  char *Data = buffer;
  char *rawData;
  static int BufferCnt = 0;
  char data[100];
  
  double angleAz, angleEl;
  
  /*Read from serial*/
  while (Serial.available() > 0)
  {
    incomingByte = Serial.read();
    /* XXX: Get position using custom and test code */
    if (incomingByte == '!')
    {
      /*Get position*/
      Serial.print("TM");
      Serial.print(1);
      Serial.print(" ");
      Serial.print("AZ");
      Serial.print(getAz(),1);
      Serial.print(" ");
      Serial.print("EL");
      Serial.print(getEl(),1);
      
    }




    
    /*new data*/
    else if (incomingByte == '\n')
    {
      buffer[BufferCnt] = 0;
      if (buffer[0] == 'A' && buffer[1] == 'Z') {
        
        if (buffer[2] == ' ' && buffer[3] == 'E' && buffer[4] == 'L'){
          
          /*Get position*/
          Serial.print("AZ");
          Serial.print(getAz(),1);
          Serial.print(" ");
          Serial.print("EL");
          Serial.print(getEl(),1);
          Serial.println(" ");
          
        }else {
          
          /*Get the absolute value of angle*/
          rawData = strtok_r(Data, " " , &Data);
          strncpy(data, rawData+2, 10);
          if (isNumber(data)){
            angleAz = atof(data);
            stepAz = angleAz;
          }

          
          /*Get the absolute value of angle*/
          rawData = strtok_r(Data, " " , &Data);
          if (rawData[0] == 'E' && rawData[1] == 'L'){
            strncpy(data, rawData+2, 10);
            if (isNumber(data)){
              angleEl = atof(data);
              stepEl = angleEl;
            }
          }
        }
      }



      
      /*Stop Moving*/
      else if (buffer[0] == 'S' && buffer[1] == 'A' && buffer[2] == ' ' && buffer[3] == 'S' && buffer[4] == 'E') {
        /*Get position*/
        Serial.print("AZ");
        Serial.print(" ");
        Serial.print("EL");
      }


      
      /*Reset the rotator*/
      else if (buffer[0] == 'R' && buffer[1] == 'E' && buffer[2] == 'S' && buffer[3] == 'E' && buffer[4] == 'T') {
        /*Get position*/
        Serial.print("AZ");
        Serial.print(" ");
        Serial.print("EL");
        /*Zero the steps*/
        stepAz = 0;
        stepEl = 0;
      } 

           
      BufferCnt = 0;
      /*Reset the disable motor time*/
      //t_DIS = 0;
    }

    
    /*Fill the buffer with incoming data*/
    else {
      buffer[BufferCnt] = incomingByte;
      BufferCnt++;
    }
  }
}




/*Error Handling*/
void error(int num_error)
{
  switch (num_error)
  {
    /*Azimuth error*/
    case (0):
      while(1)
      {
        Serial.println("AL001");
        delay(100);
      }
    /*Elevation error*/
    case (1):
      while(1)
      {
        Serial.println("AL002");
        delay(100);
      }
    default:
      while(1)
      {
        Serial.println("AL000");
        delay(100);
      }
  }
}



/*Motor del rotor*/
void mueve(int stepAz, int stepEl){

  if(getAz()>180 && stepAz < 180){  //El rotor va de -180 a +180, no de 0 a 360
    masAz();
  }else if(getAz()<180 && stepAz > 180){
    menosAz();
  }else if(stepAz > getAz() && estadoAz == 1){  //estadoAz == 1 es subiendo
    masAz(); 
    delay(200);
  }else if(stepAz < getAz() && estadoAz == -1){  //estadoAz == -1 es bajando
    menosAz();
    delay(200);
  } else{
    nadaAz();
    delay(200);
  }

  if(stepEl > (getEl()) && estadoEl == 1){  //estadoEl == 1 es subiendo
    masEl(); 
    delay(200);
  }else if(stepEl < (getEl()) && estadoEl  == -1){  //estadoEl == -1 es bajando
    menosEl();
    delay(200);
  }else{
    nadaEl();
    delay(200);
  }
}



/*Lector de la elevación*/
double getEl(){
  double elev = ( (double) analogRead(A0))/1023*180;
  return(elev);
}


/*Lector del acimut*/
double getAz(){
  double az = ( (double) analogRead(A1)-3)/937*360;

  if(az > 180){
    az -= 180;
  }else{
    az += 180;
  }
  
  return(az);
}


/* Para evitar transiciones rapidas de subida y bajada de el y az en el rotor y daño a los reles, 
 * se detecta si el satelite pasa de aumentar a disminuir su elevacion
 */
void comprueba(int stepAz, int stepEl){
  
  
  if(stepAz-getAz()>6){
    estadoAz = 1;
  }else if(stepAz-getAz()<-6){    
    estadoAz = -1;
  }
 

  if(stepEl-getEl()>3){
    estadoEl = 1;
  }else if(stepEl-getEl()<-3){
    estadoEl = -1;
  }
}


/*Metodos controladores de los reles*/


void masEl(){
  digitalWrite(pinElMas, LOW);
  digitalWrite(pinElMenos, HIGH);
}

void menosEl(){
  digitalWrite(pinElMas, HIGH);
  digitalWrite(pinElMenos, LOW);
}

void nadaEl(){
  digitalWrite(pinElMas, HIGH);
  digitalWrite(pinElMenos, HIGH);
}

void masAz(){
  digitalWrite(pinAzMas, LOW);
  digitalWrite(pinAzMenos, HIGH);
}

void menosAz(){
  digitalWrite(pinAzMas, HIGH);
  digitalWrite(pinAzMenos, LOW);
}

void nadaAz(){
  digitalWrite(pinAzMas, HIGH);
  digitalWrite(pinAzMenos, HIGH);
}

/*Check if is argument in number*/
boolean isNumber(char *input)
{
  for (int i = 0; input[i] != '\0'; i++)
  {
    if (isalpha(input[i]))
      return false;
  }
   return true;
}

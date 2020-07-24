#include <ArduinoJson.h>
#include <EEPROM.h>
#include <math.h>
 
// Input and Output pins
#define INPUT_TEMP_1        A4  // PIN_TEMPERATURA_AGUA
#define INPUT_TEMP_2        A5  // PIN_TEMPERATURA_SOLAR
#define CURRENT             A3  // PIN_CURRENT
#define QUANTITY_OUTPUT     9   // Dispositivo final son 10
#define QUANTITY_INPUT      4   
#define BUZZER_PIN          5   // BUZZER_PIN
#define INPUT_LEVEL_PIN     6   // PIN_LEVEL
#define MEMORIA_JSON        800
#define NUMBER_ID           100
#define TEMPERATURE_SAMPLES 10
#define MAX_SET             50
#define A_COEFICIENT        -0.01390016258e-3
#define B_COEFICIENT        4.034139659e-4
#define C_COEFICIENT        -3.865958320e-7


// List of parameters
#define DATA                "data"
#define ID                  "ID"
#define MSG                 "msg"
#define OUTPUT_NUMBER       "output"
#define STATUS              "Status"    //0
#define TIME_ON             "Ton"       //1
#define HOUR_ON             "HourOn"    //2
#define TIME_MAX            "TMax"      //3
#define OPERATING_TIME      "Tfun"      //4
#define LEVEL               "Level"     //5
#define CONTROL             "Control"   //6
#define FORZAR              "Forzar"    //7
#define HOUR                "Hour"      
#define SET_TEMPERATURA     "SetTemp"   //8
#define TEMP                "Temp"      

// Mensajes del protocolo MSG:
#define OK                      11  // Bidirectional
#define BAD_REQUEST             12  // Device to SV
#define BAD_PARAMETER           13  // Device to SV
#define SV_ERROR_REQUEST        14  // SV to Device
#define CONTROL_TEMP_ON         51  // Device to SV
#define CONTROL_TEMP_OFF        52  // Device to SV
#define MANUAL_OPERATION        101 // Device to SV
#define HOUR_OPERATION          102 // Device to SV
#define SHUNTDOWN_LEVEL         111 // Device to SV
#define SHUNTDOWN_TIME_ON       112 // Device to SV
#define SHUNTDOWN_TMAX          113 // Device to SV
#define SHUTDOWN_CURRENT_FAIL   114 // Device to SV
#define REQUEST_ADD_OUTPUT      201 // SV to Device
#define RESPONSE_ADD_OUTPUT     202 // Device to SV
#define REQUEST_DELETE_OUTPUT   203 // SV to Device
#define RESPONSE_DELETE_OUTPUT  204 // SV to Device
#define REQUEST_EDIT_PARAMETER  205 // SV to Device
#define RESPONSE_EDIT_PARAMETER 206 // Device to SV
#define REQUEST_EDIT_TEMP       207 // SV to Device
#define RESPONSE_EDIT_TEMP      208 // Device to SV
#define REQUEST_OPERATING_TIME  211 // SV to Device
#define RESPONSE_OPERATING_TIME 212 // Device to SV
#define REQUEST_ALL_PARAMETER   213 // SV to Device
#define RESPONSE_ALL_PARAMETER  214 // Device to SV


// Functions
byte TimeFunction(void);
void getCURRENT (void);
void GetTemperature(void);
byte ReadInput(void);
byte DependecyControl(void);
void LoadInputSequences (void);
void cleanOuput(byte in);
byte CheckRules(byte *out);
byte  Rx_Serial_input(byte *out);
void Tx_Serial_parameter(byte in,byte msg);
void UpdateOutput(void);
void UpgrateContinuosTime(void);
void PowerOnHour (void);
byte PowerTemp(byte *out);
void load(void);
void save(void);

// Variables globales
unsigned long pausa_boton = 0;

float Temp1 = 0.5; // Temperatura 1
float Temp2 = 0.5; // Temperatura 2
float TempSet = 0.0; // Seteo de temperatura  
unsigned long tiempo1 = 0;       
unsigned long tiempo2 = 0; 
unsigned long tiempo3 = 0;
unsigned long tiempo4 = 0;
unsigned long tiempo5 = 0;


byte seconds=0;
byte minutes=0;
byte hours=0;

byte number_of_bips = 0;

boolean parameters[] = {0,0,0,0,0,0,0,0,0,0};

// Output pins
byte OutputPins[] = {
   2,
   3,
   4,
   16, //A2
   15, //A1
   14, //A0
   13,
   12,
   11
   };
   
boolean InputStatus[] = {
   0,
   0,
   0,
   0
};

boolean StatusLevel = 0;

byte InputSequence[4][11] = {
  {1,0,0,0,0,0,0,0,0,0,0},
  {1,0,0,0,0,0,0,0,0,0,0},
  {1,0,0,0,0,0,0,0,0,0,0},
  {1,0,0,0,0,0,0,0,0,0,0}
};


struct Output_struct{
  boolean         Status;
  int             Ton;
  unsigned long   OperatingTime;
  unsigned int    Tmax;
  unsigned  long  ContinuousTime;
  byte            HourOn;
  byte            MinuteOn;
  byte            Control;
  byte            Level;
  byte            Forzar;
}Output[QUANTITY_OUTPUT];



void setup(){
  // Setup serial comunication 
  Serial.begin(115200);
  Serial.print("INICIO");
  pinMode(BUZZER_PIN,OUTPUT);
  digitalWrite(BUZZER_PIN,HIGH);

  // Setup input
  for (int i=0 ; i<QUANTITY_INPUT ; i++){
    pinMode(7+i,INPUT);
  } 
  pinMode(INPUT_LEVEL_PIN,INPUT);

  // Setup Output
  for (int i=0;i<QUANTITY_OUTPUT ;i++){
    pinMode(OutputPins[i],OUTPUT);
    digitalWrite(OutputPins[i],LOW);
  }
  
  for (int i = 0; i< QUANTITY_OUTPUT ; i++){
    CleanOutput(i);
  }

  load(); // Load the saved parameters
  LoadInputSequences(); // Update sequence

  digitalWrite(BUZZER_PIN, LOW);
  Serial.print("FIN");
} // FIN setup
 
void loop(){
  
  getCurrent();

  // Received serial input
  if( Serial.available()){
    
    byte out = 0;
    for(int i=0;i<9;i++){
      parameters[i]= false;
    }
    byte msg = Rx_Serial_input(&out);

    switch (msg){
      case REQUEST_OPERATING_TIME:{
        parameters[4] = true;
        Tx_Serial_parameter(out,RESPONSE_OPERATING_TIME);
        number_of_bips = 1;
        break;
      }

      case REQUEST_EDIT_PARAMETER:{
        msg = RESPONSE_EDIT_PARAMETER;
        if (parameters[0]){
          msg = CheckRules(&out);
          UpdateOutput();
        } 
         LoadInputSequences();
        if (msg == OK) msg = RESPONSE_EDIT_PARAMETER;
        Tx_Serial_parameter(out,msg);
        if (parameters[1]||parameters[2]||parameters[3]||parameters[4]||parameters[5]||parameters[6]||parameters[7]|parameters[8]) save();
        number_of_bips = 1;
        break;
      }

      case REQUEST_ADD_OUTPUT:{
        msg = RESPONSE_ADD_OUTPUT;
        if (parameters[0]){
          msg = CheckRules(&out);
          UpdateOutput();
        } 
         LoadInputSequences();
        if (msg == OK) msg = RESPONSE_ADD_OUTPUT;
        Tx_Serial_parameter(out,msg);
        if (parameters[1]||parameters[2]||parameters[3]||parameters[4]||parameters[5]||parameters[6]||parameters[7]|parameters[8]) save();
        number_of_bips = 1;
        break;
      }

      case REQUEST_EDIT_TEMP:{
        msg = RESPONSE_EDIT_TEMP;
        Tx_Serial_parameter(out,msg);
        save();
        number_of_bips = 1;
        break;
      }


      case REQUEST_DELETE_OUTPUT:{
        CleanOutput(out-1);
        save();
        LoadInputSequences();
        UpdateOutput();
        for(int i=0;i<9;i++){
          parameters[i]= true;
        }
        Tx_Serial_parameter(out,RESPONSE_DELETE_OUTPUT);
        number_of_bips = 1;
        break;
      }


      case REQUEST_ALL_PARAMETER:{
        for(int i=0;i<QUANTITY_OUTPUT;i++){
          parameters[i]= true;
        }
        for(int i = 0; i< QUANTITY_OUTPUT;i++){
           Tx_Serial_parameter(i+1,RESPONSE_ALL_PARAMETER);
        }
        break;
      }



      case BAD_PARAMETER:{
        Tx_Serial_parameter(out,BAD_PARAMETER);
        number_of_bips = 2;
        break;
      }

      case BAD_REQUEST:{
        Tx_Serial_parameter(out,BAD_REQUEST);
        number_of_bips = 2;
        break;
      }
    }
  }  
  
  if( TimeFunction() ){ // every one second
    byte out = 0;
    byte msg = 0;
    byte PowerTemp_output = 0;
    GetTemperature();
    UpgrateContinuosTime();

    if (number_of_bips){
      digitalWrite(BUZZER_PIN,LOW);
      number_of_bips--;
    }

    if(PowerOnHour(&out)){
      msg = HOUR_OPERATION;
      parameters[0] = true;
    }

    if( PowerTemp_output = PowerTemp(&out)){
      msg = PowerTemp_output;
      parameters[0] = true;
    }

    byte CheckOutput = CheckRules(&out);

    if (out){
      UpdateOutput();
      if (CheckOutput != OK){
        parameters[0] = true;
        Tx_Serial_parameter(out,CheckOutput);
      }
      else Tx_Serial_parameter(out,msg);
    }  

  }

  if (ReadInput()){
    number_of_bips = 1;
    byte out = DependecyControl();
    byte msg = CheckRules(&out);
    if (msg == OK)  msg = MANUAL_OPERATION;
    UpdateOutput();
    parameters[0] = true;
    Tx_Serial_parameter(out,msg);
  }

}




/************** Functions ***************/

/*********** Time Function **************/
byte TimeFunction(void){
  tiempo2 = millis();
  byte out = 0;
  if ( tiempo2 - tiempo1 >= 1000) {
    tiempo1=tiempo2;
    seconds++;   
    out = 1;
    if(pausa_boton) {
      pausa_boton++;
      if(pausa_boton == 2) pausa_boton = 0;
    }
  }

  if (seconds >= 60){
    seconds = 0;
    minutes++;

    for (int i =0;i<QUANTITY_OUTPUT;i++){
      if (Output[i].Status) Output[i].OperatingTime++;
    }

    if (minutes >= 60){
      minutes = 0;
      hours++;
      if (hours >= 24){
        hours = 0;
      }
    }
  }
  return out;
} // End Time Function

/*********** Get Temperature ************/
void GetTemperature(void){
  float R;
  float T;
  float aux;
  for (int i = 0; i<TEMPERATURE_SAMPLES; i++){
    Temp1 = Temp1 + analogRead(INPUT_TEMP_1);
    Temp2 = Temp2 + analogRead(INPUT_TEMP_2);
  }
  Temp1 = Temp1 / (TEMPERATURE_SAMPLES+1);
  Temp2 = Temp2 / (TEMPERATURE_SAMPLES+1);
  Temp1 = Temp1 * 5 / 1023;
  R = 10000 * Temp1 /(5-Temp1);


  aux = log(R);
  aux = aux * aux * aux;
  T = (A_COEFICIENT+B_COEFICIENT*log(R)+C_COEFICIENT*aux);
  T = 1 / T;
  Temp1 = T - 273.15;
  Temp2 = Temp2 * 5 / 1023;
  R = 10000 * Temp2 /(5-Temp2);
  aux = log(R);
  aux = aux * aux * aux;
  T = (A_COEFICIENT+B_COEFICIENT*log(R)+C_COEFICIENT*aux);
  T = 1 / T;
  Temp2 = T - 273.15;
}

/************ Get Current ***************/
void getCurrent (void){

  if (tiempo2-tiempo5 >=1){
    tiempo5 = tiempo2;
    if (analogRead(CURRENT) > 600){
      for(int i=0; i<QUANTITY_OUTPUT;i++){
        digitalWrite(OutputPins[i],LOW);
        Output[i].Status = false;
      }
      Tx_Serial_parameter(0,SHUTDOWN_CURRENT_FAIL);
    }
  }
}

/********** Read AC Input ***************/
byte ReadInput(void){
  byte InputOn = 0;
  if (!(digitalRead(7)&&digitalRead(8)&&digitalRead(9)&&digitalRead(10))&&pausa_boton==0){ 
    pausa_boton = tiempo2;
    for ( int i = 0 ; i< QUANTITY_INPUT ; i++ ){
       if (!digitalRead(7+i)) InputStatus[i] = 1;
    }
    
  }

  // level detector
  if (tiempo2 - tiempo4 >= 500){
    tiempo4 = tiempo2;
    StatusLevel = false;
  }

  if (tiempo2 - tiempo4 == 350){
    if (number_of_bips){
      digitalWrite(BUZZER_PIN,HIGH);
    }
  }

  if (tiempo2 - tiempo4 >= 400){
    if (number_of_bips){
      digitalWrite(BUZZER_PIN,LOW);
    }
    if (!digitalRead(INPUT_LEVEL_PIN)) {
      StatusLevel = true;
      tiempo4 = tiempo2;
    }
  }


  if (pausa_boton){
    if(tiempo2 - pausa_boton>1000){
       pausa_boton = 0;
    }
  }

  if(tiempo2-tiempo3>=200 && (InputStatus[0]||InputStatus[1]||InputStatus[2]||InputStatus[3])){
    tiempo3=tiempo2;
    InputOn = 1;
  }

  return InputOn;
}

/******** Dependency control ************/
byte DependecyControl(void){
  byte j = 0;
  for ( int i = 0 ; i< QUANTITY_INPUT ; i++ ){
    if (InputStatus[i]){ // hay una entrada pulsada
      byte k = InputSequence[i][0]; // Posicion en secuencia
      j = (InputSequence[i][k])-1; // Valor de salida en Secuencia
      if (Output[j].Status == true){
        Output[j].Status = false;
        if (InputSequence[i][k+1]) InputSequence[i][0]=k+1;
        else InputSequence[i][0]=1; // Vuelvo al inicio
      }
      else {
        Output[j].Status = true;
        byte CheckRules_out = 0;
        if (CheckRules(&CheckRules_out) != OK ){
          if (InputSequence[i][k+1]) InputSequence[i][0]=k+1;
          else InputSequence[i][0]=1; // Vuelvo al inicio
        }
        Output[j].Status = true;
      }
    }
    InputStatus[i]=LOW;
  }
  return j+1;
}

/********** Clean Output ****************/
void CleanOutput(byte in){
  Output[in].Status = false;
  Output[in].OperatingTime = 0;
  Output[in].Tmax = 0;
  Output[in].Ton = 0;
  Output[in].ContinuousTime = 0;
  Output[in].HourOn = 100;
  Output[in].MinuteOn = 100;
  Output[in].Control = 0;
  Output[in].Level = 0;
  Output[in].Forzar = 0;
}

/****** Update input sequences **********/
void LoadInputSequences(void){
  
  for(int i=0;i<QUANTITY_INPUT;i++){
    InputSequence[i][0]=1;
    for(int j=0;j<QUANTITY_OUTPUT ;j++){
      InputSequence[i][j+1]=0;
    }
  }
  
  for(int i=0;i<QUANTITY_INPUT;i++){ //filas
    for(int j=0;j<QUANTITY_OUTPUT ;j++){//columnas
      byte k = InputSequence[i][0];
      byte result = Output[j].Control / 10;
      result = Output[j].Control - result * 10;
      if (result == i+1){
          InputSequence[i][k] = j+1;
          InputSequence[i][0]=k+1;
          k=k+1;
      }
    }
  }

  for (int i = 0;i<QUANTITY_INPUT;i++){
    InputSequence[i][0]=1;
  }
}

/**** Rx data from Serail port **********/
byte Rx_Serial_input(byte *out){
  byte msg = 0;
  *out = 0;
  byte output_vector_number = 0;
  //StaticJsonDocument<MEMORIA_JSON> doc;
  DynamicJsonDocument doc(MEMORIA_JSON);
  // Deserializo la entra 
  DeserializationError error = deserializeJson(doc, Serial);
  int buf = 0;
  // Verifico que se trate de formato JSON
  if (error) {
    return 44;
  }

  if (doc.containsKey(MSG)){
    msg = doc[MSG];
    switch (msg){
      case OK:                      return OK; // Go out whit any change
      case BAD_REQUEST:             return OK;
      case BAD_PARAMETER:           return OK;
      case SV_ERROR_REQUEST:        return OK; 
      case CONTROL_TEMP_ON:         return OK;
      case CONTROL_TEMP_OFF:        return OK;
      case MANUAL_OPERATION:        return OK; 
      case HOUR_OPERATION:          return OK;
      case SHUNTDOWN_LEVEL:         return OK;
      case SHUNTDOWN_TIME_ON:       return OK;
      case SHUTDOWN_CURRENT_FAIL:   return OK;
      case REQUEST_ADD_OUTPUT:      break; // keep going
      case RESPONSE_ADD_OUTPUT:     return OK;
      case REQUEST_DELETE_OUTPUT:   break;
      case RESPONSE_DELETE_OUTPUT:  return OK;
      case REQUEST_EDIT_PARAMETER:  break;
      case RESPONSE_EDIT_PARAMETER: return OK;
      case REQUEST_EDIT_TEMP:       break;
      case RESPONSE_EDIT_TEMP:      return OK;
      case REQUEST_OPERATING_TIME:  break;
      case RESPONSE_OPERATING_TIME: return OK;
      case REQUEST_ALL_PARAMETER:   break;
      case RESPONSE_ALL_PARAMETER:  return OK;
      default: return BAD_PARAMETER;
    }
  }
  else return msg;

  if (doc.containsKey(HOUR)){
    String aux2 = doc[HOUR];
    if (aux2.length() <= 10 ){
      int h;
      char aux[aux2.length()+1];
      aux2.toCharArray(aux,aux2.length()+1);
      sscanf(aux, "%d:%d:%d", &h, &minutes,&seconds);
      hours = h;
    }
    else return BAD_PARAMETER;
  }

  if (msg == REQUEST_EDIT_TEMP){
    if (doc[DATA] == NULL) return BAD_REQUEST;
    if (doc[DATA][SET_TEMPERATURA]){
      parameters[8] = true;
      if (doc[DATA][SET_TEMPERATURA]>= 0 && doc[DATA][SET_TEMPERATURA]<= MAX_SET)
        TempSet = doc[DATA][SET_TEMPERATURA];
      else
        return BAD_PARAMETER;
    }
    else return BAD_REQUEST;
  }

  if (msg == REQUEST_ADD_OUTPUT || msg == REQUEST_EDIT_PARAMETER||msg == REQUEST_DELETE_OUTPUT){
    *out = doc[DATA][OUTPUT_NUMBER];
    if (*out > 0 && *out <= QUANTITY_OUTPUT){
      output_vector_number = (*out)-1;
      if (doc[DATA].containsKey(TIME_ON)){
        parameters[1] = true;
        buf = doc[DATA][TIME_ON];
        if (buf  >= 0 && buf <= 1440 ) Output[output_vector_number].Ton = buf;
        else return BAD_PARAMETER;
      }

      if (doc[DATA].containsKey(TIME_MAX)){
        parameters[3] = true;
        buf = doc[DATA][TIME_MAX];
        if (buf  >= 0 && buf <= 65535 ) Output[output_vector_number].Tmax = buf;
        else return BAD_PARAMETER;
      }
      
      if (doc[DATA].containsKey(OPERATING_TIME)){
        parameters[4] = true;
        buf = doc[DATA][OPERATING_TIME];
        if (buf  >= 0 && buf <= 65535 ) Output[output_vector_number].OperatingTime = buf;
        else return BAD_PARAMETER;
      }

      if (doc[DATA].containsKey(CONTROL)){
        parameters[6] = true;
        buf = doc[DATA][CONTROL];
        if (buf  >= 0 && buf <= 64) Output[output_vector_number].Control = buf;
        else return BAD_PARAMETER;
      }

      if (doc[DATA].containsKey(LEVEL)){
        parameters[5] = true;
        buf = doc[DATA][LEVEL];
        if ( buf  >= 0 && buf <= 2 ) Output[output_vector_number].Level = buf;
        else return BAD_PARAMETER;
      }

      if (doc[DATA].containsKey(FORZAR)){
        parameters[7] = true;
        buf = doc[DATA][FORZAR];
        if (buf  >= 0 && buf <= 10 ) Output[output_vector_number].Forzar = buf;
        else return BAD_PARAMETER;
      } 

      if (doc[DATA].containsKey(STATUS)){
        parameters[0] = true;
        buf = doc[DATA][STATUS];
        if (buf == true || buf == false ) Output[output_vector_number].Status = buf;
        else return BAD_PARAMETER;
      }

      if (doc[DATA].containsKey(HOUR_ON)){
          parameters[2] = true;
          String aux2 = doc[DATA][HOUR_ON];
          if(aux2){ 
        if (aux2.length() <=7 ){
            char aux[aux2.length()+1];
            aux2.toCharArray(aux,aux2.length()+1);
              sscanf(aux, "%d:%d", &Output[output_vector_number].HourOn, &Output[output_vector_number].MinuteOn);  
          }
          else  return BAD_PARAMETER;
          }
      }
    }
    else return BAD_PARAMETER;
    
  } // FIN loop para cada salida

  return msg;
}

/**** Tx parameters to Serial port ******/
void Tx_Serial_parameter(byte out,byte msg){

  
  DynamicJsonDocument doc(MEMORIA_JSON);
  // Allways send
  JsonObject data = doc.createNestedObject(DATA);
  data[ID] = NUMBER_ID;
  doc[MSG] = msg;

  if ( msg == RESPONSE_EDIT_TEMP){
    data[SET_TEMPERATURA] = TempSet;
  }

  if (msg == RESPONSE_OPERATING_TIME){
    JsonArray data_Temp = data.createNestedArray(TEMP);
    data_Temp.add(Temp1);
    data_Temp.add(Temp2);
    JsonArray data_opertionTime = data.createNestedArray(OPERATING_TIME);
    for(int i=0;i<QUANTITY_OUTPUT;i++){
      data_opertionTime.add(Output[i].OperatingTime);
    }
  }

  if (out){
    data[OUTPUT_NUMBER] = out;
    out = out -1;
    // Se manda según el pedido
    if (parameters[0] ){ //Status
      data[STATUS] = Output[out].Status;
    }

    if (parameters[1]){ // TIME_ON
      data[TIME_ON] = Output[out].Ton;
    }
    
    if (parameters[2]){ // HOUR_ON
      String dato = "";
      if (Output[out].HourOn <24 && Output[out].MinuteOn <60){
        if (Output[out].HourOn <10) dato = "0";
          dato.concat(String(Output[out].HourOn));
          dato.concat(":");
          if (Output[out].MinuteOn <10) dato.concat("0");
          dato.concat(String(Output[out].MinuteOn));
          data[HOUR_ON] = dato;
        }
        else data[HOUR_ON] = "xx:xx";
    }

    if (parameters[3]){ // TIME_MAX
      data[TIME_MAX] = Output[out].Tmax;
    }
    
    if (parameters[4]){ // OPERATING_TIME
      data[OPERATING_TIME] = Output[out].OperatingTime;
    }

    if (parameters[5]){ // LEVEL
      data[LEVEL] = Output[out].Level;
    }

    if (parameters[6]){ // CONTROL
      data[CONTROL] = Output[out].Control;
    }

    if (parameters[7]){ // FORZAR
      data[FORZAR]= Output[out].Forzar;
    }

    if (msg == RESPONSE_ALL_PARAMETER){
       String dato = "";
        if (hours <10) dato = "0";
          dato.concat(String(hours));
          dato.concat(":");
          if (minutes <10) dato.concat("0");
          dato.concat(String(minutes));
          doc[HOUR] = dato;
    }
  }

  serializeJson(doc,Serial);
  Serial.print("\n");
}// FIN TxSerial

/******** Upgrate operation time ********/
void UpgrateContinuosTime(void){ // every second
  for (int i=0;i<QUANTITY_OUTPUT ;i++){
    if(Output[i].Status){
      Output[i].ContinuousTime = Output[i].ContinuousTime+1;
    }
    else Output[i].ContinuousTime = 0;
  }  
}

/*********** Check rules ****************/
byte CheckRules(byte *out){
  byte salida = OK;
  for( int i = 0; i<QUANTITY_OUTPUT  ;i++ ){
      if (Output[i].Status){
        //Verificación que esté lleno
        if ( !StatusLevel ){
          if (Output[i].Level == 1){
            Output[i].Status = false;
            salida = SHUNTDOWN_LEVEL;
            *out = i+1;
          }
        } 

        //Verificación que esté vacío
        else{
          if(Output[i].Level == 2){
             Output[i].Status = false;
             salida = SHUNTDOWN_LEVEL;
             *out = i+1;
          }
        }

       //Verificación Ton
       if(Output[i].Ton && (Output[i].ContinuousTime / 60 ) >= Output[i].Ton){
          Output[i].Status = false;
          salida = SHUNTDOWN_TIME_ON;
          *out = i+1;
       }

      //Verificación Energia maxima
      if(Output[i].Tmax && Output[i].OperatingTime  >= Output[i].Tmax){
        Output[i].Status = false;
        salida = SHUNTDOWN_TMAX;
        *out = i+1;
      }    
     }    
  }// End FOR Output
  if (salida != OK) number_of_bips++;
  return salida;
}// End function 

/********** Update Output ***************/
void UpdateOutput(void){
  for(int i=0;i<QUANTITY_OUTPUT ;i++){
    if(Output[i].Status == true){
      if (Output[i].Control < 10) digitalWrite(OutputPins[i],HIGH);
    } 
    else digitalWrite(OutputPins[i],LOW);
  }
}

/******** Power ON by Hour control ******/
byte PowerOnHour(byte *out){
  *out = 0;
  for (int i = 0; i<QUANTITY_OUTPUT ; i++ ){
    if (Output[i].Status == false){
      if (Output[i].Ton && Output[i].HourOn < 24 && Output[i].MinuteOn < 60){
        if (hours == Output[i].HourOn){
          if(minutes == Output[i].MinuteOn){
            Output[i].Status = true;
            *out = i+1;
          }
        }
      }
    }
  }
  return *out; 
} 

/***** Load parameters from memory ******/
void load(void){
  int espacio = 0;
  for(int i = 0; i<QUANTITY_OUTPUT ;i++){
    Output[i].HourOn = EEPROM.read(espacio); // Byte
    espacio++;
  }  
  for(int i = 0; i<QUANTITY_OUTPUT ;i++){
   Output[i].MinuteOn = EEPROM.read(espacio); // Byte
   espacio++;
  }  
  for(int i = 0; i<QUANTITY_OUTPUT ;i++){
    Output[i].Control = EEPROM.read( espacio); // Byte
    espacio++;
  }  
  for(int i = 0; i<QUANTITY_OUTPUT ;i++){
    Output[i].Forzar = EEPROM.read( espacio); // Byte
    espacio++;
  }
  for(int i = 0; i<QUANTITY_OUTPUT ;i++){
     Output[i].Level = EEPROM.read(espacio); // Byte
     espacio++;
  }
  for(int i = 0; i<QUANTITY_OUTPUT ;i++){
    byte aux = 0;
    byte aux2 = 0;
    aux = EEPROM.read(espacio); // high Unsigned int
    espacio++;
    aux2  = EEPROM.read(espacio); // low Unsigned int
    Output[i].Tmax = word(aux,aux2);
    espacio++;
  }
  for(int i = 0; i<QUANTITY_OUTPUT ;i++){
    byte aux = 0;
    byte aux2 = 0;
    aux= EEPROM.read( espacio); // high Unsigned int 3
    espacio++;
    aux2 = EEPROM.read( espacio); // low Unsigned int 3
    Output[i].Ton = word(aux,aux2);
    espacio++;
  }
  for(int i = 0; i<QUANTITY_OUTPUT ;i++){
    byte aux = 0;
    byte aux2 = 0;
    aux= EEPROM.read( espacio); // high Unsigned int 3
    espacio++;
    aux2 = EEPROM.read( espacio); // low Unsigned int 3
    Output[i].OperatingTime = word(aux,aux2);
    espacio++;
  }
  byte aux = EEPROM.read(espacio);
  espacio++;
  byte aux2 = EEPROM.read(espacio);
  int aux3 = word(aux,aux2);
  TempSet = aux3/10;
} // Enda load

/***** Save parameters to memory ********/
void save(void){
  int espacio = 0;
  for(int i = 0; i<QUANTITY_OUTPUT ;i++){
    EEPROM.write( espacio, Output[i].HourOn); // Byte
    espacio++;
  }
  for(int i = 0; i<QUANTITY_OUTPUT ;i++){
    EEPROM.write( espacio, Output[i].MinuteOn ); // Byte
    espacio++;
  }
  for(int i = 0; i<QUANTITY_OUTPUT ;i++){
    EEPROM.write(espacio, Output[i].Control ); // Byte
    espacio++;
  }
  for(int i = 0; i<QUANTITY_OUTPUT ;i++){
    EEPROM.write(espacio, Output[i].Forzar ); // Byte
    espacio++;
  }
  for(int i = 0; i<QUANTITY_OUTPUT ;i++){
    EEPROM.write(espacio, Output[i].Level ); // Byte
    espacio++;
  }
  for(int i = 0; i<QUANTITY_OUTPUT ;i++){
    byte aux = highByte (Output[i].Tmax);
    byte aux2 = lowByte (Output[i].Tmax);
    EEPROM.write(espacio, aux ); // high Unsigned int
    espacio++;
    EEPROM.write(espacio, aux2 ); // low Unsigned int
    espacio++;
  }
  for(int i = 0; i<QUANTITY_OUTPUT ;i++){
    byte aux = highByte (Output[i].Ton);
    byte aux2 = lowByte (Output[i].Ton);
    EEPROM.write(espacio, aux ); // high Unsigned int 3
    espacio++;
    EEPROM.write( espacio, aux2 ); // low Unsigned int 3
    espacio++;
  }
  for(int i = 0; i<QUANTITY_OUTPUT ;i++){
    byte aux = highByte (Output[i].OperatingTime);
    byte aux2 = lowByte (Output[i].OperatingTime);
    EEPROM.write(espacio, aux ); // high Unsigned int 3
    espacio++;
    EEPROM.write( espacio, aux2 ); // low Unsigned int 3
    espacio++;
  }
  byte aux = highByte ((int)(TempSet*10));
  EEPROM.write(espacio,aux);
  espacio++;
  aux = lowByte((int)(TempSet*10));
  EEPROM.write(espacio,aux);

}// End save

/******** Power by temperature **********/ 
byte PowerTemp(byte *out){
  for (int i =0; i< QUANTITY_OUTPUT; i++){
    if (Output[i].Status){
      if (Output[i].Control>9){
        byte result = Output[i].Control / 10;
        switch (result){
          case 1:{
            if (Temp1>Temp2){
              if(!digitalRead(OutputPins[i])){
                digitalWrite(OutputPins[i],HIGH);
                *out = i+1;
                return CONTROL_TEMP_ON;
              } 
            }
            else{
              if(digitalRead(OutputPins[i])){
                digitalWrite(OutputPins[i],LOW);
                *out = i+1;
                return CONTROL_TEMP_OFF;
              } 
            }
            break;
          }

          case 2:{
            if (Temp2>Temp1){
              if(!digitalRead(OutputPins[i])){
                digitalWrite(OutputPins[i],HIGH);
                *out = i+1;
                return CONTROL_TEMP_ON;
              } 
            }
            else{
              if(digitalRead(OutputPins[i])){
                digitalWrite(OutputPins[i],LOW);
                *out = i+1;
                return CONTROL_TEMP_OFF;
              } 
            }
            break;
          }

          case 3:{
            if (TempSet>Temp1){
              if(!digitalRead(OutputPins[i])){
                digitalWrite(OutputPins[i],HIGH);
                *out = i+1;
                return CONTROL_TEMP_ON;
              } 
            }
            else{
              if(digitalRead(OutputPins[i])){
                digitalWrite(OutputPins[i],LOW);
                *out = i+1;
                return CONTROL_TEMP_OFF;
              } 
            }
            break;
          }

          case 4:{
            if (TempSet<Temp1){
              if(!digitalRead(OutputPins[i])){
                digitalWrite(OutputPins[i],HIGH);
                *out = i+1;
                return CONTROL_TEMP_ON;
              } 
            }
            else{
              if(digitalRead(OutputPins[i])){
                digitalWrite(OutputPins[i],LOW);
                *out = i+1;
                return CONTROL_TEMP_OFF;
              } 
            }
            break;
          }

          case 5:{
            if (TempSet>Temp2){
              if(!digitalRead(OutputPins[i])){
                digitalWrite(OutputPins[i],HIGH);
                *out = i+1;
                return CONTROL_TEMP_ON;
              } 
            }
            else{
              if(digitalRead(OutputPins[i])){
                digitalWrite(OutputPins[i],LOW);
                *out = i+1;
                return CONTROL_TEMP_OFF;
              } 
            }
            break;
          }

          case 6:{
            if (TempSet<Temp2){
              if(!digitalRead(OutputPins[i])){
                digitalWrite(OutputPins[i],HIGH);
                *out = i+1;
                return CONTROL_TEMP_ON;
              } 
            }
            else{
              if(digitalRead(OutputPins[i])){
                digitalWrite(OutputPins[i],LOW);
                *out = i+1;
                return CONTROL_TEMP_OFF;
              } 
            }
            break;
          }
        }                                                                                                                                     
      }
    }
  }
  return 0;
}

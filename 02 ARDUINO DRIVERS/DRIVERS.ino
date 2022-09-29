//Pump Variables
const int SP_AI_PumpFromPLC = A0; //Pin number for read Setpoint from PLC160 [0..5]volt - [0..1024] bit
float SPPumpFromPLC; //Setpoint from PLC160
const int MV_Min_To_PumpDriver = 8;
const int MV_Max_To_PumpDriver = 7;
const int MV_PWMBoreholeToDriver = 9; //pin number for PWM pump control signal
int MV_PWM_min = 107; // The minimum PWM value at which the pump operates corresponds to 2 Volts
int MV_PumpDriver; // Manipulated Variable PWM signal for pump driver

//Level Transmitter (LT) Variables
const int LTecho = 12;// Level transmitter pin Echo ai3+d11 ai1+d5 ai2+d6 ao4
const int LTtrig = 13;// Level transmitter pin Trig
long k;// вывод значения уровня на плк
//YF-S201
volatile int flow_frequencyS201; // измеряет частоту
float l_minuteS201; // рассчитанные литр/минуту
unsigned char flowsensorS201 = 3; // Вход сенсора
float flowcS201;
unsigned long currentTimeS201;
unsigned long cloopTimeS201;
long x;//вывод значения расходомера на плк

//YF-B1
volatile int flow_frequencyB1; // измеряет частоту
float l_minuteB1; // рассчитанные литр/минуту
unsigned char flowsensorB1 = 2; // Вход сенсора
float flowcB1;
unsigned long currentTimeB1;
unsigned long cloopTimeB1;
long z;// вывод значения расходомера на плк

//YF-S201
void flowS201 () /*функция прерывания,При обнаружении нарастающего фронта 
импульса срабатывает прерывание, считая плюс один.*/
{
flow_frequencyS201++;
}

//YF-B1
void flowB1 () /*функция прерывания,При обнаружении нарастающего фронта 
импульса срабатывает прерывание, считая плюс один.*/
{
flow_frequencyB1++;
}

void setup()
{
  Serial.begin(9600);
  /*
//Pump etup
  pinMode(SP_AI_PumpFromPLC, INPUT);
  pinMode(MV_Min_To_PumpDriver, OUTPUT);
  pinMode(MV_Max_To_PumpDriver, OUTPUT);
  pinMode(MV_PWMBoreholeToDriver, OUTPUT);*/
  
  //YF-S201
  pinMode(flowsensorS201, INPUT); //поступают данные на вход
  digitalWrite(flowsensorS201, HIGH);/*Если вход/выход (pin) был установлен в режим вход (INPUT), 
  то функция digitalWrite со значением HIGH будет активировать 
  внутренний 20K нагрузочный резистор*/
  attachInterrupt(1, flowS201, RISING); // настраиваем прерывания
  sei(); // функция разрешающая прерывания
  currentTimeS201 = millis();//Возвращает количество миллисекунд, прошедших с запуска
  cloopTimeS201 = currentTimeS201;

  //YF-B1
  pinMode(flowsensorB1, INPUT); //поступают данные на вход
  digitalWrite(flowsensorB1, HIGH);/*Если вход/выход (pin) был установлен в режим вход (INPUT), 
  то функция digitalWrite со значением HIGH будет активировать 
  внутренний 20K нагрузочный резистор*/
  attachInterrupt(0, flowB1, RISING); // настраиваем прерывания
  sei(); // функция разрешающая прерывания
  currentTimeB1 = millis();//Возвращает количество миллисекунд, прошедших с запуска
  cloopTimeB1 = currentTimeB1;
  
  //Level Transmitter Setup
  pinMode(LTecho,INPUT);//задаем эхо на вход в ардуино
  pinMode(LTtrig,OUTPUT);//задаем триг на вход в ардуино
}

void PumpDriverControl(bool MV_Max_To_PumpDriver_Value, bool MV_Min_To_PumpDriver_Value) //Control Pump Driver
{
    digitalWrite(MV_Min_To_PumpDriver, MV_Min_To_PumpDriver_Value);
    digitalWrite(MV_Max_To_PumpDriver, MV_Max_To_PumpDriver_Value); 
}

void PumpControl()
{
  SPPumpFromPLC = analogRead(SP_AI_PumpFromPLC) / 1024.0 * 4.7; //read Setpoint from PLC160 [0..5]Volt

  if (SPPumpFromPLC <= 0.3) // Stop Pump. 51 - Число выбранное из диапазона управляющего сигнала 0-5В представленного в цифровом виде 0-1023 и равное 0.25 В
  {
    PumpDriverControl(LOW, LOW);
    MV_PumpDriver = 0;
  }
  else if ((MV_PumpDriver == 0) && (SPPumpFromPLC > 0.3)) //Maximum Control Value for start pump
  {
    PumpDriverControl(HIGH, LOW);
    MV_PumpDriver = 255;   
  } 
  else  //Main work regime
  { 
    PumpDriverControl(HIGH, LOW); 
    MV_PumpDriver = map(analogRead(SP_AI_PumpFromPLC), 0, 1023, MV_PWM_min, 255); // Чтение (analogRead(c)) и дальнейшее преобразование входного управляющего сигнала 0-5В в выходной ШИМ сигнал 
    } 

  analogWrite(MV_PWMBoreholeToDriver, MV_PumpDriver); //Write PWM signal to AO
}

void LevelTransmitter()
{ 
  digitalWrite(LTtrig, LOW); // Sensor stoped
  delayMicroseconds(2); // Delay 2 microsecond
  digitalWrite(LTtrig, HIGH); //Send Trig to sensor
  delayMicroseconds(10); // Delay 10 microsecond
  digitalWrite(LTtrig, LOW); // Trig Stop
  float duration = pulseIn(LTecho, HIGH); // Recieve echo On
  float UltrasoundSpeed = 340 / 10^6 * 100; //[cm/s]
  float distance = 30 - duration / 58; // Calculate the distance in centimeters
  Serial.print(distance);
  Serial.println(" cm");
  k = distance * 8.5;//Преобразование см в аналоговый сигнал, где 8.5 - коэффициент преобразования
  if (k > 255)
  {
    k = 255;
  }
  if (k < 0)
  {
    k = 0;
  }
  analogWrite(11, k);
}

//Расходомер YF-S201
void RasxodS201()
{
  currentTimeS201 = millis();
  // Каждую секунду рассчитываем и выводим на экран литры в час
  if(currentTimeS201 >= (cloopTimeS201 + 1000))
{
  cloopTimeS201 = currentTimeS201; // Обновление cloopTime
  flowcS201 = flow_frequencyS201; 
  // Частота импульсов (Гц) = 7.5Q, Q - это расход в л/мин.
  l_minuteS201 = (flowcS201 / 7.5); // Частота / 7.5Q = расход в л/минуту
  flow_frequencyS201 = 0; // Сбрасываем счетчик
  //Serial.print(flowcS201, DEC);
  Serial.print(l_minuteS201, DEC); // Отображаем л/мин
  Serial.println(" L/minute");
  x = l_minuteS201 * 8.5;
 if (x > 255)
  {
    x = 255;
  }
  if (x < 0)
  {
    x = 0;
  }
  analogWrite(6, x);
}
}

//YF-B1
void RasxodB1()
{
  currentTimeB1 = millis();
  // Каждую секунду рассчитываем и выводим на экран литры в час
  if(currentTimeB1 >= (cloopTimeB1 + 1000))
{
  cloopTimeB1 = currentTimeB1;// Обновление cloopTime
  flowcB1 = flow_frequencyB1; 
  // Частота импульсов (Гц) = 7.5Q, Q - это расход в л/мин.
  l_minuteB1 = (flowcB1 / 11); // Частота / 7.5Q = расход в л/минуту
  flow_frequencyB1 = 0; // Сбрасываем счетчик
  Serial.print(l_minuteB1, DEC); // Отображаем л/час
  Serial.println(" L/minute");
  z = l_minuteB1 * 8.5;
 if (z > 255)
  {
    z = 255;
  }
  if (z < 0)
  {
    z = 0;
  }
  analogWrite(5, z);
}
}

void loop()
{
 PumpControl();
 LevelTransmitter();
 RasxodS201();
 RasxodB1();
 delay(100);
}   

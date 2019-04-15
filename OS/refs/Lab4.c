//*****************************************************************************
//
// Lab4.c - user programs, File system, stream data onto disk
//*****************************************************************************

// Jonathan W. Valvano 3/7/17, valvano@mail.utexas.edu
// EE445M/EE380L.6
// You may use, edit, run or distribute this file
// You are free to change the syntax/organization of this file to do Lab 4
// as long as the basic functionality is simular
// 1) runs on your Lab 2 or Lab 3
// 2) implements your own eFile.c system with no code pasted in from other sources
// 3) streams real-time data from robot onto disk
// 4) supports multiple file reads/writes
// 5) has an interpreter that demonstrates features
// 6) interactive with UART input, and switch input

// LED outputs to logic analyzer for OS profile
// PF1 is preemptive thread switch
// PF2 is periodic task
// PF3 is SW1 task (touch PF4 button)

// Button inputs
// PF0 is SW2 task
// PF4 is SW1 button input

// Analog inputs
// PE3 sequencer 3, channel 3, J8/PE0, sampling in DAS(), software start
// PE0 timer-triggered sampling, channel 0, J5/PE3, 50 Hz, processed by Producer
//******Sensor Board I/O*******************
// **********ST7735 TFT and SDC*******************
// ST7735
// Backlight (pin 10) connected to +3.3 V
// MISO (pin 9) unconnected
// SCK (pin 8) connected to PA2 (SSI0Clk)
// MOSI (pin 7) connected to PA5 (SSI0Tx)
// TFT_CS (pin 6) connected to PA3 (SSI0Fss)
// CARD_CS (pin 5) connected to PB0
// Data/Command (pin 4) connected to PA6 (GPIO), high for data, low for command
// RESET (pin 3) connected to PA7 (GPIO)
// VCC (pin 2) connected to +3.3 V
// Gnd (pin 1) connected to ground

// HC-SR04 Ultrasonic Range Finder
// J9X  Trigger0 to PB7 output (10us pulse)
// J9X  Echo0    to PB6 T0CCP0
// J10X Trigger1 to PB5 output (10us pulse)
// J10X Echo1    to PB4 T1CCP0
// J11X Trigger2 to PB3 output (10us pulse)
// J11X Echo2    to PB2 T3CCP0
// J12X Trigger3 to PC5 output (10us pulse)
// J12X Echo3    to PF4 T2CCP0

// Ping))) Ultrasonic Range Finder
// J9Y  Trigger/Echo0 to PB6 T0CCP0
// J10Y Trigger/Echo1 to PB4 T1CCP0
// J11Y Trigger/Echo2 to PB2 T3CCP0
// J12Y Trigger/Echo3 to PF4 T2CCP0

// IR distance sensors
// J5/A0/PE3
// J6/A1/PE2
// J7/A2/PE1
// J8/A3/PE0

// ESP8266
// PB1 Reset
// PD6 Uart Rx <- Tx ESP8266
// PD7 Uart Tx -> Rx ESP8266

// Free pins (debugging)
// PF3, PF2, PF1 (color LED)
// PD3, PD2, PD1, PD0, PC4

#include "OS.h"
#include "tm4c123gh6pm.h"
#include "ST7735.h"
#include "ADC.h"
#include "UART.h"
#include "interpreter.h"

#include "ff.h"
#include "diskio.h"
#include "motors.h"
#include <string.h>
#include <stdio.h>
#include <stdint.h>

//*********Prototype for FFT in cr4_fft_64_stm32.s, STMicroelectronics
void cr4_fft_64_stm32(void *pssOUT, void *pssIN, unsigned short Nbin);

#define PF0 (*((volatile unsigned long *)0x40025004))
#define PF1 (*((volatile unsigned long *)0x40025008))
#define PF2 (*((volatile unsigned long *)0x40025010))
#define PF3 (*((volatile unsigned long *)0x40025020))
#define PF4 (*((volatile unsigned long *)0x40025040))

#define PD0 (*((volatile unsigned long *)0x40007004))
#define PD1 (*((volatile unsigned long *)0x40007008))
#define PD2 (*((volatile unsigned long *)0x40007010))
#define PD3 (*((volatile unsigned long *)0x40007020))
static FATFS g_sFatFs;

void PortD_Init(void)
{
  SYSCTL_RCGCGPIO_R |= 0x08; // activate port D
  while ((SYSCTL_PRGPIO_R & 0x08) == 0)
  {
  };
  GPIO_PORTD_DIR_R |= 0x0F;    // make PE3-0 output heartbeats
  GPIO_PORTD_AFSEL_R &= ~0x0F; // disable alt funct on PD3-0
  GPIO_PORTD_DEN_R |= 0x0F;    // enable digital I/O on PD3-0
  GPIO_PORTD_PCTL_R = ~0x0000FFFF;
  GPIO_PORTD_AMSEL_R &= ~0x0F; // disable analog functionality on PD
}
unsigned long NumCreated; // number of foreground threads created
unsigned long NumSamples; // incremented every sample
unsigned long DataLost;   // data sent by Producer, but not received by Consumer
unsigned long PIDWork;    // current number of PID calculations finished
unsigned long FilterWork; // number of digital filter calculations finished

int Running; // true while robot is running

#define TIMESLICE 2 * TIME_1MS // thread switch time in system time units
long x[64], y[64];             // input and output arrays for FFT
Sema4Type doFFT;               // set every 64 samples by DAS

long median(long u1, long u2, long u3)
{
  long result;
  if (u1 > u2)
    if (u2 > u3)
      result = u2; // u1>u2,u2>u3       u1>u2>u3
    else if (u1 > u3)
      result = u3; // u1>u2,u3>u2,u1>u3 u1>u3>u2
    else
      result = u1; // u1>u2,u3>u2,u3>u1 u3>u1>u2
  else if (u3 > u2)
    result = u2; // u2>u1,u3>u2       u3>u2>u1
  else if (u1 > u3)
    result = u1; // u2>u1,u2>u3,u1>u3 u2>u1>u3
  else
    result = u3; // u2>u1,u2>u3,u3>u1 u2>u3>u1
  return (result);
}
//------------ADC2millimeter------------
// convert 12-bit ADC to distance in 1mm
// it is known the expected range is 100 to 800 mm
// Input:  adcSample 0 to 4095
// Output: distance in 1mm
long ADC2millimeter(long adcSample)
{
  if (adcSample < 494)
    return 799; // maximum distance 80cm
  return (268130 / (adcSample - 159));
}
long Distance3;  // distance in mm on IR3
uint32_t Index3; // counts to 64 samples
long x1, x2, x3;
void DAS(void)
{
  long output;
  //PD0 ^= 0x01;
  x3 = x2;
  x2 = x1;       // MACQ
  x1 = ADC_In(); // channel set when calling ADC_Init
 //PD0 ^= 0x01;
  if (Index3 < 64)
  {
    output = median(x1, x2, x3); // 3-wide median filter
    Distance3 = ADC2millimeter(output);
    FilterWork++; // calculation finished
    x[Index3] = Distance3;
    Index3++;
    if (Index3 == 64)
    {
      OS_Signal(&doFFT);
    }
  }
 // PD0 ^= 0x01;
}
void DSP(void)
{
  unsigned long DCcomponent; // 12-bit raw ADC sample, 0 to 4095
  //OS_InitSemaphore(&doFFT, 0);
  while (1)
  {
    OS_Wait(&doFFT); // wait for 64 samples
    //PD2 = 0x04;
    cr4_fft_64_stm32(y, x, 64); // complex FFT of last 64 ADC values
    //PD2 = 0x00;
    Index3 = 0;                  // take another buffer
    DCcomponent = y[0] & 0xFFFF; // Real part at frequency 0, imaginary part should be zero

    ST7735_Message(1, 0, "IR3 (mm) =", DCcomponent);
  }
}
char Name[8] = "robot0";
//******** Robot ***************
// foreground thread, accepts data from producer
// inputs:  none
// outputs: none
void Robot(void)
{
  unsigned long data;     // ADC sample, 0 to 1023
  unsigned long voltage;  // in mV,      0 to 3300
  unsigned long distance; // in mm,      100 to 800
  unsigned long time;     // in 10msec,  0 to 1000
  OS_ClearMsTime();
  DataLost = 0; // new run with no lost data
  //OS_Fifo_Init(256);
  printf("Robot running...");
  //eFile_RedirectToFile(Name); // robot0, robot1,...,robot7
  printf("time(sec)\tdata(volts)\tdistance(mm)\n\r");
  do
  {
    PIDWork++;                     // performance measurement
    time = OS_MsTime();            // 10ms resolution in this OS
    data = OS_Fifo_Get();          // 1000 Hz sampling get from producer
    voltage = (300 * data) / 1024; // in mV
    distance = ADC2millimeter(data);
    printf("%0u.%02u\t%0u.%03u \t%5u\n\r", time / 100, time % 100, voltage / 1000, voltage % 1000, distance);
  } while (time < 200); // change this to mean 2 seconds
  //eFile_EndRedirectToFile();
  ST7735_Message(0, 1, "IR0 (mm) =", distance);
  printf("done.\n\r");
  Name[5] = (Name[5] + 1) & 0xF7; // 0 to 7
  Running = 0;                    // robot no longer running
  OS_Kill();
}

//************SW1Push*************
// Called when SW1 Button pushed
// background threads execute once and return
void SW1Push(void)
{
  if (Running == 0)
  {
    Running = 1;                                // prevents you from starting two robot threads
    NumCreated += OS_AddThread(&Robot, 128, 1); // start a 2 second run
  }
}
//************SW2Push*************
// Called when SW2 Button pushed
// background threads execute once and return
void SW2Push(void)
{
}

//******** Producer ***************
// The Producer in this lab will be called from your ADC ISR
// A timer runs at 1 kHz, started by your ADC_Collect
// The timer triggers the ADC, creating the 1 kHz sampling
// Your ADC ISR runs when ADC data is ready
// Your ADC ISR calls this function with a 10-bit sample
// sends data to the Robot, runs periodically at 1 kHz
// inputs:  none
// outputs: none
void Producer(unsigned long data)
{
  if (Running)
  {
    if (OS_Fifo_Put(data))
    { // send to Robot
      NumSamples++;
    }
    else
    {
      DataLost++;
    }
  }
}

//******** IdleTask  ***************
// foreground thread, runs when no other work needed
// never blocks, never sleeps, never dies
// inputs:  none
// outputs: none
unsigned long Idlecount = 0;
void IdleTask(void)
{
  while (1)
  {
    Idlecount++; // debugging
  }
}

//******** Interpreter **************
// your intepreter from Lab 4
// foreground thread, accepts input from UART port, outputs to UART port
// inputs:  none
// outputs: none
void Interpreter(void)
{
  interpreter_task();
}
// TODO: add the following commands, remove commands that do not make sense anymore
// 1) format
// 2) directory
// 3) print file
// 4) delete file
// execute   eFile_Init();  after periodic interrupts have started

void init_fs_task(void)
{
}

//*******************lab 4 main **********
int realmain(void)
{               // lab 4 real main
  OS_Init();    // initialize, disable interrupts
  Running = 0;  // robot not running
  DataLost = 0; // lost data between producer and consumer
  NumSamples = 0;
  PortD_Init(); // user debugging profile
  f_mount(&g_sFatFs, "", 0);
  //********initialize communication channels
  OS_Fifo_Init(256);
  ADC_Collect(0, 50, &Producer);                // start ADC sampling, channel 0, PE3, 50 Hz
  ADC_Init(3);                                  // sequencer 3, channel 3, PE0, sampling in DAS()
  OS_AddPeriodicThread(&DAS, 10 * TIME_1MS, 1); // 100Hz real time sampling of PE0

  //*******attach background tasks***********
  OS_AddSW1Task(&SW1Push, 2); // PF4, SW1
  OS_AddSW2Task(&SW2Push, 3); // PF0
  OS_InitSemaphore(&doFFT, 0);

  NumCreated = 0;
  // create initial foreground threads
  NumCreated += OS_AddThread(&Interpreter, 128, 2);
  NumCreated += OS_AddThread(&DSP, 128, 3);
  NumCreated += OS_AddThread(&init_fs_task, 128, 1);
  //NumCreated += OS_AddThread(&IdleTask, 128, 7); // runs when nothing useful to do

  OS_Launch(TIMESLICE); // doesn't return, interrupts enabled in here
  return 0;             // this never executes
}

//*****************test project 2*************************

//******************* test main2 **********
// SYSTICK interrupts, period established by OS_Launch
// Timer interrupts, period established by first call to OS_AddPeriodicThread

#define PROG_STEPS (20)

void motor_task(void)
{
  static int step = 1;
  static int torque = -1000;
  // Motors_SetTorque(torque, torque);
  Motors_SetTorque(torque, torque);
  torque += step;
  if((torque > 1000) || (torque < -1000))
  {
    step = -step;
  }
}

int motor_testmain(void)
{
  OS_Init(); // initialize, disable interrupts
  Motors_Init();
  NumCreated = 0;
  // create initial foreground threads
  NumCreated += OS_AddPeriodicThread(&motor_task, 1 * TIME_1MS, 2);

  OS_Launch(TIMESLICE); // doesn't return, interrupts enabled in here
  return 0;             // this never executes
}

// Main stub
int main(void)
{
  return motor_testmain();
}

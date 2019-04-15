// Lab2.c
// Runs on LM4F120/TM4C123
// Real Time Operating System for Labs 2 and 3
// Lab2 Part 1: Testmain1 and Testmain2
// Lab2 Part 2: Testmain3 Testmain4  and main
// Lab3: Testmain5 Testmain6, Testmain7, and main (with SW2)

// Jonathan W. Valvano 2/20/17, valvano@mail.utexas.edu
// EE445M/EE380L.12
// You may use, edit, run or distribute this file
// You are free to change the syntax/organization of this file

// LED outputs to logic analyzer for OS profile
// PF1 is preemptive thread switch
// PF2 is periodic task, samples PD3
// PF3 is SW1 task (touch PF4 button)

// Button inputs
// PF0 is SW2 task (Lab3)
// PF4 is SW1 button input

// Analog inputs
// PD3 Ain3 sampled at 2k, sequencer 3, by DAS software start in ISR
// PD2 Ain5 sampled at 250Hz, sequencer 0, by Producer, timer tigger

#include "OS.h"
#include "tm4c123gh6pm.h"
#include "ST7735.h"
#include "ADC.h"
#include "PLL.h"
#include "UART.h"
#include "interpreter.h"
#include "memprotect.h"
#include "heap.h"
#include <string.h>

Sema4Type ptr_set;

volatile int *volatile illegal = 0;

void TaskA(void)
{
  int x = 1;
  int y = 1;
  x = x + y;
  y = x + y;
  illegal = (int *)Heap_Malloc(128);
  *illegal = 123;
  OS_bSignal(&ptr_set);
  while (1)
    ;
}

void TaskB(void)
{
  int z = 1;
  for (;;)
  {
    z = z << 2 | z;

    OS_bWait(&ptr_set);
    z = *illegal;
  }
}

int main(void)
{
  OS_Init();
  OS_AddThread(TaskA, 64, 0);
  OS_AddThread(TaskB, 64, 0);
  OS_InitSemaphore(&ptr_set, 0);
  OS_Launch(TIME_1MS);
  while (1)
    ;
  return 0;
}

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

Sema4Type a_malloc;
Sema4Type a_malloc2;
Sema4Type b_malloc;

volatile int *volatile illegal = 0;
volatile int *volatile illegal2 = 0;
volatile int *volatile dummy = 0;

void TaskA(void)
{
  int x = 1;
  int y = 1;
  x = x + y;
  y = x + y;
  illegal = (int *)Heap_Malloc(2000);
  Heap_Free(illegal);
  illegal = (int *)Heap_Malloc(128);
  *illegal = 123;
  OS_bSignal(&a_malloc);
  OS_bWait(&b_malloc);
  illegal2 = (int *)Heap_Malloc(128);
  OS_bSignal(&a_malloc2);
  while (1)
    ;
}

void TaskB(void)
{
  int z = 1;
  z = z << 2 | z;

  OS_bWait(&a_malloc);
  dummy = (int*)Heap_Malloc(128);
  OS_bSignal(&b_malloc);
  OS_bWait(&a_malloc2);
  z = *illegal;
  while(1);
}

int twotask_main(void)
{
  OS_Init();
  OS_AddThread(TaskA, 64, 0);
  OS_AddThread(TaskB, 64, 0);
  OS_InitSemaphore(&a_malloc, 0);
  OS_InitSemaphore(&a_malloc2, 0);
  OS_InitSemaphore(&b_malloc, 0);
  OS_Launch(TIME_1MS);
  while (1)
    ;
  return 0;
}

#define MakeTask(num) void Task##num(void) {\
  int stack_var = OS_Id();\
  while(1);\
}

MakeTask(0)
MakeTask(1)
MakeTask(2)
MakeTask(3)
MakeTask(4)
MakeTask(5)
MakeTask(6)
MakeTask(7)
MakeTask(8)
MakeTask(9)
MakeTask(10)
MakeTask(11)
MakeTask(12)
MakeTask(13)
MakeTask(14)
MakeTask(15)

void short_task(void)
{
	OS_Sleep(10);
	OS_Kill();
}

void root_task(void)
{
	while(1)
	{
		OS_AddThread(short_task, 32, 0);
	}
}

int short_task_main(void)
{
  OS_Init();
  OS_AddThread(root_task, 32, 0);
  OS_Launch(TIME_1MS);
  while (1)
    ;
  return 0;
}

void self_starter(void)
{
  OS_AddThread(self_starter, 64, 0);
  OS_AddThread(self_starter, 64, 0);
  heap_stats_t heap_stats = Heap_Stats();
	OS_Kill();
  heap_stats = Heap_Stats();
}

int self_starter_main(void)
{
  OS_Init();
  OS_AddThread(self_starter, 64, 0);
  OS_Launch(TIME_1MS);
  while (1)
    ;
  return 0;
}

int _16task_main(void)
{
  OS_Init();
  heap_stats_t heap_stats = Heap_Stats();
  OS_AddThread(Task0, 64, 0);
  heap_stats = Heap_Stats();
  OS_AddThread(Task1, 64, 0);
  heap_stats = Heap_Stats();
  OS_AddThread(Task2, 64, 0);
  heap_stats = Heap_Stats();
  OS_AddThread(Task3, 64, 0);
  heap_stats = Heap_Stats();
  OS_AddThread(Task4, 64, 0);
  heap_stats = Heap_Stats();
  OS_AddThread(Task5, 64, 0);
  heap_stats = Heap_Stats();
  OS_AddThread(Task6, 64, 0);
  heap_stats = Heap_Stats();
  OS_AddThread(Task7, 64, 0);
  heap_stats = Heap_Stats();
  OS_AddThread(Task8, 64, 0);
  heap_stats = Heap_Stats();
  OS_AddThread(Task9, 64, 0);
  heap_stats = Heap_Stats();
  OS_AddThread(Task10, 64, 0);
  heap_stats = Heap_Stats();
  OS_AddThread(Task11, 64, 0);
  heap_stats = Heap_Stats();
  OS_AddThread(Task12, 64, 0);
  OS_AddThread(Task13, 64, 0);
  heap_stats = Heap_Stats();
  OS_AddThread(Task14, 64, 0);
  heap_stats = Heap_Stats();

  // Task 15 actually can't be scheduled, it's the 17th task after the idle task
  OS_AddThread(Task15, 64, 0);
  OS_Launch(TIME_1MS);
  while (1)
    ;
  return 0;
}

int main(void)
{
  short_task_main();
}
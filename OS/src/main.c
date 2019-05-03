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
#include "ff.h"
#include "diskio.h"
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
	Heap_Free(cur_tcb);
	Heap_Free(illegal);
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



void short_task(void)
{
  heap_stats_t heap_stats = Heap_Stats();
	for(int i=0; i<10000; i++);
	OS_Kill();
}

void root_task(void)
{
	for(int i=0; i< 20; i++)
	{
		OS_AddThread(short_task, 32, 0);
	}
	while(1)
	{
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
MakeTask(16)
MakeTask(17)
MakeTask(18)
MakeTask(19)
MakeTask(20)
MakeTask(21)
MakeTask(22)
MakeTask(23)
MakeTask(24)
MakeTask(25)
MakeTask(26)
MakeTask(27)
MakeTask(28)
MakeTask(29)
MakeTask(30)

int _16task_main(void)
{
  OS_Init();
  OS_AddThread(Task0, 32, 0);
  OS_AddThread(Task1, 32, 0);
  OS_AddThread(Task2, 32, 0);
  OS_AddThread(Task3, 32, 0);
  OS_AddThread(Task4, 32, 0);
  OS_AddThread(Task5, 32, 0);
  OS_AddThread(Task6, 32, 0);
  OS_AddThread(Task7, 32, 0);
  OS_AddThread(Task8, 32, 0);
  OS_AddThread(Task9, 32, 0);
  OS_AddThread(Task10, 32, 0);
  OS_AddThread(Task11, 32, 0);
  OS_AddThread(Task12, 32, 0);
  OS_AddThread(Task13, 32, 0);
  OS_AddThread(Task14, 32, 0);
  OS_AddThread(Task15, 32, 0);
  OS_AddThread(Task16, 32, 0);
  OS_AddThread(Task17, 32, 0);
  OS_AddThread(Task18, 32, 0);
  OS_AddThread(Task19, 32, 0);
  OS_AddThread(Task20, 32, 0);
  OS_AddThread(Task21, 32, 0);
  OS_AddThread(Task22, 32, 0);
  OS_AddThread(Task23, 32, 0);
  OS_AddThread(Task24, 32, 0);
  OS_AddThread(Task25, 32, 0);
  OS_AddThread(Task26, 32, 0);
  OS_AddThread(Task27, 32, 0);
  OS_AddThread(Task28, 32, 0);
  OS_AddThread(Task29, 32, 0);
  OS_AddThread(Task30, 32, 0);
  // IdleTask is task 31, ie the 32nd task
  OS_Launch(TIME_1MS);
  while (1)
    ;
  return 0;
}

int idle_main(void)
{
  OS_Init();
  OS_Launch(TIME_1MS);
  while (1)
    ;
  return 0;
}


static FATFS g_sFatFs;
void init_fs_task(void)
{
  f_mount(&g_sFatFs, "", 0);
}

extern void disk_timerproc (void);
int Load_Process_Main(void)
{
  OS_Init();
  ST7735_InitR(INITR_REDTAB);
  ST7735_FillScreen(ST7735_WHITE);
  OS_AddPeriodicThread(disk_timerproc, TIME_1MS, 5);
  OS_AddThread(&init_fs_task, 128, 0); 
  OS_AddThread(interpreter_task, 128, 2);
  OS_Launch(TIME_1MS);
  while (1)
    ;
  return 0;
}

int main(void)
{
  idle_main();
}
/**
 * @file
 * 
 * @brief Real Time Operating System for Labs 2 and 3 
 * EE445M/EE380L.12
 * 
 * Some design inspiration came from the textbook
 * "Real-Time Operating Systems for ARM Cortex-M Microcontrollers"
 * by Jonathan Valvano
 */

#include <stdint.h>
#include <stdbool.h>
#include <assert.h>
#include "tm4c123gh6pm.h"
#include "OS.h"
#include "misc_macros.h"
#include "priorityqueue.h"
#include "Switch.h"

#define PE3 (*((volatile unsigned long *)0x40024020))

bool save_ctx_global = true;
static tcb_t *tcb_list_head = 0;
static tcb_t *tcb_sleep_head = 0;
tcb_t *cur_tcb = 0;
tcb_t *next_tcb = 0;

void pushq(tcb_t *node);
void pop_sema(tcb_t **semahead);
void push_semaq(tcb_t *node, tcb_t **semahead);
uint32_t peek_priority(tcb_t *head);
static void insert_tcb(tcb_t *new_tcb);

static void ContextSwitch(bool save_ctx)
{
  save_ctx_global = save_ctx;
  NVIC_INT_CTRL_R |= NVIC_INT_CTRL_PEND_SV;
}

unsigned int numTasks = 0;
static void IdleTask(void)
{
  while (1)
  {
    // Do maintenance
    if (numTasks > 1)
    {
      NVIC_ST_CURRENT_R = 0;
      ContextSwitch(true);
    }
  }
}

static uint64_t get_system_time(void)
{
  unsigned long high = WTIMER0_TBR_R;
  unsigned long low = WTIMER0_TAR_R;
  if (high != WTIMER0_TBR_R)
  {
    high = WTIMER0_TBR_R;
    low = WTIMER0_TAR_R;
  }
  uint64_t systime = high;
  systime <<= 32;
  systime |= low;
  return systime;
}

void SysTick_Handler(void)
{
  ContextSwitch(true);
	if(cur_tcb->next->priority == cur_tcb->priority)
		next_tcb = cur_tcb->next;
	else
		next_tcb = tcb_list_head;
}

static void TaskReturn(void)
{
  // Kill this task
  OS_Kill();
  while (1)
    ;
}

void OS_Init(void)
{
  DisableInterrupts();
  // Activate PendSV interrupt with lowest priority
  NVIC_SYS_PRI3_R |= (7 << 21);
  // Activate Systick interrupt with 2nd lowest priority
  NVIC_SYS_PRI3_R |= ((uint32_t)6 << 29);
  SYSCTL_RCGCTIMER_R |= 1 << 3;                  // activate timer3
  volatile uint32_t __delay = SYSCTL_RCGCGPIO_R; // allow time for clock to stabilize

  TIMER3_CTL_R = 0;           // Disable timer 3 during setup
  TIMER3_CFG_R = 0;           // Set timer 3 to 32-bit counter
  TIMER3_TAMR_R = 0x00000001; // configure for down-count mode
  TIMER3_TAPR_R = 0;          // prescale value for trigger
  TIMER3_TAILR_R = -1;        // start value for trigger
  TIMER3_IMR_R = 0x00000001;  // disable timer 3A time-out interrupt
  TIMER3_CTL_R |= 0x0;        // disable timer 3A

  //Enable it when it needs

  // Set up SysTick timer
  NVIC_ST_CURRENT_R = 0;

  // Set up WTIMER0 as 64-bit time source
  SYSCTL_RCGCWTIMER_R |= 1; // activate Wtimer0
  while ((SYSCTL_RCGCWTIMER_R & 1) == 0)
    ;
  WTIMER0_CTL_R &= ~1;         // Disable Wtimer 0 during setup
  WTIMER0_CFG_R = 0;           // Set Wtimer 0 to 64-bit counter, indiv
  WTIMER0_TBMR_R = 0x00000012; // configure for periodic mode, count up
  WTIMER0_TAMR_R = 0x00000012; // configure for periodic mode, count up
  WTIMER0_TBPR_R = 0;          // prescale value for trigger
  WTIMER0_TAPR_R = 0;          // prescale value for trigger
  WTIMER0_CTL_R |= 1;          // Kick off Wtimer0
  OS_AddThread(IdleTask, 128, 5);
}

void OS_InitSemaphore(Sema4Type *semaPt, long value)
{
  semaPt->head = 0;
  semaPt->Value = value;
}

void OS_Wait(Sema4Type *semaPt)
{
  tcb_t *iter = tcb_list_head;
  long sr = StartCritical();
  semaPt->Value--;
  if (semaPt->Value < 0)
  {
    do
    {
      // Until we wrap around to head
      if (iter->next == cur_tcb)
      {
        iter->next = cur_tcb->next;
        break;
      }
      iter = iter->next;
    } while (iter != tcb_list_head);
    // remove from active TCB
    cur_tcb->next = 0;
    push_semaq(cur_tcb, &semaPt->head);
    NVIC_ST_CURRENT_R = 0; // Make sure next thread gets full time slice
    ContextSwitch(true);
  }
  EndCritical(sr);
}

void OS_Signal(Sema4Type *semaPt)
{
  long sr = StartCritical();
  semaPt->Value++;
  if (semaPt->Value <= 0)
  {
    pop_sema(&semaPt->head);
  }
	EndCritical(sr);
}

void OS_bWait(Sema4Type *semaPt)
{

  int oldval;

  // read the semaphore value
  oldval = __ldrex(&(semaPt->Value));
  // loop again if it is locked and we are blocking
  // or setting it with strex failed
  while (!(oldval > 0) || __strex(oldval & 0, &(semaPt->Value)) != 0)
  {
    OS_Suspend();
    oldval = __ldrex(&(semaPt->Value));
  }
}

void OS_bSignal(Sema4Type *semaPt)
{
  int oldval;
  do
  {
    oldval = __ldrex(&(semaPt->Value));
  } while (__strex(oldval | 1, &(semaPt->Value)) != 0);
}

static void remove_tcb(tcb_t *tcb)
{
  if (tcb_list_head->next == tcb_list_head)
  {
    // One task in list
    tcb_list_head = 0;
    return;
  }
  tcb_t *iter = tcb_list_head;
  do
  {
    // Until we wrap around to head
    if (iter->next == tcb)
    {
      iter->next = tcb->next;
      tcb->magic = 0; // Free this TCB, return to pool
      return;
    }
    iter = iter->next;
  } while (iter != tcb_list_head);
}

static void insert_tcb(tcb_t *new_tcb) // priority insert
{
	tcb_t *tmp = tcb_list_head;
  if (tcb_list_head == 0)
  {
    tcb_list_head = new_tcb;
    tcb_list_head->next = tcb_list_head;
  }
  else
  {
    while (tmp->next != 0 && tmp->next->priority <= new_tcb->priority)
    {
      tmp = tmp->next;
    }
    new_tcb->next = tmp->next;
    tmp->next = new_tcb;
  }
}

#define MAX_TASKS (10)
#define MAX_STACK_DWORDS (256) // 256 * 10tasks * 8Bytes = ~20K

static int next_id = 0;
static tcb_t tcb_pool[MAX_TASKS];
// Stacks need to be dword aligned
static long long stack_pool[MAX_TASKS][MAX_STACK_DWORDS];

int OS_AddThread(void (*task)(void),
                 unsigned long stackSize,
                 unsigned long priority)
{
  unsigned long stackDWords = (stackSize / 8) + (stackSize % 8 == 0 ? 0 : 1); // Round up integer div
  tcb_t *tcb = 0;

  long sr = StartCritical();
  // Validate input
  if (task == 0)
  {
    // NULL func pointer
    EndCritical(sr);
    return 0; // Fail
  }
  if (priority > 5)
  {
    // Invalid priority
    EndCritical(sr);
    return 0; // Fail
  }
  if (stackSize < 56)
  {
    // Stack too small to fit initial state
    EndCritical(sr);
    return 0; // Fail
  }
  if (numTasks >= MAX_TASKS)
  {
    // Ran out of TCBs
    EndCritical(sr);
    return 0;
  }
  if (stackDWords > MAX_STACK_DWORDS)
  {
    // Stack requested too large
    EndCritical(sr);
    return 0;
  }

  for (unsigned int i = 0; i < MAX_TASKS; i++)
  {
    if (tcb_pool[i].magic != TCB_MAGIC)
    {
      tcb = &tcb_pool[i];
      tcb->sp = (long *)&stack_pool[i][stackDWords];
      break;
    }
  }
  if (tcb == 0)
  {
    // Didn't find a free TCB for some reason
    EndCritical(sr);
    return 0;
  }
  *(--tcb->sp) = 0x01000000L;      // xPSR, with Thumb state enabled
  *(--tcb->sp) = (long)task;       // PC
  *(--tcb->sp) = (long)TaskReturn; // LR
  *(--tcb->sp) = 0x12121212;       // R12
  *(--tcb->sp) = 0x03030303;       // R3
  *(--tcb->sp) = 0x02020202;       // R2
  *(--tcb->sp) = 0x01010101;       // R1
  *(--tcb->sp) = 0x00000000;       // R0
  *(--tcb->sp) = 0x11111111;       // R11
  *(--tcb->sp) = 0x10101010;       // R10
  *(--tcb->sp) = 0x09090909;       // R9
  *(--tcb->sp) = 0x08080808;       // R8
  *(--tcb->sp) = 0x07070707;       // R7
  *(--tcb->sp) = 0x06060606;       // R6
  *(--tcb->sp) = 0x05050505;       // R5
  *(--tcb->sp) = 0x04040404;       // R4

  tcb->id = next_id++;
  tcb->priority = priority;
  tcb->period = 0; // 0 = aperiodic
  tcb->wake_time = 0;
  tcb->next = 0;
  tcb->magic = TCB_MAGIC; // Write magic to mark valid
  tcb->task = task;
  insert_tcb(tcb);
  numTasks++;
  EndCritical(sr);
  return 1; // Success
}

unsigned long OS_Id(void)
{
  return cur_tcb->id;
}

int numPeriodicTasks = 0;
void (*WTimer1ATask)(void) = 0;
void (*WTimer1BTask)(void) = 0;

void WideTimer1A_Handler(void)
{
  WTIMER1_ICR_R = 1;    // Clear Wtimer 1A timeout interrupt
  NVIC_ST_CTRL_R &= ~1; // Stop systick
  if (WTimer1ATask)
  {
    WTimer1ATask();
  }
  NVIC_ST_CTRL_R |= 1; // Stop systick
}

void WideTimer1B_Handler(void)
{
  WTIMER1_ICR_R = 1 << 8; // Clear Wtimer 1B timeout interrupt
  NVIC_ST_CTRL_R &= ~1;   // Stop systick
  if (WTimer1BTask)
  {
    WTimer1BTask();
  }
  NVIC_ST_CTRL_R |= 1; // Stop systick
}

int OS_AddPeriodicThread(void (*task)(void),
                         unsigned long period, unsigned long priority)
{
  priority = 0;
  long sr = StartCritical();
  if (numPeriodicTasks == 0)
  {
    SYSCTL_RCGCWTIMER_R |= 1 << 1; // activate Wtimer1
    while ((SYSCTL_RCGCWTIMER_R & (1 << 1)) == 0)
      ;

    // Use WTIMER 1B
    WTIMER1_CTL_R &= ~(1 << 8);   // Disable Wtimer 1B during setup
    WTIMER1_CFG_R = 4;            // Set Wtimer 1 to 32-bit counter, indiv
    WTIMER1_TBMR_R = 0x00000002;  // configure for periodic mode, default down-count settings
    WTIMER1_TBPR_R = 0;           // prescale value for trigger
    WTIMER1_TBILR_R = period - 1; // start value for trigger
    WTIMER1_IMR_R |= 1 << 8;      // enable Wtimer 1B time-out interrupt
    WTimer1BTask = task;

    // Set up interrupt controller, interrupt 97
    NVIC_PRI24_R = (NVIC_PRI24_R & 0xFFFF1FFF) | (0 << 13);

    // Kick off Wtimer
    WTIMER1_CTL_R |= 1 << 8; // enable Wtimer1B 32-b, periodic, no interrupts
  }
  else if (numPeriodicTasks == 1)
  {

    // Use WTIMER 1A
    WTIMER1_CTL_R &= ~1; // Disable Wtimer 1A during setup
    // WTIMER1_CFG_R = 0;            // Set Wtimer 1 to 32-bit counter, indiv
    WTIMER1_TAMR_R = 0x00000002;  // configure for periodic mode, default down-count settings
    WTIMER1_TAPR_R = 0;           // prescale value for trigger
    WTIMER1_TAILR_R = period - 1; // start value for trigger
    WTIMER1_IMR_R |= 1;           // enable Wtimer 1A time-out interrupt
    WTimer1ATask = task;

    // Set up interrupt controller, interrupt 96
    NVIC_PRI24_R = (NVIC_PRI24_R & 0xFFFFFF1F) | (0 << 5);

    // Kick off Wtimer
    WTIMER1_CTL_R |= 1; // enable Wtimer1A 32-b, periodic, no interrupts
  }
  else
  {
    // Can't support more than 2 periodic tasks
    EndCritical(sr);
    return 0;
  }
  numPeriodicTasks++;
  EndCritical(sr);
  return 1;
}

int OS_AddSW1Task(void (*task)(void), unsigned long priority)
{
  Switch_Init(task, 0);

  return 0;
}

int OS_AddSW2Task(void (*task)(void), unsigned long priority)
{
  Switch2_Init(task, 0);
  return 0;
}

void OS_Sleep(unsigned long sleepTime)
{
  long sr = StartCritical();
  sleepTime *= TIME_1MS;
  cur_tcb->wake_time = ((sleepTime));
  tcb_t *iter = tcb_list_head;
  do
  {
    // Until we wrap around to head
    if (iter->next == cur_tcb)
    {
      iter->next = cur_tcb->next;
      break;
    }
    iter = iter->next;
  } while (iter != tcb_list_head);
  // remove from active TCB //
  cur_tcb->next = 0;
  pushq(cur_tcb);
  NVIC_ST_CURRENT_R = 0; // Make sure next thread gets full time slice
  ContextSwitch(true);
  EndCritical(sr);
}

void OS_Kill(void)
{
  long sr = StartCritical();
  //  tcb_t *next_tcb = cur_tcb->next;
  remove_tcb(cur_tcb);
  cur_tcb = next_tcb;
  next_tcb = cur_tcb->next;
  numTasks--;
  NVIC_ST_CURRENT_R = 0; // Make sure next thread gets full time slice
  ContextSwitch(false);
  EndCritical(sr);
}

void OS_Suspend(void)
{
  ContextSwitch(true);
}

#define OSFIFOSIZE 32            // Must be power of two
static uint32_t volatile OSPutI; // put next
static uint32_t volatile OSGetI; // get next

unsigned long static OSFifo[OSFIFOSIZE];

static Sema4Type fifo_wr_mutex;
static Sema4Type fifo_rd_mutex;
static Sema4Type fifo_level;

void OS_Fifo_Init(unsigned long size)
{
  long sr;
  sr = StartCritical(); // make atomic
  OSPutI = OSGetI = 0;  // Empty
  OS_InitSemaphore(&fifo_wr_mutex, 1);
  OS_InitSemaphore(&fifo_rd_mutex, 1);
  OS_InitSemaphore(&fifo_level, 0);
  EndCritical(sr);
}

int OS_Fifo_Put(unsigned long data)
{
  int ret = 0;
  OS_Wait(&fifo_wr_mutex);
  if ((OSPutI + 1) % OSFIFOSIZE == OSGetI)
  {
    ret = (0); // Failed, fifo full
  }
  else
  {
    OSFifo[OSPutI] = data;              // put
    OSPutI = (OSPutI + 1) % OSFIFOSIZE; // Success, update
    ret = (1);
    OS_Signal(&fifo_level); // Signal 1 more queue item
  }
  OS_Signal(&fifo_wr_mutex);
  return ret;
}

unsigned long OS_Fifo_Get(void)
{
  unsigned long result = 0;
  OS_Wait(&fifo_level); // Wait for 1+ items to be in queue
  OS_Wait(&fifo_rd_mutex);
  result = OSFifo[OSGetI];
  OSGetI = (OSGetI + 1) % OSFIFOSIZE; // Success, update
  OS_Signal(&fifo_rd_mutex);
  return result;
}

long OS_Fifo_Size(void)
{
  return ((uint32_t)(OSPutI - OSGetI));
}

Sema4Type mailboxFull;
Sema4Type mailboxMutex;
unsigned long mailBox;

void OS_MailBox_Init(void)
{
  mailBox = 0;
  OS_InitSemaphore(&mailboxFull, 0);
  OS_InitSemaphore(&mailboxMutex, 1);
}

void OS_MailBox_Send(unsigned long data)
{
  OS_Wait(&mailboxMutex);
  mailBox = data;
  OS_Signal(&mailboxFull);
  OS_Signal(&mailboxMutex);
}

unsigned long OS_MailBox_Recv(void)
{
  unsigned long data;
  OS_Wait(&mailboxFull);
  OS_Wait(&mailboxMutex);
  data = mailBox;
  OS_Signal(&mailboxMutex);
  return data;
}

unsigned long long OS_Time(void)
{
  return get_system_time();
}

unsigned long long OS_TimeDifference(unsigned long long start, unsigned long long stop)
{
  return stop - start;
}

static uint64_t time_when_cleared = 0;
void OS_ClearMsTime(void)
{
  time_when_cleared = get_system_time();
}

unsigned long OS_MsTime(void)
{
  uint64_t timediff = (get_system_time() - time_when_cleared);
  timediff = timediff / TIME_1MS;
  return (unsigned long)timediff;
}

void OS_Launch(unsigned long theTimeSlice)
{
  // Check if we can launch
  if (tcb_list_head == 0)
  {
    return;
  }
  cur_tcb = tcb_list_head;
  next_tcb = cur_tcb->next;
  NVIC_ST_RELOAD_R = theTimeSlice;
  NVIC_ST_CTRL_R |= 0x7;

  NVIC_EN1_R = 1 << (35 - 32); // 9) enable IRQ 35 in NVIC
  NVIC_EN3_R |= 1 << 0;        // Enable wtimer1A interrupt
  NVIC_EN3_R |= 1 << 1;        // Enable wtimerB interrupt

  ContextSwitch(false);
  EnableInterrupts();
}

uint32_t peek_waketime(tcb_t *head)
{
  return head->wake_time;
}

uint32_t peek_priority(tcb_t *head)
{
  return head->priority;
}

void pop()
{
  long sr = StartCritical();

  tcb_t *head = tcb_sleep_head;
  tcb_t *tmpinq = head;
  if (head == 0)
  {
    EndCritical(sr);
    return;
  }

  tmpinq = head->next;
  insert_tcb(head);
  head->wake_time = 0;
  tcb_sleep_head = tmpinq;
  head = tmpinq;

  if (head != 0)
  {
    TIMER3_TAILR_R = head->wake_time;
    TIMER3_CTL_R |= 0x1;
  }
  EndCritical(sr);
}

void pushq(tcb_t *node)
{
  // outside is already disinterrupted
  tcb_t *start = tcb_sleep_head;
  if (tcb_sleep_head == 0)
  {
    TIMER3_TAILR_R = node->wake_time;
    TIMER3_CTL_R |= 0x1;
    tcb_sleep_head = node;
  }
  else if (node->wake_time < TIMER3_TAV_R)
  {
    TIMER3_CTL_R &= ~(1); // disable timer 3A
    node->next = tcb_sleep_head;
    tcb_sleep_head = node;
    node->next->wake_time = TIMER3_TAV_R;
    TIMER3_TAILR_R = node->wake_time;

    NVIC_UNPEND1_R = 1 << (35 - 32); // if before this statement end tick could end, then remove.

    TIMER3_CTL_R |= 0x1; // enable timer 3A
  }
  else
  {
    while (start->next != 0 && start->next->wake_time < node->wake_time)
    {
      node->wake_time = node->wake_time - start->wake_time;
      start = start->next;
    }
    node->next = start->next;
    start->next = node;
  }
}

void Timer3A_Handler()
{
  TIMER3_ICR_R = TIMER_ICR_TATOCINT;
  pop();
}

void pop_sema(tcb_t **semahead)
{
  long sr = StartCritical();

  tcb_t *head = *semahead;
  tcb_t *tmpinq = head;
  if (head == 0)
  {
    EndCritical(sr);
    return;
  }

  tmpinq = head->next;
  insert_tcb(head);

  // if poped priority is higher than existing one,
  if (cur_tcb->priority > head->priority)
  {
    next_tcb = head;
    NVIC_ST_CURRENT_R = 0; // Make sure next thread gets full time slice
    ContextSwitch(true);
  }
  else if (cur_tcb->next != 0 && cur_tcb->next->priority > head->priority)
  {
    next_tcb = head;
  } // if poped one is not higher than current one, but higher than next one. order should be changed

  *semahead = tmpinq;
  EndCritical(sr);
}

void push_semaq(tcb_t *node, tcb_t **semahead)
{
  // outside is already disinterrupted && set node
  tcb_t *start = *semahead;
  if (*semahead == 0)
  {
    *semahead = node;
  }
  else
  {
    while (start->next != 0 && start->next->priority <= node->priority)
    {
      start = start->next;
    }
    node->next = start->next;
    start->next = node;
  }
}
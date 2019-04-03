
#include <stdint.h>
#include "ADC.h"
#include "tm4c123gh6pm.h"
#include "profiler.h"

static void (*sample_handler)(unsigned long) = 0;

void ADC0Seq0_Handler(void)
{
  Profiler_Event(EVENT_PTH_START, "ADC");
  while (!(ADC0_SSFSTAT0_R & (1 << 8))) // FIFO not empty and still need more samples
  {
   // if(sample_handler)
		{
			sample_handler(ADC0_SSFIFO0_R); // 12-bit result
		}
  }
  Profiler_Event(EVENT_PTH_END, "ADC");
  ADC0_ISC_R = 0x01; // acknowledge ADC sequence 0 completion
}

int ADC_Init(uint32_t channelNum)
{
  volatile uint32_t __delay;
  SYSCTL_RCGCADC_R |= 0x01;     // activate ADC0
  while(!(SYSCTL_RCGCADC_R & 1)); // allow time for clock to stabilize
  __delay = SYSCTL_RCGCGPIO_R;  // allow time for clock to stabilize

  // Set up ADC
  ADC0_PC_R = 0x01;                                     // configure for 125K samples/sec
  ADC0_SSPRI_R = 0x3210;                                // sequencer 0 is highest, sequencer 3 is lowest
  ADC0_ACTSS_R &= ~(1<<3);                              // disable sample sequencer 3
  ADC0_EMUX_R = (ADC0_EMUX_R & 0xFFFF0FFF) | (0xF<<12); // timer trigger event for SS3
  ADC0_SSMUX3_R = channelNum;                           // Sample channel once
  ADC0_SSCTL3_R = 1<<1;                                 // End seq after one sample
  ADC0_ACTSS_R |= 1<<3;                                 // enable sample sequencer 3

  return 0;
}

uint16_t ADC_In(void)
{
  static uint16_t sample = 0xFFFF;
  if(!(ADC0_SSFSTAT3_R & (1 << 8))) // FIFO not empty
  {
    sample = ADC0_SSFIFO3_R;
  }
  return sample;
}

#define ADC_CLK_FREQ (80000000)

// Use ADC sequence sampler 0
int ADC_Collect(uint32_t channelNum, uint32_t fs, void (*handler)(unsigned long))
{
  volatile uint32_t __delay;
  uint32_t period = ADC_CLK_FREQ / fs;

  NVIC_EN0_R &= ~(1 << 14); // disable interrupt 14 in NVIC

  SYSCTL_RCGCADC_R |= 0x01;     // activate ADC0
  SYSCTL_RCGCTIMER_R |= 1 << 2; // activate timer2
  // allow time for clock to stabilize
  while(!(SYSCTL_RCGCADC_R & 1));
  while(!(SYSCTL_RCGCTIMER_R & 4));

  __delay = SYSCTL_RCGCGPIO_R;  // allow time for clock to stabilize

  // Set up timer2A
  TIMER2_CTL_R = 0;            // Disable timer 2 during setup
  TIMER2_CFG_R = 0;            // Set timer 2 to 32-bit counter
  TIMER2_CTL_R |= 0x21;        // Enable timer 2A and use as ADC trigger
  TIMER2_TAMR_R = 0x00000002;  // configure for periodic mode, default down-count settings
  TIMER2_TAPR_R = 0;           // prescale value for trigger
  TIMER2_TAILR_R = period - 1; // start value for trigger
  TIMER2_IMR_R = 0x00000000;   // disable all interrupts

  // Set up ADC
  ADC0_PC_R = 0x01;                               // configure for 125K samples/sec
  ADC0_SSPRI_R = 0x3210;                          // sequencer 0 is highest, sequencer 3 is lowest
  ADC0_ACTSS_R &= ~0x01;                          // disable sample sequencer 0
  ADC0_EMUX_R = (ADC0_EMUX_R & 0xFFFFFFF0) + 0x5; // timer trigger event for SS0
  ADC0_SSMUX0_R = channelNum;                     // Sample channel once
  ADC0_SSCTL0_R = 0x06;                           // set flag and end at 8th sample
  ADC0_IM_R |= 0x01;                              // enable SS0 interrupts
  ADC0_ACTSS_R |= 0x01;                           // enable sample sequencer 0

  // Set up interrupt controller
  NVIC_PRI3_R = (NVIC_PRI3_R & 0xFF1FFFFF) | (2 << 21); // priority 2
  NVIC_EN0_R |= 1 << 14;                                // enable interrupt 14 in NVIC

  sample_handler = handler;

  // Kick off timer
  TIMER2_CTL_R |= 0x00000001; // enable timer2A 32-b, periodic, no interrupts


  return 0; // Success
}

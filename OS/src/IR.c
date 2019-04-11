#include "IR.h"
#include "ADC.h"


static void IR_handler(unsigned long data) {
    // process data
}

/**
 * channel 0 (PE3)
 * frequency: 50 Hz
 * 
 */
void IR_Init(void) {
    ADC_Collect(0, 50, IR_handler);
 }

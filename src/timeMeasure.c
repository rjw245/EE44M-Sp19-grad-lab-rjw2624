
#include <stdint.h>
#include <stdbool.h>
#include <assert.h>
#include "tm4c123gh6pm.h"
#include "OS.h"
#include "timeMeasure.h"

static unsigned long long __startTime;
static unsigned long long __total_interupt_time;
static unsigned long long __disable_interupt_T;
static int enabled;
static int start ;


void timeMeasureInit(void)
{
	enabled = 0;
	start = 0;
	__startTime = OS_Time();
	__total_interupt_time = 0;
} // initialize time measurement. At main, it should be exist
	
void timeMeasurestart(void)
{
	start = 1;
	enabled =1;
}


void disableTimeget(void)
{
	if(start == 1&& enabled == 1){
		enabled = 0;
		__disable_interupt_T = OS_Time();
	}
}

void enableTimeget(void)
{
	if (start == 1){
	unsigned long long cur = OS_Time();
	unsigned long long diff = OS_TimeDifference(__disable_interupt_T,cur);
	__total_interupt_time += diff;
		enabled = 1;
	}
}

int getDisablePercent(void)
{
	if (start ){
	unsigned long long cur = OS_Time();
	unsigned long long diff = OS_TimeDifference(__startTime,cur);

	return (int)__total_interupt_time * 100 / diff;
	}
	else 
		return 0;
	
}


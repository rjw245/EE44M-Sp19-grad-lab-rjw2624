#ifndef __MEASURE_TIME
#define __MEASURE_TIME

#include <stdint.h>
#include <stdbool.h>
#include <assert.h>
#include "tm4c123gh6pm.h"
#include "OS.h"

unsigned long long __startTime;
unsigned long long __total_interupt_time;
unsigned long long __disable_interupt_T;
bool start = false;


void timeMeasureInit()
{
	start = false;
	__startTime = OS_Time();
	__total_interupt_time = 0;
} // initialize time measurement. At main, it should be exist
	
void timeMeasurestart()
{
	start = true;
}


void disableTimeget()
{
	if(start){
	__disable_interupt_T = OS_Time();
	}
}

void enableTimeget()
{
	if (start){
	unsigned long long cur = OS_Time();
	unsigned long long diff = OS_TimeDifference(__disable_interupt_T,cur);
	__total_interupt_time += diff;
	}
}

double getDisablePercent()
{
	if (start){
	unsigned long long cur = OS_Time();
	unsigned long long diff = OS_TimeDifference(__startTime,cur);
	return (double)__total_interupt_time / diff * 100;
	}
	else 
		return 0;
	
}




#endif
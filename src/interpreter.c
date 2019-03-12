
#include <string.h>
#include <stdint.h>
#include <stdio.h>
#include "interpreter.h"
#include "ADC.h"
#include "UART.h"
#include "OS.h"
#include "tm4c123gh6pm.h"
#include "ST7735.h"
#include "misc_macros.h"
#include "profiler.h"
#include "timeMeasure.h"

#define MAX_LINE_LENGTH (128)

static uint16_t sample_buffer[10];

static void readAndParse(char *cmd_str, char **cmd, char **arg1, char **arg2, char **arg3, char **arg4, char **arg5, char **arg6)
{
  if (!(*cmd = strtok(cmd_str, "\t\n ,")))
    return;

  if (!(*arg1 = strtok(NULL, "\t\n ,")))
    return;

  if (!(*arg2 = strtok(NULL, "\t\n ,")))
    return;

  if (!(*arg3 = strtok(NULL, "\t\n ,")))
    return;

  if (!(*arg4 = strtok(NULL, "\t\n ,")))
    return;

  if (!(*arg5 = strtok(NULL, "\t\n ,")))
    return;

  if (!(*arg6 = strtok(NULL, "\t\n ,")))
    return;
}

void interpreter_task(void)
{
  static char uart_in_buf[256];
  while (1)
  {
    UART_OutString("> ");
    UART_InString(uart_in_buf, lengthof(uart_in_buf) - 1);
    UART_OutString("\r\n");
    interpreter_cmd(uart_in_buf);

    // UART_OutString("\r\n");
    // UART_OutString(uart_in_buf);
    // UART_OutString("\r\n");
    // ADC_Collect(0, 100, sample_buffer, lengthof(sample_buffer));
    // while(ADC_Status());
    // char adc_string[64];
    // sprintf(adc_string, "ADC: %d, %d  ", sample_buffer[0], sample_buffer[1]);
    // ST7735_DrawString(0, 0, adc_string, ST7735_BLACK, ST7735_WHITE);
    // UART_OutString(adc_string);
  }
}

static void print_event(const event_t* event)
{
    char event_str[80];
    char *event_types[EVENT_NUM_TYPES] = {
      [EVENT_FGTH_START] = "FG START",
      [EVENT_PTH_START] = "PT START",
      [EVENT_PTH_END] = "PT END",
    };
    sprintf(event_str, "Name: %s  Time: %llu cycles  Type: %s\r\n",
            event->name, event->timestamp, event_types[event->type]);
    UART_OutString(event_str);
}

void interpreter_cmd(char *cmd_str)
{
  char *cmd, *arg1, *arg2, *arg3, *arg4, *arg5, *arg6;
  readAndParse(cmd_str, &cmd, &arg1, &arg2, &arg3, &arg4, &arg5, &arg6);
  if (strcmp(cmd, "adc") == 0)
  {
    zeroes(sample_buffer);
    // ADC_Collect(0, 100, sample_buffer, lengthof(sample_buffer));
    char adc_string[64];
    sprintf(adc_string, "ADC: %d, %d  ", sample_buffer[0], sample_buffer[1]);
    UART_OutString(adc_string);
    UART_OutString("\r\n");
  }
  else if (strcmp(cmd, "lcd") == 0)
  {
    char lcd_string[64];
    int count = 0;
    ST7735_FillScreen(0xFFFF); // set screen to white
    for (int device = 0; device < 2; device++)
    {
      for (int line = 0; line < 4; line++)
      {
        sprintf(lcd_string, "Device: %d  Line: %d   ", device, line);
        ST7735_Message(device, line, lcd_string, count++);
      }
    }
  }
  else if (strcmp(cmd, "time") == 0)
  {
    char time[64];
    sprintf(time, "Time: %llu ms\r\n", OS_Time()/TIME_1MS);
    UART_OutString(time);
  }
  else if (strcmp(cmd, "log") == 0)
  {
    Profiler_Foreach(print_event);
  }
	
	else if (strcmp(cmd, "critical") == 0)
  {
		char time[64];
    int res = getDisablePercent();
    sprintf(time, "critical time: %d percent\r\n", res);
		UART_OutString(time);
  }
	
  else if (strcmp(cmd, "clear") == 0)
  {
    Profiler_Clear();
	  timeMeasureInit();
	  timeMeasurestart();
  }
}

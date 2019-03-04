/**
 * @file 
 * 
 * List of commands
 *   - adc
 *     - Prints 2 consecutive ADC samples of channel 0 to the LCD and UART0
 *   - lcd
 *     - Prints strings on each line of each logical display on the LCD.
 */

#ifndef INTERPRETER_H
#define INTERPRETER_H

/**
 * @brief OS Task that sends characters to the interpreter
 * 
 */
void interpreter_task(void);


/**
 * @brief Pass user input to the interpreter and act on their command.
 * @param cmd_str String containing the entire user command.
 */
void interpreter_cmd(char* cmd_str);

#endif

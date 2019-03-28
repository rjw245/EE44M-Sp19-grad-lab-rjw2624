/**
 * @file
 * @brief Runs on LM4F120/TM4C123
 *        Use UART0 to implement bidirectional data transfer to and from a
 *        computer running HyperTerminal.  This time, interrupts and FIFOs
 *        are used.
 * @author Daniel Valvano
 */

/* This example accompanies the book
   "Embedded Systems: Real Time Interfacing to Arm Cortex M Microcontrollers",
   ISBN: 978-1463590154, Jonathan Valvano, copyright (c) 2015
   Program 5.11 Section 5.6, Program 3.10

 Copyright 2015 by Jonathan W. Valvano, valvano@mail.utexas.edu
    You may use, edit, run or distribute this file
    as long as the above copyright notice remains
 THIS SOFTWARE IS PROVIDED "AS IS".  NO WARRANTIES, WHETHER EXPRESS, IMPLIED
 OR STATUTORY, INCLUDING, BUT NOT LIMITED TO, IMPLIED WARRANTIES OF
 MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE APPLY TO THIS SOFTWARE.
 VALVANO SHALL NOT, IN ANY CIRCUMSTANCES, BE LIABLE FOR SPECIAL, INCIDENTAL,
 OR CONSEQUENTIAL DAMAGES, FOR ANY REASON WHATSOEVER.
 For more information about my classes, my research, and my books, see
 http://users.ece.utexas.edu/~valvano/
 */

#ifndef UART_H
#define UART_H

#include <stdint.h>

// U0Rx (VCP receive) connected to PA0
// U0Tx (VCP transmit) connected to PA1

// standard ASCII symbols
#define CR 0x0D
#define LF 0x0A
#define BS 0x08
#define ESC 0x1B
#define SP 0x20
#define DEL 0x7F

//------------UART_Init------------
/**
 * @brief Initialize the UART for 115,200 baud rate (assuming 50 MHz clock),
 *        8 bit word length, no parity bits, one stop bit, FIFOs enabled
 * 
 */
void UART_Init(void);

//------------UART_InChar------------
/**
 * @brief Wait for new serial port input
 * 
 * @return char ASCII code for key typed
 */
char UART_InChar(void);

//------------UART_OutChar------------
/**
 * @brief 8-bit to serial port
 * 
 * @param data letter is an 8-bit ASCII character to be transferred
 */
void UART_OutChar(char data);

//------------UART_OutString------------
/**
 * @brief Output String (NULL termination)
 * 
 * @param pt pointer to a NULL-terminated string to be transferred
 */
void UART_OutString(char *pt);

//------------UART_InUDec------------
/**
 * @brief InUDec accepts ASCII input in unsigned decimal format
 *        and converts to a 32-bit unsigned number
 *        valid range is 0 to 4294967295 (2^32-1)
 *        If you enter a number above 4294967295, it will return an incorrect value
 *        Backspace will remove last digit typed
 * 
 * @return uint32_t 32-bit unsigned number
 */
uint32_t UART_InUDec(void);

//-----------------------UART_OutUDec-----------------------
/**
 * @brief Output a 32-bit number in unsigned decimal format
 * 
 * @param n 32-bit number to be transferred
 */
void UART_OutUDec(uint32_t n);

//---------------------UART_InUHex----------------------------------------
/**
 * @brief Accepts ASCII input in unsigned hexadecimal (base 16) format
 *        No '$' or '0x' need be entered, just the 1 to 8 hex digits
 *        It will convert lower case a-f to uppercase A-F
 *            and converts to a 16 bit unsigned number
 *            value range is 0 to FFFFFFFF
 *        If you enter a number above FFFFFFFF, it will return an incorrect value
 *        Backspace will remove last digit typed
 * 
 * @return uint32_t 32-bit unsigned number
 */
uint32_t UART_InUHex(void);

//--------------------------UART_OutUHex----------------------------
/**
 * @brief Output a 32-bit number in unsigned hexadecimal format
 *        Variable format 1 to 8 digits with no space before or after
 * 
 * @param number 32-bit number to be transferred
 */
void UART_OutUHex(uint32_t number);

//------------UART_InString------------
/**
 * @brief  Accepts ASCII characters from the serial port
 *            and adds them to a string until <enter> is typed
 *            or until max length of the string is reached.
 *         It echoes each character as it is inputted.
 *         If a backspace is inputted, the string is modified
 *            and the backspace is echoed
 *         terminates the string with a null character
 *         uses busy-waiting synchronization on RDRF
 *         Modified by Agustinus Darmawan + Mingjie Qiu
 * 
 * @param bufPt pointer to empty buffer
 * @param max size of buffer
 */
void UART_InString(char *bufPt, uint16_t max);

//------------UART_setRedirect------------
/**
 * @brief  Accept Filename and make it as redirect file
 * 
 * @param string of filename
 */
void UART_setRedirect(char *F);

//------------UART_endRedirect------------
/**
 * @brief  End redirection
 * 
 */
void UART_endRedirect();

#endif

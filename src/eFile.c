// filename ************** eFile.c *****************************
// High-level routines to implement a solid-state disk 
// Jonathan W. Valvano 3/9/17

#include <string.h>
#include "eDisk.h"
#include "UART.h"
#include <stdio.h>

#define SUCCESS 0
#define FAIL 1

typedef uint32_t block_addr_t;
                       

int eFile_Init(void)
{

}


int eFile_Format(void)
{

}


int eFile_Create(char name[])
{

}


int eFile_WOpen(char name[])
{

}


int eFile_Write(char data)
{

}


int eFile_Close(void)
{

}


int eFile_WClose(void)
{

}


int eFile_ROpen(char name[])
{

}
   

int eFile_ReadNext(char *pt)
{

}
                              

int eFile_RClose(void)
{

}


int eFile_Directory(void(*fp)(char))
{

}


int eFile_Delete(char name[])
{

}


int eFile_RedirectToFile(char *name)
{

}


int eFile_EndRedirectToFile(void)
{

}

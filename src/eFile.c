// filename ************** eFile.c *****************************
// High-level routines to implement a solid-state disk 
// Jonathan W. Valvano 3/9/17

#include <string.h>
#include "eDisk.h"
#include "UART.h"
#include <stdio.h>

#define SUCCESS 0
#define FAIL 1

// Sector size in bytes
#define SECTOR_BYTES 512

typedef uint32_t sector_addr_t;

#define LONGEST_FILENAME 7
typedef struct {
  char file_name[LONGEST_FILENAME + 1]; // space for null terminator
  sector_addr_t start;
  uint32_t padding;
} dir_entry_t;

#define DIR_START 32 // Sector number
#define DIR_SECTORS 2 // Number of sectors directory may span
#define DIR_ENTRIES ((DIR_SECTORS*SECTOR_BYTES)/(sizeof(dir_entry_t)))
dir_entry_t dir[DIR_ENTRIES];

#define FAT_SECTORS 131072 // Number of sectors directory may span
#define FAT_ENTRIES ((FAT_SECTORS*SECTOR_BYTES)/sizeof(sector_addr_t))
#define FAT_START 34

sector_addr_t fat_cache[SECTOR_BYTES/sizeof(sector_addr_t)];
sector_addr_t cached_fat_sector = 0;

int eFile_Init(void)
{
  memset(dir, 0, sizeof(dir));
  memset(fat_cache, 0, sizeof(dir));
  eDisk_Init(0);
  for(int i=0; i<DIR_SECTORS; i++)
  {
    eDisk_ReadBlock(((uint8_t*)dir)+i*SECTOR_BYTES, DIR_START+i);
  }
  cached_fat_sector = FAT_START;
  eDisk_ReadBlock(fat_cache, FAT_START);
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

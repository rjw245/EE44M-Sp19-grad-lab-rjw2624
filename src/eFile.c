// filename ************** eFile.c *****************************
// High-level routines to implement a solid-state disk
// Jonathan W. Valvano 3/9/17

#include <string.h>
#include "eDisk.h"
#include "UART.h"
#include <stdio.h>
#include <stdbool.h>

#define SUCCESS 0
#define FAIL 1

// Sector size in bytes
#define SECTOR_BYTES 512

typedef uint32_t sector_addr_t;

#define LONGEST_FILENAME 7
typedef struct
{
  char file_name[LONGEST_FILENAME + 1]; // space for null terminator
  sector_addr_t start;
  uint32_t size; // size of zero indicates unused
} dir_entry_t;

#define DIR_START 32  // Sector number
#define DIR_SECTORS 2 // Number of sectors directory may span
#define DIR_ENTRIES ((DIR_SECTORS * SECTOR_BYTES) / (sizeof(dir_entry_t)))
static dir_entry_t dir[DIR_ENTRIES];

#define FAT_SECTORS 130056 // Number of sectors FAT may span
#define FAT_ENTRIES ((FAT_SECTORS * SECTOR_BYTES) / sizeof(sector_addr_t))
#define FAT_START 34
#define CACHED_SECTORS (SECTOR_BYTES / sizeof(sector_addr_t)) //

#define DATA_START 130090

static sector_addr_t fat_cache[CACHED_SECTORS];
static sector_addr_t cached_fat_sector = 0;
static bool fat_cache_dirty = false;

static void writeback_fat_cache(void)
{
  eDisk_WriteBlock((BYTE *)fat_cache, cached_fat_sector);
}

static void cache_fat_sector(sector_addr_t abs_sector_num)
{
  if (cached_fat_sector != abs_sector_num)
  {
    if (fat_cache_dirty)
    {
      writeback_fat_cache();
    }
    cached_fat_sector = abs_sector_num;
    eDisk_ReadBlock((BYTE *)fat_cache, abs_sector_num);
  }
}

int eFile_Init(void)
{
  memset(dir, 0, sizeof(dir));
  memset(fat_cache, 0, sizeof(fat_cache));
  eDisk_Init(0);
  for (int i = 0; i < DIR_SECTORS; i++)
  {
    eDisk_ReadBlock(((uint8_t *)dir) + i * SECTOR_BYTES, DIR_START + i);
  }
  cache_fat_sector(FAT_START);
  return SUCCESS;
}

int eFile_Format(void)
{
  memset(dir, 0, sizeof(dir));
  memset(fat_cache, 0, sizeof(fat_cache));
  for (int i = 0; i < DIR_SECTORS; i++)
  {
    eDisk_WriteBlock(((uint8_t *)dir) + i * SECTOR_BYTES, DIR_START + i);
  }

  for (int i = 0; i < FAT_SECTORS; i++)
  {
    for (int j = 0; j < CACHED_SECTORS; j++)
    {
      fat_cache[j] = i * CACHED_SECTORS + j + 1;
    }
    eDisk_WriteBlock((BYTE *)fat_cache, FAT_START + i);
  }

  for (int j = 0; j < 34; j++)
  {
    fat_cache[j] = 0;
  }
  for (int j = 34; j < CACHED_SECTORS; j++)
  {
    fat_cache[j] = j + 1;
  }

  eDisk_WriteBlock((BYTE *)fat_cache, FAT_START);
  fat_cache_dirty = false;
  dir[0].start = 0; // sector offset from DATA_START
  dir[0].size = 1;
  return SUCCESS;
}

int eFile_Create(char name[])
{
  dir_entry_t new_file;
  int idx;
  int freespace;
  int FATTABLE;
  for (idx = 0; idx < DIR_ENTRIES; idx++)
  {
    if (dir[idx].size == 0)
      break;
  }
  if (idx == DIR_ENTRIES)
  {
    return FAIL; // FULL Files
  }
  strncpy(dir[idx].file_name, name, LONGEST_FILENAME);
  // Create directory with given name.

  dir[idx].size = 1;                                        // set size.
  FATTABLE = dir[0].start / CACHED_SECTORS;                 // FATTABLE will save which sector to load from the disk
  eDisk_ReadBlock((BYTE *)fat_cache, FATTABLE + FAT_START); // Read sector from disk

  freespace = dir[0].start;                             // get free space that we could use
  dir[0].start = fat_cache[freespace % CACHED_SECTORS]; // set head of free space to nextone. (Next freespace using linked list kind of work)

  fat_cache[freespace % CACHED_SECTORS] = 0;                 // This is creating part, make sure currently allocated space is last one.
  eDisk_WriteBlock((BYTE *)fat_cache, FATTABLE + FAT_START); // write back to the disk.
  return SUCCESS;
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

int eFile_Directory(void (*fp)(char))
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

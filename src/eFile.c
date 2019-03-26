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
  uint32_t size; // size of zero indicates unused, size increase as write. size-1 is the point to write or read.
} dir_entry_t;

// Sectors 0 through 31 are reserved
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
static bool write_mode = false;
static BYTE DATAarray[SECTOR_BYTES];

static int get_writepoint_in_sector(int dir_idx)
{
  return dir[dir_idx].size - 1;
}

static void writeback_fat_cache(void)
{
  eDisk_WriteBlock((BYTE *)fat_cache, cached_fat_sector);
  fat_cache_dirty = false;
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

static void writeback_dir(void)
{
  for (int i = 0; i < DIR_SECTORS; i++)
  {
    eDisk_WriteBlock(((uint8_t *)dir) + i * SECTOR_BYTES, DIR_START + i);
  }
}

static void cache_dir(void)
{
  for (int i = 0; i < DIR_SECTORS; i++)
  {
    eDisk_ReadBlock(((uint8_t *)dir) + i * SECTOR_BYTES, DIR_START + i);
  }
}

int eFile_Init(void)
{
  memset(dir, 0, sizeof(dir));
  memset(fat_cache, 0, sizeof(fat_cache));
  eDisk_Init(0);
  cache_dir();
  cache_fat_sector(FAT_START);
  return SUCCESS;
}

int eFile_Format(void)
{
  memset(dir, 0, sizeof(dir));
  memset(fat_cache, 0, sizeof(fat_cache));
  writeback_dir();

  for (int i = 0; i < FAT_SECTORS; i++)
  {
    cached_fat_sector = FAT_START + i;
    for (int j = 0; j < CACHED_SECTORS; j++)
    {
      fat_cache[j] = i * CACHED_SECTORS + j + 1;
    }
    writeback_fat_cache();
  }

  cached_fat_sector = 0;
  for (int j = 0; j < 34; j++)
  {
    fat_cache[j] = 0;
  }
  for (int j = 34; j < CACHED_SECTORS; j++)
  {
    fat_cache[j] = j + 1;
  }
  writeback_fat_cache();

  dir[0].start = 0; // sector offset from DATA_START
  dir[0].size = 1;
  return SUCCESS;
}

int eFile_Create(char name[])
{
  dir_entry_t new_file;
  int idx;
  int freespace;
  int FAT_sec_offset; // Sector offset from base sector of FAT
  for (idx = 0; idx < DIR_ENTRIES; idx++)
  {
    if (strncmp(dir[idx].file_name, name, LONGEST_FILENAME) == 0)
    {
      return FAIL; // There already is a file with this name
    }
    if (dir[idx].size == 0)
    {
      break; // Free dir entry at idx, we will allocate it to this file
    }
  }
  if (idx == DIR_ENTRIES)
  {
    return FAIL; // Directory is full, cannot create new file
  }
  strncpy(dir[idx].file_name, name, LONGEST_FILENAME);

  // Create directory entry with given name.
  dir[idx].size = 1; // set size to indicate entry occupied. True size of file is dir[].size - 1
  FAT_sec_offset = dir[0].start / CACHED_SECTORS;
  cache_fat_sector(FAT_sec_offset + FAT_START); // Read sector of FAT from disk

  freespace = dir[0].start;                             // get free space that we could use
  dir[0].start = fat_cache[freespace % CACHED_SECTORS]; // set head of free space to nextone. (Next freespace using linked list kind of work)

  fat_cache[freespace % CACHED_SECTORS] = 0; // This is creating part, make sure currently allocated space is last one.
  fat_cache_dirty = true;
  return SUCCESS;
}

int eFile_WOpen(char name[])
{
  int open_idx = -1;
  int FAT_sec_offset; // Sector offset from base sector of FAT
  int iter = 0;
  int prev_iter = 0;
  for (int i = 0; i < DIR_ENTRIES; i++)
  {
    if (strncmp(name, dir[i].file_name, LONGEST_FILENAME) == 0)
    {
      open_idx = i;
      write_mode = true;
      break;
    }
  }
  if (open_idx == -1)
    return FAIL;
  FAT_sec_offset = dir[open_idx].start / CACHED_SECTORS;
  prev_iter = dir[open_idx].start;
  cache_fat_sector(FAT_sec_offset + FAT_START);
  iter = fat_cache[prev_iter % CACHED_SECTORS];
  while (iter != 0)
  {
    FAT_sec_offset = iter / CACHED_SECTORS;
    cache_fat_sector(FAT_sec_offset + FAT_START);
    prev_iter = iter;
    iter = fat_cache[prev_iter % CACHED_SECTORS];
  }
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

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

#define LONGEST_FILENAME 7 // Excluding null terminator
typedef struct
{
  char file_name[LONGEST_FILENAME + 1]; // space for null terminator
  sector_addr_t start;
  uint32_t size; // size of zero indicates unused, size increase as write. size-1 is the point to write or read.
} dir_entry_t;

typedef struct
{
  int dir_idx;
  int sectornum; // Sector offset from DATA_START
  int bytenum;
} open_file_metadata_t;

// Sectors 0 through 31 are reserved
#define DIR_START 32  // Sector number
#define DIR_SECTORS 2 // Number of sectors directory may span
#define DIR_ENTRIES ((DIR_SECTORS * SECTOR_BYTES) / (sizeof(dir_entry_t)))
static dir_entry_t dir[DIR_ENTRIES];

#define FAT_SECTORS (130056 / 32) // Number of sectors FAT may span
#define FAT_ENTRIES ((FAT_SECTORS * SECTOR_BYTES) / sizeof(sector_addr_t))
#define FAT_START 34
#define CACHED_SECTORS (SECTOR_BYTES / sizeof(sector_addr_t)) //

#define DATA_START 130090

static sector_addr_t fat_cache[CACHED_SECTORS];
static sector_addr_t cached_fat_sector = -1; // Sector offset from start of FAT
static bool fat_cache_dirty = false;
static bool write_mode = false;
static BYTE DATAarray[SECTOR_BYTES];
static open_file_metadata_t open_file;

static int get_writepoint_in_sector(int dir_idx)
{
  return dir[dir_idx].size - 1;
}

static void cache_file_sector(sector_addr_t abs_sector_num)
{
  eDisk_ReadBlock((BYTE *)DATAarray, abs_sector_num);
}

/**
 * @brief Get the next sector of a file.
 * 
 * @param data_sector_offset Sector offset from DATA_START to lookup in the FAT.
 * @return sector_addr_t Return value is interpreted as an offset from DATA_START.
 */
static sector_addr_t get_next_file_sector(sector_addr_t data_sector_offset)
{
  sector_addr_t fat_sector = data_sector_offset / CACHED_SECTORS;
  cache_fat_sector(fat_sector);
  return fat_cache[data_sector_offset % CACHED_SECTORS];
}

static void writeback_file_sector(void)
{
  eDisk_WriteBlock((BYTE *)DATAarray, DATA_START + open_file.sectornum);
}

static void writeback_fat_cache(void)
{
  eDisk_WriteBlock((BYTE *)fat_cache, FAT_START + cached_fat_sector);
  fat_cache_dirty = false;
}

static int lookup_file_dir_idx(char name[])
{
  for (int i = 0; i < DIR_ENTRIES; i++)
  {
    if ((dir[i].size > 0) && (strncmp(name, dir[i].file_name, LONGEST_FILENAME) == 0))
    {
      return i;
    }
  }
  return -1;
}

static void cache_fat_sector(sector_addr_t fat_sector_offset)
{
  if (cached_fat_sector != fat_sector_offset)
  {
    if (fat_cache_dirty)
    {
      writeback_fat_cache();
    }
    cached_fat_sector = fat_sector_offset;
    eDisk_ReadBlock((BYTE *)fat_cache, FAT_START + fat_sector_offset);
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
  cache_fat_sector(0);
  return SUCCESS;
}

int eFile_Format(void)
{
  memset(dir, 0, sizeof(dir));
  memset(fat_cache, 0, sizeof(fat_cache));

  for (int i = 0; i < FAT_SECTORS; i++)
  {
    cached_fat_sector = i; // Offset from start of FAT in sectors
    for (int j = 0; j < CACHED_SECTORS; j++)
    {
      fat_cache[j] = i * CACHED_SECTORS + j + 1;
    }
    writeback_fat_cache();
  }

  dir[0].start = 0; // sector offset from DATA_START
  dir[0].size = 1;

  writeback_dir();
  return SUCCESS;
}

int eFile_Create(char name[])
{
  dir_entry_t new_file;
  int idx;
  int freespace;
  int FAT_sec_offset; // Sector offset from base sector of FAT
  int already_exists_at_idx = lookup_file_dir_idx(name);
  if(already_exists_at_idx != -1)
    return FAIL; // File already exists with this name

  // Find empty dir entry to use
  for (idx = 0; idx < DIR_ENTRIES; idx++)
  {
    if (dir[idx].size == 0)
      break; // Free dir entry at idx, we will allocate it to this file
  }
  if (idx == DIR_ENTRIES)
    return FAIL; // Directory is full, cannot create new file
  strncpy(dir[idx].file_name, name, LONGEST_FILENAME);

  // Create directory entry with given name.
  dir[idx].size = 1; // set size to indicate entry occupied. True size of file is dir[].size - 1
  FAT_sec_offset = dir[0].start / CACHED_SECTORS;
  cache_fat_sector(FAT_sec_offset); // Read sector of FAT from disk

  freespace = dir[0].start;                             // get free space that we could use
  dir[0].start = fat_cache[freespace % CACHED_SECTORS]; // set head of free space to nextone. (Next freespace using linked list kind of work)

  fat_cache[freespace % CACHED_SECTORS] = -1; // This is creating part, make sure currently allocated space is last one.
  fat_cache_dirty = true;
  return SUCCESS;
}

int eFile_WOpen(char name[])
{
  int FAT_sec_offset; // Sector offset from base sector of FAT
  int iter = 0;
  int prev_iter = 0;
  int open_idx = lookup_file_dir_idx(name);
  if (open_idx == -1)
    return FAIL;
  write_mode = true;
  FAT_sec_offset = dir[open_idx].start / CACHED_SECTORS;
  prev_iter = dir[open_idx].start;
  cache_fat_sector(FAT_sec_offset);
  iter = fat_cache[prev_iter % CACHED_SECTORS];
  while (iter != -1)
  {
    FAT_sec_offset = iter / CACHED_SECTORS;
    cache_fat_sector(FAT_sec_offset);
    prev_iter = iter;
    iter = fat_cache[prev_iter % CACHED_SECTORS];
  }
  //write_point_in_sector = (dir[open_idx].size - 1)%SECTOR_BYTES;
  eDisk_ReadBlock(DATAarray, prev_iter + DATA_START);
	open_file.bytenum = (dir[open_idx].size - 1);
	open_file.dir_idx = open_idx;
	open_file.sectornum = prev_iter;

  return SUCCESS;
}

int eFile_Write(char data)
{
	int idx = open_file.bytenum;
  if (!write_mode)
  {
    return FAIL;
  }
	if(idx !=0 && idx%SECTOR_BYTES ==0)
	{
		writeback_file_sector();
		int freespace = dir[0].start;                             // get free space that we could use
		fat_cache[open_file.sectornum%CACHED_SECTORS] = freespace;

		//need to change fatcache to freespace fatcache
		cache_fat_sector(freespace / CACHED_SECTORS);
		dir[0].start = fat_cache[freespace % CACHED_SECTORS]; // set head of free space to nextone. (Next freespace using linked list kind of work)
		fat_cache[freespace % CACHED_SECTORS] = -1; // This is creating part, make sure currently allocated space is last one.
		fat_cache_dirty = true;
		open_file.sectornum = freespace;
		cache_file_sector(freespace + DATA_START);
	}
	DATAarray[idx%SECTOR_BYTES] = data;
	open_file.bytenum++;
	return SUCCESS;
	
}

int eFile_Close(void)
{
}

int eFile_WClose(void)
{
	dir[open_file.dir_idx].size = open_file.bytenum + 1;
	writeback_dir();
	writeback_fat_cache();
	writeback_file_sector();
	write_mode = false;
	return SUCCESS;
}

int eFile_ROpen(char name[])
{
  int FAT_sec_offset; // Sector offset from base sector of FAT
  int iter = 0;
  int prev_iter = 0;
  int open_idx = lookup_file_dir_idx(name);
  if (open_idx == -1)
    return FAIL;
  write_mode = false;
  open_file.dir_idx = open_idx;
  open_file.bytenum = 0;
  open_file.sectornum = dir[open_idx].start;
  cache_file_sector(DATA_START + prev_iter);
  return SUCCESS;
}

static bool read_reached_eof(void)
{
  return (open_file.bytenum >= (dir[open_file.dir_idx].size - 1));
}

int eFile_ReadNext(char *pt)
{
  if (write_mode == false)
  {
    if (!read_reached_eof())
    {
      // More data left to read
      *pt = DATAarray[open_file.bytenum++ % SECTOR_BYTES];
      if ((open_file.bytenum % SECTOR_BYTES == 0) && !read_reached_eof())
      {
        // Get next sector
        open_file.sectornum = get_next_file_sector(open_file.sectornum);
        cache_file_sector(DATA_START + open_file.sectornum);
      }
      return SUCCESS;
    }
  }
  return FAIL;
}

int eFile_RClose(void)
{
}

int eFile_Directory(void (*fp)(char))
{
  for (int i = 0; i < DIR_ENTRIES; i++)
  {
    if(dir[i].size > 0)
    {
      char file_desc[32];
      memset(file_desc, 0, sizeof(file_desc));
      snprintf(file_desc, sizeof(file_desc), "%s: %dB\r\n", dir[i].file_name, dir[i].size);
      char *c = file_desc;
      while(*c != 0)
      {
          fp(*c);
      }
    }
  }
}

int eFile_Delete(char name[])
{
  int del_idx = -1;
  if (del_idx == -1)
    return FAIL;

  // Attach head of file's sector list to the tail of the free space linked list
  sector_addr_t sect_iter = dir[0].start; // Free space head
  sector_addr_t next_sect = get_next_file_sector(sect_iter);
  while(next_sect != 0) {
      sect_iter = next_sect;
      next_sect = get_next_file_sector(sect_iter);
  }
  cache_fat_sector(sect_iter /  CACHED_SECTORS);
  fat_cache[sect_iter % CACHED_SECTORS] = dir[del_idx].start; // freespace tail --> deleted file head

  // Erase directory entry
  memset(dir[del_idx].file_name, 0, sizeof(dir[0].file_name));
  dir[del_idx].size = 0;
  dir[del_idx].start = 0;

  writeback_fat_cache();
  writeback_dir();
}

int eFile_RedirectToFile(char *name)
{
}

int eFile_EndRedirectToFile(void)
{
}

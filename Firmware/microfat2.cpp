// Copyright (c) 2009, Charlie Robson
// All rights reserved.

// See license.txt for further information.


#if 0
#include <inttypes.h>
#include <stdio.h>
#include <string.h>
#endif

#include "Utils.h"
#include "microfat2.h"

const unsigned BYTES_PER_SECTOR = 512;


// Private information about the device's FAT structure.
//
static struct
{
  unsigned sectors_per_cluster, root_directory_sector_count;

  unsigned long root_directory_sector, cluster_2_sector;

  byte* sector_buffer;

  READFN read_sectors;
}
vars;


bool microfat2::initialize(byte* sector_buffer, READFN sector_reader)
{
  bool return_val = false;

  vars.sector_buffer = sector_buffer;
  vars.read_sectors = sector_reader;

  if (0 == vars.read_sectors(vars.sector_buffer, 0, 1))
  {
    partition_record_t* p = (partition_record_t*)&vars.sector_buffer[0x1be];
    long boot_sector = p->lbaAddrOfFirstSector;

    if (0 == vars.read_sectors(vars.sector_buffer, boot_sector, 1))
    {
      boot_sector_t* b = (boot_sector_t*)vars.sector_buffer;
      if (BYTES_PER_SECTOR == b->bytesPerSector)
      {
        vars.sectors_per_cluster = b->sectorsPerCluster;
        vars.root_directory_sector = boot_sector + b->reservedSectors + (b->fatCopies * b->sectorsPerFAT);
        vars.root_directory_sector_count = (b->rootDirectoryEntries * 32) / BYTES_PER_SECTOR;
        vars.cluster_2_sector = vars.root_directory_sector + vars.root_directory_sector_count;
        return_val = true;
      }
    }
  }

  return return_val;
}

//  Cache the last file opened...
directory_entry_t _last_directory_entry;

// Directory walker. Useful for all kinds of things. probably.
//
int microfat2::walkDirectory(WALKER_FN walkerFunction, void* locator_data)
{
  unsigned entry_idx = 0;

// cache hit
    if (walkerFunction(&_last_directory_entry, entry_idx, locator_data))
        return 0x7FFF;
        
  for (unsigned i = 0; i < vars.root_directory_sector_count; ++i)
  {
    if (0 == vars.read_sectors(vars.sector_buffer, vars.root_directory_sector + i, 1))
    {
      for (unsigned j = 0; j < BYTES_PER_SECTOR / 32; ++j)
      {
        directory_entry_t* directory_entry = (directory_entry_t*)(vars.sector_buffer + (32 * j));
        if (0 == directory_entry->filespec[0])
        {
          // All done.
          //
          return -1;
        }
        if (0xe5 == byte(directory_entry->filespec[0]))
        {
          // Deleted file
          //
          continue;
        }
        if (0 != (directory_entry->attributes & 0x18))
        {
          // This is a directory, volume label or LFN marker
          //
          continue;
        }

        if (walkerFunction(directory_entry, entry_idx, locator_data))
        {
          // All finished
          //
          return entry_idx;
        }
        ++entry_idx;
      }
    }
  }

  // directory was full.
  //
  return -1;
}


// A vague helper. Prevents the need to expose the FAT info block.
//
void microfat2::getFileInformation(directory_entry_t* directory_entry, unsigned long& sector, unsigned long& size)
{
  sector = vars.cluster_2_sector + ((directory_entry->startCluster-2) * vars.sectors_per_cluster);
  size = directory_entry->fileSize;
}


// Private helper struct
//
typedef struct
{
  unsigned long sector;
  unsigned long size;
  const char* cooked_name;
}
locator_t;

// Here's an example of a callback function for the walkDirectory() function.
// In this case we use it to find a specified file. It's called by the walker
// for each valid entry in the root directory. A pointer to locator_t data is
// passed in as the user data, which specified the required name, and receives
// the information about the file.
//
// This returns true if we've done what we came to do, so the walker terminates.
//

#define UPPER(_x) ((_x >= 'a' && _x <= 'z') ? (_x + 'A' - 'a') : _x)
static bool match83(const char* file83, const char* name)
{
    //  Match prefix
    for (byte i = 0; i < 8; i++)
    {
        char c = *name++;            
        if (c == '.')
            break;
        if (c == 0)
            return false;
        c = UPPER(c);
        if (c != file83[i])
            return false;
    }
    return true; // BUGBUG check ext too
    file83 += 8;
    for (byte i = 0; i < 3; i++)
    {
        char c = *name++;
        c = UPPER(c);
        if (c != file83[i])
            return false;
    }
    //  Match ext
    return true;
}



static bool locator(directory_entry_t* directory_entry,
   unsigned idx,
      void* locator_data)
{
  locator_t* user_data = (locator_t*)locator_data;

  // This is why we only deal with pre-cooked names.
  //
  if (match83(directory_entry->filespec, user_data->cooked_name))
  {
    long s = directory_entry->startCluster-2;
    long sp = vars.sectors_per_cluster;
    
     user_data->sector = vars.cluster_2_sector + s*sp;
     user_data->size = directory_entry->fileSize;
     if (directory_entry != &_last_directory_entry)
        _last_directory_entry = *directory_entry;  // cache this
    return true;
  }

  // Not done yet.
  //
  return false;
}


// Returns true if file located.
//
// Name cooking: See header file!
//
// The filename needs to be stored in PROGMEM, see the appropriate tutorial
// and the header file.
//
bool microfat2::locateFileStart(const char* cooked_name, unsigned long& file_start_sector, unsigned long& file_size_bytes, byte* sector_buffer)
{
  locator_t locator_user_data;
  locator_user_data.cooked_name = cooked_name;
  vars.sector_buffer = sector_buffer;   //

  // Walk the root directory looking for the specified file.
  //
  int entry_idx = walkDirectory(locator, (void*)&locator_user_data);

  // Data will only be valid if the walker returns a positive integer...
  //
  file_start_sector = locator_user_data.sector;
  file_size_bytes = locator_user_data.size;

  // ... and we subsequently return true.
  //
  return entry_idx != -1;
}

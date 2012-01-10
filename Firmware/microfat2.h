// Copyright (c) 2009, Charlie Robson
// All rights reserved.

// See license.txt for further information.

#ifndef __MICROFAT2_H__
#define __MICROFAT2_H__

//#include <WConstants.h>

// The original MicroFat was dependent upon specific hardware access code.
// This code doesn't suffer the same problem, but is a little more complex
// as a result. It shouldn't be beyond the ken of an intermediate level
// programmer, and should be usable by noobs.

// The uintX_t forms of integer type are used here to get around a problem
// with the callback definitions. It appears that if you use a typedef within
// a typedef then strange things are starting to happen.
//
// For example this works:
//
//   typedef uint8_t (*READFN)(uint8_t*, unsigned long, uint8_t);
//
// While this does not:
//
//   typedef byte (*READFN)(byte*, unsigned long, byte);
//
// And to be perfectly honest there are better things to do then chase this!
// Feel free to enlighten me if you know the anwer though :)


// These are the basic structure types used to access a FAT formatted device.

typedef struct
{
   uint8_t bootable;
   uint8_t chsAddrOfFirstSector[3];
   uint8_t partitionType;
   uint8_t chsAddrOfLastSector[3];
   uint32_t lbaAddrOfFirstSector;
   uint32_t partitionLengthSectors;
}
partition_record_t;

//#pragma pack(1)
typedef struct
{
   uint8_t jump[3];
   char oemName[8];
   uint16_t bytesPerSector;
   uint8_t sectorsPerCluster;
   uint16_t reservedSectors;
   uint8_t fatCopies;
   uint16_t rootDirectoryEntries;
   uint16_t totalFilesystemSectors;
   uint8_t mediaDescriptor;
   uint16_t sectorsPerFAT;
   uint16_t sectorsPerTrack;
   uint16_t headCount;
   uint32_t hiddenSectors;
   uint32_t totalFilesystemSectors2;
   uint8_t logicalDriveNum;
   uint8_t reserved;
   uint8_t extendedSignature;
   uint32_t partitionSerialNum;
   char volumeLabel[11];
   char fsType[8];
   uint8_t bootstrapCode[447];
   uint8_t signature[2];
}
boot_sector_t;

typedef struct
{
   char filespec[11];
   uint8_t attributes;
   uint8_t reserved[10];
   uint16_t time;
   uint16_t date;
   uint16_t startCluster;
   uint32_t fileSize;
}
directory_entry_t;


// Sector read function definition. If your hardware support library doesn't
// provide a sector read function that uses this signature then you need to
// provide a proxy function that does.
//
typedef uint8_t (*READFN)(uint8_t* buffer,
   unsigned long sector,
      uint8_t count);

// Directory entry processor function. It will receive a pointer to data forming
// a directory_entry_t structure, the index of the entry within the directory,
// and a pointer to the arbitrary user data specified when calling walkDirectory().
//
typedef bool (*WALKER_FN)(directory_entry_t* directory_entry_data,
   unsigned index,
      void* user_data);

namespace microfat2
{
   // Specify the sector-sized buffer that should be used for reads, and a pointer-
   // to-a-function that can be used to read sectors. Using function pointers like
   // this breaks a dependency upon a specific hardware access library.
   //
   bool initialize(uint8_t* buffer, READFN read_function);

   // Walk the directory structure, calling the supplied function for each valid
   // entry. Returns index of entry if the callback function terminated the walk
   // by returning true, else -1.
   //
   int walkDirectory(WALKER_FN walker, void* user_data);

   // Given a pointer to a valid directory info structure, return some useful
   // information about the file.
   //
   void getFileInformation(directory_entry_t* directory_entry,
      unsigned long& sector,
         unsigned long& size);

   // Get start sector, file size for given pre-cooked filename.
   //
   // Filename should be stored in PROGMEM, use PSTR("filenameEXT").
   // The 8:3 form should be used when saving files to a FAT filesystem.
   // These are stored internally as 11 upper-case space-padded bytes.
   // The extension always starts at byte 8.
   // 'a.b' is stored as 'A       B  '.
   // 'readthis.txt' as 'READTHISTXT'.
   //
   bool locateFileStart(const char* progMemCookedName,
      unsigned long& sector,
         unsigned long& size,uint8_t* buffer);
};

#endif // __MICROFAT2_H__

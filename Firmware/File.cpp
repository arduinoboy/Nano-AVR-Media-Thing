
/* Copyright (c) 2009, Peter Barrett  
**  
** Permission to use, copy, modify, and/or distribute this software for  
** any purpose with or without fee is hereby granted, provided that the  
** above copyright notice and this permission notice appear in all copies.  
**  
** THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL  
** WARRANTIES WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED  
** WARRANTIES OF MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR  
** BE LIABLE FOR ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES  
** OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS,  
** WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION,  
** ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS  
** SOFTWARE.  
*/

#include "Utils.h"
#include "File.h"
#include "microfat2.h"

#ifdef _WIN32
#define pgm_read_byte(_x) *((byte*)(_x))
byte reader(byte* buffer, unsigned long sector, byte count);
HANDLE _h = INVALID_HANDLE_VALUE;
#else
uint8_t reader(uint8_t* buffer, unsigned long sector, uint8_t count)
{
    return mmc::readSectors(buffer,sector,count);
}
#endif

byte readSector(long sector, byte* dst)
{
  //  char s[32];
  //  sprintf(s,"readSector %ldn",sector);
  //  print(s);
  #if _WIN32
    DWORD r;
    ::SetFilePointer(_h,sector*512,0,0);
    ::ReadFile(_h,dst,512,&r,NULL);
    return 0;
  #else
    return reader(dst,sector,1);
  #endif
}

File::File() : _mark(512),_progmem(0),_sector(-1),_origin(0),_fileLength(-1)
{
}

byte File::OpenMem(const byte* data)
{
    _progmem = data;
    return 0;
}

#if 0
bool File::FileInfo(const char* path, ulong* length)
{
#if _WIN32
    *length = 0;
    return ::GetFileAttributesA(path) != INVALID_FILE_ATTRIBUTES;
#else
    ulong sector;
    return microfat2::locateFileStart(path,sector,*length);
#endif
}
#endif

void Console(const char* s);

byte File::Open(const char* path)
{
#if _WIN32
    char s[1024];
    sprintf(s,"C:\\Users\\peter\\Documents\\Visual Studio 2008\\Projects\\LCDEmu\\root\\%s",path);
    if (_h != INVALID_HANDLE_VALUE)
        ::CloseHandle(_h);
    _h = ::CreateFileA(s,
            GENERIC_READ,
            FILE_SHARE_READ,
            NULL,
            OPEN_EXISTING,
            FILE_ATTRIBUTE_NORMAL,
            NULL );
    Load(0);
    _fileLength = GetFileSize( _h, NULL );

    return _h == INVALID_HANDLE_VALUE ? 0 : 1;
#else
    if (microfat2::locateFileStart(path,_origin,_fileLength,_buffer))
    {
        Load(0);    // load 0?
        return 1;
    }
    #if 0
        char s[32];
        sprintf(s,"Opened %s %ld:%ld\n",path,_origin,_fileLength);
        print(s);
        Console(s);
    #endif
    return 0;
#endif
}

ulong File::GetFileLength()
{
    return _fileLength;
}

byte File::ReadByte()
{
    if (_mark == 512)
        Load(_sector + 1);
    return _buffer[_mark++];
}
      
int File::Read(void* d, int len)
{
    byte* dst = (byte*)d;
    for (int i = 0; i < len; i++)
        dst[i] = ReadByte();
    return len;
}

const byte* File::GetBuffer(int* count)
{
    if (_mark == 512)
        Load(_sector + 1);
    *count = sizeof(_buffer) - _mark;
    return _buffer + _mark;
}

void File::Skip(int count)
{
    _mark += count; // 
    if (_mark > 0x200)
    {
        _mark -= count;
        SetPos(GetPos() + count);
    }
    ASSERT(_mark >= 0 && _mark <= 0x200);
}

void File::Load(long sector)
{
    _mark = 0;
    if (_sector == sector)
        return;
    _sector = sector;
    
    if (_progmem)
    {
        const byte* p = _progmem + (sector << 9);
        for (ushort i = 0; i < 512; i++)
            _buffer[i] = pgm_read_byte(p++); // File system buffer?
    }
    else
        readSector(_sector + _origin,_buffer);
}

long  File::GetPos()
{
    return (_sector << 9) + _mark;
}

void  File::SetPos(long pos)
{
    Load(pos >> 9);
    _mark = pos & 0x1FF;
}
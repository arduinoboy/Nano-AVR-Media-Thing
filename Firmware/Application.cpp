

/* Copyright (c) 2010, Peter Barrett  
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
#include "microfat2.h"
#include "File.h"

void OLED_FrameBegin();
void OLED_Pixels(const uchar* p, int count);
void shutdown();

#define TACT_UP     4
#define TACT_DOWN   5
#define TACT_LEFT   6
#define TACT_RIGHT  7

short DoomEvent(KeyEvent& e);
short TunnelEvent(KeyEvent& e);
 
class AppState
{
    KeyEvent _keyEvent;
    byte _keyEventlast;
    
    byte _nodeSize;
    byte _node[128];    // MAX SIZE OF NODE DATA
    char _fileName[8+1+3+1];    // TODO: file references instead of names
    
    const char* _moviePath;
    long _movieOffset;
    long _movieEnd;
    int  _headerLength;
    
    byte _appID;
    char _path[8+1+3+1];
    
    enum
    {
        Unknown,
        UpEvent,
        DownEvent,
        LeftEvent,
        RightEvent,
        TimeoutEvent,
        DefaultEvent,

        LocalNodeRef = 32,  // 2
        NodeRef,            // 2:path

        LocalMovieRef = 64, // 4:4
        MovieRef,            // 4:4:path

        NodeName = 96,
        NodePath = 97
    };

public:    
    void Init()
    {
        _keyEventlast = 0;
        _keyEvent.msg = 0;
    }
    
    #define R16(_x) *((short*)(_x))  // ARMfix
    
    //  Decided to play a node
    void Play(const byte* d, int len)
    {
        const byte* end = d + len;
        long movieOffset = -1;
        long movieLength = -1;
        int id = -1;
        while (d < end)
        {
            switch (*d++)
            {
                case LocalMovieRef:
                    movieOffset = R16(d);
                    movieLength = R16(d+2);
                    d += 4;
                    break;
                case LocalNodeRef:
                    id = R16(d);
                    d += 2;
                    break;
                default:
                    ASSERT("bad node");
                    return;
            }
        }
        Load(_fileName, id);
        
        //  Movie data in local file
        movieOffset *= 96*64*2; // Frame to byte offsets
        movieLength *= 96*64*2;
        movieOffset += _headerLength;
        PlayMovie(_fileName,movieOffset, movieLength);
    }
    
    bool Nav(byte tag)
    {
        int i = 0;
        while (i < _nodeSize)
        {
            if (_node[i] == tag)
            {
                Play(_node+i+2, _node[i + 1]);
                return true;
            }
            i += 2 + _node[i + 1];
        }
        return false;
    }
        
    void PlayMovie(const char* fileName, long movieOffset, long movieLength)
    {
        _moviePath = fileName;
        _movieOffset = movieOffset;
        _movieEnd = movieOffset + movieLength;
    }
    
    void StopMovie()
    {
        _movieOffset = _movieEnd;
    }
    
    void ShowFrame(File& file)
    {
        OLED_FrameBegin();
        for (byte i = 0; i < 24; i++)
        {
            int count;
            file.SetPos(_movieOffset);
            _movieOffset += 512;
            const byte* d = file.GetBuffer(&count);
            OLED_Pixels(d,count>>1);
        }
        
        //  trick modes
        if (_appID == 'M')
        {
            long dst = _movieOffset;
            if (_keyEvent.keys & (1 << TACT_LEFT))
                dst -= 6L*96*64*2;
            else if (_keyEvent.keys & (1 << TACT_RIGHT))
                dst += 5L*96*64*2;    
            if (dst > 0 && dst < _movieEnd)
                _movieOffset = dst;           
        }
    }
    
    //  Play movie
    void NextFrame()
    {
        if (_movieOffset == _movieEnd)
        {
            CodeInit();
            return;
        }
        
        File file;
        file.Open(_moviePath);
        ShowFrame(file);
    }
    
    int Load(const char* path, int blobNum)
    {
        File file;
        if (!file.Open(path))
            return -1;
        _nodeSize = ReadBlob(_node,sizeof(_node),file,blobNum);
        
        _path[0] = 0;
        _appID = 0;
        int i = 0;
        while (i < _nodeSize)
        {
            switch(_node[i])
            {
                case NodePath:
                    {
                        byte len = min(_node[i+1],sizeof(_path)-1);
                        strncpy(_path,(char*)_node+i+2,len);
                        _path[len] = 0;
                    }
                    break;
            }
            i += 2 + _node[i + 1];
        }
        return _nodeSize;
    }
    
    int Load(const char* path)
    {
        _movieOffset = _movieEnd = 0;
        strcpy(_fileName,path);
        return Load(path,1);
    }
    
    void Splash()
    {
        strcpy(_path,"splash.rmv");
    }
    
    int ReadBlob(uchar* dst, int maxLen, File& file, ushort blobNum)
    {
        ushort hdr[3];
        file.SetPos(0);
        file.Read(hdr,6);   // hdr,size,count
        if (hdr[0] != 0x646E)   // 'nd'
            return -1;
        if (blobNum <= 0 || blobNum > hdr[2])
            return -1;
        file.SetPos(2 + (blobNum<<1));
        _headerLength = hdr[1] + 4;
        
        ushort offset[2];
        file.Read(offset,4);    // read offset of this blob
        if (blobNum == 1)
            offset[0] = 0;
            
        int count = offset[1] - offset[0];
        count = min(count,maxLen);
        file.SetPos(6 + (hdr[2]<<1) + offset[0]);
        file.Read(dst,count);
        return count;
    }
    
    void CodeInit()
    {
        if (_path[0] == ':')
        {
            _appID = _path[1];
            _keyEvent.msg = 0;
            CodeEvent();
            _keyEvent.msg = 1;
        }
        else if (_path[0] && _appID == 0)
        {
            _appID = 'M';   // Playing movie in FILE
            File f;
            if (f.Open(_path))
                PlayMovie(_path,512,f.GetFileLength()-512);
        }
    }
    
    void CodeEvent()
    {
        short r = 0;
        switch (_appID)
        {
            case 'X':
                shutdown();
                r = -1; // Won't get here unless in dev mode
                _path[0] = 0;
                break;
            case '0':   r = TunnelEvent(_keyEvent); break;
            case '1':   r = DoomEvent(_keyEvent);   break;
            case 'M':
                if (_keyEvent.upEvent & (1 << TACT_UP) || _movieOffset == _movieEnd)
                {
                    _path[0] = 0;
                    StopMovie();
                    r = -1;
                }
                else
                    NextFrame();
                break;  // Playing a movie
        }
        if (r == -1)    // quit
        {
            _appID = 0;
            Nav(LeftEvent);
        }
    }
    
    void Loop(byte k)
    {
        byte changed = k ^ _keyEventlast;
        _keyEventlast = k;
        _keyEvent.keys = k;
        _keyEvent.downEvent = changed & k;
        _keyEvent.upEvent = changed & ~k;

        //  Handle events to doom, joomp etc
        if (_appID)
        {
            CodeEvent();
            return;
        }
        
        //  Handle framework events       
        NextFrame();
        byte up = _keyEvent.upEvent;
        if (up)
        {            
            for (byte i = 0; i < 4; i++)
                if ((1 << (TACT_UP+i)) & up)
                    Nav(i + UpEvent);
        }
    }
};

//=====================================================
//=====================================================

byte reader(byte* buffer, unsigned long sector, byte count);
AppState _appState;

void Application::Init()
{    
        
    #ifndef _WIN32
    mmc::initialize();
    #endif
    
    {
        byte fatBuffer[512];
        microfat2::initialize(fatBuffer,reader);
    }
    _appState.Init();
    _appState.Load("test.rmv");
    _appState.Splash();
}

void Application::Loop(byte keyMap)
{
    _appState.Loop(keyMap);
}
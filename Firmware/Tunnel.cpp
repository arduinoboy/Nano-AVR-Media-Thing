

#include "Utils.h"

extern const byte Cell16x16[] PROGMEM;  // 8x64 (512 bytes)
extern const byte _tunnel[48*32*2] PROGMEM;
extern const byte _pal[] PROGMEM;

#include "tunnel.h"
#include "Palette.h"
#include "Cell16x16.h"

void OLED_Slice(uchar x,byte* buffer,byte *palette);
void OLED_Row(uchar y,byte* buffer,byte *palette);

#define SCREEN_WIDTH 96
#define SCREEN_HEIGHT 64

extern const byte _pattern1[] PROGMEM;
extern const byte _pattern2[] PROGMEM;
extern const byte _pattern3[] PROGMEM;
extern const byte _pattern4[] PROGMEM;

const byte _pattern1[] = {
    1,0,0,0,0,0,0,0,
    0,1,0,0,0,0,0,0,
    0,0,1,0,0,0,0,0,
    0,0,0,1,0,0,0,0,
    0,0,0,0,1,0,0,0,
    0,0,0,0,0,1,0,0,
    0,0,0,0,0,0,1,0,
    0,0,0,0,0,0,0,1,
};

const byte _pattern2[] = {
    1,0,0,0,0,0,0,0,
    2,1,0,0,0,0,0,1,
    2,2,1,0,0,0,1,2,
    2,2,2,1,0,1,2,2,
    2,2,2,2,1,2,2,2,
    2,2,2,2,2,2,2,2,
    2,2,2,2,2,2,2,2,
    3,3,3,3,3,3,3,3,
};

const byte _pattern3[] = {
    0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,
    4,4,4,4,4,4,4,4,    
    5,5,5,5,5,5,5,5,
    0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,
};

const byte _pattern4[] = {
    0,0,0,7,0,0,0,0,
    0,8,0,0,0,7,0,0,
    0,0,0,0,0,0,0,0,
    0,7,0,0,8,0,0,0,
    0,0,0,0,0,0,0,7,
    0,0,7,0,0,8,0,0,
    8,0,0,0,0,0,0,0,
    0,0,0,0,8,0,0,0,
};

class Tunnel
{        
    int _angleOffset;
    int _depthOffset;
    const byte* _level[32];
    
    byte _depthDelta;
    char _angleDelta;
    
    public:
    short Init()
    {
        _depthOffset = 0;
        _angleOffset = 0;
        _angleDelta = _depthDelta = 0;
        
        for (byte i = 0; i < 32; i += 4)
        {
            _level[i] = _pattern1;
            _level[i+1] = _pattern2;
            _level[i+2] = _pattern3;
            _level[i+3] = _pattern4;
        }
        return 0;
    }
    
    void move(KeyEvent& e)
    {        
        if (e.keys & 0x40)
        {
            if (_angleDelta < 0)
            _angleDelta = 0;
            if (_angleDelta < 16)
                _angleDelta++;
        }
        else if (e.keys & 0x80)
        {
            if (_angleDelta > 0)
            _angleDelta = 0;
            if (_angleDelta > -16)
                _angleDelta--;
        }
        else if (_angleDelta > 0)
        {
            _angleDelta--;
        }
        else if (_angleDelta < 0)
        {
            _angleDelta++;
        }
        
        if (e.keys & 0x20)
        {
            if (_depthDelta < 8)
                _depthDelta++;
        }
        else if (_depthDelta > 0)
        {
            _depthDelta--;
        }
                 
        _angleOffset += _angleDelta;
        _depthOffset += _depthDelta+1;
    }
    
    inline const byte* GetLine(int depth)
    {
        byte line = depth >> 4;                         // block:line:pixel 5:3:4
        return _level[line >> 3] + (line & 0x7)*8;      // Get line 0..7 of tile indexes from depth
    }
    
    void RenderLine(const byte* ad, int bottom, byte y)
    {
        int angleOffset = _angleOffset;
        int depthOffset = _depthOffset;
               
        byte buffer[SCREEN_WIDTH];     // horizontal buffer (could b vertical to save mem)
        byte x0 = 0;
        byte x1 = SCREEN_WIDTH;      // Write from both sides
        while (x0<x1)
        {
            int angle0 = pgm_read_byte(ad++);
            if (bottom)
                angle0 = -angle0;
            int angle1 = 512-angle0;

            int depth = pgm_read_byte(ad++);
            byte p0,p1;
            p0=p1=0;
            if (depth > 200)
            {
            }
            else
            {
                // Make this faster
                angle0 += angleOffset;
                angle1 += angleOffset;
                depth += depthOffset;   // roll into texture calc?
                                
                //  Tiles are 16x16
                const byte* line = GetLine(depth);  // Get the line to display at this depth            
                byte x0 = angle0 >> 3;
                byte x1 = angle1 >> 3;
                byte y = depth & 0xF;   // pixel y
                
                byte t0 = pgm_read_byte(&line[(x0 >> 4) & 0x7]);    // tile indexes
                byte t1 = pgm_read_byte(&line[(x1 >> 4) & 0x7]);    // lines are 8 tiles wide
                                
                if (t0 != 0)
                    p0 = pgm_read_byte(&Cell16x16[t0*256 - 256 + y*16 + (x0 & 0xF)]);
                if (t1 != 0)
                    p1 = pgm_read_byte(&Cell16x16[t1*256 - 256 + y*16 + (x1 & 0xF)]);
            }
            buffer[x0++] = p0;
            buffer[--x1] = p1;
        }
        OLED_Row(y,buffer,(byte*)_pal);
    }
    
    short Loop(KeyEvent& e)
    {
        if (e.upEvent & 0x10)
            return -1;
        move(e);
                    
         byte y=0;
         const byte* ad = _tunnel;      // angle/depth data        
         while (y < SCREEN_HEIGHT/2)
         {
            RenderLine(ad,0,y++);
            ad += SCREEN_WIDTH;
         }
         while (y < SCREEN_HEIGHT)
         {
            ad -= SCREEN_WIDTH;
            RenderLine(ad,1,y++);
         }
         return 0;
    }
};

Tunnel _tunnelInstance;
short TunnelEvent(KeyEvent& e)
{
    if (e.msg == 0)
        return _tunnelInstance.Init();
    return _tunnelInstance.Loop(e);
}

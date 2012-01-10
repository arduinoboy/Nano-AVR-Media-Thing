

#include "Utils.h"

extern const byte textures32_pal_dark[] PROGMEM; // 254 colors (512 bytes)
extern const byte textures32[] PROGMEM;    // 32x256 (8192 bytes)
extern const byte textures32_pal[] PROGMEM;// 254 colors (512 bytes)

#include "Textures32.h"

void OLED_Slice(uchar x,byte* buffer,byte *palette);
void OLED_Row(uchar y,byte* buffer,byte *palette);

#if 1
extern const byte _trig[64] PROGMEM;
const byte _trig[64] = 
{
    0,6,13,19,25,31,37,44,
    50,56,62,68,74,80,86,92,
    98,103,109,115,120,126,131,136,
    142,147,152,157,162,167,171,176,
    180,185,189,193,197,201,205,208,
    212,215,219,222,225,228,231,233,
    236,238,240,242,244,246,247,249,
    250,251,252,253,254,254,255,255
};

short SIN(byte angle)
{
    if ((angle & 0x7F) == 0)
        return 0;
    byte b = angle & 0x3F;
    if (angle & 0x40)
        b = 0x3F - b;
    short i = pgm_read_byte(_trig+b) + 1;
    if (angle & 0x80)
        return -i;
    return i;
}

short COS(byte angle)
{
    return SIN(angle + 64);
}
#endif

#define TEXTURE_SIZE 32

#define _S 1    // Steel Wall
#define _W 2    // Wood wall
#define WD 3    // wood door
#define _H 4    // Hex brick wall

#define _C 5    // Clinker brick wall
#define _A 6    // Ammo
#define _B 7    // Blue wall
#define SD 8    // Steel door

extern const byte _map[64] PROGMEM;
const byte _map[64] =
{
    _S,_S,_S,_S,_A,_W,_W,_W,
    SD, 0, 0,_S, 0, 0, 0,WD,
    _S,_S, 0,_S, 0,_W,_W,_W,
    _A, 0, 0, 0, 0, 0, 0,_C,
    _H,_B,_B, 0,_B,_B, 0,_C,
    _H, 0, 0, 0,_B,_C, 0,WD,
    WD, 0, 0, 0, 0,SD, 0,_C,
    _H,_H,_H,_H,_H,_C,_C,_C,
};

//  Range if uv is 0..2 in 16:16
//  Always positive
long RECIP(short uv)
{
    uv >>= 1;   // 0..1;
    if (uv < 4)
        return 0x7FFFFFFF;
    if (uv == 0x10000)      // 2 really
        return 0x8000;      // 1/2 = 0.5
    return 0x80000000/uv;   // Long divide to provide 16:16 result
}

//  a is +- 256 representing a 8:8 numver
//  dduv is always positive 16:16 number may be very large, might resonably be trimmed
long MUL8(short a, long dduv)
{
    return a*(dduv >> 8);
}

#define SCREEN_WIDTH 96
#define SCREEN_HEIGHT 64

short SIN(byte angle);  // angle is 0..1024
short COS(byte angle);



class Doomed
{
    short _playerPosX;    
    short _playerPosY;
    
    byte _angle;
    byte _rate;
    ushort _angle16;
    char _arate;
            
    public:
    short Init()
    {
        _angle16 = 0;
        _angle = 0;
        _rate = _arate = 0;
        _playerPosX = 0x380;
        _playerPosY = 0x400;
        return 0;
    }
    
    #define PLAYERWIDTH 0x20
    bool InWall(short dx, short dy)
    {
        dx += _playerPosX;
        dy += _playerPosY;
        byte x0 = (dx-PLAYERWIDTH)>>8;
        byte y0 = (dy-PLAYERWIDTH)>>8;
        byte x1 = (dx+PLAYERWIDTH)>>8;
        byte y1 = (dy+PLAYERWIDTH)>>8;
        while (x0 <= x1)
        {
            for (byte y = y0; y <= y1; y++)
                if (pgm_read_byte(&_map[y * 8 + x0]))
                    return true;
            x0++;
        }
        return false;
    }

    // Add a little acceleration to movement
    void move(byte key)
    {
        if ((key & 0x40))   
        {
            if (_arate < 0)
                _arate = 0;
            if (_arate < 32)
                _arate++;
        }
        else if ((key & 0x80))
        {
            if (_arate > 0)
                _arate = 0;
            if (_arate > -32)
                --_arate;
        }   
        else if (_arate > 0)
            _arate--;
        else if (_arate < 0)
            _arate++;
        
        if (_arate)
        {
            _angle16 += _arate;
            _angle = _angle16 >> 4;
        }
            
        if ((key & 0x20))
        {
            if (_rate < 32)
                _rate++;
        } else if (_rate > 0)
            _rate--;
        
        //  Rather dumb wall avoidance
        while (_rate)
        {
            short dx = ((COS(_angle) >> 1)*_rate) >> 7;
            short dy = ((SIN(_angle) >> 1)*_rate) >> 7;
            if (InWall(dx,dy))
            {
                if (!InWall(0,dy))
                    dx = 0;
                else if (!InWall(dx,0))
                    dy = 0;
            }               
            if (!InWall(dx,dy))
            {
                _playerPosX += dx;
                _playerPosY += dy;           
                break;
            }
            _rate--;
        }
    }
    
    //  a*((mpos<<8) - n) / b;
    //  return 3 extra bits than is required
    byte TEXTURE(short a, short b, short mpos, short n)
    {
        long o = (mpos<<8) - n;
        return o*a/b;
    }
 
    short Loop(KeyEvent& e)
    {        
        if (e.upEvent & 0x10)
            return -1;
            
        move(e.keys);
         
        // cast all rays here
        short sina = SIN(_angle) << 6;
        short cosa = COS(_angle) << 6;
        short u = cosa - sina;          // Range of u/v is +- 2 TODO: Fit in 16 bit
        short v = sina + cosa;
        short du = sina / (SCREEN_WIDTH>>1);     // Range is +- 1/24 - 16:16
        short dv = -cosa / (SCREEN_WIDTH>>1);
                        
        byte buffer[SCREEN_HEIGHT];    // vertical buffer
        for (byte ray = 0; ray < SCREEN_WIDTH; ++ray, u += du, v += dv)
        {           
            short uu = (u < 0) ? -u : u;
            short vv = (v < 0) ? -v : v;
            long duu = RECIP(uu);
            long dvv = RECIP(vv);
            char stepx = (u < 0) ? -1 : 1;
            char stepy = (v < 0) ? -1 : 1;

            // Initial position
            byte mapx = _playerPosX >> 8;
            byte mapy = _playerPosY >> 8;      
            byte mx = _playerPosX;
            byte my = _playerPosY;
            if (u > 0)
                mx = 0xFF-mx;
            if (v > 0)
                my = 0xFF-my;
            long distx = MUL8(mx, duu);
            long disty = MUL8(my, dvv);

            // the position and texture for the hit
            byte texture = 0;
            long hitdist = 0.1*65536;
            bool dark = false;
            byte t;
                
            // loop until we hit something
            while (texture <= 0)
            {
                if (distx > disty) {
                    // shorter distance to a hit in constant y line
                    hitdist = disty;
                    disty += dvv;
                    mapy += stepy;
                    texture = pgm_read_byte(&_map[mapy * 8 + mapx]);
                    if (texture > 0) {
                        dark = true;
                        if (stepy > 0)
                            t = _playerPosX + TEXTURE(u,v,mapy,_playerPosY);
                        else
                            t = -(_playerPosX + TEXTURE(u,v,mapy+1,_playerPosY));
                    }
                } else {
                    // shorter distance to a hit in constant x line
                    hitdist = distx;
                    distx += duu;
                    mapx += stepx;
                    texture = pgm_read_byte(&_map[mapy * 8 + mapx]);
                    if (texture > 0) {
                        if (stepx > 0)
                            t = _playerPosY + TEXTURE(v,u,mapx,_playerPosX);
                        else
                            t = -(_playerPosY + TEXTURE(v,u,mapx+1,_playerPosX));
                    }
                }
            }

            
            // start from the texture center (horizontally)
            short dy = hitdist / (((SCREEN_WIDTH >> 1) * ((256<<2)/TEXTURE_SIZE)));
            short p1 = ((TEXTURE_SIZE / 2) << 8) - dy;

            //  when dy <= 128, use smaller texture (mipmap)
            const byte* tex;
            const byte* palette = dark ? textures32_pal_dark : textures32_pal;
            short tt = --texture;
            
            #if 0
            if (dy >= 256) // MIPMAP if desired
            {
                tt = tt*(TEXTURE_SIZE>>1) + (t>>4);
                tex = textures16 + tt*(TEXTURE_SIZE>>1);
                p1 = TEXTURE_SIZE >> 2;
                dy >>= 1;
            } else {
            #endif
                tt = tt*TEXTURE_SIZE + (t>>3);
                tex = textures32 + tt*TEXTURE_SIZE;
                p1 = TEXTURE_SIZE >> 1;
           // }
            p1 = (p1 << 8) - dy;
            short p2 = p1 + dy;

            // start from the screen center (vertically)
            // y1 will go up (decrease), y2 will go down (increase)
            byte y1 = SCREEN_HEIGHT / 2;
            byte y2 = y1 + 1;
        
            // texture
            memset(buffer,0,sizeof(buffer));
            
            while (y1 >= 0 && y2 < SCREEN_HEIGHT && p1 >= 0)
            {
                buffer[y1] = pgm_read_byte(&tex[p1 >> 8]);
                buffer[y2] = pgm_read_byte(&tex[p2 >> 8]);
                p1 -= dy;
                p2 += dy;
                --y1;
                ++y2;
            }
                
            // ceiling and floor
            OLED_Slice(ray,buffer,(byte*)palette);
        }
    return 0;
    }
};


Doomed _doomedInstance;
short DoomEvent(KeyEvent& e)
{
    if (e.msg == 0)
        return _doomedInstance.Init();    
    return _doomedInstance.Loop(e);
}

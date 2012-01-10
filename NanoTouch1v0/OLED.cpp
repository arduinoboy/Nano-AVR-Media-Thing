// Simple analog to digitial conversion, similar to Wiring/Arduino


#include <avr/io.h>
#include <avr/pgmspace.h>
#include <avr/interrupt.h>
#include <util/delay.h>

#include "Utils.h"
#include "Board.h"
//#include "SplashImage.h"

typedef unsigned char byte;
typedef unsigned char uchar;
typedef unsigned short ushort;
typedef unsigned long ulong;
#define false 0

#define delay _delay_ms

void DataOut(uchar d)
{
    DATAPORTLO = d;
    CS0;
    WR0;
    WR1;
    CS1;
}

void ComOut(uchar c)
{
    DATAPORTLO = c;
    DC0;
    CS0;
    WR0;
    WR1;
    CS1;
    DC1;
}

//  BeginCom
//  EndCom


void OLED_draw_img(uchar x,uchar y,uchar x_size,uchar y_size,const uchar* image)
{    
    ComOut(0x15);          // Set Column Address
	ComOut(x);             // Start
	ComOut(x+x_size-1);    // End
	ComOut(0x75);          // Set Row Address
	ComOut(y);             // Start
	ComOut(y+y_size-1);    // End
	
	for(int i=0;i<(x_size*y_size*2);i++)
	{
        DataOut(pgm_read_byte(image + i));
	}
}

void Line(uchar x1,uchar y1,uchar x2,uchar y2,uchar r, uchar g, uchar b)
{
    ComOut(0x21);
    ComOut(x1);
    ComOut(y1);
    ComOut(x2);
    ComOut(y2);
    ComOut(b >> 2);
    ComOut(g >> 2);
    ComOut(r >> 2);
}

void OLED_FrameBegin(); // eh?
//  check command lock
void Rectangle(uchar x1,uchar y1,uchar x2,uchar y2,uchar r, uchar g, uchar b)
{   // Note: same as Line, framerect capable
    OLED_FrameBegin();
    ComOut(0x26);
    ComOut(0x11);   // reverse copy enable, fill enable

    ComOut(0x22);
    ComOut(x1);
    ComOut(y1);
    ComOut(x2);
    ComOut(y2);
    ComOut(b >> 2);
    ComOut(g >> 2);
    ComOut(r >> 2);
    ComOut(b >> 2);
    ComOut(g >> 2);
    ComOut(r >> 2);
}

extern const uchar _SSD1332[49] PROGMEM;
const uchar _SSD1332[49] = {
    0x81,0x9f,  //  Set Contrast for Color A TODO: BGR or RGB?
    0x82,0x3F,  //  Set Contrast for Color B
    0x83,0xFF,  //  Set Contrast for Color C These can be used to fade in / out
    
    0x87,0x0f,  //  Master Current Control
    
    0xa0,0x70,  //  Set Re-map & DataFormat, 65k
    
    0xa4,       //  Normal Display
    
    0xa8,0x3f,  //  Multiplex Ratio
    
    0xa9,0x03,  //  Power Control  SET POWER ON
    
    0xaf,       //  Display on
    
    
    0xb8,       //  Gray Scale Table
    
    #if 0
    0x01,       //  32 bytes
    0x05,
    0x09,
    0x0d,
    0x11,
    0x15,
    0x19,
    0x1d,
    
    0x21,
    0x25,
    0x29,
    0x2d,
    0x31,
    0x35,
    0x39,
    0x3d,
    
    0x41,
    0x45,
    0x49,
    0x4d,
    0x51,
    0x55,
    0x59,
    0x5d,
    
    0x61,
    0x65,
    0x69,
    0x6d,
    0x71,
    0x75,
    0x79,
    0x7d,
    #else
    0,
    1,
    2,
    3,
    5,
    7,
    9,
    12,
    15,
    18,
    21,
    24,
    28,
    32,
    36,
    40,
    44,
    49,
    53,
    58,
    63,
    68,
    73,
    79,
    84,
    90,
    96,
    102,
    108,
    114,
    121,
    127
#endif
};

void Initial(void)
{
    CS0;
    RESET0;
    delay(10);
    CS1;
    RESET1;
    delay(10);
    
    uchar i = sizeof(_SSD1332);
    const uchar* d = _SSD1332;
    while (i--)
        ComOut(pgm_read_byte(d++));
    SDN1;   // Turn on 12v
}

void OLED_Init(void)
{
    Initial(); 
    //OLED_draw_img(0,0,96,64,frame);
    Rectangle(0,0,96,64,0xFF,0x00,0x00);    // RED ish
}

void OLED_FrameBegin()
{
    uchar x = 0;
    uchar y = 0;
    uchar x_size = 96;
    uchar y_size = 64;
    
    ComOut(0x15);          // Set Column Address
	ComOut(x);             // Start
	ComOut(x+x_size-1);    // End
	ComOut(0x75);          // Set Row Address
	ComOut(y);             // Start
	ComOut(y+y_size-1);    // End
}

void OLED_Pixels(const uchar* p, int count)
{
    CS0;
    do {
        byte c = min(255,count);
        count -= c;
	    while (c--)
	    {
            DATAPORTLO = *p++;
            WR0;
            WR1;
            DATAPORTLO = *p++;
            WR0;
            WR1;
        }
    } while (count);
    CS1;
}

void BlitPalette(uchar c,byte* buffer,byte *palette)
{
	CS0;
    while (c--)
    {
        byte* p = palette + ((*buffer++) << 1);
        DATAPORTLO = pgm_read_byte(p++);
        WR0;
        WR1;
        DATAPORTLO = pgm_read_byte(p++);
        WR0;
        WR1;
    }
    CS1;
}

void OLED_Row(uchar y,byte* buffer,byte *palette)
{
    ComOut(0x15);          // Set Column Address
	ComOut(0);             // Start
	ComOut(96-1);         // End
	ComOut(0x75);          // Set Row Address
	ComOut(y);             // Start
	ComOut(y);             // End
	BlitPalette(96,buffer,palette);
}

void OLED_Slice(uchar x,byte* buffer,byte *palette)
{
    ComOut(0x15);          // Set Column Address
	ComOut(x);             // Start
	ComOut(x);             // End
	ComOut(0x75);          // Set Row Address
	ComOut(0);             // Start
	ComOut(64-1);         // End
	BlitPalette(64,buffer,palette);
}


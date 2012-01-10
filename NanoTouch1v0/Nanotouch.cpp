

#include <avr/io.h>
#include <avr/pgmspace.h>
#include <avr/interrupt.h>
#include <util/delay.h>
#include <stdint.h>

#include "Board.h"
#include "Utils.h"
#include "mmc.h"
#include "File.h"

void OLED_Init();

void shutdown()
{
    REGEN0;
    // This won't execute under normal circumstances
}

int main(void)
{
//  Set for 16 MHz clock
    CPU_PRESCALE(0);
    BOARD_INIT();
	OLED_Init();
    Application::Init();
    for (;;)
        Application::Loop(~PINF);
}



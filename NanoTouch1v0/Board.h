
//  Nanotouch board definitions

//  PORTB
//  0..7    i/o DATAPORTLO and SPIPORT
//  
//  PORTC
//  NOT USED
//
//  PORTD0  o   RD - OLED
//  PORTD1  o   WR - OLED
//  PORTD2  o   DC(A0)
//  PORTD3  o   RES - OLED reset
//  PORTD4  o   SDN - 12v enable
//  PORTD5  o   CS
//  PORTD6  o   NOT USED
//  PORTD7  o   NOT USED

//  PORTC6  NOT USED
//  PORTC7  NOT USED

//  PORTE2  o   HWB
//  PORTE6  o   REGEN - Power *pullup allows input?

//  PORTF0  o   ACCCS - Accelerometer select
//  PORTF1  o   MMCS - MMC select
//  PORTF4  i   UP
//  PORTF5  i   Down
//  PORTF6  i   Left
//  PORTF7  i   Right

//  Port D would have been a better choice for 8 bit port, leaves SS for MMC clear MOSI DDR etc

//  OLED Display
#define DATAPORTLO PORTB
#define DATAPORTLO_DDR DDRB
#define CONTROLPORT PORTD
#define CONTROLPORT_DDR DDRD

#define RD 0
#define WR 1
#define DC 2
#define RESET 3
#define CS 5

#define sbi(port,bitnum)		port |= _BV(bitnum)
#define cbi(port,bitnum)		port &= ~(_BV(bitnum))

#define CS0 cbi(CONTROLPORT,CS)
#define CS1 sbi(CONTROLPORT,CS)
#define DC0 cbi(CONTROLPORT,DC)
#define DC1 sbi(CONTROLPORT,DC)
#define RD0 cbi(CONTROLPORT,RD)
#define RD1 sbi(CONTROLPORT,RD)
#define WR0 cbi(CONTROLPORT,WR)
#define WR1 sbi(CONTROLPORT,WR)
#define RESET0 cbi(CONTROLPORT,RESET)
#define RESET1 sbi(CONTROLPORT,RESET)


// SPI
#define SPIPORT PORTB
#define SPIPORT_DDR DDRB

#define SS    1 // On Port F
#define SCK   1 // On port B
#define MOSI  2 // On port B
#define MISO  3 // On port B

//  SPIPORT is the same as OLED DATAPORTLO; pay close attention please...
#define SPI_SS_HIGH()  PORTF |= _BV(SS); SPIPORT_DDR = 0xFF; SPIPORT |=  _BV(SCK) | _BV(MOSI);
#define SPI_SS_LOW()   PORTF &= ~_BV(SS);  SPIPORT_DDR = ~_BV(MISO); // MISO as input

#define REGEN 6
#define REGEN0 cbi(PORTE,REGEN)
#define REGEN1 sbi(PORTE,REGEN)

#define SDN 4      // 12v power to oled
#define SDN0    cbi(PORTD,SDN)
#define SDN1    sbi(PORTD,SDN)

//  PORTF
#define TACT_UP     4
#define TACT_DOWN   5
#define TACT_LEFT   6
#define TACT_RIGHT  7

#define LED0 cbi(PORTD,4)
#define LED1 sbi(PORTD,4)

#define POWER0 cbi(PORTD,5)
#define POWER1 sbi(PORTD,5)

#define BACKLIGHT0 cbi(PORTD,7)
#define BACKLIGHT1 sbi(PORTD,7)

#define CPU_PRESCALE(n)	(CLKPR = 0x80, CLKPR = (n))
#define DISABLE_JTAG()  MCUCR = (1 << JTD) | (1 << IVCE) | (0 << PUD); MCUCR = (1 << JTD) | (0 << IVSEL) | (0 << IVCE) | (0 << PUD);
#define BOARD_INIT() PORTB = 0xFF; DDRB = 0xFF; PORTD = ~_BV(SDN); DDRD = 0xFF; DDRE =_BV(REGEN); PORTF = 0xFF; DDRF = 0x0F; REGEN1; DISABLE_JTAG();

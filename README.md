The font was created using the freefont fontconvert utility in the Adafruit_GFX_Library and the freefonts from http://ftp.gnu.org/gnu/freefont/

./fontconvert ~/Desktop/freefont/FreeSans.ttf 32 > ~/Desktop/Millcloud/PlatformIO/Projects/Simrad_IS15_instruments/src/FreeSans32pt7b.h

My Setup

The TFT pinout is as follows: (from here https://www.waveshare.com/wiki/3.5inch_RPi_LCD_(A)) 
Pin Number 	Identification 	Description
1 	3.3V 	Power (3.3V input)
2 	5V 	Power (5V input)
3 	NC 	NC
4 	5V 	Power (5V input)
5 	NC 	NC
6 	GND 	Ground
7 	NC 	NC
8 	NC 	NC
9 	GND 	Ground
10 	NC 	NC
11 	TP_IRQ 	The touch panel is interrupted, and it is low when it is detected that the touch panel is pressed
12 	NC 	NC
13 	NC 	NC
14 	GND 	Ground
15 	NC 	NC
16 	NC 	NC
17 	3.3V 	Power (3.3V input)
18 	LCD_RS 	Command/Data Register Select
19 	LCD_SI / TP_SI 	LCD display / SPI data input of touch panel
20 	GND 	Ground
21 	TP_SO 	SPI data output of touch panel
22 	RST 	Reset
23 	LCD_SCK / TP_SCK 	SPI clock signal for LCD display / touch panel
24 	LCD_CS 	LCD chip select signal, low level selects LCD
25 	GND 	Ground
26 	TP_CS 	Touch panel chip select signal, low level selects touch panel 

==SPI TFT SETUP==
--------------------------------------
|TFT pin| Function | Wire | EXP32 Pin|
--------------------------------------
|Pin 1  | 3.3v     |   RED      |   5V|
|Pin 2  | 5V       |  ORANGE    | 3v3|
|Pin 6  | 0V       |  BLACK     | Ov|
|Pin 11 | TP_IRQ   |  Pink      | GP21|
|Pin 18 | LCD_RS/DC|  BLUE      | GP2 |
|Pin 19 | MOSI     |  WHITE     | GP23|
|Pin 21 | MISO     |  PURPLE    | GP19|
|Pin 22 | RST      |  YELLOW    | GP4|
|Pin 23 | CLK      |  Dark Green| GP18|
|Pin 24 | LCD_CS   |  GREY      | GP15|
|Pin 26 | TP_CS    |  Light Green| GP22|
----------------------------------------

Rotary Encoder
#define ROTARY_ENCODER_A_PIN 32
#define ROTARY_ENCODER_B_PIN 21
#define ROTARY_ENCODER_BUTTON_PIN 25
#define ROTARY_ENCODER_VCC_PIN 27


Output form running Read_User_Setup.cpp

[code]
TFT_eSPI ver = 2.4.71
Processor    = ESP32
Frequency    = 240MHz
Transactions = Yes
Interface    = SPI
Display driver = 9486
Display width  = 320
Display height = 480

MOSI    = GPIO 23
MISO    = GPIO 19
SCK     = GPIO 18
TFT_CS   = GPIO 15
TFT_DC   = GPIO 2
TFT_RST  = GPIO 4
TOUCH_CS = GPIO 22

TFT_BL           = GPIO 21

Font GLCD   loaded
Font 2      loaded
Font 4      loaded
Font 6      loaded
Font 7      loaded
Font 8      loaded
Smooth font enabled

Display SPI frequency = 15.00
Touch SPI frequency   = 2.50
[/code]

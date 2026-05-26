:: batch file for automatic compilation of mcu specific and uart specific versions
:: --------------------------------------------------------------------------------------------------------------------
:: compiler version: gcc version 4.3.2 (WinAVR 20090313)


:: get parameters for RS485 versions
set DEF485=%1
set NAME485=%2

::goto test


:: ATmega162
:: --------------------------------------------------------
::set MCU=atmega162
::set BOOTADDR=0x3800
::set FREQ=14745600

::avr-gcc.exe  -mmcu=%MCU% -Wall -gdwarf-2 -std=gnu99 -DF_CPU=%FREQ% %DEF485% -Os -fsigned-char -funsigned-bitfields -fpack-struct -fshort-enums -MD -MP -MT main.o uart.o helpers.o -MF main.o.d uart.o.d helpers.o.d  -c  main.c uart.c helpers.c
avr-gcc.exe -mmcu=atmega162 -Wall -gdwarf-2 -std=gnu99 -DF_CPU=14745600 -O0 -fsigned-char -funsigned-bitfields -fpack-struct -fshort-enums -c main.c uart.c helpers.c
::avr-gcc.exe -mmcu=atmega162 -Wall -gdwarf-2 -std=gnu99 -DF_CPU=14745600 -O0 -fsigned-char -funsigned-bitfields -fpack-struct -fshort-enums -c uart.c 
::avr-gcc.exe -mmcu=atmega162 -Wall -gdwarf-2 -std=gnu99 -DF_CPU=14745600 -O0 -fsigned-char -funsigned-bitfields -fpack-struct -fshort-enums -c helpers.c
avr-gcc.exe -mmcu=atmega162 -Wl,-Map=uart_kline_bridge.map main.o uart.o helpers.o -o uart_kline_bridge.elf
avr-objcopy -O ihex -R .eeprom uart_kline_bridge.elf uart_kline_bridge_atmega162_v.hex

.\srecord-1.65.0-win64\bin\srec_cat.exe uart_kline_bridge_atmega162_v.hex -Intel -Output uart_kline_bridge_atmega162_srec.hex -Intel -Line_Length 44


::avr-gcc.exe -mmcu=%MCU% -nostartfiles -Wl,-Map=uart_kline_bridge.map main.o uart.o helpers.o -o uart_kline_bridge.elf
::avr-objcopy -O ihex -R .eeprom chip45boot2.elf build/chip45boot2_%MCU%_%NAME485%v%REV%.hex
::avr-objcopy -O ihex -R .eeprom uart_kline_bridge.elf uart_kline_bridge_atmega162_v.hex
:: --------------------------------------------------------

:ende
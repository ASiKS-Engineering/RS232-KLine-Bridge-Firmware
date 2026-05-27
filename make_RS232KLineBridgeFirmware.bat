:: batch file for automatic compilation of mcu specific and uart specific versions
:: --------------------------------------------------------------------------------------------------------------------
:: compiler version: gcc version 4.3.2 (WinAVR 20090313)

:: ATmega162
:: --------------------------------------------------------

avr-gcc.exe -mmcu=atmega162 -Wall -gdwarf-2 -std=gnu99 -DF_CPU=14745600 -O2 -fsigned-char -funsigned-bitfields -fpack-struct -fshort-enums -c main.c uart.c e2p.c
avr-gcc.exe -mmcu=atmega162 -Wl,-Map=uart_kline_bridge.map main.o uart.o e2p.o -o uart_kline_bridge.elf
avr-objcopy -O ihex -R .eeprom uart_kline_bridge.elf uart_kline_bridge_atmega162_v.hex

srec_cat.exe uart_kline_bridge_atmega162_v.hex -Intel -Output uart_kline_bridge_atmega162_srec.hex -Intel -Line_Length 44
:: --------------------------------------------------------

:ende

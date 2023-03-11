
# Waveshare 2.9" ePaper display Custom Chip 

- [todo - link to image](./ws29v2.png)
- [chip data sheet](https://www.waveshare.com/w/upload/7/79/2.9inch-e-paper-v2-specification.pdf)

The WS29v2 Custom Chip simulates the Waveshare 2.9" (V2) ePaper display module. 

The chip has the following pins:

| Pin | Name | Description                                            |
| --- | ------------ | ------------------------------------------------------ |
| 1 | BUSY| Busy state output pin Note 1.5-4
| 2 | RST| # Reset Note 1.5-3
| 3 | DC | Data /Command control pin Note 1.5-2
| 4 | CS | # Chip Select input pin Note 1.5-1
| 5 | CLK | serial clock pin (SPI)
| 6 | DIN | serial data pin (SPI)
| 7 | VCC | Power for interface logic pins
| 8 | GND | Ground




### Addressing
The module is addressed using the CS pin. 
### SPI Comms 
The device uses SPI for comms with the MCU.

Note that the chip only has a MOSI pin (DIN) to receive commands/data. Current implementation 
does not multiplex this pin to allow reading from the module.

A good Arduino library to use on the Arduino side is the Waveshare Library (https://github.com/waveshare/e-Paper)


## Implementation details
The custom chip supports many (but not all) of the SPI commands specified in the Waveshare datasheet. Moreover, for the simulation some of the commands make no sense to implement (there's no internal circuitry to drive for example, so the various driver commands are not needed.)
The section below lists the supported command groups with links to the appropriate section in the document when useful.

### reset 
This command resets the chip to power on. This is slightly different than described in the 
document and may be changed in future to align better

### write memory coordinate addresses
These commands are used to:
- set limits on the values of the x,y coordinates (useful when creating 'windowed' writes) 
- set the values of the x,y pointers themselves, 
- set the manner of incrementing of the pointers when writing to display memory 

### write to memory areas
These commands are used to write data to video memory (not the display) using values of x,y coordinates. 

### update display
These commands are used to control the manner in which the display should be updated (partially implemented) as well as actually executing the display.

> *Note* Partial update is not supported well yet

ink - Section 7.3.6
## Attributes
The chip defines a number of attributes that can aid in debugging operation when used in Wokwi. 

| Name         | Description                                            | Default value             |
| ------------ | ------------------------------------------------------ | ------------------------- |
| `debug`   | (0 or 1) when set to true, debug logs can be seen in the browser's developer tool window. This can be useful to confirm whether the operation of the program and chip is as expected | `0` |
| `debug_mask`   | 8 bit mask used to contro various debug aspects:<br> <li> `0x01` pin changes<br> <li> `0x02` SPI comms <br> <li> `0x08` timers<br> <li> `0x80` general chip behaviour | `0` |
| `version`   | (1 or 2) WS29 board version. V2 is better supported | `1` |
| `act_mode`   | Activation mode refers to the way the chip implements activation:<br><li>`0` - the default mode, causes the display to update like a full refresh has taken place<br><li>`1` - this activation mode is meant to simulate fading in/out of the display<br><li>`2` - scanline mode refreshes the screen one scanline at a time<br>Note that the display is busy while activation takes place   | `0` |

## Simulator examples

- [Waveshare 2.9" Custom Chip](https://wokwi.com/projects/348856116302578258)

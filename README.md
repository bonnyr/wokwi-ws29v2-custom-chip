
# Waveshare 29" ePaper display Custom Chip 

- [todo - link to image](./ws29v2.png)
- [chip data sheet](https://www.waveshare.com/w/upload/7/79/2.9inch-e-paper-v2-specification.pdf)

The WS29v2 Custom Chip simulates the Waveshare 2.9" ePaper display module. 

The chip has the following pins. Note the Used column for pins that
are used by the simulation. Unused pins do not have to be connected and will 
not react to changes or produce changes.

| Pin | Name         | Used | Description                                            |
| --- | ------------ | ---- | ------------------------------------------------------ |
| 1 | BUSY | Y| Busy state output pin Note 1.5-4
| 2 | RST | Y| # Reset Note 1.5-3
| 3 | DC | Y | Data /Command control pin Note 1.5-2
| 4 | CS | Y | # Chip Select input pin Note 1.5-1
| 5 | CLK | Y | serial clock pin (SPI)
| 6 | DIN | Y | serial data pin (SPI)
| 7 | VCC | Y | Power for interface logic pins
| 8 | GND | Y | Ground




### Addressing
The module is addressed using the CS pin. 
### SPI Comms 
The device uses SPI for comms with the MCU.

Note that the chip only has a MOSI pin (DIN) to receive commands/data. Current implementation does not
multiplex this pin to allow reading from the module.

A good Arduino library to use on the Arduino side is the Waveshare Library (https://github.com/waveshare/e-Paper)


## Implementation details
The custom chip supports many (but not all) of the SPI commands specified in the Waveshare datasheet. 
The section below lists the supported commands links to the appropriate section in the NXP document.

### reset 

Link - Section 7.3.6

## Attributes
The chip defines a number of attributes that can aid in debugging operation when used in Wokwi. 

| Name         | Description                                            | Default value             |
| ------------ | ------------------------------------------------------ | ------------------------- |
| debug   | (0 or 1) when set to true, debug logs can be seen in the browser's developer tool window. This can be useful to confirm whether the operation of the program and chip is as expected | "0" |

## Simulator examples

- [Waveshare 2.9" Custom Chip](https://wokwi.com/projects/348856116302578258)

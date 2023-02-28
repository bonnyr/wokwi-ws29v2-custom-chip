
# Waveshare 29" ePaper display Custom Chip 

- [todo - link to image](./ws29v2.png)
- [chip data sheet](https://www.waveshare.com/w/upload/7/79/2.9inch-e-paper-v2-specification.pdf)

The WS29v2 Custom Chip simulates the Waveshare 2.9" ePaper display module. 

The chip has the following pins. Note the Used column for pins that
are used by the simulation. Unused pins do not have to be connected and will 
not react to changes or produce changes.

| Pin | Name         | Used | Description                                            |
| --- | ------------ | ---- | ------------------------------------------------------ |
| 1 | NC | N | No connection and do not connect with other NC pins Keep Open
| 2 | GDR | N | N-Channel MOSFET Gate Drive Control
| 3 | RESE | N | Current Sense Input for the Control Loop
| 4 | NC | | N | No connection and do not connect with other NC pins e Keep Open
| 5 | VSH2 | N | This pin is Positive Source driving voltage
| 6 | TSCL | N*| I2C Interface to digital temperature sensor Clock pin
| 7 | TSDA | N*| I2C Interface to digital temperature sensor Date pin
| 8 | BS1 | Y| Bus selection pin Note 1.5-5
| 9 | BUSY | Y| Busy state output pin Note 1.5-4
| 10 | RES | Y| # Reset Note 1.5-3
| 11 | D/C# | Y | Data /Command control pin Note 1.5-2
| 12 | CS | Y | # Chip Select input pin Note 1.5-1
| 13 | SCL | Y | serial clock pin (SPI)
| 14 | SDA | Y | serial data pin (SPI)
| 15 | VDDIO | Y | Power for interface logic pins
| 16 | VCI | Y | Power Supply pin for the chip
| 17 | VSS | Y | Ground
| 18 | VDD | Y | Core logic power pin
| 19 | VPP | N* | Power Supply for OTP Programming
| 20 | VSH1 | N* | This pin is Positive Source driving voltage
| 21 | VGH | N* | This pin is Positive Gate driving voltage
| 22 | VSL | N* | This pin is Negative Source driving voltage
| 23 | VGL | N* | This pin is Negative Gate driving voltage
| 24 | VCOM | N* | These pins are VCOM driving voltage




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
| debug_timer   | (0 or 1) used to set the internal timer frequency such that a PWM cycle lasts ~1s. This is useful when needing to inspect the visual impact of PWM                   | "0"                 |
| gen_debug | (0 or 1) when set to true, debug logs can be seen in the browser's developer tool window. This can be useful to confirm whether the operation of the program and chip is as expected | "0" |
| spi_debug     | (0 or 1) enables debug log of the i2c communication exchange. This can be useful when debuggin i2c transactions      | "0"             |

## Simulator examples

- [Waveshare 2.9" Custom Chip](https://wokwi.com/projects/348856116302578258)
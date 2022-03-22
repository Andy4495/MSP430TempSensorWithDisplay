# MSP430 Temperature Sensor with Display and Fuel Tank BoosterPack Support

[![Arduino Compile Sketches](https://github.com/Andy4495/MSP430TempSensorWithDisplay/actions/workflows/arduino-compile-sketches.yml/badge.svg)](https://github.com/Andy4495/MSP430TempSensorWithDisplay/actions/workflows/arduino-compile-sketches.yml)
[![Check Markdown Links](https://github.com/Andy4495/MSP430TempSensorWithDisplay/actions/workflows/CheckMarkdownLinks.yml/badge.svg)](https://github.com/Andy4495/MSP430TempSensorWithDisplay/actions/workflows/CheckMarkdownLinks.yml)

Wireless temperature sensor which uses an [MSP-EXP430FR2433][1] LauchPad, [430BOOST-CC110L][2] wireless transceiver, [BOOSTXL-BATTPACK][3] LiPo power source, and [430BOOST-SHARP96][4] low-power display.

In addition to using the SHARP96 LCD to display the temperature and other settings, a [Wireless Sensor Receiver Hub][5] can be used to process and store the data.

## Hardware Modifications

Because of some BoosterPack/LaunchPad pin incompatibilities and to decrease power consumption, it is necessary to make the following hardware changes:

- [FR2433][1] LaunchPad

  1. Remove jumper J10 to disconnect LED1.
  2. Remove jumper J11 to disconnect LED2.
  3. Remove all jumpers from J101 to disconnect the emulation section from the target section of the LaunchPad.
  
  Note that you will need to connect the GND, SBWTDIO, and SBWTDCK jumpers to program the board.
  
  **Do not supply USB power to the LaunchPad** when it is plugged into the FuelTank BoosterPack unless the 5V and 3V3 jumppers are removed.

- [FuelTank][3] BoosterPack

  1. Route the I2C signals to pins 9 and 10
     - Cut the PC traces on the front of the FuelTank that go to pins 14 and 15 (these traces start at two vias directly beneath R5).
     - Gently remove the JP1 female header by inserting a thin flat screwdriver between the header and the PC board. Set the header aside so it can be replaced later.
     - Solder a wire from the right pad of R6 (leaving the resistor in place) to BoosterPack pin 10, as close to the PC board as possible (this is the new SDA signal).
     - Solder a wire from the right pad of R5 (leaving the resistor in place) to BoosterPack pin 9, as close to the PC board as possible (this is the new SCL signal).
     - Use a small diagonal cutter or other tool to clip away excess plastic on the removed header where pins 9 and 10 are located. This is to allow some clearance to the newly soldered wires when replacing the header.
     - Replace the JP1 female header.
  2. Remove resistors R11, R12, R13 from the [FuelTank][3] BoosterPack so that the CHARGE, EN, and POWER_GOOD signals don't interfere with LCD control and SPI signals.
  3. Remove 10KΩ resistors R18 and R20 and install pulldown 10KΩ or 22KΩ resistors in R17 and R19 to significantly reduce current consumption. The configuration of the [TPS6300x][16] buck/boost converters on the FuelTank causes a relatively high current drain in low-power configurations. See [this article][15] for more information.

## Library Modifications

This uses an updated and modified version of the `LCD_SharpBoosterPack_SPI` library, rather than version 1.0.0 included with Energia.

Copy version 1.0.3 from [GitHub][10] and make the following modifications to `LCD_SharpBoosterPack_SPI.cpp`:

- To make the constructor definitions equivalent, update the second constructor definition (lines 106 - 108) by replacing this:

    ```cpp
    digitalWrite(RED_LED, HIGH);
    lcd_vertical_max = model;
    lcd_horizontal_max = model;
    ```

    with this:

    ```cpp
    lcd_vertical_max = model;
    lcd_horizontal_max = model;
    static uint8_t * _frameBuffer;
    _frameBuffer = new uint8_t[_index(lcd_vertical_max, lcd_horizontal_max)];
    DisplayBuffer = (uint8_t *) _frameBuffer;
    ```

## Program details

The sketch collects the following data:

- MSP430
  - Die temperature in degrees Fahrenheit * 10
    - For example, 733 represents 73.3 degrees F
  - Number of times "loop()" has run since the last reboot

- FuelTank BoosterPack readings from the [BQ27510 Fuel Gauge][12]
  - LiPo Battery voltage (millivolts)
    - For example, 2977 represents 2.977 volts
  - StandbyTimeToEmpty value (minutes)
    - This is sent using the `Millis` field in the transmitted data structure

After collecting the sensor data, the data is packaged and transmitted to a
receiver hub which can then further process and store the data over time.

The calibration data programmed into both of my FR2433 chips improves the temperature readings, but still produces readings that are off by several degrees. I have added a `#define` to allow further refinement of the calibrated temperature readings:

```cpp
#define TEMP_CALIBRATION_OFFSET 0
```

This offset is added to the calibrated reading. Since the temperature values are represented as tenth degrees, the offset value also needs to be in tenth degrees. For example, if you want to increase the temperature reading by 2 degrees, then set the `TEMP_CALIBRATION_OFFSET` value to 20.

When using this program, start with `TEMP_CALIBRATION_OFFSET` set to zero, and compare the readings with a known good thermometer. Then update the value as needed for the specific board/chip that you are using (if necessary).

There is a conflict with the LCD library's use of the `OneMsTaskTimer` and `sleep()`, such that `sleep()` does not work properly. This appears to be related to the software VCOM clock generation. The VCOM clock (as described in the LCD [application note][18]) is needed to prevent DC bias buildup in the display which can cause burn-in. However, this also significantly impacts power usage of the overall sketch. Therefore, the sketch disables the automatic screen refresh function by setting the autoVCOM parameter in the constructor to false. To limit burn-in, the sketch reverses the display every time it updates.

With the microcontroller sleeping most of the time, overall power consumption is very low. Per [EnergyTrace][14] measurements, the module pulls a mean current draw of 0.027 mA. A fully charged FuelTank can power this setup for several months.

In order to save program space, this sketch uses [software I2C][7] to get data from the [BQ27510 Fuel Gauge][12] on the Fuel Tank [BoosterPack][3] instead of the [Fuel Tank Library][13]. The BQ27510 has a simple I2C interface which makes it easy to implement directly without the use of a specialized library.

## LaunchPad and BoosterPack Pin usage

```text
     FR2433  Fuel Tank CC110L SHARP96  Comment
   --------- --------- ------ -------  -------
 1     3V3      3V3      3V3           Power -- Note that SHARP96 uses I/O pin for power instead of VCC
 2    LED1              GDO2   LCDPWR  CC110L Energia library does not use GDO2 and configures it as high impedance. Disconnect LED1 jumper (J10) on FR2433 LaunchPad
 3     RXD
 4     TXD
 5                             LCD_EN
 6                             LCD_CS
 7     SCK               SCK      SCK
 8
 9     SCL      SCL                    Software I2C. Fuel Tank hardware modified to move I2C to pins 9/10 instead of 14/15
10     SDA      SDA                    Software I2C. Fuel Tank hardware modified to move I2C to pins 9/10 instead of 14/15
11
12
13
14    MISO              MISO
15    MOSI              MOSI     MOSI
16   RESET
17
18                        CS
19    LED2              GDO0           Disconnect LED2 jumper (J11) on FR2433 LaunchPad
20     GND      GND      GND      GND
```

## External Libraries

- [Calibrated Temperature and Vcc Library][6]
- [Software I2C Library][7]
- [OneMsTaskTimer Library][19]  
  This library is part of the Energia IDE installation. However, it is not part of the board packages, and therefore isn't included when compiling using the Arduino IDE or CLI. So I have also included a copy of the `.cpp` and `.h` files in the `extras/OneMsTaskTimer` folder in this repo.
- [LCD_SharpBoosterPack_SPI][20]  
  This library is part of the Energia IDE installation. However, it is not part of the board packages, and therefore isn't included when compiling using the Arduino IDE or CLI. So I have also included a copy of the `.cpp` and `.h` files in the `extras/LCD_SharpBoosterPack_SPI` folder in this repo.  

## References

- [MSP-EXP430FR2433][1] LaunchPad
- [CC110L][2] BoosterPack
- [BOOSTXL-BATTPACK][3] LiPo Fuel Tank BoosterPack
  - Note that this BoosterPack has been discontinued. This sketch is specific to this BoosterPack and will require modifications to work with the [Fuel Tank II][8] BoosterPack
- [430BOOST-SHARP96][4] Display BoosterPack
  - Note that this BoosterPack has been discontinued. This sketch is specific to this BoosterPack and may require modifications to work with the [SHARP128][9] BoosterPack
- [Wireless Sensor Receiver Hub][5]
- Version 1.0.3 of [LCD_SharpBoosterPack_SPI library][10]
- Sharp Memory LCD [datasheet][17]
- Sharp Memory LCD [application note][18]

## License

The software and other files in this repository (with the exception of the files in the `extras` folder) are released under what is commonly called the [MIT License][100]. See the file [`LICENSE.txt`][101] in this repository.

The files in the `extras` folder are provided for convenience when compiling without the Energia IDE. They have their own licensing terms as noted in the comments in the files themselves.

[1]: http://www.ti.com/tool/MSP-EXP430FR2433
[2]: https://www.ti.com/lit/ml/swru312b/swru312b.pdf
[3]: http://www.ti.com/tool/BOOSTXL-BATTPACK
[4]: http://www.ti.com/tool/430BOOST-SHARP96
[5]: https://github.com/Andy4495/Wireless-Sensor-Receiver-Hub
[6]: https://github.com/Andy4495/mspTandV
[7]: https://github.com/Andy4495/SWI2C
[8]: http://www.ti.com/tool/BOOSTXL-BATPAKMKII
[9]: http://www.ti.com/tool/BOOSTXL-SHARP128
[10]: https://github.com/energia/Energia/tree/master/libraries/LCD_SharpBoosterPack_SPI
[11]: https://www.mouser.com/datasheet/2/365/LS013B4DN04(3V_FPC)-1202885.pdf
[12]: https://www.ti.com/product/BQ27510
[13]: https://forum.43oh.com/topic/4915-energia-library-fuel-tank-boosterpack/
[14]: http://www.ti.com/tool/ENERGYTRACE
[15]: https://embeddedcomputing.weebly.com/fuel-tank-boosterpack.html
[16]: https://www.ti.com/lit/ds/symlink/tps63002.pdf
[17]: https://www.mouser.com/catalog/specsheets/LS013B4DN04(3V_FPC).pdf
[18]: https://www.sharpmemorylcd.com/resources/SharpMemoryLCDTechnologyB.pdf
[19]: https://github.com/energia/Energia/tree/master/libraries/OneMsTaskTimer
[20]: https://github.com/energia/Energia/tree/master/libraries/LCD_SharpBoosterPack_SPI
[100]: https://choosealicense.com/licenses/mit/
[101]: ./LICENSE.txt
[200]: https://github.com/Andy4495/MSP430TempSensorWithDisplay

[//]: # (Old TI product link that is no longer active: http://www.ti.com/tool/430BOOST-CC110L)

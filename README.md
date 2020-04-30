MSP430 Temperature Sensor with Display and Fuel Tank BoosterPack Support
==============================

Wireless temperature sensor which uses an [MSP-EXP430FR2433][1] LauchPad, [430BOOST-CC110L][2] wireless transceiver, [BOOSTXL-BATTPACK][3] LiPo power source, and [430BOOST-SHARP96][4] low-power display.

In addition to displaying the temperature and other settings, a [Wireless Sensor Receiver Hub][5] can be used to process and store the data.

In order to save program space, this sketch uses [software I2C][7] to get data from the [BQ27510 Fuel Gauge][12] on the Fuel Tank [BoosterPack][3] instead of the [Fuel Tank Library][13]. The BQ27510 has a simple I2C interface which makes it easy to implement directly without the use of a specialized library.

## Hardware Modifications ##

Because of some BoosterPack/LaunchPad pin incompatibilities, it is necessary to do the following with this design:
1. Modify the [FuelTank][3] BoosterPack hardware and move SCL from pin 14 to pin 9.
2. Modify the [FuelTank][3] BoosterPack hardware and move SDA from pin 15 to pin 10.
3. Remove jumper JP10 from the [FR2433][1] LaunchPad to disconnect LED1.
4. Remove jumper JP11 from the [FR2433][1] LaunchPad to disconnect LED2.
5. Remove resistors R11, R12, R13 from the [FuelTank][3] BoosterPack so that the CHARGE, EN, and POWER_GOOD signals don't interfere with LCD control and SPI signals.

To decrease overall power consumption, disconnect the emulation section of the LaunchPad -- remove all jumpers from J101.

## Library Modifications ##

This uses an updated and modified version of the `LCD_SharpBoosterPack_SPI` library, rather than version 1.0.0 included with Energia.

Copy version 1.0.3 from [GitHub][10] and make the following modifications to `LCD_SharpBoosterPack_SPI.cpp`:
+ To make the constructor definitions equivalent, update the second constructor definition (lines 106 - 108) and replace this:
```    
digitalWrite(RED_LED, HIGH);
lcd_vertical_max = model;
lcd_horizontal_max = model;
```
with this:
```
lcd_vertical_max = model;
lcd_horizontal_max = model;
static uint8_t * _frameBuffer;
_frameBuffer = new uint8_t[_index(lcd_vertical_max, lcd_horizontal_max)];
DisplayBuffer = (uint8_t *) _frameBuffer;
```

## Program details ##

The sketch collects the following data:

- MSP430
     - Die temperature in degrees Fahrenheit * 10
         - For example, 733 represents 73.3 degrees F
     - LiPo Battery voltage (Vcc) in millivolts
         - For example, 2977 represents 2.977 volts
         - Battery voltage is read from the BQ27510 Fuel Gauge on the [FuelTank][3] BoosterPack.
     - Number of times "loop()" has run since the last reboot
     - Value of the TimeToEmpty register reported by the BQ27510 Fuel Gauge
         - This is sent using the "Millies" field in the transmitted data structure

After collecting the sensor data, the data is packaged and transmitted to a
receiver hub which can then further process and store the data over time.

The calibration data programmed into both of my FR2433 chips improves the temperature readings, but still produces readings that are off by several degrees. I have added a #define to allow further refinement of the calibrated temperature readings:

    #define TEMP_CALIBRATION_OFFSET 0

This offset is added to the calibrated reading. Since the temperature values are represented as tenth degrees, the offset value also needs to be in tenth degrees. For example, if you want to increase the temperature reading by 2 degrees, then set the `TEMP_CALIBRATION_OFFSET` value to 20.

When using this program, start with `TEMP_CALIBRATION_OFFSET` set to zero, and compare the readings with a known good thermometer. Then update the value as needed for the specific board/chip that you are using (if necessary).

There is a conflict with the library's use of the OneMsTaskTimer and sleep(), such that sleep() does not work properly. Although the LCD [datasheet][11] discusses the procedure to refresh the display at a minimum of once per second, it turns out that the refresh is not necessary. Therefore, the sketch disables the automatic screen refresh function (by setting the autoVCOM parameter in the constructor to false). This allows the microcontroller to sleep most of the time, drastically reducing power requirements. Per EnergyTrace measurements, the module pulls a mean current draw of 0.027 mA.

## LaunchPad and BoosterPack Pin usage
```
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

## External Libraries ##

* [Calibrated Temperature and Vcc Library][6]
* [Software I2C Library][7]

## References ##

* [MSP-EXP430FR2433][1] LaunchPad
* [CC110L][3] BoosterPack
* [BOOSTXL-BATTPACK][3] LiPo Fuel Tank BoosterPack
    * Note that this BoosterPack has been discontinued. This sketch is specific to this BoosterPack and will require modifications to work with the [Fuel Tank II][8] BoosterPack
* [430BOOST-SHARP96][4] Display BoosterPack
    * Note that this BoosterPack has been discontinued. This sketch is specific to this BoosterPack and may require modifications to work with the [SHARP128][9] BoosterPack
* [Wireless Sensor Receiver Hub][5]
* Version 1.0.3 of [LCD_SharpBoosterPack_SPI library][10]


[1]: http://www.ti.com/tool/MSP-EXP430FR2433
[2]: http://www.ti.com/tool/430BOOST-CC110L
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

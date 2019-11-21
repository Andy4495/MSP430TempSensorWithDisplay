MSP430 Temperature Sensor with Display and Fuel Tank BoosterPack Support
==============================

Wireless temperature sensor which uses an [MSP-EXP430FR2433][1] LauchPad, [430BOOST-CC110L][2] wireless transceiver, [BOOSTXL-BATTPACK][3] LiPo power source, and [430BOOST-SHARP96][4] low-power display.

Since it is a wireless sensor, it depends on a receiver hub to process and
store the data. See [Wireless Sensor Receiver Hub][5]
for an implementation of a receiver hub.

## Hardware Modifications ##

Because of some BoosterPack/LaunchPad pin incompatibilities, it is necessary to do the following with this design:
1. Modify the [FuelTank][3] BoosterPack hardware and move SCL from pin 14 to pin 9.
2. Modify the [FuelTank][3] BoosterPack hardware and move SDA from pin 15 to pin 10.
3. Remove jumper JP10 from the [FR2433][1] LaunchPad to disconnect LED1.
4. Remove jumper JP11 from the [FR2433][1] LaunchPad to disconnect LED2.

To decrease overall power consumption, disconnect the emulation section of the LaunchPad -- remove all jumpers from J101.

## Program details ##

The sketch collects the following data:

- MSP430
     - Die temperature in degrees Fahrenheit * 10
         - For example, 733 represents 73.3 degrees F
     - LiPo Battery voltage (Vcc) in millivolts
         - For example, 2977 represents 2.977 volts
         - Battery voltage is read from the BQ27510 Fuel Gauge on the [FuelTank][3] BoosterPack.
     - Number of times "loop()" has run since the last reboot
     - Current "millis()" value at the time of transmission

After collecting the sensor data, the data is packaged and transmitted to a
receiver hub which can then further process and store the data over time.

## LaunchPad and BoosterPack Pin usage
```
Pin    FR2433  Fuel Tank CC110L SHARP96  Comment
---  --------- --------- ------ -------  -------
 1      3V3       3V3     3V3            Power -- Note that SHARP96 uses I/O pin for power instead of VCC
 2     LED1              GDO2   LCDPWR   CC110L Energia library does not use GDO2 and configures it as high impedance. Disconnect LED1 jumper (J10) on FR2433 LaunchPad
 3      RXD
 4      TXD
 5                              LCD_EN
 6
 7      SCK                     SCK
 8
 9      SCL       SCL                    Software I2C. Fuel Tank hardware modified to move I2C to pins 9/10 instead of 14/15
10      SDA       SDA                    Software I2C. Fuel Tank hardware modified to move I2C to pins 9/10 instead of 14/15
11
12
13
14     MISO
15     MOSI                      MOSI
16    RESET
17
18
19     LED2               CS             Disconnect LED2 jumper (J11) on FR2433 LaunchPad
20      GND       GND    GND      GND
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


[1]: http://www.ti.com/tool/MSP-EXP430FR2433
[2]: http://www.ti.com/tool/430BOOST-CC110L
[3]: http://www.ti.com/tool/BOOSTXL-BATTPACK
[4]: http://www.ti.com/tool/430BOOST-SHARP96
[5]: https://github.com/Andy4495/Wireless-Sensor-Receiver-Hub
[6]: https://github.com/Andy4495/mspTandV
[7]: https://github.com/Andy4495/SWI2C
[8]: http://www.ti.com/tool/BOOSTXL-BATPAKMKII
[9]: http://www.ti.com/tool/BOOSTXL-SHARP128

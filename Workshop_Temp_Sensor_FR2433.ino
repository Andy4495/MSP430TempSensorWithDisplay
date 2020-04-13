/* -----------------------------------------------------------------
    MSP430 Remote Temp Sensor with LCD Display
    https://github.com/Andy4495/MSP430TempSensorWithDisplay

    01/17/2018 - A.T. - Initial version
    02/01/18 - A.T. - Update message structure to align on word boundary.
    03/17/18 - A.T. - Updated to use SWI2C instead of Wire
    11/04/19 - A.T. - Modified to use FR2433 LaunchPad and Sharp 96 Display
                    - Assumes Fuel Tank I BoosterPack, modified to use
                      pins 9/10 for I2C.
    11/21/19 - A.T. - Add comment documenting pin usage, GitHub URL.
    11/21/19 - A.T. - Send "Tank Remaining Minutes" in millis field and display.
    11/23/19 - A.T. - Change sleep() to delay()
                      -- sleep() does not work correctly on FR2433 board
                      -- per EnergyTrace, does not reduce power
                      -- seems to get stuck in a ~16 second delay, regardless of value
                      -- There may be a conflict with the 1 ms timer used to refresh the SHARP96
    11/26/19 - A.T. - Use updated SHARP96 library (version 1.7 from Energia GitHub, slightly modified)
                      Call .end() method before sleeping. This seems to work around the conflict with the
                      ms timer used for refresh from interfering with sleep timing.
                      - Drastically cuts down on power usage without impacting display readability.
                      Added a couple #ifdefs so that sketch works with or without FuelTank
                      Cleaned up some comments
    04/13/20 - A.T. - Add a TEMP_CALIBRATION_OFFSET #define to adjust the calibrated temperature.
                      Both of my FR2433 boards have factory-programmed temperature calibration 
                      values that cause the calibrated temp readings to be off by several degrees 
                      (uncalibrated readings are even farther off). 
                      

*/
/* -----------------------------------------------------------------
   Code structure:
   Requires CC110L BoosterPack

   setup()
     I/O and sensor setup

   loop()
   Collect and send the following data:
   - MSP430: Internal Temperature                F * 10
   - MSP430: Supply voltage (Vcc)                mV
   - Internal Timing:
                  # of times loop() has run
                  Current value of millis()
*/

/*
    External libraries:
    - SWI2C Library by A.T.
*/

/* Pin Definitions -- Stacking Fuel Tank I, CC110L, and SHARP96
          FR2433  Fuel Tank CC110L SHARP96  Comment
        --------- --------- ------ -------  -------
     1      3V3       3V3     3V3           Power -- Note that SHARP96 uses I/O pin for power instead of VCC
     2     LED1              GDO2   LCDPWR  CC110L Energia library does not use GDO2 and configures it as high impedance. Disconnect LED1 jumper (J10) on FR2433 LaunchPad
     3      RXD
     4      TXD
     5                              LCD_EN
     6                              LCD_CS
     7      SCK                     SCK
     8
     9      SCL       SCL                   Software I2C. Fuel Tank hardware modified to move I2C to pins 9/10 instead of 14/15
    10      SDA       SDA                   Software I2C. Fuel Tank hardware modified to move I2C to pins 9/10 instead of 14/15
    11
    12
    13
    14     MISO
    15     MOSI                      MOSI
    16    RESET
    17
    18
    19     LED2               CS            Disconnect LED2 jumper (J11) on FR2433 LaunchPad
    20      GND       GND    GND      GND
*/

// If using the Fuel Tank BoosterPack (Version 1, not Version 2),
// then uncomment the following line which will then send
// voltage levels from the Fuel Tank and not directly from Vcc.
// Fuel tank shuts down at ~3.65 LiPo voltage
#define FUEL_TANK_ENABLED

// Use this value to adjust the temperature reading as needed. 
// Start with zero and check the displayed temperature against a known good thermometer
// This value will be added to the calibrated temperature.
// Note that it is in tenth degrees, so to increase the calibrated temp reading by 2
// degrees, then set the offset to 20. 
#define TEMP_CALIBRATION_OFFSET 90

#ifdef FUEL_TANK_ENABLED
#define SWI2C_ENABLED
#include "SWI2C.h"
#endif

// Comment out following line if testing without CC110L BoosterPack
#define RADIO_ENABLED

#include <SPI.h>
#include <AIR430BoostFCC.h>
#include "MspTandV.h"

#include "OneMsTaskTimer.h"
#include "LCD_SharpBoosterPack_SPI.h"
static const uint8_t SHARP_CS   = 6;
static const uint8_t SHARP_DISP = 5;
static const uint8_t SHARP_VCC  = 2;
//LCD_SharpBoosterPack_SPI(uint8_t pinChipSelect, uint8_t pinDISP, uint8_t pinVCC, bool autoVCOM, uint8_t model)
LCD_SharpBoosterPack_SPI myScreen(SHARP_CS, SHARP_DISP, SHARP_VCC, false, SHARP_96);

int orientation = 1;

// Fuel Tank Register Addresses
#define BQ27510_Control                        0x00
#define BQ27510_Temperature                    0x06
#define BQ27510_Voltage                        0x08
#define BQ27510_Flags                          0x0a
#define BQ27510_NominalAvailableCapacity       0x0c
#define BQ27510_FullAvailableCapacity          0x0e
#define BQ27510_RemainingCapacity              0x10
#define BQ27510_FullChargeCapacity             0x12
#define BQ27510_AverageCurrent                 0x14
#define BQ27510_TimeToEmpty                    0x16
#define BQ27510_G3_StandbyCurrent              0x18
#define BQ27510_G3_StandbyTimeToEmpty          0x1a
#define BQ27510_G3_StateOfHealth               0x1c
#define BQ27510_G3_CycleCount                  0x1e
#define BQ27510_G3_StateOfCharge               0x20
#define BQ27510_G3_InstantaneousCurrent        0x22
#define BQ27510_G3_InternalTemperature         0x28
#define BQ27510_G3_ResistanceScale             0x2a
#define BQ27510_G3_OperationConfiguration      0x2c
#define BQ27510_G3_DesignCapacity              0x2e

// CC110L Declarations
#define ADDRESS_REMOTE  0x01  // Receiver hub address
#define     TxRadioID     4   // Transmitter address

enum {WEATHER_STRUCT, TEMP_STRUCT};

struct SensorData {
  int             MSP_T;     // Tenth degrees F
  unsigned int    Batt_mV;   // milliVolts
  unsigned int    Loops;
  unsigned long   Millis;
};

struct sPacket
{
  uint8_t from;           // Local node address that message originated from
  uint8_t struct_type;    // Flag to indicate type of message structure
  union {
    uint8_t message[20];    // Local node message [MAX. 58 bytes]
    SensorData sensordata;
  };
};

struct sPacket txPacket;

MspTemp myTemp;
#ifndef FUEL_TANK_ENABLED
MspVcc myVcc;
#endif

#ifdef SWI2C_ENABLED
#define SDA_PIN 10
#define SCL_PIN 9
#define FUEL_TANK_ADDR  0x55
#endif

#ifdef FUEL_TANK_ENABLED
SWI2C myFuelTank = SWI2C(SDA_PIN, SCL_PIN, FUEL_TANK_ADDR);
uint16_t  data16;
uint8_t   data8;
#endif

#define MAXTEXTSIZE 17
char text[MAXTEXTSIZE];      // buffer to store text to print

unsigned int loopCount = 0;
const unsigned long sleepTime = 240;  // In seconds. FR2433 seems to be off by factor of 4 (so 240 => 60 seconds)

void setup() {

#ifdef FUEL_TANK_ENABLED
  myFuelTank.begin();
#endif

  myScreen.begin();
  myScreen.clearBuffer();
  myScreen.setOrientation(orientation);

  // CC110L Setup
  txPacket.from = TxRadioID;
  txPacket.struct_type = TEMP_STRUCT;
  memset(txPacket.message, 0, sizeof(txPacket.message));
#ifdef RADIO_ENABLED
  Radio.begin(TxRadioID, CHANNEL_1, POWER_MAX);
#endif
}


void loop() {

  loopCount++;

  myTemp.read(CAL_ONLY);

  txPacket.sensordata.MSP_T = myTemp.getTempCalibratedF() + TEMP_CALIBRATION_OFFSET;
#ifdef FUEL_TANK_ENABLED
  myFuelTank.read2bFromRegister(BQ27510_Voltage, &data16);
  txPacket.sensordata.Batt_mV = data16;
  myFuelTank.read2bFromRegister(BQ27510_TimeToEmpty, &data16);
  txPacket.sensordata.Millis = data16;
#else
  txPacket.sensordata.Batt_mV = myVcc.getVccCalibrated();
  txPacket.sensordata.Millis = millis();
#endif

  txPacket.sensordata.Loops = loopCount;

  // Send the data over-the-air
#ifdef RADIO_ENABLED
  Radio.transmit(ADDRESS_REMOTE, (unsigned char*)&txPacket, sizeof(txPacket.sensordata) + 4);
#endif

  myScreen.clear();
  myScreen.setFont(0);
  snprintf(text, MAXTEXTSIZE, "Sensor:   %d", TxRadioID);
  myScreen.text(1, 1, text);
  snprintf(text, MAXTEXTSIZE, "Channel:  %d", 1);
  myScreen.text(1, 10, text);
  snprintf(text, MAXTEXTSIZE, "Vcc (mV): %d", txPacket.sensordata.Batt_mV);
  myScreen.text(1, 19, text);
  snprintf(text, MAXTEXTSIZE, "Min Rmn:  %lu", txPacket.sensordata.Millis);
  myScreen.text(1, 28, text);
  myScreen.setFont(1);
  snprintf(text, MAXTEXTSIZE, "Temp");
  myScreen.text(24, 44, text);
  snprintf(text, MAXTEXTSIZE, "%d", (txPacket.sensordata.MSP_T + 5) / 10);
  myScreen.text(36, 64, text);
  myScreen.setFont(0);
  snprintf(text, MAXTEXTSIZE, "%5u", loopCount);
  myScreen.text(64, 85, text);
  myScreen.flush();

  sleepSeconds(sleepTime);

}

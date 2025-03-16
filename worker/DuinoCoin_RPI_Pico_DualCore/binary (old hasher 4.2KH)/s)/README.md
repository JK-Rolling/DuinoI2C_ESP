# DuinoCoin_RPI_Pico_DualCore

## Info
Press Pico `BOOTSEL` button while inserting USB will bring up the Pico as flash drive. Copy and paste one .uf2 file per Pico.

* 0x1-0x2 means the Pico will be assigned I2C addresses of 0x1 and 0x2
* Pin info: I2C0_SDA = GP20, I2C0_SCL = GP21, I2C1_SDA = GP26, I2C1_SCL = GP27
* WIRE_CLOCK = 400KHz
* CPU Frequency = 100MHz
* board library: 3.6.3
* WDT: Enabled
* CRC8: Enabled
* LED: Builtin LED
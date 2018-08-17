/*
 * bme280Interfacing.h
 *
 *  Created on: Mar 8, 2017
 *      Author: i_anu
 */

#ifndef BME280INTERFACING_H_
#define BME280INTERFACING_H_

#include "bme280.h"
#include <stdint.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>

#define BMP280_REG_START__ADDR     (0xF2)
#define BMP280_REG_DATA__LEN       (13)
#define BME280_DATA_INDEX   1
#define BME280_ADDRESS_INDEX    2

volatile uint32_t intervalCnt;
volatile uint32_t com_rslt;
volatile uint32_t MCLKfreq, SMCLKfreq;
volatile uint8_t ms_timeout;
volatile uint32_t uncompTemp;
volatile uint8_t regVal;
volatile uint8_t regData[13];

volatile struct bme280_t bme280;
volatile struct bme280_t *p_bme280; /**< pointer to BME280 */

/************** I2C/SPI buffer length ******/
#define I2C_BUFFER_LEN 8

#define ADC_VREF 2.5
#define ADC_MAX_QUANT 16384.0
#define LDR_PULL_DOWN_OHM   240

int LightCode;
char LightCondition [15];
int pressureCompensation;

float temperature_bme280;
float pressure_bme280;
float humidity_bme280;

typedef struct Sensor{\
    char temperature[7];
    char pressure[7];
    char humidity[7];
    char light[1];
}SENSOR;


/*-------------------------------------------------------------------*
*   This is a sample code for read and write the data by using I2C/SPI
*   Use either I2C or SPI based on your need
*   The device address defined in the bme280.h file
*-----------------------------------------------------------------------*/
 /* \Brief: The function is used as I2C bus write
 *  \Return : Status of the I2C write
 *  \param dev_addr : The device address of the sensor
 *  \param reg_addr : Address of the first register, will data is going to be written
 *  \param reg_data : It is a value hold in the array,
 *      will be used for write the value into the register
 *  \param cnt : The no of byte of data to be write
 */

u8 v_stand_by_time_u8;
    /* The variable used to read uncompensated temperature*/
    s32 v_data_uncomp_temp_s32;
    /* The variable used to read uncompensated pressure*/
    s32 v_data_uncomp_pres_s32;
    /* The variable used to read uncompensated pressure*/
    s32 v_data_uncomp_hum_s32;
    /* The variable used to read compensated temperature*/
    s32 v_comp_temp_s32[2];
    /* The variable used to read compensated pressure*/
    u32 v_comp_press_u32[2];
    /* The variable used to read compensated humidity*/
    u32 v_comp_humidity_u32[2];




    /* Statics */
    static volatile uint16_t curADCResult;
    static volatile float LDR_Value;
    static  float resistance;

//    float LDR_resistance;





s8 BME280_I2C_bus_write(u8 dev_addr, u8 reg_addr, u8 *reg_data, u8 cnt);

s8 BME280_I2C_bus_read(u8 dev_addr, u8 reg_addr, u8 *reg_data, u8 cnt);
void BME280_delay_msek(u32 msek);
s8 I2C_routine(void);

void bme280Init();

void getSensorValues(char *temperature,char *pressure,char *humidity,char *light, float *humidity_alert);

void adcInit();
int getLightSensorValues();

#endif /* BME280INTERFACING_H_ */

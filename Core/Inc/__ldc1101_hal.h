#pragma once
#include <stdint.h>

#define LDC1101_SENSOR_COUNT 32
typedef const uint8_t* T_HAL_P;



void hal_spiMap(T_HAL_P spiObj);
void hal_gpioMap(T_HAL_P gpioObj);
void hal_gpio_csSet(uint8_t value);
void hal_spiWrite(uint8_t *buffer, uint16_t len);
void hal_spiRead(uint8_t *buffer, uint16_t len);
uint8_t hal_gpio_intGet(void);
void Delay_100ms(void);
void hal_gpio_csRelease(uint8_t sensor_idx);
void hal_gpio_csSelect(uint8_t sensor_idx);


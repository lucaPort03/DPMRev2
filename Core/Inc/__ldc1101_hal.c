
#include "main.h"
#include "__ldc1101_driver.h"
#include <stdint.h>

/* MikroE originally defines T_HAL_P as "const uint8_t*".
 * We just need a compatible type for hal_spiMap/hal_gpioMap parameters
 */

#ifndef T_HAL_P
#define T_HAL_P const uint8_t*  // Checks if T_HAL_P is defined. if not defined
#endif							// it defines it and continues to the rest of the page


extern SPI_HandleTypeDef hspi1;

static void SPI_SetPrescaler(uint32_t prescaler)
{
    // Wait until SPI is not busy
    while(hspi1.State == HAL_SPI_STATE_BUSY);

    // Disable SPI
    __HAL_SPI_DISABLE(&hspi1);


    hspi1.Instance->CFG1 &= ~SPI_CFG1_MBR_Msk;

    // Set new prescaler
    hspi1.Instance->CFG1 |= prescaler;

    // Re-enable SPI
    __HAL_SPI_ENABLE(&hspi1);
}

void hal_spiMap (T_HAL_P spiObj)
{
	(void)spiObj;
}

void hal_gpioMap (T_HAL_P gpioObj)
{
	(void)gpioObj;
}

/*
 * Chip Select (CS) control for LDC1101
 * value = 0 -> CS low (active)
 * value = 1 -> CS high (inactive)
 */

void hal_gpio_csSet(uint8_t value)
{
	if (value)
	{
		HAL_GPIO_WritePin(LDC_CS_GPIO_Port, LDC_CS_Pin, GPIO_PIN_SET);

	}
	else
	{
		HAL_GPIO_WritePin(LDC_CS_GPIO_Port, LDC_CS_Pin, GPIO_PIN_RESET);
	}
}

/*
 * SPI blocking write: send 'len' bytes from 'buffer'.
 * CS is handled in ldc1101_writeByte(), so don't touch CS here.
 */

void hal_spiWrite(uint8_t *buffer, uint16_t len)
{
	SPI_SetPrescaler(SPI_BAUDRATEPRESCALER_32);
	HAL_SPI_Transmit(&hspi1, buffer, len, HAL_MAX_DELAY);
}

/*
 * SPI blocking read: receive 'len' bytes into 'buffer'.
 */
void hal_spiRead(uint8_t *buffer, uint16_t len)
{
	SPI_SetPrescaler(SPI_BAUDRATEPRESCALER_32);
	HAL_SPI_Receive(&hspi1, buffer, len, HAL_MAX_DELAY);
}

void hal_spiTransfer(uint8_t *tx, uint8_t *rx, uint16_t len)
{
	SPI_SetPrescaler(SPI_BAUDRATEPRESCALER_32);
	HAL_SPI_TransmitReceive(&hspi1,tx,rx,len,HAL_MAX_DELAY);

}

/*
 * Read INTB pin (used by ldc1101_getInterrupt()).
 */
/*uint8_t hal_gpio_intGet(void)
{
	GPIO_PinState state = HAL_GPIO_ReadPin(LDC1101_INTB_GPIO_Port,LDC1101_INTB_Pin);
	return (state ==GPIO_PIN_SET)?1u:0u;
}

/*
 * 100 ms delay used inside ldc1101_init()
 */

void Delay_100ms(void)
{
	HAL_Delay(100);

}

/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file    app_netxduo.h
  * @author  MCD Application Team
  * @brief   DPM netxduo header flie
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2020-2021 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */
/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __APP_NETXDUO_H__
#define __APP_NETXDUO_H__

#ifdef __cplusplus
extern "C" {
#endif

/* USER CODE BEGIN 1 */

/* USER CODE END 1 */

/* Includes ------------------------------------------------------------------*/
#include "nx_api.h"

/* Private includes ----------------------------------------------------------*/
#include "nx_stm32_eth_driver.h"

/* USER CODE BEGIN Includes */
#include "main.h"

/* USER CODE END Includes */

/* Exported types ------------------------------------------------------------*/
/* USER CODE BEGIN ET */

/* USER CODE END ET */

/* Exported constants --------------------------------------------------------*/
/* USER CODE BEGIN EC */

/* USER CODE END EC */
/* The DEFAULT_PAYLOAD_SIZE should match with RxBuffLen configured via MX_ETH_Init */
#ifndef DEFAULT_PAYLOAD_SIZE
#define DEFAULT_PAYLOAD_SIZE      1536
#endif

#ifndef DEFAULT_ARP_CACHE_SIZE
#define DEFAULT_ARP_CACHE_SIZE    1024
#endif

/* Exported macro ------------------------------------------------------------*/
/* USER CODE BEGIN EM */
#define PRINT_IP_ADDRESS(addr)             do { \
                                                printf("STM32 %s: %lu.%lu.%lu.%lu \n", #addr, \
                                                (addr >> 24) & 0xff, \
                                                (addr >> 16) & 0xff, \
                                                (addr >> 8) & 0xff, \
                                                addr& 0xff);\
                                           }while(0)

#define PRINT_DATA(addr, port, data)       do { \
                                                printf("[%lu.%lu.%lu.%lu:%u] -> '%s' \n", \
                                                (addr >> 24) & 0xff, \
                                                (addr >> 16) & 0xff, \
                                                (addr >> 8) & 0xff,  \
                                                (addr & 0xff), port, data); \
                                           } while(0)

/* USER CODE END EM */

/* Exported functions prototypes ---------------------------------------------*/
UINT MX_NetXDuo_Init(VOID *memory_ptr);

/* USER CODE BEGIN EFP */

/* USER CODE END EFP */

/* Private defines -----------------------------------------------------------*/
/* USER CODE BEGIN PD */


#define WINDOW_SIZE              512

#define LINK_PRIORITY            11

#define NULL_ADDRESS             0

#define DEFAULT_PORT             5005
#define TCP_SERVER_PORT          DEFAULT_PORT
#define PC_IP_ADDRESS       	 IP_ADDRESS(192, 168, 0, 69)
#define PC_PORT 				 5005


#define STM_IP_ADDRESS 			IP_ADDRESS(192, 168, 0, 123)
#define UDP_SERVER_ADDRESS 		IP_ADDRESS(192, 168, 0, 123)

#define STM_NETMASK 			IP_ADDRESS(255,255,255,0)
	#define STM_GATEWAY 			IP_ADDRESS(192,168,0,1)

#define MAX_PACKET_COUNT         100
#define DEFAULT_MESSAGE          "HELLO FROM STM!"
#define DEFAULT_TIMEOUT          10 * NX_IP_PERIODIC_RATE

#define NX_APP_CABLE_CONNECTION_CHECK_PERIOD  (1 * NX_IP_PERIODIC_RATE)
/* USER CODE END PD */

#define NX_APP_DEFAULT_TIMEOUT               (10 * NX_IP_PERIODIC_RATE)

#define NX_APP_PACKET_POOL_SIZE              ((DEFAULT_PAYLOAD_SIZE + sizeof(NX_PACKET)) * 30)

#define NX_APP_THREAD_STACK_SIZE             4 * 1024

#define Nx_IP_INSTANCE_THREAD_SIZE           4 * 1024

#define NX_APP_THREAD_PRIORITY               10

#define NX_APP_INSTANCE_PRIORITY             4

#define UDP_SERVER_PRIORITY					 6

#define NX_APP_DEFAULT_IP_ADDRESS                   0

#define NX_APP_DEFAULT_NET_MASK                     0


//-------------------------- Flap defines -----------------------------------//

#define VELOCITY 2.0
#define PACKET_RATE 0.7

// The minimum number of packets needed to synchronise all sensor outputs
#define DELAY_COUNT 800 //COIL_18_OFFSET / (VELOCITY * PACKET_RATE),

#define COIL_18_OFFSET 390.857
#define COIL_17_OFFSET 390.857
#define COIL_20_OFFSET 371.65
#define COIL_19_OFFSET 371.65
#define COIL_22_OFFSET 352.443
#define COIL_21_OFFSET 352.443
#define COIL_24_OFFSET 333.236
#define COIL_23_OFFSET 333.236
#define COIL_26_OFFSET 314.029
#define COIL_25_OFFSET 314.029
#define COIL_28_OFFSET 294.822
#define COIL_27_OFFSET 294.822
#define COIL_30_OFFSET 275.615
#define COIL_29_OFFSET 275.615
#define COIL_32_OFFSET 256.408
#define COIL_31_OFFSET 256.408
#define COIL_2_OFFSET 134.449
#define COIL_1_OFFSET 134.449
#define COIL_4_OFFSET 115.242
#define COIL_3_OFFSET 115.242
#define COIL_6_OFFSET 96.035
#define COIL_5_OFFSET 96.035
#define COIL_8_OFFSET 76.828
#define COIL_7_OFFSET 76.828
#define COIL_10_OFFSET 57.621
#define COIL_9_OFFSET 57.621
#define COIL_12_OFFSET 38.414
#define COIL_11_OFFSET 38.414
#define COIL_14_OFFSET 19.207
#define COIL_13_OFFSET 19.207
#define COIL_16_OFFSET 0
#define COIL_15_OFFSET 0

/* USER CODE BEGIN 2 */
#pragma pack(push, 1)
typedef struct {
    uint8_t  magic[2];
    uint16_t seq;
    uint32_t timestamp_ms;
    uint16_t ldc[32];
    uint16_t rp[32];
    uint32_t freq_hz[32];
    int32_t  deviation[32];
    int32_t  threshold[32];
    int32_t  baseline[32];
    int32_t  raw_mean[32];
    int32_t  filt_std[32];
    uint8_t  is_metal[32];
    uint8_t  calib_done[32];
} AMCISS_Packet_t;
/* sizeof = 968 bytes */
#pragma pack(pop)


#pragma pack(push, 1)
typedef struct {
    uint8_t  magic[2];
    uint16_t seq;
    uint32_t timestamp_ms;
    uint16_t ldc[64];
    uint16_t rp[64];
    uint8_t  is_metal[64];
} DPM_Forward_Packet_t;
/* sizeof = 8 + 128 + 128 + 64 = 332 bytes */
#pragma pack(pop)
/* USER CODE END PTD */
/* USER CODE END 2 */

#ifdef __cplusplus
}
#endif
#endif /* __APP_NETXDUO_H__ */

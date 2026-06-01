/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file    app_netxduo.c
  * @author  MCD Application Team
  * @brief   DPM NetxDuo File
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

/* USER CODE BEGIN 1 */

/* USER CODE END 1 */

/* Includes ------------------------------------------------------------------*/
#include "app_netxduo.h"

/* Private includes ----------------------------------------------------------*/
#include "nxd_dhcp_client.h"
/* USER CODE BEGIN Includes */
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
TX_THREAD      NxAppThread;
NX_PACKET_POOL NxAppPool;
NX_IP          NetXDuoEthIpInstance;
TX_SEMAPHORE   DHCPSemaphore;
NX_DHCP        DHCPClient;
/* USER CODE BEGIN PV */

TX_THREAD AppTCPThread;
TX_THREAD AppUDPThread;
TX_THREAD AppLinkThread;



ULONG IpAddress;
ULONG NetMask;

NX_TCP_SOCKET TCPSocket;
NX_UDP_SOCKET UDPSocket;

static uint32_t packets_received = 0;
static int coil_packet_delay[32];
static uint8_t current_flap_state = 0;
static uint8_t previous_flap_state = 0;
static uint8_t LDC_array_1[32][DELAY_COUNT] = {0};
static uint8_t LDC_array_2[32][DELAY_COUNT] = {0};

volatile int head = DELAY_COUNT -1;
static volatile uint32_t last_packet_tick = 0;

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
static VOID nx_app_thread_entry (ULONG thread_input);
static VOID ip_address_change_notify_callback(NX_IP *ip_instance, VOID *ptr);
/* USER CODE BEGIN PFP */
static VOID App_Link_Thread_Entry(ULONG thread_input);
static VOID App_UDP_Server_Thread_Entry (ULONG thread_input);
static void process_packet(DPM_Forward_Packet_t *fwd_pkt,
                            AMCISS_Packet_t *incoming,
                            ULONG sender_ip,
                            uint8_t *dcm_received);


void calculate_LDC_packet_delays(int coil_packet_delay[]);


/* USER CODE END PFP */

/**
  * @brief  Application NetXDuo Initialization.
  * @param memory_ptr: memory pointer
  * @retval int
  */
UINT MX_NetXDuo_Init(VOID *memory_ptr)
{
  UINT ret = NX_SUCCESS;
  TX_BYTE_POOL *byte_pool = (TX_BYTE_POOL*)memory_ptr;
  CHAR *pointer;

  /* USER CODE BEGIN MX_NetXDuo_MEM_POOL */

  /* USER CODE END MX_NetXDuo_MEM_POOL */

  /* USER CODE BEGIN 0 */
  printf("Nx_UDP_Echo_Client application started..\n");
  /* USER CODE END 0 */

  /* Initialize the NetXDuo system. */
  nx_system_initialize();

    /* Allocate the memory for packet_pool.  */
  if (tx_byte_allocate(byte_pool, (VOID **) &pointer, NX_APP_PACKET_POOL_SIZE, TX_NO_WAIT) != TX_SUCCESS)
  {
	printf("packet pool malloc fail\n");
    return TX_POOL_ERROR;
  }

  /* Create the Packet pool to be used for packet allocation,
   * If extra NX_PACKET are to be used the NX_APP_PACKET_POOL_SIZE should be increased
   */
  ret = nx_packet_pool_create(&NxAppPool, "NetXDuo App Pool", DEFAULT_PAYLOAD_SIZE, pointer, NX_APP_PACKET_POOL_SIZE);

  if (ret != NX_SUCCESS)
  {
	printf("packet pool create fail\n");
    return NX_POOL_ERROR;
  }

    /* Allocate the memory for Ip_Instance */
  if (tx_byte_allocate(byte_pool, (VOID **) &pointer, Nx_IP_INSTANCE_THREAD_SIZE, TX_NO_WAIT) != TX_SUCCESS)
  {
	printf("ip malloc fail\n");
    return TX_POOL_ERROR;
  }

   /* Create the main NX_IP instance */
  ret = nx_ip_create(&NetXDuoEthIpInstance, "NetX Ip instance", NX_APP_DEFAULT_IP_ADDRESS, NX_APP_DEFAULT_NET_MASK, &NxAppPool, nx_stm32_eth_driver,
                     pointer, Nx_IP_INSTANCE_THREAD_SIZE, NX_APP_INSTANCE_PRIORITY);

  if (ret != NX_SUCCESS)
  {
	printf("ip instance fail\n");
    return NX_NOT_SUCCESSFUL;
  }

    /* Allocate the memory for ARP */
  if (tx_byte_allocate(byte_pool, (VOID **) &pointer, DEFAULT_ARP_CACHE_SIZE, TX_NO_WAIT) != TX_SUCCESS)
  {
	 printf("arp malloc fail");
    return TX_POOL_ERROR;
  }

  /* Enable the ARP protocol and provide the ARP cache size for the IP instance */

  /* USER CODE BEGIN ARP_Protocol_Initialization */

  /* USER CODE END ARP_Protocol_Initialization */

  ret = nx_arp_enable(&NetXDuoEthIpInstance, (VOID *)pointer, DEFAULT_ARP_CACHE_SIZE);

  if (ret != NX_SUCCESS)
  {
	printf("arp enable fail\n");
    return NX_NOT_SUCCESSFUL;
  }

  /* Enable the ICMP */

  /* USER CODE BEGIN ICMP_Protocol_Initialization */

  /* USER CODE END ICMP_Protocol_Initialization */

  ret = nx_icmp_enable(&NetXDuoEthIpInstance);

  if (ret != NX_SUCCESS)
  {
	  printf("imcp fail");
    return NX_NOT_SUCCESSFUL;
  }

  /* Enable TCP Protocol */

  /* USER CODE BEGIN TCP_Protocol_Initialization */
  /* Allocate the memory for TCP server thread   */

	if (tx_byte_allocate(byte_pool, (VOID **) &pointer, 16 *  DEFAULT_ARP_CACHE_SIZE, TX_NO_WAIT) != TX_SUCCESS)
		  {
			printf("server alloc fail\n");
		    return TX_POOL_ERROR;
		  }

	ret = tx_thread_create(&AppUDPThread, "App UDP Server Thread", App_UDP_Server_Thread_Entry, 0,
	                           pointer, 16 * 1024,
	                           UDP_SERVER_PRIORITY, UDP_SERVER_PRIORITY,
	                           TX_NO_TIME_SLICE, TX_DONT_START);

  if (ret != TX_SUCCESS)
  {
	printf("udp thread create fail\n");
    return NX_NOT_SUCCESSFUL;
  }
  /* USER CODE END TCP_Protocol_Initialization */

  ret = nx_tcp_enable(&NetXDuoEthIpInstance);

  if (ret != NX_SUCCESS)
  {
	printf("tcp enable fail\n");
    return NX_NOT_SUCCESSFUL;
  }

  /* Enable the UDP protocol required for  DHCP communication */

  /* USER CODE BEGIN UDP_Protocol_Initialization */

  /* USER CODE END UDP_Protocol_Initialization */

  ret = nx_udp_enable(&NetXDuoEthIpInstance);

  if (ret != NX_SUCCESS)
  {
	printf("udp instance creation fail");
    return NX_NOT_SUCCESSFUL;
  }

   /* Allocate the memory for main thread   */
  if (tx_byte_allocate(byte_pool, (VOID **) &pointer, NX_APP_THREAD_STACK_SIZE, TX_NO_WAIT) != TX_SUCCESS)
  {
	printf("main thread mem alloc fail\n");
    return TX_POOL_ERROR;
  }


  /* Create the main thread */
  ret = tx_thread_create(&NxAppThread, "NetXDuo App thread", nx_app_thread_entry , 0, pointer, NX_APP_THREAD_STACK_SIZE,
                         NX_APP_THREAD_PRIORITY, NX_APP_THREAD_PRIORITY, TX_NO_TIME_SLICE, TX_AUTO_START);

  if (ret != TX_SUCCESS)
  {
	printf("main thread creation fail\n");
    return TX_THREAD_ERROR;
  }

  /* USER CODE BEGIN MX_NetXDuo_Init */
  /* Allocate the memory for Link thread   */
  if (tx_byte_allocate(byte_pool, (VOID **) &pointer,NX_APP_THREAD_STACK_SIZE, TX_NO_WAIT) != TX_SUCCESS)
  {
	  printf("link thread mem alloc fail\n");
    return TX_POOL_ERROR;
  }


  /* Create the Link thread */
  ret = tx_thread_create(&AppLinkThread, "App Link Thread", App_Link_Thread_Entry, 0, pointer, NX_APP_THREAD_STACK_SIZE,
                         LINK_PRIORITY, LINK_PRIORITY, TX_NO_TIME_SLICE, TX_AUTO_START);

  if (ret != TX_SUCCESS)
  {
	  printf("link thread creation fail\n");
    return NX_NOT_ENABLED;
  }
  /* USER CODE END MX_NetXDuo_Init */

  return ret;
}

/**
* @brief  ip address change callback.
* @param ip_instance: NX_IP instance
* @param ptr: user data
* @retval none
*/
static VOID ip_address_change_notify_callback(NX_IP *ip_instance, VOID *ptr)
{
  /* USER CODE BEGIN ip_address_change_notify_callback */
	nx_ip_address_set(&NetXDuoEthIpInstance, STM_IP_ADDRESS, STM_NETMASK);
	nx_ip_gateway_address_set(&NetXDuoEthIpInstance, STM_GATEWAY);
	printf("IP and ARP set\n");
	tx_thread_resume(&AppUDPThread);
  /* USER CODE END ip_address_change_notify_callback */
}

/**
* @brief  Main thread entry.
* @param thread_input: ULONG user argument used by the thread entry
* @retval none
*/
static VOID nx_app_thread_entry (ULONG thread_input)
{
  /* USER CODE BEGIN Nx_App_Thread_Entry 0 */

  /* USER CODE END Nx_App_Thread_Entry 0 */

  UINT ret = NX_SUCCESS;
  ULONG actual_status;

  /* USER CODE BEGIN Nx_App_Thread_Entry 1 */
  nx_ip_address_set(&NetXDuoEthIpInstance, STM_IP_ADDRESS, STM_NETMASK);
      nx_ip_gateway_address_set(&NetXDuoEthIpInstance, STM_GATEWAY);
      printf("Static IP set\n");


      ret = nx_ip_interface_status_check(&NetXDuoEthIpInstance, 0,
                                             NX_IP_LINK_ENABLED,
                                             &actual_status, NX_WAIT_FOREVER);
          if (ret != NX_SUCCESS)
          {
              printf("Link down\n");
              Error_Handler();
          }

          printf("Link up - starting UDP server\n");
          tx_thread_resume(&AppUDPThread);
  /* USER CODE END Nx_App_Thread_Entry 1 */

  /* register the IP address change callback */
  ret = nx_ip_address_change_notify(&NetXDuoEthIpInstance, ip_address_change_notify_callback, NULL);
  if (ret != NX_SUCCESS)
  {
    /* USER CODE BEGIN IP address change callback error */

    /* USER CODE END IP address change callback error */
  }

  /* start the DHCP client */
  /* USER CODE BEGIN Nx_App_Thread_Entry 2 */

  /* USER CODE END Nx_App_Thread_Entry 2 */

}
/* USER CODE BEGIN 2 */
/**
* @brief  UDP thread entry.
* @param thread_input: thread user data
* @retval none
*/
static VOID App_UDP_Server_Thread_Entry(ULONG thread_input)
{
    UINT ret;
    NX_PACKET *recv_packet;
    NX_UDP_SOCKET FwdSocket;
    ULONG sender_ip;
    UINT sender_port;

    static DPM_Forward_Packet_t fwd_pkt = {0};
    static AMCISS_Packet_t incoming = {0};
    static uint8_t dcm_received = 0;
    static uint16_t dpm_seq = 0;
    static uint8_t delays_calculated = 0;

    fwd_pkt.magic[0] = 0xAA;
    fwd_pkt.magic[1] = 0xBB;

    NX_PACKET *fwd_nx_pkt = NULL;
    ret = nx_packet_allocate(&NxAppPool, &fwd_nx_pkt, NX_UDP_PACKET, TX_WAIT_FOREVER);
    if (ret != NX_SUCCESS) {
    	printf("fwd pkt alloc fail\n");
    	Error_Handler();
    }

    ret = nx_udp_socket_create(&NetXDuoEthIpInstance, &UDPSocket, "UDP Server Socket",
                               NX_IP_NORMAL, NX_FRAGMENT_OKAY, NX_IP_TIME_TO_LIVE, 100);
    if (ret != NX_SUCCESS) {

    	Error_Handler();
    }

    ret = nx_udp_socket_bind(&UDPSocket, DEFAULT_PORT, NX_WAIT_FOREVER);
    if (ret != NX_SUCCESS) {
    	printf("brokee 2\n"); Error_Handler();
    }

    ret = nx_udp_socket_create(&NetXDuoEthIpInstance, &FwdSocket, "UDP Fwd Socket",
                               NX_IP_NORMAL, NX_FRAGMENT_OKAY, NX_IP_TIME_TO_LIVE, 200);
    if (ret != NX_SUCCESS) {
    	Error_Handler();
    }

    ret = nx_udp_socket_bind(&FwdSocket, NX_ANY_PORT, NX_WAIT_FOREVER);
    if (ret != NX_SUCCESS) {
    	Error_Handler();
    }

    printf("DPM ready on port %u\n", DEFAULT_PORT);

    if (!delays_calculated) {
        calculate_LDC_packet_delays(coil_packet_delay);
        delays_calculated = 1;
    }

    while (1)
    {
        ret = nx_udp_socket_receive(&UDPSocket, &recv_packet, NX_WAIT_FOREVER);
        if (ret != NX_SUCCESS) continue;

        // get sender address
        ULONG sender_ip;
        UINT  sender_port;
        nx_udp_source_extract(recv_packet, &sender_ip, &sender_port);

        // copy payload into local struct
        ULONG bytes_copied = 0;
        nx_packet_data_retrieve(recv_packet, &incoming, &bytes_copied);
        nx_packet_release(recv_packet);

        if (bytes_copied < sizeof(AMCISS_Packet_t)) continue;

        process_packet(&fwd_pkt, &incoming, sender_ip, &dcm_received);

        // forward once both DCMs have reported
        if (dcm_received == 0x03)
            {
                dcm_received = 0;
                fwd_pkt.seq  = dpm_seq++;

                // Reset packet pointers and append fresh data
                fwd_nx_pkt->nx_packet_prepend_ptr = fwd_nx_pkt->nx_packet_data_start +
                                                     NX_UDP_PACKET;
                fwd_nx_pkt->nx_packet_append_ptr  = fwd_nx_pkt->nx_packet_prepend_ptr;
                fwd_nx_pkt->nx_packet_length      = 0;

                nx_packet_data_append(fwd_nx_pkt, &fwd_pkt, sizeof(DPM_Forward_Packet_t),
                                      &NxAppPool, TX_NO_WAIT);

                nx_udp_socket_send(&FwdSocket, fwd_nx_pkt, PC_IP_ADDRESS, PC_PORT);

                // nx_udp_socket_send releases the packet — reallocate for next use
                ret = nx_packet_allocate(&NxAppPool, &fwd_nx_pkt, NX_UDP_PACKET, TX_NO_WAIT);
                if (ret != NX_SUCCESS)
                {
                    printf("pool dry\n");
                    // re-try with wait on next iteration — allocate blocking to recover
                    nx_packet_allocate(&NxAppPool, &fwd_nx_pkt, NX_UDP_PACKET, TX_WAIT_FOREVER);
                }
        }
    }
}

static VOID App_Link_Thread_Entry(ULONG thread_input)
{
  ULONG actual_status;
  UINT linkdown = 0, status;
  printf("link thread started");

  while(1)
  {
    /* Send request to check if the Ethernet cable is connected. */
    status = nx_ip_interface_status_check(&NetXDuoEthIpInstance, 0, NX_IP_LINK_ENABLED,
                                      &actual_status, 10);


    if(status == NX_SUCCESS)
    {
      if(linkdown == 1)
      {
        linkdown = 0;

        /* The network cable is connected. */
        printf("The network cable is connected.\n");

        /* Send request to enable PHY Link. */
        nx_ip_driver_direct_command(&NetXDuoEthIpInstance, NX_LINK_ENABLE,
                                      &actual_status);

        /* Send request to check if an address is resolved. */
        status = nx_ip_interface_status_check(&NetXDuoEthIpInstance, 0, NX_IP_ADDRESS_RESOLVED,
                                      &actual_status, 10);
        if(status == NX_SUCCESS)
        {

          /* Wait until an IP address is ready */


        	nx_ip_address_set(&NetXDuoEthIpInstance, STM_IP_ADDRESS, STM_NETMASK);
        	nx_ip_gateway_address_set(&NetXDuoEthIpInstance, STM_GATEWAY);

        	printf("Link reconnected. Static IP reapplied: 192.168.0.50\n");
          PRINT_IP_ADDRESS(IpAddress);
        }

      }
    }
    else
    {
      if(0 == linkdown)
      {
        linkdown = 1;
        /* The network cable is not connected. */
        printf("The network cable is not connected.\n");
        nx_ip_driver_direct_command(&NetXDuoEthIpInstance, NX_LINK_DISABLE,
                                      &actual_status);
      }
    }

    tx_thread_sleep(NX_APP_CABLE_CONNECTION_CHECK_PERIOD);
  }
}

static void process_packet(DPM_Forward_Packet_t *fwd_pkt,
                            AMCISS_Packet_t *incoming,
                            ULONG sender_ip,
                            uint8_t *dcm_received)
{
    if (incoming->magic[0] != 0xAA || incoming->magic[1] != 0xBB) return;

    if (sender_ip == IP_ADDRESS(192, 168, 0, 69))        // DCM1 → slots [0..31]
    {
        memcpy(&fwd_pkt->ldc[0],      incoming->ldc,      32 * sizeof(uint16_t));
        memcpy(&fwd_pkt->rp[0],       incoming->rp,       32 * sizeof(uint16_t));
        memcpy(&fwd_pkt->is_metal[0], incoming->is_metal, 32 * sizeof(uint8_t));
        fwd_pkt->timestamp_ms = incoming->timestamp_ms;
        *dcm_received |= 0x03; //change to 0x01
    }
    else if (sender_ip == IP_ADDRESS(192, 168, 0, 52))   // DCM2 → slots [32..63]
    {
        memcpy(&fwd_pkt->ldc[32],      incoming->ldc,      32 * sizeof(uint16_t));
        memcpy(&fwd_pkt->rp[32],       incoming->rp,       32 * sizeof(uint16_t));
        memcpy(&fwd_pkt->is_metal[32], incoming->is_metal, 32 * sizeof(uint8_t));
        *dcm_received |= 0x02;
    }
}

void calculate_LDC_packet_delays(int coil_packet_delay[])
{
	coil_packet_delay[0] = COIL_1_OFFSET / (VELOCITY * PACKET_RATE);
	coil_packet_delay[1] = COIL_2_OFFSET / (VELOCITY * PACKET_RATE);
	coil_packet_delay[2] = COIL_3_OFFSET / (VELOCITY * PACKET_RATE);
	coil_packet_delay[3] = COIL_4_OFFSET / (VELOCITY * PACKET_RATE);
	coil_packet_delay[4] = COIL_5_OFFSET / (VELOCITY * PACKET_RATE);
	coil_packet_delay[5] = COIL_6_OFFSET / (VELOCITY * PACKET_RATE);
	coil_packet_delay[6] = COIL_7_OFFSET / (VELOCITY * PACKET_RATE);
	coil_packet_delay[7] = COIL_8_OFFSET / (VELOCITY * PACKET_RATE);
	coil_packet_delay[8] = COIL_9_OFFSET / (VELOCITY * PACKET_RATE);
	coil_packet_delay[9] = COIL_10_OFFSET / (VELOCITY * PACKET_RATE);
	coil_packet_delay[10] = COIL_11_OFFSET / (VELOCITY * PACKET_RATE);
	coil_packet_delay[11] = COIL_12_OFFSET / (VELOCITY * PACKET_RATE);
	coil_packet_delay[12] = COIL_13_OFFSET / (VELOCITY * PACKET_RATE);
	coil_packet_delay[13] = COIL_14_OFFSET / (VELOCITY * PACKET_RATE);
	coil_packet_delay[14] = COIL_15_OFFSET / (VELOCITY * PACKET_RATE);
	coil_packet_delay[15] = COIL_16_OFFSET / (VELOCITY * PACKET_RATE);
	coil_packet_delay[16] = COIL_17_OFFSET / (VELOCITY * PACKET_RATE);
	coil_packet_delay[17] = COIL_18_OFFSET / (VELOCITY * PACKET_RATE);
	coil_packet_delay[18] = COIL_19_OFFSET / (VELOCITY * PACKET_RATE);
	coil_packet_delay[19] = COIL_20_OFFSET / (VELOCITY * PACKET_RATE);
	coil_packet_delay[20] = COIL_21_OFFSET / (VELOCITY * PACKET_RATE);
	coil_packet_delay[21] = COIL_22_OFFSET / (VELOCITY * PACKET_RATE);
	coil_packet_delay[22] = COIL_23_OFFSET / (VELOCITY * PACKET_RATE);
	coil_packet_delay[23] = COIL_24_OFFSET / (VELOCITY * PACKET_RATE);
	coil_packet_delay[24] = COIL_25_OFFSET / (VELOCITY * PACKET_RATE);
	coil_packet_delay[25] = COIL_26_OFFSET / (VELOCITY * PACKET_RATE);
	coil_packet_delay[26] = COIL_27_OFFSET / (VELOCITY * PACKET_RATE);
	coil_packet_delay[27] = COIL_28_OFFSET / (VELOCITY * PACKET_RATE);
	coil_packet_delay[28] = COIL_29_OFFSET / (VELOCITY * PACKET_RATE);
	coil_packet_delay[29] = COIL_30_OFFSET / (VELOCITY * PACKET_RATE);
	coil_packet_delay[30] = COIL_31_OFFSET / (VELOCITY * PACKET_RATE);
	coil_packet_delay[31] = COIL_32_OFFSET / (VELOCITY * PACKET_RATE);
}



/* USER CODE END 2 */

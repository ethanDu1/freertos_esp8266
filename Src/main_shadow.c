/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * <h2><center>&copy; Copyright (c) 2019 STMicroelectronics.
  * All rights reserved.</center></h2>
  *
  * This software component is licensed by ST under Ultimate Liberty license
  * SLA0044, the "License"; You may not use this file except in compliance with
  * the License. You may obtain a copy of the License at:
  *                             www.st.com/SLA0044
  *
  ******************************************************************************
  */
/* USER CODE END Header */
#if 0
/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "cmsis_os.h"

#include "uiot_export.h"
#include "uiot_import.h"
#include "shadow_client.h"
#include "uiot_export_shadow.h"
/* Private includes ----------------------------------------------------------*/
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

UART_HandleTypeDef huart2;
UART_HandleTypeDef huart3;
UART_HandleTypeDef huart6;

PCD_HandleTypeDef hpcd_USB_OTG_FS;

osThreadId defaultTaskHandle;
/* USER CODE BEGIN PV */

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_USART3_UART_Init(void);
static void MX_USB_OTG_FS_PCD_Init(void);
static void MX_USART2_UART_Init(void);
static void MX_USART6_UART_Init(void);
void StartDefaultTask(void const * argument);

#ifdef __GNUC__
/* With GCC/RAISONANCE, small printf (option LD Linker->Libraries->Small printf
   set to 'Yes') calls __io_putchar() */
#define PUTCHAR_PROTOTYPE int __io_putchar(int ch)
#else
#define PUTCHAR_PROTOTYPE int fputc(int ch, FILE *f)
#endif /* __GNUC__ */

/* USER CODE BEGIN PFP */
#define UIOT_MY_PRODUCT_SN            "iwfrdgwhmwscqbmv"

#define UIOT_MY_DEVICE_SN             "mosjgqhqqx1aut0a"

#define UIOT_MY_DEVICE_SECRET         "zn9srzorb96kwat7"

#define MAX_SIZE_OF_TOPIC_CONTENT     100

#define SIZE_OF_JSON_BUFFER 256
/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */
static UIoT_Shadow    *sg_pshadow;
static MQTTInitParams sg_initParams = DEFAULT_MQTT_INIT_PARAMS;


//当设备直接按照desired字段中的属性值更新时不需要上报
void RegCallback_update(void *pClient, RequestParams *pParams, char *pJsonValueBuffer, uint32_t valueLength, DeviceProperty *pProperty)
{
    LOG_DEBUG("key:%s val:%s\n",pProperty->key, pJsonValueBuffer);
    IOT_Shadow_Direct_Update_Value(pJsonValueBuffer, pProperty);
    return;
}

//当设备没有完全按照desired字段中的属性更新时,需要将当前真实值上报
void RegCallback_hold(void *pClient, RequestParams *pParams, char *pJsonValueBuffer, uint32_t valueLength, DeviceProperty *pProperty)
{
    LOG_DEBUG("key:%s val:%s\n",pProperty->key, pJsonValueBuffer);
    int num = 10;
    pProperty->data = &num;
    IOT_Shadow_Request_Add_Delta_Property(pClient, pParams,pProperty);
    return;
}

static void _update_ack_cb(void *pClient, Method method, RequestAck requestAck, const char *pReceivedJsonDocument, void *pUserdata) 
{
	LOG_DEBUG("requestAck=%d\n", requestAck);

    if (NULL != pReceivedJsonDocument) {
        LOG_DEBUG("Received Json Document=%s\n", pReceivedJsonDocument);
    } else {
        LOG_DEBUG("Received Json Document is NULL\n");
    }

    *((RequestAck *)pUserdata) = requestAck;
    return;
}

/**
 * 设置MQTT connet初始化参数
 *
 * @param initParams MQTT connet初始化参数
 *
 * @return 0: 参数初始化成功  非0: 失败
 */
static int _setup_connect_init_params(MQTTInitParams* initParams)
{
    int ret = SUCCESS_RET;
	initParams->device_sn = (char *)UIOT_MY_DEVICE_SN;
	initParams->product_sn = (char *)UIOT_MY_PRODUCT_SN;
	initParams->device_secret = (char *)UIOT_MY_DEVICE_SECRET;

	initParams->command_timeout = UIOT_MQTT_COMMAND_TIMEOUT;
	initParams->keep_alive_interval = UIOT_MQTT_KEEP_ALIVE_INTERNAL;
	initParams->auto_connect_enable = 1;

    return ret;
}


/* USER CODE END PFP */
/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */
void shadow_test(void *pvParameters)
{
    int ret = SUCCESS_RET;
    ret = _setup_connect_init_params(&sg_initParams);
    if(ret != SUCCESS_RET)
    {
        HAL_Printf("_setup_connect_init_params fail:%d\n", ret);
    }
    
    void *mqtt_client = IOT_MQTT_Construct(&sg_initParams);
    if(mqtt_client == NULL)
    {
        HAL_Printf("IOT_MQTT_Construct fail\n");
    }
    
    void *shadow_client = IOT_Shadow_Construct(UIOT_MY_PRODUCT_SN, UIOT_MY_DEVICE_SN, mqtt_client);
    if(shadow_client == NULL)
    {
        HAL_Printf("IOT_Shadow_Construct fail\n");
    }
    
    sg_pshadow = (UIoT_Shadow *)shadow_client;
    bool isConnected = IOT_MQTT_IsConnected(sg_pshadow->mqtt);
    if(isConnected != true)
    {
        HAL_Printf("IOT_MQTT_IsConnected fail\n");
    }
    
    int time_sec = MAX_WAIT_TIME_SEC;
	RequestAck ack_update = ACK_NONE;

    DeviceProperty *Property1 = (DeviceProperty *)HAL_Malloc(sizeof(DeviceProperty));
    int32_t num1 = 18;
    char str1[6] = "data1";
    Property1->key= str1;
    Property1->data = &num1;
    Property1->type = JINT32;
    ret = IOT_Shadow_Register_Property(sg_pshadow, Property1, RegCallback_hold); 
    if(SUCCESS_RET != ret)
    {
        HAL_Printf("Register Property1 fail:%d\n", ret);
    }
    
    DeviceProperty *Property2 = (DeviceProperty *)HAL_Malloc(sizeof(DeviceProperty));
    float num2 = 20.2;
    char str2[6] = "data2";
    Property2->key= str2;
    Property2->data = &num2;
    Property2->type = JFLOAT;
    ret = IOT_Shadow_Register_Property(sg_pshadow, Property2, RegCallback_update); 
    if(SUCCESS_RET != ret)
    {
        HAL_Printf("Register Property2 fail:%d\n", ret);
    }

    DeviceProperty *Property3 = (DeviceProperty *)HAL_Malloc(sizeof(DeviceProperty));
    double num3 = 22.9;
    char str3[6] = "data3";
    Property3->key= str3;
    Property3->data = &num3;
    Property3->type = JDOUBLE;
    ret = IOT_Shadow_Register_Property(sg_pshadow, Property3, RegCallback_update); 
    if(SUCCESS_RET != ret)
    {
        HAL_Printf("Register Property3 fail:%d\n", ret);
    }
    
    DeviceProperty *Property4 = (DeviceProperty *)HAL_Malloc(sizeof(DeviceProperty));
    char num4[5] = "num4";
    char str4[6] = "data4";
    Property4->key= str4;
    Property4->data = num4;
    Property4->type = JSTRING;
    ret = IOT_Shadow_Register_Property(sg_pshadow, Property4, RegCallback_update); 
    if(SUCCESS_RET != ret)
    {
        HAL_Printf("Register Property4 fail:%d\n", ret);
    }

    DeviceProperty *Property5 = (DeviceProperty *)HAL_Malloc(sizeof(DeviceProperty));
    bool num5 = false;
    char str5[6] = "data5";
    Property5->key= str5;
    Property5->data = &num5;
    Property5->type = JBOOL;
    ret = IOT_Shadow_Register_Property(sg_pshadow, Property5, RegCallback_update); 
    if(SUCCESS_RET != ret)
    {
        HAL_Printf("Register Property5 fail:%d\n", ret);
    }

    DeviceProperty *Property6 = (DeviceProperty *)HAL_Malloc(sizeof(DeviceProperty));
    char num6[20] = "{\"temp\":25}";
    char str6[6] = "data6";
    Property6->key= str6;
    Property6->data = num6;
    Property6->type = JOBJECT;
    ret = IOT_Shadow_Register_Property(sg_pshadow, Property6, RegCallback_update); 
    if(SUCCESS_RET != ret)
    {
        HAL_Printf("Register Property6 fail:%d\n", ret);
    }


    while (1)
    {
        ret = IOT_Shadow_Get_Sync(sg_pshadow, _update_ack_cb, time_sec, &ack_update);
        if(SUCCESS_RET != ret)
        {
            HAL_Printf("Get Sync fail:%d\n", ret);
        }

    	while (ACK_NONE == ack_update) {
            IOT_Shadow_Yield(sg_pshadow, MAX_WAIT_TIME_MS);
        }
       
        /* update */    
        ack_update = ACK_NONE;
        ret = IOT_Shadow_Update(sg_pshadow, _update_ack_cb, time_sec, &ack_update, 6, Property1, Property2, Property3, Property4, Property5, Property6);
        if(SUCCESS_RET != ret)
        {
            HAL_Printf("Update Property1 Property2 Property3 Property4 Property5 Property6 fail:%d\n", ret);
        }
        
    	while (ACK_NONE == ack_update) {
            IOT_Shadow_Yield(sg_pshadow, MAX_WAIT_TIME_MS);
        }

        ack_update = ACK_NONE;
        ret = IOT_Shadow_Get_Sync(sg_pshadow, _update_ack_cb, time_sec, &ack_update);

    	while (ACK_NONE == ack_update) {
            IOT_Shadow_Yield(sg_pshadow, MAX_WAIT_TIME_MS);
        }

        /* update */    
        num1 = 123;
        Property1->data = &num1;

        char num9[5] = "num9";
        Property4->data = num9;

        ack_update = ACK_NONE;
        ret = IOT_Shadow_Update(sg_pshadow, _update_ack_cb, time_sec, &ack_update, 2, Property1, Property4);
        if(SUCCESS_RET != ret)
        {
            HAL_Printf("Update Property1 Property4 fail:%d\n", ret);
        }
        
    	while (ACK_NONE == ack_update) {
            IOT_Shadow_Yield(sg_pshadow, MAX_WAIT_TIME_MS);
        }

        /* delete */    
        ack_update = ACK_NONE;
        ret = IOT_Shadow_Delete(sg_pshadow, _update_ack_cb, time_sec, &ack_update, 2, Property1, Property2);
        if(SUCCESS_RET != ret)
        {
            HAL_Printf("Delete Property1 Property2 fail:%d\n", ret);
        }

    	while (ACK_NONE == ack_update) {
            IOT_Shadow_Yield(sg_pshadow, MAX_WAIT_TIME_MS);
        }

        ack_update = ACK_NONE;
        ret = IOT_Shadow_Get_Sync(sg_pshadow, _update_ack_cb, time_sec, &ack_update);


    	while (ACK_NONE == ack_update) {
            IOT_Shadow_Yield(sg_pshadow, MAX_WAIT_TIME_MS);
        }

        /* delete all */
        ack_update = ACK_NONE;
        ret = IOT_Shadow_Delete_All(sg_pshadow, _update_ack_cb, time_sec, &ack_update);
        if(SUCCESS_RET != ret)
        {
            HAL_Printf("Delete All fail:%d\n", ret);
        }


    	while (ACK_NONE == ack_update) {
            IOT_Shadow_Yield(sg_pshadow, MAX_WAIT_TIME_MS);
        }

        ack_update = ACK_NONE;
        ret = IOT_Shadow_Get_Sync(sg_pshadow, _update_ack_cb, time_sec, &ack_update);


    	while (ACK_NONE == ack_update) {
            IOT_Shadow_Yield(sg_pshadow, MAX_WAIT_TIME_MS);
        }

        Property1->data = &num1;
        Property4->data = num4;
        Property5->data = &num5;
        Property6->data = num6;

        /* update */    
        ack_update = ACK_NONE;
        ret = IOT_Shadow_Update_And_Reset_Version(sg_pshadow, _update_ack_cb, time_sec, &ack_update, 4, Property1, Property4, Property5, Property6);
        if(SUCCESS_RET != ret)
        {
            HAL_Printf("Update and Reset Ver fail:%d\n", ret);
        }
        
    	while (ACK_NONE == ack_update) {
            IOT_Shadow_Yield(sg_pshadow, MAX_WAIT_TIME_MS);
        }

        ack_update = ACK_NONE;
        ret = IOT_Shadow_Get_Sync(sg_pshadow, _update_ack_cb, time_sec, &ack_update);

    	while (ACK_NONE == ack_update) {
            IOT_Shadow_Yield(sg_pshadow, MAX_WAIT_TIME_MS);
        }

        HAL_SleepMs(1000);

    }
}


/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{
  /* USER CODE BEGIN 1 */

  /* USER CODE END 1 */
  

  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_USART3_UART_Init();
  MX_USB_OTG_FS_PCD_Init();
  MX_USART2_UART_Init();
  MX_USART6_UART_Init();
  /* USER CODE BEGIN 2 */

  /* USER CODE END 2 */

  /* USER CODE BEGIN RTOS_MUTEX */
  /* add mutexes, ... */
  /* USER CODE END RTOS_MUTEX */

  /* USER CODE BEGIN RTOS_SEMAPHORES */
  /* add semaphores, ... */
  /* USER CODE END RTOS_SEMAPHORES */

  /* USER CODE BEGIN RTOS_TIMERS */
  /* start timers, add new ones, ... */
  /* USER CODE END RTOS_TIMERS */

  /* USER CODE BEGIN RTOS_QUEUES */
  /* add queues, ... */
  /* USER CODE END RTOS_QUEUES */

  /* Create the thread(s) */
  /* definition and creation of defaultTask */
  osThreadDef(defaultTask, StartDefaultTask, osPriorityNormal, 0, 128);
  defaultTaskHandle = osThreadCreate(osThread(defaultTask), NULL);

  /* USER CODE BEGIN RTOS_THREADS */
  xTaskCreate(shadow_test, "shadow_test", 2000, NULL, 1, NULL);
  /* USER CODE END RTOS_THREADS */

  /* Start scheduler */
  osKernelStart();
  
  /* We should never get here as control is now taken by the scheduler */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
  }
  /* USER CODE END 3 */
}

/**
  * @brief  Retargets the C library printf function to the USART.
  * @param  None
  * @retval None
  */
PUTCHAR_PROTOTYPE
{
  /* Place your implementation of fputc here */
  /* e.g. write a character to the USART3 and Loop until the end of transmission */
  HAL_UART_Transmit(&huart6, (uint8_t *)&ch, 1, 0xFFFF);

  return ch;
}


/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};
  RCC_PeriphCLKInitTypeDef PeriphClkInitStruct = {0};

  /** Configure the main internal regulator output voltage 
  */
  __HAL_RCC_PWR_CLK_ENABLE();
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE3);
  /** Initializes the CPU, AHB and APB busses clocks 
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLM = 4;
  RCC_OscInitStruct.PLL.PLLN = 96;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;
  RCC_OscInitStruct.PLL.PLLQ = 4;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }
  /** Activate the Over-Drive mode 
  */
  if (HAL_PWREx_EnableOverDrive() != HAL_OK)
  {
    Error_Handler();
  }
  /** Initializes the CPU, AHB and APB busses clocks 
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_3) != HAL_OK)
  {
    Error_Handler();
  }
  PeriphClkInitStruct.PeriphClockSelection = RCC_PERIPHCLK_USART2|RCC_PERIPHCLK_USART3
                              |RCC_PERIPHCLK_USART6|RCC_PERIPHCLK_CLK48;
  PeriphClkInitStruct.Usart2ClockSelection = RCC_USART2CLKSOURCE_PCLK1;
  PeriphClkInitStruct.Usart3ClockSelection = RCC_USART3CLKSOURCE_PCLK1;
  PeriphClkInitStruct.Usart6ClockSelection = RCC_USART6CLKSOURCE_PCLK2;
  PeriphClkInitStruct.Clk48ClockSelection = RCC_CLK48SOURCE_PLL;
  if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInitStruct) != HAL_OK)
  {
    Error_Handler();
  }
}

/**
  * @brief USART2 Initialization Function
  * @param None
  * @retval None
  */
static void MX_USART2_UART_Init(void)
{

  /* USER CODE BEGIN USART2_Init 0 */

  /* USER CODE END USART2_Init 0 */

  /* USER CODE BEGIN USART2_Init 1 */

  /* USER CODE END USART2_Init 1 */
  huart2.Instance = USART2;
  huart2.Init.BaudRate = 115200;
  huart2.Init.WordLength = UART_WORDLENGTH_8B;
  huart2.Init.StopBits = UART_STOPBITS_1;
  huart2.Init.Parity = UART_PARITY_NONE;
  huart2.Init.Mode = UART_MODE_TX_RX;
  huart2.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart2.Init.OverSampling = UART_OVERSAMPLING_16;
  huart2.Init.OneBitSampling = UART_ONE_BIT_SAMPLE_DISABLE;
  huart2.AdvancedInit.AdvFeatureInit = UART_ADVFEATURE_NO_INIT;
  if (HAL_UART_Init(&huart2) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN USART2_Init 2 */

  /* USER CODE END USART2_Init 2 */

}

/**
  * @brief USART3 Initialization Function
  * @param None
  * @retval None
  */
static void MX_USART3_UART_Init(void)
{

  /* USER CODE BEGIN USART3_Init 0 */

  /* USER CODE END USART3_Init 0 */

  /* USER CODE BEGIN USART3_Init 1 */

  /* USER CODE END USART3_Init 1 */
  huart3.Instance = USART3;
  huart3.Init.BaudRate = 115200;
  huart3.Init.WordLength = UART_WORDLENGTH_8B;
  huart3.Init.StopBits = UART_STOPBITS_1;
  huart3.Init.Parity = UART_PARITY_NONE;
  huart3.Init.Mode = UART_MODE_TX_RX;
  huart3.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart3.Init.OverSampling = UART_OVERSAMPLING_16;
  huart3.Init.OneBitSampling = UART_ONE_BIT_SAMPLE_DISABLE;
  huart3.AdvancedInit.AdvFeatureInit = UART_ADVFEATURE_NO_INIT;
  if (HAL_UART_Init(&huart3) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN USART3_Init 2 */

  /* USER CODE END USART3_Init 2 */

}

/**
  * @brief USART6 Initialization Function
  * @param None
  * @retval None
  */
static void MX_USART6_UART_Init(void)
{

  /* USER CODE BEGIN USART6_Init 0 */

  /* USER CODE END USART6_Init 0 */

  /* USER CODE BEGIN USART6_Init 1 */

  /* USER CODE END USART6_Init 1 */
  huart6.Instance = USART6;
  huart6.Init.BaudRate = 115200;
  huart6.Init.WordLength = UART_WORDLENGTH_8B;
  huart6.Init.StopBits = UART_STOPBITS_1;
  huart6.Init.Parity = UART_PARITY_NONE;
  huart6.Init.Mode = UART_MODE_TX_RX;
  huart6.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart6.Init.OverSampling = UART_OVERSAMPLING_16;
  huart6.Init.OneBitSampling = UART_ONE_BIT_SAMPLE_DISABLE;
  huart6.AdvancedInit.AdvFeatureInit = UART_ADVFEATURE_NO_INIT;
  if (HAL_UART_Init(&huart6) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN USART6_Init 2 */

  /* USER CODE END USART6_Init 2 */

}

/**
  * @brief USB_OTG_FS Initialization Function
  * @param None
  * @retval None
  */
static void MX_USB_OTG_FS_PCD_Init(void)
{

  /* USER CODE BEGIN USB_OTG_FS_Init 0 */

  /* USER CODE END USB_OTG_FS_Init 0 */

  /* USER CODE BEGIN USB_OTG_FS_Init 1 */

  /* USER CODE END USB_OTG_FS_Init 1 */
  hpcd_USB_OTG_FS.Instance = USB_OTG_FS;
  hpcd_USB_OTG_FS.Init.dev_endpoints = 6;
  hpcd_USB_OTG_FS.Init.speed = PCD_SPEED_FULL;
  hpcd_USB_OTG_FS.Init.dma_enable = DISABLE;
  hpcd_USB_OTG_FS.Init.phy_itface = PCD_PHY_EMBEDDED;
  hpcd_USB_OTG_FS.Init.Sof_enable = ENABLE;
  hpcd_USB_OTG_FS.Init.low_power_enable = DISABLE;
  hpcd_USB_OTG_FS.Init.lpm_enable = DISABLE;
  hpcd_USB_OTG_FS.Init.vbus_sensing_enable = ENABLE;
  hpcd_USB_OTG_FS.Init.use_dedicated_ep1 = DISABLE;
  if (HAL_PCD_Init(&hpcd_USB_OTG_FS) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN USB_OTG_FS_Init 2 */

  /* USER CODE END USB_OTG_FS_Init 2 */

}

/**
  * @brief GPIO Initialization Function
  * @param None
  * @retval None
  */
static void MX_GPIO_Init(void)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};

  /* GPIO Ports Clock Enable */
  __HAL_RCC_GPIOC_CLK_ENABLE();
  __HAL_RCC_GPIOH_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();
  __HAL_RCC_GPIOD_CLK_ENABLE();
  __HAL_RCC_GPIOG_CLK_ENABLE();

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOB, LD3_Pin|LD2_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(USB_PowerSwitchOn_GPIO_Port, USB_PowerSwitchOn_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin : USER_Btn_Pin */
  GPIO_InitStruct.Pin = USER_Btn_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_IT_RISING;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(USER_Btn_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pins : RMII_MDC_Pin RMII_RXD0_Pin RMII_RXD1_Pin */
  GPIO_InitStruct.Pin = RMII_MDC_Pin|RMII_RXD0_Pin|RMII_RXD1_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
  GPIO_InitStruct.Alternate = GPIO_AF11_ETH;
  HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

  /*Configure GPIO pins : RMII_REF_CLK_Pin RMII_MDIO_Pin RMII_CRS_DV_Pin */
  GPIO_InitStruct.Pin = RMII_REF_CLK_Pin|RMII_MDIO_Pin|RMII_CRS_DV_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
  GPIO_InitStruct.Alternate = GPIO_AF11_ETH;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  /*Configure GPIO pin : RMII_TXD1_Pin */
  GPIO_InitStruct.Pin = RMII_TXD1_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
  GPIO_InitStruct.Alternate = GPIO_AF11_ETH;
  HAL_GPIO_Init(RMII_TXD1_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pins : LD3_Pin LD2_Pin */
  GPIO_InitStruct.Pin = LD3_Pin|LD2_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

  /*Configure GPIO pin : USB_PowerSwitchOn_Pin */
  GPIO_InitStruct.Pin = USB_PowerSwitchOn_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(USB_PowerSwitchOn_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pin : USB_OverCurrent_Pin */
  GPIO_InitStruct.Pin = USB_OverCurrent_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(USB_OverCurrent_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pins : RMII_TX_EN_Pin RMII_TXD0_Pin */
  GPIO_InitStruct.Pin = RMII_TX_EN_Pin|RMII_TXD0_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
  GPIO_InitStruct.Alternate = GPIO_AF11_ETH;
  HAL_GPIO_Init(GPIOG, &GPIO_InitStruct);

}

/* USER CODE BEGIN 4 */

/* USER CODE END 4 */

/* USER CODE BEGIN Header_StartDefaultTask */
/**
  * @brief  Function implementing the defaultTask thread.
  * @param  argument: Not used 
  * @retval None
  */
/* USER CODE END Header_StartDefaultTask */
void StartDefaultTask(void const * argument)
{
    
    
    

  /* USER CODE BEGIN 5 */
  /* Infinite loop */
  for(;;)
  {
    osDelay(1000);
  }
  /* USER CODE END 5 */ 
}

/**
  * @brief  Period elapsed callback in non blocking mode
  * @note   This function is called  when TIM1 interrupt took place, inside
  * HAL_TIM_IRQHandler(). It makes a direct call to HAL_IncTick() to increment
  * a global variable "uwTick" used as application time base.
  * @param  htim : TIM handle
  * @retval None
  */
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
  /* USER CODE BEGIN Callback 0 */

  /* USER CODE END Callback 0 */
  if (htim->Instance == TIM1) {
    HAL_IncTick();
  }
  /* USER CODE BEGIN Callback 1 */

  /* USER CODE END Callback 1 */
}

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  /* User can add his own implementation to report the HAL error return state */

  /* USER CODE END Error_Handler_Debug */
}

#ifdef  USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t *file, uint32_t line)
{ 
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line number,
     tex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */
#endif
/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
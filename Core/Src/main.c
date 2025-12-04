/* USER CODE BEGIN Header */
/**
 ******************************************************************************
 * @file           : main.c
 * @brief          : Main program body
 ******************************************************************************
 * @attention
 *
 * Copyright (c) 2022 STMicroelectronics.
 * All rights reserved.
 *
 * This software is licensed under terms that can be found in the LICENSE file
 * in the root directory of this software component.
 * If no LICENSE file comes with this software, it is provided AS-IS.
 *
 ******************************************************************************
 */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "dma.h"
#include "i2c.h"
#include "tim.h"
#include "usart.h"
#include "gpio.h"
#include "oled.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */

/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */
typedef enum {
    SERVO_IDLE = 0,
    SERVO_RETURNING,
    SERVO_RUNNING
} ServoState;
/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
#define SERVO_MAX_ANGLE 180
#define SERVO_MIN_ANGLE 0
#define SERVO_PWM_MIN 500   // 0.5ms -> 0度
#define SERVO_PWM_MAX 2500  // 2.5ms -> 180度
#define UNDERFLOW_THRESHOLD 60000
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/

/* USER CODE BEGIN PV */
ServoState servo_state = SERVO_IDLE;
uint16_t servo_current_angle = 0;
uint8_t servo_direction = 0;
uint32_t last_motion_time = 0;
uint32_t servo_run_end_time = 0;
uint32_t servo_motion_time = 5000; // 默认运动周期5秒
uint16_t servo_angle_range = 90;   // 默认角度范围90度
uint32_t servo_start_time = 0;
uint16_t servo_run_duration = 10;  // 默认运行时间10分钟
uint8_t setting_mode = 1;          // 默认进入设置模式

uint16_t bluetooth_angle = 0;
uint8_t bluetooth_angle_valid = 0;
uint16_t bluetooth_time = 0;
uint8_t bluetooth_time_valid = 0;
uint16_t bluetooth_speed = 0;
uint8_t bluetooth_speed_valid = 0;

uint8_t receiveDate[50]; // DMA接收缓冲
uint16_t current_count = 0;
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
/* USER CODE BEGIN PFP */
void SetServoAngle(uint16_t angle);
void UpdateServoPosition(void);
uint16_t MapAngleToPWM(uint16_t angle);
void ParseBluetoothData(uint8_t* data, uint16_t size);
/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

// 将角度映射到PWM值
uint16_t MapAngleToPWM(uint16_t angle)
{
    if(angle > SERVO_MAX_ANGLE) angle = SERVO_MAX_ANGLE;
    if(angle < SERVO_MIN_ANGLE) angle = SERVO_MIN_ANGLE;
    
    // 线性映射: PWM = MIN + (angle / 180) * (MAX - MIN)
    return SERVO_PWM_MIN + (uint16_t)((float)angle / SERVO_MAX_ANGLE * (SERVO_PWM_MAX - SERVO_PWM_MIN));
}

// 设置舵机角度
void SetServoAngle(uint16_t angle)
{
    if(angle > SERVO_MAX_ANGLE) angle = SERVO_MAX_ANGLE;
    if(angle < SERVO_MIN_ANGLE) angle = SERVO_MIN_ANGLE;

    uint16_t pwm_value = MapAngleToPWM(angle);
    __HAL_TIM_SET_COMPARE(&htim4, TIM_CHANNEL_3, pwm_value);
}

// 更新舵机位置
void UpdateServoPosition(void)
{
    if(servo_state == SERVO_RETURNING)
    {
        // 舵机回到原位状态
        if(servo_current_angle > 0)
        {
            servo_current_angle--;
            SetServoAngle(servo_current_angle);
        }
        else
        {
            // 回到原位后，开始按角度范围运动
            servo_state = SERVO_RUNNING;
            servo_direction = 0;  // 正向运动
            last_motion_time = HAL_GetTick();
        }
    }
    else if(servo_state == SERVO_RUNNING)
    {
        // 舵机往复运动逻辑 - 平滑运动
        uint32_t current_time = HAL_GetTick();

        // 检查是否超过运行时间
        if(current_time >= servo_run_end_time)
        {
            // 运行时间到，停止舵机
            servo_state = SERVO_IDLE;
            servo_current_angle = 0;
            SetServoAngle(servo_current_angle);

            // 发送运行结束信息
            HAL_UART_Transmit(&huart2, (uint8_t *)"Servo run completed\r\n", strlen("Servo run completed\r\n"), 10);
            return;
        }

        // 计算运动进度（0-1之间）
        uint32_t motion_cycle_time = servo_motion_time * 2; // 一个完整周期的时间
        uint32_t cycle_elapsed = (current_time - last_motion_time) % motion_cycle_time;
        float progress = (float)cycle_elapsed / motion_cycle_time;

        if(progress < 0.5f)
        {
            // 前半周期：从0度到设定角度
            servo_direction = 0;
            float phase_progress = progress * 2.0f; // 0-1
            servo_current_angle = (uint16_t)(servo_angle_range * phase_progress);
        }
        else
        {
            // 后半周期：从设定角度到0度
            servo_direction = 1;
            float phase_progress = (progress - 0.5f) * 2.0f; // 0-1
            servo_current_angle = (uint16_t)(servo_angle_range * (1.0f - phase_progress));
        }

        SetServoAngle(servo_current_angle);
    }
    else
    {
        // 舵机停止状态，保持在当前位置
        SetServoAngle(servo_current_angle);
    }
}

// 解析蓝牙接收的数据
void ParseBluetoothData(uint8_t* data, uint16_t size)
{
    // 确保数据以null结尾
    if(size < sizeof(receiveDate))
    {
        data[size] = '\0';
    }
    else
    {
        data[sizeof(receiveDate)-1] = '\0';
    }
    
    // 移除换行符和回车符
    for(int i = 0; i < size; i++)
    {
        if(data[i] == '\r' || data[i] == '\n')
        {
            data[i] = '\0';
            break;
        }
    }
    
    // 检查是否为空行
    if(strlen((char*)data) == 0)
    {
        return;
    }
    
    // 检查是否是特殊命令
    if(strncmp((char*)data, "TEST", 4) == 0)
    {
        return;
    }
    else if(strncmp((char*)data, "START", 5) == 0)
    {
        // 蓝牙启动命令
        if(servo_state == SERVO_IDLE)
        {
            servo_state = SERVO_RETURNING;
            servo_current_angle = 0;
            servo_direction = 0;
            last_motion_time = HAL_GetTick();
            HAL_UART_Transmit(&huart2, (uint8_t *)"ACK: Servo started\r\n", strlen("ACK: Servo started\r\n"), 10);
        }
        else
        {
            HAL_UART_Transmit(&huart2, (uint8_t *)"INFO: Servo already running\r\n", strlen("INFO: Servo already running\r\n"), 10);
        }
        return;
    }
    else if(strncmp((char*)data, "STOP", 4) == 0)
    {
        // 蓝牙停止命令
        servo_state = SERVO_IDLE;
        HAL_UART_Transmit(&huart2, (uint8_t *)"ACK: Servo stopped\r\n", strlen("ACK: Servo stopped\r\n"), 10);
        return;
    }
    
    // 解析参数命令
    if(strncmp((char*)data, "A", 1) == 0) // 角度设置：A90
    {
        int value = atoi((char*)data + 1);
        if(value >= 0 && value <= 180)
        {
            bluetooth_angle = (uint16_t)value;
            bluetooth_angle_valid = 1;
            servo_angle_range = bluetooth_angle;
            __HAL_TIM_SET_COUNTER(&htim1, bluetooth_angle);
            
            char ack_msg[50];
            snprintf(ack_msg, sizeof(ack_msg), "ACK: Angle set to %d\r\n", bluetooth_angle);
            HAL_UART_Transmit(&huart2, (uint8_t *)ack_msg, strlen(ack_msg), 10);
        }
        else
        {
            HAL_UART_Transmit(&huart2, (uint8_t *)"ERROR: Angle must be 0-180\r\n", strlen("ERROR: Angle must be 0-180\r\n"), 10);
        }
    }
    else if(strncmp((char*)data, "T", 1) == 0) // 时间设置：T10
    {
        int value = atoi((char*)data + 1);
        if(value >= 1 && value <= 60)
        {
            bluetooth_time = (uint16_t)value;
            bluetooth_time_valid = 1;
            servo_run_duration = bluetooth_time;
            
            char ack_msg[50];
            snprintf(ack_msg, sizeof(ack_msg), "ACK: Run time set to %d minutes\r\n", bluetooth_time);
            HAL_UART_Transmit(&huart2, (uint8_t *)ack_msg, strlen(ack_msg), 10);
        }
        else
        {
            HAL_UART_Transmit(&huart2, (uint8_t *)"ERROR: Time must be 1-60 minutes\r\n", strlen("ERROR: Time must be 1-60 minutes\r\n"), 10);
        }
    }
    else if(strncmp((char*)data, "S", 1) == 0) // 速度设置：S5
    {
        int value = atoi((char*)data + 1);
        if(value >= 1 && value <= 10)
        {
            bluetooth_speed = (uint16_t)value;
            bluetooth_speed_valid = 1;
            servo_motion_time = bluetooth_speed * 1000;
            
            char ack_msg[50];
            snprintf(ack_msg, sizeof(ack_msg), "ACK: Speed set to %d seconds\r\n", bluetooth_speed);
            HAL_UART_Transmit(&huart2, (uint8_t *)ack_msg, strlen(ack_msg), 10);
        }
        else
        {
            HAL_UART_Transmit(&huart2, (uint8_t *)"ERROR: Speed must be 1-10 seconds\r\n", strlen("ERROR: Speed must be 1-10 seconds\r\n"), 10);
        }
    }
    else
    {
        // 兼容旧格式：纯数字输入
        int value = atoi((char*)data);
        
        if(value >= 0 && value <= 180)
        {
            // 角度值
            bluetooth_angle = (uint16_t)value;
            bluetooth_angle_valid = 1;
            servo_angle_range = bluetooth_angle;
            __HAL_TIM_SET_COUNTER(&htim1, bluetooth_angle);
            
            char ack_msg[50];
            snprintf(ack_msg, sizeof(ack_msg), "ACK: Angle set to %d\r\n", bluetooth_angle);
            HAL_UART_Transmit(&huart2, (uint8_t *)ack_msg, strlen(ack_msg), 10);
        }
        else
        {
            char error_msg[80];
            snprintf(error_msg, sizeof(error_msg), "ERROR: Invalid value %d. Use A90, T10, S5 format\r\n", value);
            HAL_UART_Transmit(&huart2, (uint8_t *)error_msg, strlen(error_msg), 10);
        }
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
  MX_DMA_Init();
  MX_TIM1_Init();
  MX_USART2_UART_Init();
  MX_I2C1_Init();
  MX_USART3_UART_Init();
  MX_TIM4_Init();
  /* USER CODE BEGIN 2 */
    HAL_UARTEx_ReceiveToIdle_DMA(&huart3, receiveDate, sizeof(receiveDate));
    __HAL_DMA_DISABLE_IT(&hdma_usart3_rx, DMA_IT_HT);
    HAL_TIM_Encoder_Start(&htim1, TIM_CHANNEL_ALL);
    OLED_Init();
    HAL_TIM_PWM_Start(&htim4, TIM_CHANNEL_3);

    // 初始化变量
    uint16_t cnt_encoder = 0;

    // 初始化舵机到0度位置
    SetServoAngle(0);
    servo_current_angle = 0;
    
    // 串口测试 - 发送启动信息
    HAL_UART_Transmit(&huart2, (uint8_t *)"STM32 Started\r\n", strlen("STM32 Started\r\n"), 1000);
    HAL_Delay(1000);
    HAL_UART_Transmit(&huart2, (uint8_t *)"System Ready\r\n", strlen("System Ready\r\n"), 1000);
    HAL_Delay(1000);
    HAL_UART_Transmit(&huart2, (uint8_t *)"Setting Mode - Use encoder to set angle\r\n", strlen("Setting Mode - Use encoder to set angle\r\n"), 1000);
    HAL_Delay(1000);

  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
    while (1)
    {
        // 读取编码器值
        current_count = __HAL_TIM_GET_COUNTER(&htim1);

        // 处理边界情况 - 限制在0-180度范围内
        if (current_count > UNDERFLOW_THRESHOLD) {
            current_count = 0;
            __HAL_TIM_SET_COUNTER(&htim1, 0);
        } else if (current_count > 180) {
            current_count = 180;
            __HAL_TIM_SET_COUNTER(&htim1, 180);
        }

        // 只有当计数值真正改变时才更新显示
        if (cnt_encoder != current_count)
        {
            cnt_encoder = current_count;
            // 只在设置模式下显示编码器值
            if(setting_mode)
            {
                char encoder_msg[30];
                snprintf(encoder_msg, sizeof(encoder_msg), "Encoder: %d\r\n", cnt_encoder);
                HAL_UART_Transmit(&huart2, (uint8_t *)encoder_msg, strlen(encoder_msg), 100);
            }
        }
        
        // 按钮检测（轮询方式）
        static uint32_t last_btn1_time = 0;
        static uint8_t btn1_pressed = 0;
        static uint8_t btn2_pressed = 0;
        static uint32_t btn2_press_start = 0;
        static uint8_t btn2_long_press = 0;
        
        // 检测按钮1
        if(HAL_GPIO_ReadPin(button1_GPIO_Port, button1_Pin) == GPIO_PIN_RESET)
        {
            if(!btn1_pressed && (HAL_GetTick() - last_btn1_time > 200))
            {
                btn1_pressed = 1;
                last_btn1_time = HAL_GetTick();
                
                // 按钮1按下 - 启动舵机
                if(setting_mode)
                {
                    setting_mode = 0;
                    servo_state = SERVO_RETURNING;
                    servo_current_angle = 0;
                    servo_direction = 0;
                    
                    // 显示当前设置的角度范围
                    char start_msg[50];
                    snprintf(start_msg, sizeof(start_msg), "Button1: Start Servo, Angle Range: %d\r\n", servo_angle_range);
                    HAL_UART_Transmit(&huart2, (uint8_t *)start_msg, strlen(start_msg), 100);
                }
                else if(servo_state == SERVO_IDLE)
                {
                    servo_state = SERVO_RETURNING;
                    servo_current_angle = 0;
                    servo_direction = 0;
                    
                    // 显示当前设置的角度范围
                    char restart_msg[50];
                    snprintf(restart_msg, sizeof(restart_msg), "Button1: Restart Servo, Angle Range: %d\r\n", servo_angle_range);
                    HAL_UART_Transmit(&huart2, (uint8_t *)restart_msg, strlen(restart_msg), 100);
                }
            }
        }
        else
        {
            btn1_pressed = 0;
        }
        
        // 检测按钮2
        if(HAL_GPIO_ReadPin(button2_GPIO_Port, button2_Pin) == GPIO_PIN_RESET)
        {
            if(!btn2_pressed)
            {
                btn2_pressed = 1;
                btn2_press_start = HAL_GetTick();
                btn2_long_press = 0;
            }
            else
            {
                // 检查长按
                if((HAL_GetTick() - btn2_press_start > 2000) && !btn2_long_press)
                {
                    // 长按：返回设置模式
                    setting_mode = 1;
                    servo_state = SERVO_IDLE;
                    btn2_long_press = 1;
                    HAL_UART_Transmit(&huart2, (uint8_t *)"Button2: Setting Mode - Use encoder to set angle\r\n", strlen("Button2: Setting Mode - Use encoder to set angle\r\n"), 100);
                }
            }
        }
        else
        {
            if(btn2_pressed)
            {
                // 按钮释放，检查是否为长按
                if(btn2_press_start > 0)
                {
                    uint32_t press_duration = HAL_GetTick() - btn2_press_start;
                    
                    if(press_duration > 2000) // 长按2秒
                    {
                        // 长按已在按下时处理
                    }
                    else if(!btn2_long_press)
                    {
                        // 短按：停止舵机
                        servo_state = SERVO_IDLE;
                        // 不改变servo_current_angle，保持在当前位置
                        HAL_UART_Transmit(&huart2, (uint8_t *)"Button2: Stop Servo\r\n", strlen("Button2: Stop Servo\r\n"), 100);
                    }
                    btn2_press_start = 0;
                }
            }
            btn2_pressed = 0;
        }
        
        // 在设置模式下，通过编码器或蓝牙设定舵机角度范围
        if(setting_mode)
        {
            // 如果蓝牙设置了角度，优先使用蓝牙值（仅使用一次）
            if(bluetooth_angle_valid)
            {
                servo_angle_range = bluetooth_angle;
                // 立即清除标志，允许编码器继续调整
                bluetooth_angle_valid = 0;
            }
            else
            {
                // 使用编码器值直接对应角度（0-180度）
                servo_angle_range = current_count;
                if(servo_angle_range > 180) servo_angle_range = 180;
            }
        }
        
        // 舵机位置更新
        UpdateServoPosition();
        
        // OLED显示更新
        OLED_NewFrame();
        
        if(setting_mode)
        {
            // 设置模式显示
            OLED_PrintString(0, 0, "SETTING MODE", &font16x16, OLED_COLOR_NORMAL);
            
            char angle_buf[30];
            snprintf(angle_buf, sizeof(angle_buf), "Angle: %d", servo_angle_range);
            OLED_PrintString(0, 20, angle_buf, &font16x16, OLED_COLOR_NORMAL);
            
            char time_buf[30];
            snprintf(time_buf, sizeof(time_buf), "Time: %dmin", servo_run_duration);
            OLED_PrintString(0, 40, time_buf, &font16x16, OLED_COLOR_NORMAL);
        }
        else
        {
            // 运行模式显示
            OLED_PrintString(0, 0, "RUNNING MODE", &font16x16, OLED_COLOR_NORMAL);
            
            // 显示状态
            const char* state_str = (servo_state == SERVO_IDLE) ? "IDLE" : 
                                   (servo_state == SERVO_RETURNING) ? "RETURNING" : "RUNNING";
            char state_buf[30];
            snprintf(state_buf, sizeof(state_buf), "State: %s", state_str);
            OLED_PrintString(0, 20, state_buf, &font16x16, OLED_COLOR_NORMAL);
            
            // 显示当前角度
            char angle_buf[30];
            snprintf(angle_buf, sizeof(angle_buf), "Angle: %d/%d", servo_current_angle, servo_angle_range);
            OLED_PrintString(0, 40, angle_buf, &font16x16, OLED_COLOR_NORMAL);
        }
        
        OLED_ShowFrame();
        
        // 蓝牙接收测试（轮询方式）
        static uint8_t bt_buffer[20];
        static uint8_t bt_index = 0;
        static uint32_t last_bt_check = 0;
        
        // 每10ms检查一次蓝牙接收
        if(HAL_GetTick() - last_bt_check > 10)
        {
            last_bt_check = HAL_GetTick();
            
            uint8_t received_char;
            if(HAL_UART_Receive(&huart3, &received_char, 1, 1) == HAL_OK)
            {
                if(received_char == '\r' || received_char == '\n')
                {
                    if(bt_index > 0)
                    {
                        bt_buffer[bt_index] = '\0';
                        // 移除蓝牙接收显示，直接解析
                        ParseBluetoothData(bt_buffer, bt_index);
                    }
                    bt_index = 0;
                }
                else if(bt_index < 19)
                {
                    bt_buffer[bt_index] = received_char;
                    bt_index++;
                }
                else
                {
                    // 缓冲区满，重置
                    bt_index = 0;
                }
            }
        }
        
        // 简单延时
        HAL_Delay(1);
    }
  /* USER CODE END WHILE */

  /* USER CODE BEGIN 3 */

  /* USER CODE END 3 */
}

/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.HSEPredivValue = RCC_HSE_PREDIV_DIV1;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLMUL = RCC_PLL_MUL9;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2) != HAL_OK)
  {
    Error_Handler();
  }
}

/* USER CODE BEGIN 4 */

/* USER CODE END 4 */

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
    /* User can add his own implementation to report the HAL error return state */
    __disable_irq();
    while (1)
    {
    }
  /* USER CODE END Error_Handler_Debug */
}
#ifdef USE_FULL_ASSERT
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
    ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */

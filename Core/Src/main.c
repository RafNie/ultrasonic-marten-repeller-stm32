#include "stm32f1xx.h"
#include "player.h"
#include "gpio.h"
#include "borsuk.h"
#include "bb.h"
#include <stdlib.h>
#include <sys/types.h>

void Error_Handler(void);
void SystemClock_Config(void);

volatile unsigned int delayNextPlay = 0;// 10ms period

static inline int isInputHigh() {
	return GPIOB->IDR & GPIO_IDR_IDR0;
}

void delayRand2s() {
	delayNextPlay = rand()%200;
}

extern Player* player;

int main(void) {
	SystemClock_Config();
	SysTick_Config(44000000/100); //10ms

	player = createPlayer(wave, sizeof(wave)/sizeof(wave[0]), 350);
	setEndPlayCallback(player, delayRand2s);

	BB(RCC->APB2ENR, RCC_APB2ENR_IOPBEN) = 1;
	gpio_pin_cfg(GPIOB, PB0, gpio_mode_input_floating);

	while (1) {
		if ( !isPlaying(player) && !delayNextPlay && isInputHigh()) {
			u_int32_t time_ms = (rand()%4000)+2000; //(2-6s)
			playMS(player, time_ms);
		}
	}
}

__attribute__((interrupt)) void SysTick_Handler(void){
	if (delayNextPlay) delayNextPlay--;
}

void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  /** Initializes the CPU, AHB and APB busses clocks
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSI_DIV2;
  RCC_OscInitStruct.PLL.PLLMUL = RCC_PLL_MUL11;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
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

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_1) != HAL_OK)
  {
    Error_Handler();
  }
}

void Error_Handler(void)
{
}

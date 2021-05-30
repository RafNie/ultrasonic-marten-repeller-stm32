#include <badger.h>
#include "stm32f1xx.h"
#include "player.h"
#include "gpio.h"
#include "bb.h"
#include <stdlib.h>
#include <sys/types.h>

void Error_Handler(void);
void SystemClock_Config(void);

volatile unsigned int delayBeforeNextPlay = 0;// 10ms period

static inline int isInputHigh() {
	return GPIOB->IDR & GPIO_IDR_IDR0;
}

void delayRand2s() {
	delayBeforeNextPlay = rand()%200;
}

extern Player* player;

int main(void) {
	SystemClock_Config();
	SysTick_Config(44000000/100); //10ms

	player = createPlayer(wave, sizeof(wave)/sizeof(wave[0]), wave_sampling_rate);
	setEndPlayCallback(player, delayRand2s);

	BB(RCC->APB2ENR, RCC_APB2ENR_IOPBEN) = 1;
	gpio_pin_cfg(GPIOB, PB0, gpio_mode_input_floating);

	int mixRandomPeriod = 350;
	while (1) {
		if ( !isPlaying(player) && !delayBeforeNextPlay && isInputHigh()) {
			u_int32_t time_ms = (rand()%3000)+2000; //(2-5s)
			playLoopsOverTimeMixRandomly(player, time_ms, mixRandomPeriod);
		}
	}
}

__attribute__((interrupt)) void SysTick_Handler(void){
	if (delayBeforeNextPlay) delayBeforeNextPlay--;
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

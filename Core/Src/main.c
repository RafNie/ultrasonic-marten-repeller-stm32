#include "badger.h"
#include "stm32f1xx.h"
#include "player.h"
#include "gpio.h"
#include "bb.h"
#include <stdlib.h>
#include <sys/types.h>

void SystemClock_Config(void);

volatile unsigned int delayBeforeNextPlay = 0;// 10ms period

static inline int isInputHigh() {
	return GPIOB->IDR & GPIO_IDR_IDR0;
}

void delayRand2s() {
	delayBeforeNextPlay = rand()%200;
}

extern Player* player;
unsigned int SYS_CLK = 72000000;

int main(void) {
	SystemClock_Config();
	SysTick_Config(SYS_CLK/100); //10ms

	player = createPlayer(wave, sizeof(wave)/sizeof(wave[0]), wave_sampling_rate, SYS_CLK/2);
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
	 RCC->CR |= RCC_CR_HSEON;
	 RCC->CFGR = RCC_CFGR_PLLMULL9 | RCC_CFGR_PLLSRC | RCC_CFGR_ADCPRE_DIV6 |
	 RCC_CFGR_PPRE1_DIV2 | RCC_CFGR_USBPRE;
	 while (!(RCC->CR & RCC_CR_HSERDY));
	 RCC->CR |= RCC_CR_PLLON;
	 FLASH->ACR |= FLASH_ACR_LATENCY_1;
	 while (!(RCC->CR & RCC_CR_PLLRDY));
	 RCC->CFGR |= RCC_CFGR_SW_PLL;
	 while ( (RCC->CFGR & RCC_CFGR_SWS) != RCC_CFGR_SWS_PLL);
	 RCC->CR &= ~RCC_CR_HSION;
}


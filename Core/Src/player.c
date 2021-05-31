/*
 * player.c
 *
 *  Created on: 26 maj 2021
 *      Author: Rafal Niedzwiedzinski
 */
#include "player.h"
#include "bb.h"
#include "gpio.h"
#include <stdlib.h>

Player* player;

static const int PWM_RESOLUTION = 255;
static const int PWM_MIDLE_VAL = 127;

static inline void setPWM(unsigned char val) {
	TIM1->CCR1 = val;
}
static void configureTIM1_PWMMode();
static void configureTIM2(uint32_t sampling_rate);

static void fixSampleIndex(Player *player);
static int getNextSampleIndex(Player* player);
static void handleNextSample(Player* player);
static void randomMix(Player* player);
static void tryMix(Player *player);
static inline void startPlaing(Player* player) {
	player->enable = 1;
}
static inline void stopPlaing(Player* player) {
	player->enable = 0;
	player->next_sample = 0;
	setPWM(PWM_MIDLE_VAL);
	if (player->endPlayCallback)
		player->endPlayCallback();
}

Player* createPlayer(const unsigned char* data, int dataSize, int sampeRate, unsigned int TIM2_CLK) {

	Player* player = malloc(sizeof(Player));
	if (player) {
		player->play_ms = 0;
		player->next_sample = 0;
		player->sample_rate = sampeRate;
		player->samples_per_ms = player->sample_rate/1000;
		player->TIM2_CLK = TIM2_CLK;
		player->data = data;
		player->dataSize = dataSize;
		player->randomMixPeriod = 0;
		player->enable = 0;
		player->endPlayCallback = NULL;

		configureTIM1_PWMMode();
		configureTIM2(sampeRate);
		NVIC_ClearPendingIRQ(TIM1_CC_IRQn);
		NVIC_ClearPendingIRQ(TIM2_IRQn);
		NVIC_EnableIRQ(TIM1_CC_IRQn);
		NVIC_EnableIRQ(TIM2_IRQn);

		setPWM(PWM_MIDLE_VAL);
	}
	return player;
}

void playOnce(Player* player) {
	if (player) {
		int timeOfSample = player->dataSize/player->samples_per_ms;
		player->play_ms = timeOfSample;
		player->next_sample = 0;
		player->randomMixPeriod = 0;
		startPlaing(player);
	}
}

void playLoopsOverTime(Player* player, int time_ms) {
	if (player) {
		player->play_ms = time_ms;
		player->next_sample = 0;
		player->randomMixPeriod = 0;
		startPlaing(player);
	}
}

void playLoopsOverTimeMixRandomly(Player* player, int time_ms, int randomMixPeriodMs) {
	if (player) {
		player->play_ms = time_ms;
		player->next_sample = 0;
		int timeOfSample = player->dataSize/player->samples_per_ms;
		player->randomMixPeriod = randomMixPeriodMs>=timeOfSample ?
								timeOfSample :
								randomMixPeriodMs;
		startPlaing(player);
	}
}


int isPlaying(Player* player) {
	if (player)
		return player->play_ms;
	else
		return 0;
}

void setEndPlayCallback(Player* player, void (*endPlayCallback)()) {
	if (player)
		player->endPlayCallback = endPlayCallback;
}

void fixSampleIndex(Player *player) {
	if (player->next_sample >= player->dataSize)
		player->next_sample = 0;
}

int getNextSampleIndex(Player* player) {
	int sample = player->next_sample++;
	fixSampleIndex(player);
	return sample;
}

void randomMix(Player* player) {
	int newSamplePosition = (rand()%player->randomMixPeriod) * player->samples_per_ms; // draws sample position in range ramdomMixPeriod
	player->next_sample += newSamplePosition;
	fixSampleIndex(player);
}

void tryMix(Player *player) {
	if (player->randomMixPeriod
			&& (player->play_ms) % (player->randomMixPeriod) == 0)
		randomMix(player);
}

void handleNextSample(Player* player) {
	if (player) {
		int sample = getNextSampleIndex(player);
		int milisecondFraction = sample%(player->samples_per_ms);
		if (milisecondFraction == 0 ) {
			player->play_ms--;
			tryMix(player);
		}
		setPWM(player->data[sample]);
		if (player->play_ms == 0) {
			stopPlaing(player);
		}
	}
}

void configureTIM2(uint32_t sampling_rate) {
	BB(RCC->APB1ENR, RCC_APB1ENR_TIM2EN) = 1;
	TIM2->PSC = 0;
	TIM2->ARR = player->TIM2_CLK/sampling_rate;
	TIM2->DIER = TIM_DIER_UIE;
	TIM2->CR1 = TIM_CR1_CEN;
}

void configureTIM1_PWMMode() {
	RCC->APB2ENR = RCC_APB2ENR_IOPAEN | RCC_APB2ENR_IOPBEN | RCC_APB2ENR_TIM1EN;
	gpio_pin_cfg(GPIOA, PA8, gpio_mode_alternate_OD_2MHz);
	gpio_pin_cfg(GPIOB, PB13, gpio_mode_alternate_OD_2MHz);

	TIM1->CCMR1 = TIM_CCMR1_OC1PE | TIM_CCMR1_OC1M_2 | TIM_CCMR1_OC1M_1;
	TIM1->CCER = TIM_CCER_CC1E | TIM_CCER_CC1NE;
	TIM1->BDTR = TIM_BDTR_MOE;

	TIM1->PSC = 0;
	TIM1->ARR = PWM_RESOLUTION;
	TIM1->CCR1 = PWM_MIDLE_VAL;
	TIM1->EGR = TIM_EGR_UG;
	TIM1->CR1 = TIM_CR1_ARPE | TIM_CR1_CEN;
}

__attribute__((interrupt)) void TIM2_IRQHandler(void){
	if (TIM2->SR & TIM_SR_UIF) {
		TIM2->SR = 0;
		if (player && player->enable)
			handleNextSample(player);
	}
}

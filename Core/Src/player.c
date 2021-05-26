/*
 * player.c
 *
 *  Created on: 26 maj 2021
 *      Author: rafal
 */
#include "player.h"
#include "bb.h"
#include "gpio.h"
#include <stdlib.h>

Player* player;

int isPlaying(Player* player) {
	return player->play_ms;
}

Player* createPlayer(const unsigned char* data, int dataSize, int randomMixPeriodMs) {
	Player* player = malloc(sizeof(Player));

	player->play_ms = 0;
	player->next_sample = 0;
	player->sample_rate = 44000000/512;//88k
	player->sample_per_ms = player->sample_rate/1000;
	player->data = data;
	player->dataSize = dataSize;
	int sampleLen = dataSize/player->sample_per_ms;
	player->randomMixPeriod = randomMixPeriodMs>=sampleLen ?
							sampleLen :
							randomMixPeriodMs;
	player->enable = 0;
	player->endPlayCallback = NULL;

	configureTIM1_PWMMode();
	NVIC_ClearPendingIRQ(TIM1_CC_IRQn);
	NVIC_EnableIRQ(TIM1_CC_IRQn);
	BB(TIM1->CR1, TIM_CR1_CEN) = 1;
	setPWM(PWM_MIDLE_VAL);

	return player;
}

void playMS(Player* player, int time_ms) {
	if (player) {
		player->play_ms = time_ms;
		player->next_sample = 0;
		startPlaing(player);
	}
}

void setEndPlayCallback(Player* player, void (*endPlayCallback)()) {
	player->endPlayCallback = endPlayCallback;
}

int getNextSampleIndex(Player* player) {
	int sample = player->next_sample++;
	if (player->next_sample >= player->dataSize)
		player->next_sample = 0;
	return sample;
}

void randomMix(Player* player) {
	int newSamplePosition = (rand()%200) * player->sample_per_ms; // rand sample position in range 200ms
	player->next_sample = ++newSamplePosition;
}

void handleNextSample(Player* player) {
	if (player) {
		int sample = getNextSampleIndex(player);
		int milisecondBorder = sample%(player->sample_per_ms);
		if (milisecondBorder == 0 ) {
			player->play_ms--;
			if (player->randomMixPeriod && (player->play_ms)%250 == 0)//rand new sample position every 250ms
				randomMix(player);
		}
		setPWM(player->data[sample]);
		if (player->play_ms == 0) {
			stopPlaing(player);
			player->next_sample = 0;
			if (player->endPlayCallback)
				player->endPlayCallback();
		}
	}
}

void configureTIM1_PWMMode() {
	RCC->APB2ENR = RCC_APB2ENR_IOPAEN | RCC_APB2ENR_TIM1EN;
	gpio_pin_cfg(GPIOA, PA8, gpio_mode_alternate_PP_2MHz);

	TIM1->CCMR1 = TIM_CCMR1_OC1PE | TIM_CCMR1_OC1M_2 | TIM_CCMR1_OC1M_1;
	TIM1->CCER = TIM_CCER_CC1E;
	TIM1->BDTR = TIM_BDTR_MOE;

	// interrupt
	TIM1->DIER = TIM_DIER_CC1IE;

	TIM1->PSC = 0; // 0 - ~172kHz
	TIM1->ARR = PWM_RESOLUTION;
	TIM1->CCR1 = PWM_MIDLE_VAL;
	TIM1->EGR = TIM_EGR_UG;
	TIM1->CR1 = TIM_CR1_ARPE;
}

volatile unsigned int step = 0;
__attribute__((interrupt)) void TIM1_CC_IRQHandler(void){
	if (TIM1->SR & TIM_SR_CC1IF) {
		TIM1->SR = ~TIM_SR_CC1IF;
		if (player && player->enable && !((step++)%2)) // && next sample every 2 IRQ event (4 for 41k rate)
			handleNextSample(player);
	}
}


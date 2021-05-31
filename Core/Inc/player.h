/*
 * player.h
 *
 *  Created on: 26 maj 2021
 *      Author: Rafal Niedzwiedzinski
 *
 *      Description: Audio wave player. Sound is generated via PWM.
 *
 *      Hardware configuration dedicated for STM32F1xx. TIM1 and TIM2 are used.
 */

#ifndef INC_PLAYER_H_
#define INC_PLAYER_H_
#include "stm32f1xx.h"

typedef struct {
	unsigned int enable;
	volatile unsigned int play_ms;
	unsigned int sample_rate;
	unsigned int samples_per_ms;
	unsigned int TIM2_CLK;
	volatile unsigned int next_sample;
	unsigned int randomMixPeriod; // 0 disable this feature
	unsigned int dataSize;
	const unsigned char* data;
	void (*endPlayCallback)();
} Player;

Player* createPlayer(const unsigned char* data, int dataSize, int sampeRateunsigned, unsigned int TIM2_CLK);
void playOnce(Player* player);
void playLoopsOverTime(Player* player, int time_ms);
void playLoopsOverTimeMixRandomly(Player* player, int time_ms, int randomMixPeriodMs);
int isPlaying(Player* player);
void setEndPlayCallback(Player* player, void (*endPlayCallback)());

#endif /* INC_PLAYER_H_ */

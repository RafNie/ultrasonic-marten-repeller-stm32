/*
 * player.h
 *
 *  Created on: 26 maj 2021
 *      Author: rafal
 */

#ifndef INC_PLAYER_H_
#define INC_PLAYER_H_
#include "stm32f1xx.h"
#include <sys/types.h>

static const int PWM_RESOLUTION = 255;
static const int PWM_MIDLE_VAL = 127;

void configureTIM1_PWMMode();
static inline void setPWM(unsigned char val) {
	TIM1->CCR1 = val;
}

typedef struct Player {
	volatile u_int32_t play_ms;
	u_int32_t sample_rate;
	u_int32_t sample_per_ms;
	volatile u_int32_t next_sample;
	const unsigned char* data;
	u_int32_t dataSize;
	u_int32_t randomMixPeriod; // 0 disable this feature
	u_int32_t enable;
	void (*endPlayCallback)();
} Player;

Player* createPlayer(const unsigned char* data, int dataSize, int randomMixPeriodMs);
void playMS(Player* player, int time_ms);
int isPlaying(Player* player);
void setEndPlayCallback(Player* player, void (*endPlayCallback)());

//private
int getNextSampleIndex(Player* player);
void handleNextSample(Player* player);
void randomMix(Player* player);
static inline void startPlaing(Player* player) {
	player->enable = 1;
}
static inline void stopPlaing(Player* player) {
	player->enable = 0;
	setPWM(PWM_MIDLE_VAL);
}

#endif /* INC_PLAYER_H_ */

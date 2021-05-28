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

#endif /* INC_PLAYER_H_ */

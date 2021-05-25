#include "stm32f1xx.h"
#include "gpio.h"
#include "efekt.h"
#include "bb.h"
#include <stdlib.h>

void Error_Handler(void);
void configureTIM1_PWMMode();
void SystemClock_Config(void);

static inline void setPWM(unsigned char val) {
	TIM1->CCR1 = val;
}

static inline void onPWM() {
	BB(TIM1->CR1, TIM_CR1_CEN) = 1;
}

static inline void offPWM() {
	BB(TIM1->CR1, TIM_CR1_CEN) = 0;
}

typedef struct Player {
	volatile int play_ms;
	int sample_rate;
	int sample_per_ms;
	volatile int next_sample;
	const unsigned char* data;
	int dataSize;
	int randomMixPeriod; // 0 disable this feature
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


Player* player;
volatile unsigned int delay = 0;// 10ms period

static inline int isInputHigh() {
	return GPIOB->IDR & GPIO_IDR_IDR0;
}

void delayRand2s() {
	delay = rand()%200;
}

int main(void) {
	SystemClock_Config();
	SysTick_Config(44000000/100); //10ms

	player = createPlayer(wave, sizeof(wave)/sizeof(wave[0]), 350);
	setEndPlayCallback(player, delayRand2s);

	BB(RCC->APB2ENR, RCC_APB2ENR_IOPBEN) = 1;
	gpio_pin_cfg(GPIOB, PB0, gpio_mode_input_floating);

	while (1) {
		/*
		 * algorytm:
		 * 1. sprawdzam pin
		 * 2. 	jesli aktywny to gram na losowy czas
		 * 		i cisza losowy czas (0,5-2)
		 */
		if ( isInputHigh() && !isPlaying(player) && !delay) {
			int time_ms = (rand()%4000)+2000; //(2-6s)
			playMS(player, time_ms);
		}
	}
}

__attribute__((interrupt)) void SysTick_Handler(void){
	if (delay) delay--;
}

int isPlaying(Player* player) {
	return player->play_ms;
}

Player* createPlayer(const unsigned char* data, int dataSize, int randomMixPeriodMs) {
	Player* player = malloc(sizeof(Player));

	player->play_ms = 0;
	player->next_sample = 0;
	player->sample_rate = 44000000/512;// 85937,5Hz
	player->sample_per_ms = player->sample_rate/1000;
	player->data = data;
	player->dataSize = dataSize;
	int sampleLen = dataSize/player->sample_per_ms;
	player->randomMixPeriod = randomMixPeriodMs>=sampleLen ?
							sampleLen :
							randomMixPeriodMs;
	player->endPlayCallback = NULL;

	configureTIM1_PWMMode();
	NVIC_ClearPendingIRQ(TIM1_CC_IRQn);
	NVIC_EnableIRQ(TIM1_CC_IRQn);

	setPWM(127);

	return player;
}

void playMS(Player* player, int time_ms) {
	if (player) {
		player->play_ms = time_ms;
		player->next_sample = 0;
		onPWM();
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
			offPWM();
			player->next_sample = 0;
			if (player->endPlayCallback)
				player->endPlayCallback();
		}
	}
}

__attribute__((interrupt)) void TIM1_CC_IRQHandler(void){
	if (TIM1->SR & TIM_SR_CC1IF) {
		TIM1->SR = ~TIM_SR_CC1IF;
		if (player) //TODO change to callback and not disanling pwm, only set to 127
			handleNextSample(player);
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

	TIM1->PSC = 1;
	TIM1->ARR = 255;
	TIM1->CCR1 = 127;
	TIM1->EGR = TIM_EGR_UG;
	TIM1->CR1 = TIM_CR1_ARPE;
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

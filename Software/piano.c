/*
 * Firmware para placa de piano
 *
 * CH32V002, saida de som em PC7, trimpot de calibração em PA1, teclas em PA2
 *
 * Daniel Quadros, SET-2026
 */

#include "ch32fun.h"

#include <stdio.h>
#include <string.h>

#define FALSE 0
#define TRUE  1

/*
Timer 2 pin mappings by AFIO->PCFR1 bits 16 to 14
	000 (default)
		D4		T2CH1ETR
		D3		T2CH2
		C0		T2CH3
		D7		T2CH4
	001
		C1		T2CH1ETR_
		D3		T2CH2_
		C0		T2CH3_
		D7		T2CH4_
	010
		C5		T2CH1ETR_
		C2		T2CH2_
		D2		T2CH3_
		C1		T2CH4_
	011
		C1		T2CH1ETR_
		C7		T2CH2        <--
		D6		T2CH3
		D5		T2CH4
	100
		C0		T2CH1ETR_
		C1		T2CH2_
		C3		T2CH3_
		B6		T2CH4_
	101
		A0		T2CH1ETR_
		A1		T2CH2_
		A2		T2CH3_
		A3		T2CH4_
	110
		B1		T2CH1ETR_
		A1		T2CH2_
		A2		T2CH3_
		A3		T2CH4_
	111
		D3		T2CH1ETR_
		D4		T2CH2_
		A2		T2CH3_
		A3		T2CH4_
*/

// Contador para gerar as notas, assume Clock = 48MHz, Prescaler = 100
// Valores atualizados conforme o trimpot de calibração
// Frequencia das notas: 523, 587, 659, 698, 784, 880, 987
// 0 = Do, 1 = Re, etc
const uint16_t NOTAS_DEFAULT[] = { 918, 818, 728, 688, 612, 545, 486 };
uint16_t NOTAS[7];

// Número de leituras para calcular o valor medio do ADC
#define NLEITURAS 5

// Limiares para detecção das teclas pelo ADC
const uint16_t limiar[] = { 3277, 2294, 1557, 1106, 860, 574, 250 };

// LED de debug
#define acende_led() GPIOC->BSHR |= (1<<6)
#define apaga_led() GPIOC->BSHR |= (1<<(16+6))

/*
 * configura TIM2 para operar em PWM
 * adaptado do exemplo tim2_pwm do ch32fun 
 */
void t2pwm_init( void )
{
	// Enable GPIOC, AFIO and TIM2 clocks
	RCC->APB2PCENR |= RCC_APB2Periph_GPIOC | RCC_APB2Periph_AFIO;
	RCC->APB1PCENR |= RCC_APB1Periph_TIM2;

	// Select Timer 2 pin mapping
	AFIO->PCFR1 &= ~(0x07 << 14);
	AFIO->PCFR1 |= (0x03 << 14);
	
	// PC7 is T2CH2, Output alt func, push-pull
	GPIOC->CFGLR &= 0x0FFFFFFF; //~(0xf<<(4*7));
	GPIOC->CFGLR |= 0x90000000;  //(GPIO_Speed_50MHz | GPIO_CNF_OUT_PP_AF)<<(4*7);

	// SMCFGR: default clk input is CK_INT
	// set TIM2 clock prescaler divider 
	TIM2->PSC = 99;  		// actual divider is PSC+1

	// for channel 2, let CCxS stay 00 (output), set OCxM to 110 (PWM I)
	// enabling preload causes the new pulse width in compare capture register only to come into effect 
	// when UG bit in SWEVGR is set (= initiate update) (auto-clears)
	TIM2->CHCTLR1 |= TIM2_CHCTLR1_OC2M_2 | TIM2_CHCTLR1_OC2M_1 | TIM2_CHCTLR1_OC2PE;

	// CTLR1: default is up, events generated, edge align
	// enable auto-reload of preload
	TIM2->CTLR1 |=  TIM2_CTLR1_ARPE;

	// Enable Channel outputs, keep default state LOW (TIM2_CCER_CC2P = 0)
	TIM2->CCER |= TIM2_CCER_CC2E;
}

/*
 * Inicia uma nota
*/
void toca_nota(int nota) {
	TIM2->ATRLR = NOTAS[nota];
	TIM2->CH2CVR = NOTAS[nota]/2;
	TIM2->CTLR1 |= TIM2_CTLR1_CEN;
	TIM2->SWEVGR |= TIM2_SWEVGR_UG;
}

/*
  Para de tocar a nota atual
*/
void para_nota() {
	TIM2->CH2CVR = 0; // para forçar LOW
	Delay_Ms(1);
	TIM2->CTLR1 &= ~TIM2_CTLR1_CEN;
}

/*
  Inicia ADC
*/
void adc_init() {

  // ADCCLK = 24 MHz => RCC_ADCPRE = 0: divide by 2
	RCC->CFGR0 = (RCC->CFGR0 & ~(RCC_ADCPRE | RCC_CFGR0_ADC_CLK_MODE | RCC_CFGR0_ADC_CLK_ADJ)) | RCC_ADCPRE_DIV2;
	
	// Enable GPIOA and ADC
	RCC->PB2PCENR |= RCC_ADCEN | RCC_IOPDEN;
	
	// PA2 is analog input chl 0
	GPIOA->CFGLR &= ~(GPIO_CFGLR_MASK<<(4*2));
	GPIOA->CFGLR |= (GPIO_CFGLR_IN_ANALOG<<(4*2));
	
	// PA1 is analog input chl 1
	GPIOA->CFGLR &= ~(GPIO_CFGLR_MASK<<(4*1));
	GPIOA->CFGLR |= (GPIO_CFGLR_IN_ANALOG<<(4*1));
	
	// Reset the ADC to init all regs
	RCC->PB2PRSTR |= RCC_ADC1RST;
	RCC->PB2PRSTR &= ~RCC_ADC1RST;
	
	// Turn on ADC module
	ADC1->CTLR2 |= ADC_ADON;

	// Set up single conversion on chl 0
	ADC1->RSQR1 = 0;
	ADC1->RSQR2 = 0;
	
	// set sampling time for channel 0 & 1
	ADC1->SAMPTR2 &= ~(ADC_SMP0<<(3*0));
	ADC1->SAMPTR2 |= 7<<(3*0);	// 0:7 => 3/9/15/30/43/57/73/241 cycles
	ADC1->SAMPTR2 &= ~(ADC_SMP0<<(3*1));
	ADC1->SAMPTR2 |= 7<<(3*1);	// 0:7 => 3/9/15/30/43/57/73/241 cycles

	// set External trigger event to SWSTART
	ADC1->CTLR2 = ADC1->CTLR2 & ~ADC_EXTSEL_SWSTART;
}

/*
  Faz uma leitura do ADC
	Retorna valor de 0 a 2^12-1 (4095)
*/
uint16_t adc_get( void )
{
	// start sw conversion (auto clears)
	ADC1->CTLR2 |= ADC_FLAG_STRT;
	
	// wait for conversion complete
	while(!(ADC1->STATR & ADC_EOC))
		;
	
	// get result
	return ADC1->RDATAR;
}

/*
  Seleciona canal do ADC
*/
void sel_adc_channel(uint16_t canal) {
	ADC1->RSQR3 = canal;	// 0-9 for 8 ext inputs and two internals
	Delay_Ms(10);
	adc_get();  // despreza a primeira leitura após mudar de canal
}

// Calcula o valor absoluto da diferença de dois uint16
uint16_t absdif(uint16_t a, uint16_t b) {
	if (a > b) {
		return a-b;
	} else {
		return b-a;
	}
}

// Retorna a media de NLEITURAS do ADC
uint16_t le_adc_media() {
  // Passo 1 - aguarda estabilizar
	uint16_t anterior = adc_get();
	Delay_Ms(5);
	uint16_t atual  = adc_get();
	while (absdif(anterior, atual) > 15) {
		anterior = atual;
		atual  = adc_get();
		Delay_Ms(5);
	}
	// Passo 2 - calcula a media
	int soma = 0;
	for (int i = 0; i < NLEITURAS; i++) {
		soma += adc_get();
	}
	return (uint16_t) (soma / NLEITURAS);
}


/*
Determina qual tecla está apertada (0 a 7, 0 = nenhuma)
*/
int le_tecla() {
	uint16_t leitura = le_adc_media();

	// determina a tecla correspondente a leitura
	for (int i = 0; i < 7; i++) {
		if (leitura > limiar[i]) {
			return i;
		}
	}
	return 7;
}

/*
Ajusta as notas conforme a leitura do trimpot de calibração
*/
void ajusta_notas() {
	sel_adc_channel(1);   // trimpot de calibração
	uint16_t trimpot = le_adc_media();  // 0 a 4095
	int16_t ajuste =  (trimpot >> 5) - 64;
	for (int i = 0; i < 7; i++) {
		NOTAS[i] = NOTAS_DEFAULT[i] + ajuste;
	}
	sel_adc_channel(0);   // Leitura de tecla
}

/*
Efetua o procedimento de calibração
*/
void calibra() {
	// Espera soltar a tecla
	while (le_tecla() != 0) {
		Delay_Ms(200); // debounce
	}
	while (le_tecla() == 0) {
		ajusta_notas();
		toca_nota(0); // DO
		Delay_Ms(200);
	}
	para_nota();
	while (le_tecla() != 0) {
		Delay_Ms(200); // debounce
	}
}

// Programa principal
int main() {
	// 48MHz internal clock
	SystemInit();
	Delay_Ms(10);

	// Inicia LED de debug
  RCC->APB2PCENR |= RCC_APB2Periph_GPIOC;
	GPIOC->CFGLR &= ~(0xf<<(4*6));
	GPIOC->CFGLR |= (GPIO_Speed_10MHz | GPIO_CNF_OUT_PP)<<(4*6);

	// Incia o PWM
	t2pwm_init();

	// Inicia ADC
	adc_init();

  // Testa se quer calibrar (tecla apertada ao ligar)
	sel_adc_channel(0);  // Leitura de tecla
	int tecla = le_tecla();
	if (tecla != 0) {
		calibra();
	}

	// Aplica a calibração
	ajusta_notas();

	// Laco principal
	int tecla_ant = 0;
	while (TRUE) {
		tecla = le_tecla();
		if (tecla != tecla_ant) {
			// Apertou ou soltou
			tecla_ant = tecla;
			if (tecla == 0) {
				para_nota();
			} else {
				toca_nota(tecla-1);
			}
			Delay_Ms(100); // debounce
		}
	}
}

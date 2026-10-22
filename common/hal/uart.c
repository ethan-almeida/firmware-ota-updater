/**
    uart.c

    UART driver for STM32. Contains init, transmit one char and transmit whole string functions


**/

#include "uart.h"
#include "stm32f407.h"

#ifndef UART_CLK_HZ
#define UART_CLK_HZ 16000000u
#endif

#ifndef UART_BAUD
#define UART_BAUD 115200u
#endif

#define USART1_TX_PIN 6u 
#define USART1_RX_PIN 7u 
#define USART1_AF     7u

void uart_init(void)
{
    RCC->AHB1ENR |= RCC_AHB1ENR_GPIOBEN;
    RCC->APB2ENR |= RCC_APB2ENR_USART1EN;
    (void)RCC->APB2ENR; 

    const uint32_t pins[2] = { USART1_TX_PIN, USART1_RX_PIN };
    for (unsigned i = 0; i < 2; i++) {
        GPIOB->MODER = (GPIOB->MODER & ~(3u << (pins[i] * 2u))) | (GPIO_MODER_ALT_FUNC << (pins[i] * 2u));
        GPIOB->AFR[0] = (GPIOB->AFR[0] & ~(0xFu << (pins[i] * 4u))) | (USART1_AF << (pins[i] * 4u));
    }

    USART1->BRR = (UART_CLK_HZ + (UART_BAUD / 2u)) / UART_BAUD;
    USART1->CR1 = USART_CR1_UE | USART_CR1_TE | USART_CR1_RE;
}

void uart_putc(char c)
{
    while ((USART1->SR & USART_SR_TXE) == 0u) {

    }
    USART1->DR = (uint16_t)c;
}

void uart_puts(const char *s)
{
    while (*s != '\0') {
        if (*s == '\n') {
            uart_putc('\r');
        }
        uart_putc(*s);
        s++;
    }
}

void uart_deinit(void)
{
    while ((USART1->SR & USART_SR_TC) == 0u) {

    }
    USART1->CR1 = 0u;
}

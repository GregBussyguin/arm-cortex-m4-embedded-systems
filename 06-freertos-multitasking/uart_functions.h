// uart_functions.h
// Desenvolvido para a placa EK-TM4C1294XL
// Funções relacionadas a UART
// Edson, Fabio, Joao Victor
// 11/02/2025

#ifndef UART_FUNCTIONS_H
#define UART_FUNCTIONS_H
#endif

#include <stdint.h>

void UARTStringSend(const uint8_t *String);
void UART_Interruption_Handler(void);

/*##############################################################################
Acyr M. & Gregório B.
Desenvolvido para a placa EK-TM4C1294XL com SDK TivaWare e FreeRTOS
##############################################################################*/

/* TivaWare uC: Usado internamente para identificar o uC em alguns .h da TivaWare */
#define PART_TM4C1294NCPDT 1

/* Bibliotecas Base */
#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include "inc/hw_memmap.h"
#include "driverlib/debug.h"
#include "driverlib/gpio.h"
#include "driverlib/sysctl.h"
#include "driverlib/systick.h"
#include "driverlib/pin_map.h"
#include "driverlib/interrupt.h"
#include "driverlib/uart.h"
#include "driverlib/pwm.h"
#include "driverlib/i2c.h"

/* Bibliotecas UART */
#include "relay_functions.h"
#include "uart_functions.h"

/* Bibliotecas periféricos */
#include "i2c.h"
//#include "ssd1306.h"
#include "bitmap.h"
#include "teclado.h"

/* Bibliotecas FreeRTOS */
#include <math.h>
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "semphr.h"

/* Variável Global com o valor do SystemClock: */
uint32_t SystemCoreClock=16000000;

/* Variáveis para armazenamento dos handles da Tasks */
TaskHandle_t vTaskGateKeeperHandle = NULL;
TaskHandle_t vTaskSensorHandle = NULL;
TaskHandle_t vTaskPWMHandle = NULL;
TaskHandle_t vTaskReleHandle = NULL;

/* Protótipos das Tasks */
void vTask1(void *pvParameters);

/* Definições */
#define GATEKEEPER_QUEUE_LENGTH  10
#define SENSOR_QUEUE_LENGTH  5
#define PWM_QUEUE_LENGTH  5
#define RELE_QUEUE_LENGTH  5

/* Estrutura do pacote de comunicação */
typedef struct {
    uint8_t startByte;
    char command[2];
    uint8_t data[8];
    uint16_t checksum;
    uint8_t stopByte;
} CommandPacket;



//variável para receber o retorno do cfg do clk
uint32_t SysClock;

//variável que conta os ticks(1ms) - Volatile não permite o compilador otimizar o código 
static volatile unsigned int SysTicks1ms;

/* Filas para comunicação entre tarefas */
QueueHandle_t xQueueGateKeeper;
QueueHandle_t xQueueSensor;
QueueHandle_t xQueuePWM;
QueueHandle_t xQueueRele;
QueueHandle_t xQueueResponse;

/* ---------- Protótipos ---------- */
/* Protótipos das tarefas */
void vTaskGateKeeper(void *pvParameters);
void vTaskSensor(void *pvParameters);
void vTaskPWM(void *pvParameters);
void vTaskRele(void *pvParameters);

/* Protótipos das funções */
void Ports_setup(void);
void PWM0_setup(void);
void SetupUart(void);
void I2C0_config(void);
void I2C7_config(void);
void SetupSystick(void);
void SysTickIntHandler(void);
void setup(void);
void UARTStringSend(const uint8_t *String);
void ConvertPacketToString(CommandPacket *responsePacket, uint8_t *buffer);

/* ---------- Implementação das tarefas ---------- */

/* Variável global para armazenar o estado dos relés */
bool statusRelays[3] = {0, 0, 0};
/* Variável global para armazenar a intensidade do LED */
char ledIntensity[2] = {'0', '0'};

void vTaskGateKeeper(void *pvParameters) {
    CommandPacket receivedPacket, responsePacket;
    BaseType_t xStatus;
    uint8_t bufferIndex = 0;
    uint8_t uartBuffer[sizeof(CommandPacket)];
    QueueHandle_t targetQueue = NULL;

    while (1) {
        bufferIndex = 0;

        // Aguardar um comando via UART utilizando interrupção
        while (bufferIndex < sizeof(CommandPacket)) {
            int receivedByte = UARTCharGetNonBlocking(UART0_BASE);
            if (receivedByte != -1) {
                uartBuffer[bufferIndex++] = (uint8_t)receivedByte;
            }
        }

        // Copiar os dados recebidos para a estrutura `receivedPacket`
        memcpy(&receivedPacket, uartBuffer, sizeof(CommandPacket));

        // Verificar qual fila deve receber o comando
        if (strncmp(receivedPacket.command, "TS", 2) == 0) {
            targetQueue = xQueueSensor;
        } else if (strncmp(receivedPacket.command, "PS", 2) == 0) {
            targetQueue = xQueuePWM;
        } else if (strncmp(receivedPacket.command, "RA", 2) == 0) {
            targetQueue = xQueueRele;
        }

        if (targetQueue != NULL) {
            xQueueSend(targetQueue, &receivedPacket, portMAX_DELAY);
            xStatus = xQueueReceive(xQueueResponse, &responsePacket, portMAX_DELAY);
        } else {
            responsePacket.startByte = '#';
            strncpy(responsePacket.command, receivedPacket.command, 2);
            memset(responsePacket.data, 0, 8);
            responsePacket.checksum = 0xFFFF;
            responsePacket.stopByte = '@';
            xStatus = pdPASS;
        }

        // Enviar resposta via UART apenas se recebida corretamente
        if (xStatus == pdPASS) {
            uint8_t uartResponseBuffer[sizeof(CommandPacket)];
            ConvertPacketToString(&responsePacket, uartResponseBuffer);
            UARTStringSend(uartResponseBuffer);
        }
    }
}



void vTaskSensor(void *pvParameters) {
    CommandPacket receivedPacket, responsePacket;
    float temperatura;

    while (1) {
        if (xQueueReceive(xQueueSensor, &receivedPacket, portMAX_DELAY) == pdPASS) {
            if (receivedPacket.startByte == '#' && receivedPacket.stopByte == '@') {
                float receivedValue;
                memcpy(&receivedValue, receivedPacket.data, sizeof(float));
                temperatura = (float)(receivedValue * (7.0 / 3.0) * sqrt(5));
                
								// Gera mensagem de resposta
                responsePacket.startByte = '#';
								strcpy(responsePacket.command, "TR");
                memcpy(responsePacket.data, &temperatura, sizeof(float));
                snprintf((char*)&responsePacket.data[4], 4, "%.2f", temperatura);
                responsePacket.checksum = 0; // Calcular checksum
                responsePacket.stopByte = '@';
                
								// Manda mensagem de resposta para o gatekeeper
                xQueueSend(xQueueGateKeeper, &responsePacket, portMAX_DELAY);
            }
        }
    }
}


void vTaskPWM(void *pvParameters) {
    CommandPacket receivedPacket, responsePacket;

    while (1) {
        if (xQueueReceive(xQueuePWM, &receivedPacket, portMAX_DELAY) == pdPASS) {
            if (receivedPacket.startByte == '#' && receivedPacket.stopByte == '@') {
                ledIntensity[0] = receivedPacket.data[0];
                ledIntensity[1] = receivedPacket.data[1];
                
								// Gera mensagem de resposta
                responsePacket.startByte = '#';
								strcpy(responsePacket.command, "PR");
                responsePacket.data[0] = ledIntensity[0];
                responsePacket.data[1] = ledIntensity[1];
                responsePacket.checksum = 0; // Calcular checksum
                responsePacket.stopByte = '@';
                
								// Manda mensagem de resposta para o gatekeeper
                xQueueSend(xQueueGateKeeper, &responsePacket, portMAX_DELAY);
            }
        }
    }
}


void vTaskRele(void *pvParameters) {
    CommandPacket receivedPacket, responsePacket;

    while (1) {
        if (xQueueReceive(xQueueRele, &receivedPacket, portMAX_DELAY) == pdPASS) {
            if (receivedPacket.startByte == '#' && receivedPacket.stopByte == '@') {
                statusRelays[0] = receivedPacket.data[0] & 0x01;
                statusRelays[1] = receivedPacket.data[1] & 0x01;
                statusRelays[2] = receivedPacket.data[2] & 0x01;
                
								// Gera mensagem de resposta
                responsePacket.startByte = '#';
								strcpy(responsePacket.command, "RR");
                responsePacket.data[0] = statusRelays[0];
                responsePacket.data[1] = statusRelays[1];
                responsePacket.data[2] = statusRelays[2];
                responsePacket.checksum = 0; // Calcular checksum
                responsePacket.stopByte = '@';
                
								// Manda mensagem de resposta para o gatekeeper
                xQueueSend(xQueueGateKeeper, &responsePacket, portMAX_DELAY);
            }
        }
    }
}


/* ---------- Funções de Setup ---------- */
//função setup
void setup(void) {
  //SysClock = SysCtlClockFreqSet((SYSCTL_XTAL_25MHZ | SYSCTL_OSC_MAIN | SYSCTL_USE_PLL | SYSCTL_CFG_VCO_240), 120000000);
  SetupSystick();
	Ports_setup();
	//I2C0_config();
	//OLED0_Init();
	//SSD1306_I2C0_cls();
  SetupUart();
}
//função para setup das portas
void Ports_setup(void){
	//port F
	SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOF);
	while(!SysCtlPeripheralReady(SYSCTL_PERIPH_GPIOF)) {/*espera habilitar o port*/}
  GPIOPinTypeGPIOOutput(GPIO_PORTF_BASE, GPIO_PIN_4);
	PWM0_setup();
	
	//Port L
	SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOL);
	while(!SysCtlPeripheralReady(SYSCTL_PERIPH_GPIOL)) {/*Espera habilitar o port*/}
	GPIOPinTypeGPIOInput(GPIO_PORTL_BASE, GPIO_PIN_0 | GPIO_PIN_1 | GPIO_PIN_2 | GPIO_PIN_3);	//configura os pinos para 2mA como limite de corrente e com week pull-up
	GPIOPadConfigSet(GPIO_PORTL_BASE, GPIO_PIN_0 | GPIO_PIN_1 | GPIO_PIN_2 | GPIO_PIN_3, GPIO_STRENGTH_2MA, GPIO_PIN_TYPE_STD_WPU);

	//Port M
	SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOM);
	while(!SysCtlPeripheralReady(SYSCTL_PERIPH_GPIOM)) {/*Espera habilitar o port*/}
	GPIOPinTypeGPIOOutput(GPIO_PORTM_BASE, GPIO_PIN_4 | GPIO_PIN_5 | GPIO_PIN_6  | GPIO_PIN_7);
	
	//Port N
	SysCtlPeripheralEnable(SYSCTL_PERIPH_GPION);
	while(!SysCtlPeripheralReady(SYSCTL_PERIPH_GPION)) {/*espera habilitar o port*/}
  GPIOPinTypeGPIOOutput(GPIO_PORTN_BASE, GPIO_PIN_0 | GPIO_PIN_1);

}

//função para setup PWM 0
void PWM0_setup(void) {
	//habilitar pwm 1
	SysCtlPeripheralEnable(SYSCTL_PERIPH_PWM0);
	//aguarda o periférico ficar pronto para uso	
	while(!SysCtlPeripheralReady(SYSCTL_PERIPH_PWM0)) {/*espera habilitar o pwm*/}
	//configura o pin_0 como PWM
	GPIOPinConfigure(GPIO_PF0_M0PWM0);
	GPIOPinTypePWM(GPIO_PORTF_BASE, GPIO_PIN_0);
	// Configura o gerador PWM
	PWMGenConfigure(PWM0_BASE, PWM_GEN_0, PWM_GEN_MODE_DOWN | PWM_GEN_MODE_NO_SYNC);
	// Configura o periodo. Para uma frequencia de 50 KHz, o periodo = 1/50,000, or 20us. 
	// Para um clock de 120 MHz, resulta em 2400 ticks.
	PWMGenPeriodSet(PWM0_BASE, PWM_GEN_0, 2400);
	// Configurando o pulso de PWM0 para 50% de duty cycle.
  PWMPulseWidthSet(PWM0_BASE, PWM_OUT_0, 1200);
	// Configurando o pulso de PWM1 para 50% de duty cycle.
	PWMPulseWidthSet(PWM0_BASE, PWM_OUT_1, 1200);
	// Inicia o timer no gerador em 0
  PWMGenEnable(PWM0_BASE, PWM_GEN_0);
	// Habilita a saida.
	PWMOutputState(PWM0_BASE, (PWM_OUT_0_BIT | PWM_OUT_1_BIT), true);
}


//função para setup I2C0
void I2C0_config(void) {
	// Configure I2C0 for pins PB2 and PB3
	SysCtlPeripheralEnable(SYSCTL_PERIPH_I2C0);
	SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOB);
	while (!SysCtlPeripheralReady(SYSCTL_PERIPH_I2C0) || !SysCtlPeripheralReady(SYSCTL_PERIPH_GPIOB));  
	//Configuração da PORTB PIN2 e PIN3 - B.M.
	GPIOPinConfigure(GPIO_PB2_I2C0SCL);
	GPIOPinConfigure(GPIO_PB3_I2C0SDA);
	GPIOPinTypeI2CSCL(GPIO_PORTB_BASE, GPIO_PIN_2);
	GPIOPinTypeI2C(GPIO_PORTB_BASE, GPIO_PIN_3);
	I2CMasterInitExpClk(I2C0_BASE, SysCtlClockGet(), false);
}


//função para setup I2C7
void I2C7_config(void) {
	SysCtlPeripheralEnable(SYSCTL_PERIPH_I2C7);
	SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOD);
	while (!SysCtlPeripheralReady(SYSCTL_PERIPH_I2C7) || !SysCtlPeripheralReady(SYSCTL_PERIPH_GPIOD));  
	//Configuração da PORTD PIN0 e PIN1 - B.M.
	GPIOPinConfigure(GPIO_PD0_I2C7SCL);
	GPIOPinConfigure(GPIO_PD1_I2C7SDA);
	GPIOPinTypeI2CSCL(GPIO_PORTD_BASE, GPIO_PIN_0);
	GPIOPinTypeI2C(GPIO_PORTD_BASE, GPIO_PIN_1);
	I2CMasterInitExpClk(I2C7_BASE, SysCtlClockGet(), false);
}


//função para setup Uart
void SetupUart(void) {
    // Habilitar o periférico UART0
    SysCtlPeripheralEnable(SYSCTL_PERIPH_UART0);
    while (!SysCtlPeripheralReady(SYSCTL_PERIPH_UART0));

    // Configurar a UART0 com baud rate de 115200, 8 bits de dados, sem paridade, 1 stop bit
    UARTConfigSetExpClk(UART0_BASE, SysClock, 115200, 
                        (UART_CONFIG_WLEN_8 | UART_CONFIG_STOP_ONE | UART_CONFIG_PAR_NONE));

    // Desabilitar FIFO para leitura byte a byte
    UARTFIFODisable(UART0_BASE);

    // Habilitar o clock para GPIOA (onde os pinos RX e TX da UART0 estão mapeados)
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOA);
    while (!SysCtlPeripheralReady(SYSCTL_PERIPH_GPIOA));

    // Configurar os pinos PA0 (RX) e PA1 (TX) para função UART
    GPIOPinConfigure(GPIO_PA0_U0RX);
    GPIOPinConfigure(GPIO_PA1_U0TX);
    GPIOPinTypeUART(GPIO_PORTA_BASE, GPIO_PIN_0 | GPIO_PIN_1);
}
//função para configurar e inicializar o periférico Systick a 1ms
void SetupSystick(void){
  SysTicks1ms=0;
  //desliga o SysTick para poder configurar
  SysTickDisable();
  //clock 40MHz <=> SysTick deve contar 1ms=80k-1 do Systick_Counter - 12 trocas de contexto PP->IRQ - (1T Mov, 1T Movt, 3T LDR, 1T INC ... STR e IRQ->PP já não contabilizam atrasos para a variável)  
  SysTickPeriodSet(80000-1-12-6);
  //registra a função de atendimento da interrupção
  SysTickIntRegister(SysTickIntHandler);
  //liga o atendimento via interrupção
  SysTickIntEnable();
  //liga novamente o SysTick
  SysTickEnable();
}

//função de tratamento da interrupção do SysTick
void SysTickIntHandler(void){
  SysTicks1ms++;
}




/* ---------- Funções de Extra ---------- */
void UARTStringSend(const uint8_t *String) {
    // Calcular o tamanho da string
    uint32_t tamanho = 0;
    while (String[tamanho] != '\0') {
        tamanho++;
    }

    // Enviar a string caractere por caractere
    while (tamanho--) {
        UARTCharPut(UART0_BASE, *String++);
    }
}
void ConvertPacketToString(CommandPacket *responsePacket, uint8_t *buffer) {
    // Copiar o startByte
    buffer[0] = responsePacket->startByte;

    // Copiar o comando (2 primeiros caracteres)
    strncpy((char *)&buffer[1], responsePacket->command, 2);

    // Copiar os dados (8 caracteres)
    memcpy(&buffer[3], responsePacket->data, 8);

    // Copiar o checksum (supondo que seja um valor de 2 bytes)
    buffer[11] = (responsePacket->checksum >> 8) & 0xFF;  // byte alto
    buffer[12] = responsePacket->checksum & 0xFF;         // byte baixo

    // Copiar o stopByte
    buffer[13] = responsePacket->stopByte;

    // Não esquecer de adicionar o caractere nulo no final da string
    buffer[14] = '\0';  // Terminador de string
}

/* ---------- MAIN ---------- */
int main(void)
{
		BaseType_t xReturnedGK, xReturnedS, xReturnedP, xReturnedR;
		/* função de Setup */
		setup();
	
		/* Criação das filas */
		xQueueGateKeeper = xQueueCreate(GATEKEEPER_QUEUE_LENGTH, sizeof(CommandPacket));
		xQueueSensor = xQueueCreate(SENSOR_QUEUE_LENGTH, sizeof(CommandPacket));
		xQueuePWM = xQueueCreate(PWM_QUEUE_LENGTH, sizeof(CommandPacket));
		xQueueRele = xQueueCreate(RELE_QUEUE_LENGTH, sizeof(CommandPacket));
		
		/* Criação das tarefas */
		xReturnedGK = xTaskCreate(vTaskGateKeeper, "GateKeeper", configMINIMAL_STACK_SIZE, NULL, 3, NULL);
		xReturnedS = xTaskCreate(vTaskSensor, "Sensor", configMINIMAL_STACK_SIZE, NULL, 2, NULL);
		xReturnedP = xTaskCreate(vTaskPWM, "PWM", configMINIMAL_STACK_SIZE, NULL, 2, NULL);
		xReturnedR = xTaskCreate(vTaskRele, "Rele", configMINIMAL_STACK_SIZE, NULL, 2, NULL);
		
	
	  if (xReturnedGK == pdFAIL ||
				xReturnedS == pdFAIL ||
				xReturnedP == pdFAIL ||
				xReturnedR == pdFAIL)
     {
      while(1);
     }
		 
		vTaskStartScheduler();
}


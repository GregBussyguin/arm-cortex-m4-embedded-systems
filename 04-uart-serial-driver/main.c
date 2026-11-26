/*##############################################################################
Acyr Eduardo Marconato e Gregório 
Lab4 - Comunicacao Serial
##############################################################################*/
#define PART_TM4C1294NCPDT 1

#include <stdint.h>
#include <stdio.h>
#include <stdbool.h>
#include "inc/hw_memmap.h"
#include "driverlib/debug.h"
#include "driverlib/gpio.h"
#include "driverlib/sysctl.h"
#include "driverlib/pin_map.h"
#include "driverlib/systick.h"
#include "driverlib/uart.h"
	

//-----------------------------------------------------------------------//
//---Setup---//
void UARTHandler(void);

//clock do sistema
uint32_t g_ui32SysClock;

//Numero de Ticks de 1ms
static volatile unsigned int SysTicks1ms;

//fun��o de tratamento da interrup��o do SysTick
void SysTickIntHandler(void)
{
  SysTicks1ms++;
}

//fun��o para configurar e inicializar o perif�rico Systick a 1ms
void SetupSystick()
{
  //Configura o clock do sistema para 25MHz
  g_ui32SysClock = SysCtlClockFreqSet((SYSCTL_XTAL_25MHZ | SYSCTL_OSC_MAIN | SYSCTL_USE_PLL | SYSCTL_CFG_VCO_240), 120000000);
	
  SysTicks1ms=0;
  SysTickDisable();
  SysTickPeriodSet(120000-1-12-6);
  SysTickIntRegister(SysTickIntHandler);
  SysTickIntEnable();
  SysTickEnable();
}

void Setup_Inicial()
{
  //Ports
	//LEDs - PN0,PN1,PF0,PF4
	//UART -PA0(RX), PA1(TX)

  SysCtlPeripheralEnable(SYSCTL_PERIPH_GPION);  //GPIO port N
  while(!SysCtlPeripheralReady(SYSCTL_PERIPH_GPION)) {}
  SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOF);  //GPIO port F
  while(!SysCtlPeripheralReady(SYSCTL_PERIPH_GPIOF)) {}   
  
  //Define como entrada ou saida
  GPIOPinTypeGPIOOutput(GPIO_PORTN_BASE, GPIO_PIN_0 | GPIO_PIN_1); //lED PN0,PN1
  GPIOPinTypeGPIOOutput(GPIO_PORTF_BASE, GPIO_PIN_0 | GPIO_PIN_4); //LED PF0,PF4
	
  
  /*-----UART----*/
  // Habilitando a UART0
		
  SysCtlPeripheralEnable(SYSCTL_PERIPH_UART0);
  while(!SysCtlPeripheralReady(SYSCTL_PERIPH_UART0))  {/*Espera habilitar a uart0*/}
  UARTConfigSetExpClk(UART0_BASE, g_ui32SysClock, 115200, (UART_CONFIG_WLEN_8 | UART_CONFIG_STOP_ONE | UART_CONFIG_PAR_NONE));
	UARTFIFODisable(UART0_BASE);
  UARTIntEnable(UART0_BASE,UART_INT_RX);
  UARTIntRegister(UART0_BASE,UARTHandler);
  SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOA);
  while(!SysCtlPeripheralReady(SYSCTL_PERIPH_GPIOA));

  //Configurando os pinos para o modo UART
  GPIOPinConfigure(GPIO_PA0_U0RX); //pino RX
  GPIOPinConfigure(GPIO_PA1_U0TX); //pino TX
  GPIOPinTypeUART(GPIO_PORTA_BASE, GPIO_PIN_0 | GPIO_PIN_1); //seta PA0 e PA1  
  
}


//-----------------------------------------------------------------------//
//---Funcoes de Rale---//

bool Led1=false,Led2=false,Led3=false,Led4=false; //bool para os Leds
unsigned char msg[4]; //recebe msg na UART
int interrupt = 1;


void Rale1(unsigned char msg[4]){
	if(msg[3]=='1'){ //ligar
		Led1=true;
		GPIOPinWrite(GPIO_PORTN_BASE, GPIO_PIN_1, Led1<<1);
	}
	else{ //desligar 
		Led1=false;
		GPIOPinWrite(GPIO_PORTN_BASE, GPIO_PIN_1, Led1<<1);
	}  
}

void Rale2(unsigned char msg[4]){
	if(msg[3]=='1'){ //ligar 
		Led2=true;
		GPIOPinWrite(GPIO_PORTN_BASE, GPIO_PIN_0, Led2);
	}
	else{ //desligar 
		Led2=false;
		GPIOPinWrite(GPIO_PORTN_BASE, GPIO_PIN_0, Led2);
	}       
}

void Rale3 (unsigned char msg [4]){
	if(msg[3]=='1'){ //ligar
		Led3=true;
		GPIOPinWrite(GPIO_PORTF_BASE, GPIO_PIN_4, Led3<<4);
	}
	else{ //desligar
		Led3=false;
		GPIOPinWrite(GPIO_PORTF_BASE, GPIO_PIN_4, Led3<<4);
	}   
}

void Rale4 (unsigned char msg[4]){
	if(msg[3]=='1'){ //ligar 
		Led4=true;
		GPIOPinWrite(GPIO_PORTF_BASE, GPIO_PIN_0, Led4);
	}
	else{ //desligar 
		Led4=false;
		GPIOPinWrite(GPIO_PORTF_BASE, GPIO_PIN_0, Led4);
	}    
	
}	

void Tx1(unsigned char msg[4]){
	Led1=true;
	Led2=true;
	Led3=true;
	Led4=true;
	GPIOPinWrite(GPIO_PORTN_BASE, GPIO_PIN_1, Led1<<1);
	GPIOPinWrite(GPIO_PORTN_BASE, GPIO_PIN_0, Led2);
	GPIOPinWrite(GPIO_PORTF_BASE, GPIO_PIN_4, Led3<<4);
	GPIOPinWrite(GPIO_PORTF_BASE, GPIO_PIN_0, Led4);
}

void Tx0 (unsigned char msg[4]){
	Led1=false;
	Led2=false;
	Led3=false;
	Led4=false;
	GPIOPinWrite(GPIO_PORTN_BASE, GPIO_PIN_1, Led1<<1);
	GPIOPinWrite(GPIO_PORTN_BASE, GPIO_PIN_0, Led2);
	GPIOPinWrite(GPIO_PORTF_BASE, GPIO_PIN_4, Led3<<4);
	GPIOPinWrite(GPIO_PORTF_BASE, GPIO_PIN_0, Led4);
	
}

void UARTHandler (void){
  uint8_t last;
  //limpar IRQ exec
  UARTIntClear(UART0_BASE,UARTIntStatus(UART0_BASE,true));
  // Ler o próximo caractere na uart.
  last = (uint8_t)UARTCharGetNonBlocking(UART0_BASE);
  //rotacionar buffer circular
  msg[0]=msg[1];
  msg[1]=msg[2];
  msg[2]=msg[3];
  msg[3]=last;
	
	interrupt++;
}

void UARTStringSend(const uint8_t *String, uint32_t tamanho){
	while (tamanho--) UARTCharPut(UART0_BASE, *String++);
}

//-----------------------------------------------------------------------//
//---Main---//


int main(void){
	
  //RODA AS INICIALIZA��ES
  SetupSystick();
  Setup_Inicial();
	
  //main loop
  while (true){
		if (!(interrupt%5)){
						if(	//verifica se tem algo na UART
				msg[0]!= NULL &&
				msg[1]!= NULL &&
				msg[2]!= NULL &&
				msg[3]!= NULL) 
			{
				//Chacando o comando
				if(	
					(msg[0]=='#')&&
					(msg[1]=='R' || msg[1]=='T')&&
					(msg[2]=='0'||msg[2]=='1'||msg[2]=='2'||msg[2]=='3'||msg[2]=='X')&&
					(msg[3]=='0' || msg[3]=='1')){
					// Checa qual Rale
					if(msg[2]=='0'){
						Rale1(msg);         
					}
					
					if(msg[2]=='1'){
						Rale2(msg);            
					}
					
					if(msg[2]=='2'){
						Rale3(msg);
					}
					
					if(msg[2]=='3'){
						Rale4(msg);
					}
					
					// Checa para Tx
					if((msg[1]=='T')&&(msg[2]=='X')){
						if (msg[3]=='1'){// #TX1 Liga
							Tx1(msg);
						}
						
						if (msg[3]=='0'){ // #TX0 desligar
							Tx0(msg);
						}
					}
							
					//resposta
					
				} //Chacando o comando
			} //verifica se tem algo na UART
			msg[0]='@';
			UARTStringSend(msg, 4);
			interrupt = 1;
		}
	} //While(true)
} //main


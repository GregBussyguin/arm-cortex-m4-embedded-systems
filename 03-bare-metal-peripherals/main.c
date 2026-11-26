#define PART_TM4C1294NCPDT 1
#include <stdint.h>
#include <stdio.h>
#include <stdbool.h>
#include "inc/hw_ints.h"
#include "inc/hw_memmap.h"
#include "driverlib/debug.h"
#include "driverlib/gpio.h"
#include "driverlib/sysctl.h"
#include "driverlib/systick.h"
#include "driverlib/interrupt.h"
#include "driverlib/pin_map.h"
#include "driverlib/rom.h"
#include "driverlib/rom_map.h"
#include "driverlib/uart.h"

static volatile unsigned int SysTicks1ms;
unsigned char rxbuffer[4];
uint32_t SysClock;
void SysTickIntHandler(void);
void UARTStringSend(char *String, uint32_t tamanho);
void UART_Interruption_Handler(void);
void SetupSystick(void);
void SetupUart(void);
void SysTick_Wait1ms(uint32_t delay);

void colunas(uint8_t x);
uint8_t linhas(void);
uint8_t Leitura_Teclado(void);

void LCD_init(void);
void LCD_Command(uint8_t x);
void LCD_Print(uint8_t x);
void LCD_Clear(void);
void new_line(void);
void LCD_Print_String(char mensagem[31]);


int main(void)
{
  SysClock = SysCtlClockFreqSet((SYSCTL_XTAL_25MHZ | SYSCTL_OSC_MAIN | SYSCTL_USE_PLL | SYSCTL_CFG_VCO_240), 120000000);
  SetupSystick();  
  SysCtlPeripheralEnable(SYSCTL_PERIPH_GPION);
  while(!SysCtlPeripheralReady(SYSCTL_PERIPH_GPION)) {}
  SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOF);
  while(!SysCtlPeripheralReady(SYSCTL_PERIPH_GPIOF)) {}
  SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOJ);
  while(!SysCtlPeripheralReady(SYSCTL_PERIPH_GPIOJ)) {}
  GPIOPinTypeGPIOOutput(GPIO_PORTN_BASE, GPIO_PIN_0 | GPIO_PIN_1);
  GPIOPinTypeGPIOOutput(GPIO_PORTF_BASE, GPIO_PIN_0 | GPIO_PIN_4);
  GPIOPinTypeGPIOInput(GPIO_PORTJ_BASE, GPIO_PIN_0 | GPIO_PIN_1);
  GPIOPadConfigSet(GPIO_PORTJ_BASE, GPIO_PIN_0 | GPIO_PIN_1, GPIO_STRENGTH_2MA, GPIO_PIN_TYPE_STD_WPU);
  SetupUart(); 
	SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOL);
  while(!SysCtlPeripheralReady(SYSCTL_PERIPH_GPIOL)) {}
	SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOM);
  while(!SysCtlPeripheralReady(SYSCTL_PERIPH_GPIOM)) {}
  GPIOPinTypeGPIOOutput(GPIO_PORTL_BASE, GPIO_PIN_0 | GPIO_PIN_1 | GPIO_PIN_2 | GPIO_PIN_3);
  GPIOPinTypeGPIOInput(GPIO_PORTM_BASE, GPIO_PIN_7 | GPIO_PIN_6 | GPIO_PIN_5 | GPIO_PIN_4);
  GPIOPadConfigSet(GPIO_PORTM_BASE, GPIO_PIN_7 | GPIO_PIN_6 | GPIO_PIN_5 | GPIO_PIN_4, GPIO_STRENGTH_2MA, GPIO_PIN_TYPE_STD_WPU);
	SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOK);
  while(!SysCtlPeripheralReady(SYSCTL_PERIPH_GPIOK)) {}
  GPIOPinTypeGPIOOutput(GPIO_PORTK_BASE, 0xFF);
	GPIOPinTypeGPIOOutput(GPIO_PORTM_BASE, 0x07);
	LCD_init();
	LCD_Clear();
	bool db=true;
	unsigned int dbtime=0;
	#define dbt 100;
	uint8_t Teclado = '0';
	uint8_t Caracteres = 0;
	char string[10]; 
	char LCD_msg[31];
  while (1)
        {		
				Teclado = Leitura_Teclado();
          if (db) 
	     {
	       if ((SysTicks1ms>=dbtime))
	          {
                    db=false;
                    dbtime=SysTicks1ms+dbt;
	          }
	     }
          else
	     {
	       if (((Teclado)!=0)&&(SysTicks1ms>=dbtime))
	          {
	            db=true;
	            dbtime=SysTicks1ms+dbt;
										Caracteres = sprintf(string,"%c",Teclado);
										UARTStringSend(string, Caracteres);
										sprintf(LCD_msg,"Botao press. = %c                 ",Teclado);
										LCD_Clear();
										LCD_Print_String(LCD_msg);
										
			}
		}
	}       
}

void colunas(uint8_t x)
{
		GPIOPinWrite(GPIO_PORTL_BASE, 0x0F,x);
		return;
}

uint8_t linhas()
{
		uint8_t x = GPIOPinRead(GPIO_PORTM_BASE,0xF0);
		x = x | 0x0F;
		return x;
}
uint8_t Leitura_Teclado()
{
		uint8_t key = 0;
    const uint8_t columnPatterns[4] = {0x0E, 0x0D, 0x0B, 0x07};
    const char keys[4][4] = {
        {'7', '8', '9', 'A'},
        {'4', '5', '6', 'B'},
        {'1', '2', '3', 'C'},
        {'#', '0', '*', 'D'}
    };
    for (uint8_t col = 0; col < 4; col++) {
        colunas(columnPatterns[col]);
        uint8_t rowStatus = linhas();
        for (uint8_t row = 0; row < 4; row++) {
            if ((rowStatus & (0x10 << row)) == 0) {
                key = keys[col][row];
                break;
            }
        }
        if (key) break;
    }
    return key;
}
void LCD_init (){
		GPIOPinWrite(GPIO_PORTK_BASE, 0xFF,0x38);
		SysTick_Wait1ms (2);
		GPIOPinWrite(GPIO_PORTM_BASE, 0x07,0x04);
		SysTick_Wait1ms (1);
		GPIOPinWrite(GPIO_PORTM_BASE, 0x07,0x00);
		GPIOPinWrite(GPIO_PORTK_BASE, 0xFF,0x06);
		SysTick_Wait1ms (2);
		GPIOPinWrite(GPIO_PORTM_BASE, 0x07,0x04);
		SysTick_Wait1ms (1);
		GPIOPinWrite(GPIO_PORTM_BASE, 0x07,0x00);
		GPIOPinWrite(GPIO_PORTK_BASE, 0xFF,0x0C);
		SysTick_Wait1ms (2);
		GPIOPinWrite(GPIO_PORTM_BASE, 0x07,0x04);
		SysTick_Wait1ms (1);
		GPIOPinWrite(GPIO_PORTM_BASE, 0x07,0x00);
		GPIOPinWrite(GPIO_PORTK_BASE, 0xFF,0x01);
		SysTick_Wait1ms (2);
		GPIOPinWrite(GPIO_PORTM_BASE, 0x07,0x04);
		SysTick_Wait1ms (1);
		GPIOPinWrite(GPIO_PORTM_BASE, 0x07,0x00);
		return;
}	

void LCD_Command(uint8_t x)
{
		GPIOPinWrite(GPIO_PORTK_BASE, 0xFF,x);
		SysTick_Wait1ms (2);
		GPIOPinWrite(GPIO_PORTM_BASE, 0x07,0x04);
		SysTick_Wait1ms (1);
	
		GPIOPinWrite(GPIO_PORTM_BASE, 0x07,0x00);
	
		return;
}	
void LCD_Print(uint8_t x)
{
		GPIOPinWrite(GPIO_PORTK_BASE, 0xFF,x);
		SysTick_Wait1ms (2);
		GPIOPinWrite(GPIO_PORTM_BASE, 0x07,0x05);
		SysTick_Wait1ms (1);
		GPIOPinWrite(GPIO_PORTM_BASE, 0x07,0x00);
		return;
}
void LCD_Clear(void)
	{
		LCD_Command(0x01);
	}
void new_line(void)
	{
		LCD_Command(0xC0);
	}
	
void LCD_Print_String(char mensagem[31]){
		uint16_t i = 0;
		for(i = 0; i<31; i++)
		{
			LCD_Print(mensagem[i]);
			if(i==15)
			{
				new_line();
			}
		}
		
		return;
	}
void SysTickIntHandler(void)
{
  SysTicks1ms++;
}

void SysTick_Wait1ms(uint32_t delay){
	unsigned int x = SysTicks1ms;
	while(SysTicks1ms<=delay+x);
}
void UARTStringSend(char *String, uint32_t tamanho)
{
while (tamanho--) UARTCharPut(UART0_BASE, *String++);
}
void UART_Interruption_Handler(void) 
{
  uint8_t last;
  UARTIntClear(UART0_BASE,UARTIntStatus(UART0_BASE,true));
  last = (uint8_t)UARTCharGetNonBlocking(UART0_BASE);
  rxbuffer[0]=rxbuffer[1];
  rxbuffer[1]=rxbuffer[2];
  rxbuffer[2]=rxbuffer[3];
  rxbuffer[3]=last;
}

void SetupSystick(void)
{
  SysTicks1ms=0;
  SysTickDisable();  
  SysTickPeriodSet(120000-1-12-6);
  SysTickIntRegister(SysTickIntHandler);
  SysTickIntEnable();
  SysTickEnable();
}

void SetupUart(void)
{
  SysCtlPeripheralEnable(SYSCTL_PERIPH_UART0);
  while(!SysCtlPeripheralReady(SYSCTL_PERIPH_UART0));
  UARTConfigSetExpClk(UART0_BASE, SysClock, 115200, (UART_CONFIG_WLEN_8 | UART_CONFIG_STOP_ONE | UART_CONFIG_PAR_NONE));
  UARTFIFODisable(UART0_BASE);
  UARTIntEnable(UART0_BASE,UART_INT_RX);
  UARTIntRegister(UART0_BASE,UART_Interruption_Handler);
  SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOA);
  while(!SysCtlPeripheralReady(SYSCTL_PERIPH_GPIOA));
  GPIOPinConfigure(GPIO_PA0_U0RX);
  GPIOPinConfigure(GPIO_PA1_U0TX);
  GPIOPinTypeUART(GPIO_PORTA_BASE,(GPIO_PIN_0|GPIO_PIN_1));
}

# ARM Cortex-M4 embedded systems and RTOS (TM4C1294)

Projetos em C e Assembly desenvolvidos para o microcontrolador Texas Instruments TM4C1294NCPDT (ARM Cortex-M4F a 120 MHz), utilizando o kit de desenvolvimento EK-TM4C1294XL.

O repositório aborda desde acesso direto a registradores mapeados em memória até sistemas operacionais de tempo real com FreeRTOS, drivers de comunicação (I2C, UART) e comparação de desempenho entre C e Assembly ARM Thumb-2.

---

## Módulos do repositório

```text
arm-cortex-m4-embedded-systems/
├── 01-gpio-blinky/             # Inicialização, CMSIS e controle de GPIO
├── 02-assembly-vs-c-benchmark/ # Benchmark de histograma: Assembly Thumb-2 vs C
├── 03-bare-metal-peripherals/  # Registradores diretos, teclado matricial e LCD
├── 04-uart-serial-driver/      # Driver UART com interrupções e retarget de I/O
├── 05-i2c-oled-keypad/         # Barramento I2C, display OLED SSD1306 e periféricos
└── 06-freertos-multitasking/   # Multitarefa com FreeRTOS (filas, semáforos e tarefas)
```

### Descrição dos módulos

* `01-gpio-blinky`: configuração de pinos digitais, temporização por laço de software e estrutura de inicialização com CMSIS e `startup_rvmdk.S`.
* `02-assembly-vs-c-benchmark`: cálculo de histograma implementado em C e em Assembly ARM Thumb-2, com comparação de ciclos de clock e consumo de memória.
* `03-bare-metal-peripherals`: varredura de teclado matricial 4x4 e controle de periféricos via registradores diretos de memória mapeada, sem bibliotecas de abstração.
* `04-uart-serial-driver`: comunicação serial assíncrona, rotinas de interrupção (ISR) de transmissão e recepção, e integração com `uartstdio`.
* `05-i2c-oled-keypad`: driver I2C para o display gráfico OLED SSD1306 (128x64), desenho de bitmaps, interface com teclado matricial e acionamento de relés.
* `06-freertos-multitasking`: sistema multitarefa preemptivo com FreeRTOS separando tarefas de display (SSD1306), leitura de teclado não bloqueante, logs via UART e controle de atuadores com sincronização por filas (`xQueue`) e semáforos.

---

## Como compilar e executar

1. Instale o Keil uVision 5 com o pacote de dispositivos para Texas Instruments Tiva C Series.
2. Mantenha o SDK TivaWare for C Series instalado no computador (caminho padrão `C:\ti\TivaWare_C_Series-2.2.0.295`).
3. Abra o arquivo `.uvprojx` do módulo desejado no Keil.
4. Conecte a placa EK-TM4C1294XL pela porta USB Debug (ICDI).
5. Compile o projeto e inicie a sessão de gravação e depuração.

---

## Ferramentas e ambiente

* Microcontrolador: TI TM4C1294NCPDT (ARM Cortex-M4F a 120 MHz, 1 MB Flash, 256 KB SRAM)
* Placa: EK-TM4C1294XL Connected LaunchPad
* SDK: TI TivaWare C Series
* Compilador: Arm Compiler (Keil uVision 5)
* RTOS: FreeRTOS Kernel
* Linguagens: C (C99) e Assembly (ARMv7E-M / Thumb-2)

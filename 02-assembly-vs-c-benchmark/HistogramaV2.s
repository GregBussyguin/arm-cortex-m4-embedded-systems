; -------------------------------------------------------------------------------
        AREA    |.text|, CODE, READONLY, ALIGN=2
        THUMB                        ; Instruções do tipo Thumb-2
        EXPORT EightBitHistogram  


EightBitHistogram  
    MUL R0, R1                  ; Calcula o tamanho da imagem (width * height)
    CMP R0, #65536              ; Verifica se o tamanho excede 65.536
    BHI Erro                    ; Se maior, vai para a rotina de erro

    MOV R6, #0                  ; Índice auxiliar
    MOV R9, #0                  ; Valor zero

; Inicializa o histograma zerando todas as 256 posições
Zera
    STRH R9, [R3, R6]           ; Armazena zero no histograma (armazenamento de 16 bits)
    ADD R6, #2                  ; Incrementa índice do histograma (16 bits por entrada)
    CMP R6, #512                ; Verifica se todas as posições foram zeradas
    BNE Zera                    ; Se não, continua zerando
   
Programa
    MOV R1, #0                  ; Índice da imagem

Loop
    LDRB R6, [R2, R1]           ; Lê um pixel da imagem
    LDRH R7, [R3, R6, LSL #1]   ; Carrega contagem do histograma
    ADD R7, R7, #1              ; Incrementa a contagem
    STRH R7, [R3, R6, LSL #1]   ; Armazena de volta no histograma
    ADD R1, #1                  ; Avança para o próximo pixel
    CMP R1, R0                  ; Verifica se chegou ao final da imagem
    BNE Loop                    ; Se não, continua

    MOV R0, #1                  ; Retorno de sucesso
    BX LR                       ; Retorna

Erro
    MOV R0, #0                  ; Retorna 0 para indicar erro
    BX LR                       ; Retorna

  END
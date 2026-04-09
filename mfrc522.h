#ifndef MFRC522_H
#define MFRC522_H

#include "stm32f4xx_hal.h"

extern SPI_HandleTypeDef hspi1;

#define RFID_CS_PORT GPIOA
#define RFID_CS_PIN  GPIO_PIN_4

#define RFID_RST_PORT GPIOB
#define RFID_RST_PIN  GPIO_PIN_0

typedef struct {
    uint8_t uid[5];
} RFID_Card;
void MFRC522_Halt(void);

void MFRC522_Init(void);
uint8_t MFRC522_Check(uint8_t *uid);
uint8_t MFRC522_ReadVersion(void);  // <-- ADD THIS LINE

#endif

#include "mfrc522.h"

#define CommandReg      0x01
#define ComIEnReg       0x02
#define DivIEnReg       0x03
#define ComIrqReg       0x04
#define DivIrqReg       0x05
#define ErrorReg        0x06
#define Status1Reg      0x07
#define Status2Reg      0x08
#define FIFODataReg     0x09
#define FIFOLevelReg    0x0A
#define ControlReg      0x0C
#define BitFramingReg   0x0D
#define CollReg         0x0E
#define ModeReg         0x11
#define TxModeReg       0x12
#define RxModeReg       0x13
#define TxControlReg    0x14
#define TxASKReg        0x15
#define CRCResultRegH   0x21
#define CRCResultRegL   0x22
#define TModeReg        0x2A
#define TPrescalerReg   0x2B
#define TReloadRegH     0x2C
#define TReloadRegL     0x2D
#define VersionReg      0x37

#define PCD_IDLE        0x00
#define PCD_CALCCRC     0x03
#define PCD_TRANSCEIVE  0x0C
#define PCD_SOFTRESET   0x0F

#define PICC_REQIDL     0x26
#define PICC_ANTICOLL   0x93
#define PICC_HALT       0x50

static void RFID_Select(void)
{
    HAL_GPIO_WritePin(RFID_CS_PORT, RFID_CS_PIN, GPIO_PIN_RESET);
}

static void RFID_Unselect(void)
{
    HAL_GPIO_WritePin(RFID_CS_PORT, RFID_CS_PIN, GPIO_PIN_SET);
}

static void Write_Reg(uint8_t addr, uint8_t val)
{
    uint8_t data[2];
    data[0] = (addr << 1) & 0x7E;
    data[1] = val;

    RFID_Select();
    HAL_SPI_Transmit(&hspi1, data, 2, 100);
    RFID_Unselect();
}

static uint8_t Read_Reg(uint8_t addr)
{
    uint8_t txData[2];
    uint8_t rxData[2];

    txData[0] = ((addr << 1) & 0x7E) | 0x80;
    txData[1] = 0x00;

    RFID_Select();
    HAL_SPI_TransmitReceive(&hspi1, txData, rxData, 2, 100);
    RFID_Unselect();

    return rxData[1];
}

static void Set_Bit_Mask(uint8_t reg, uint8_t mask)
{
    uint8_t tmp = Read_Reg(reg);
    Write_Reg(reg, tmp | mask);
}

static void Clear_Bit_Mask(uint8_t reg, uint8_t mask)
{
    uint8_t tmp = Read_Reg(reg);
    Write_Reg(reg, tmp & (~mask));
}

static void Antenna_On(void)
{
    uint8_t temp = Read_Reg(TxControlReg);
    if ((temp & 0x03) != 0x03)
    {
        Set_Bit_Mask(TxControlReg, 0x03);
    }
}

static void CalculateCRC(uint8_t *pIndata, uint8_t len, uint8_t *pOutData)
{
    uint8_t i, n;

    Clear_Bit_Mask(DivIrqReg, 0x04);
    Set_Bit_Mask(FIFOLevelReg, 0x80);

    for (i = 0; i < len; i++)
    {
        Write_Reg(FIFODataReg, pIndata[i]);
    }

    Write_Reg(CommandReg, PCD_CALCCRC);

    i = 0xFF;
    do
    {
        n = Read_Reg(DivIrqReg);
        i--;
    } while ((i != 0) && !(n & 0x04));

    pOutData[0] = Read_Reg(CRCResultRegL);
    pOutData[1] = Read_Reg(CRCResultRegH);
}

void MFRC522_Init(void)
{
    HAL_GPIO_WritePin(RFID_RST_PORT, RFID_RST_PIN, GPIO_PIN_RESET);
    HAL_Delay(10);
    HAL_GPIO_WritePin(RFID_RST_PORT, RFID_RST_PIN, GPIO_PIN_SET);
    HAL_Delay(10);

    Write_Reg(CommandReg, PCD_SOFTRESET);
    HAL_Delay(50);

    Write_Reg(TModeReg, 0x8D);
    Write_Reg(TPrescalerReg, 0x3E);
    Write_Reg(TReloadRegL, 30);
    Write_Reg(TReloadRegH, 0);

    Write_Reg(TxASKReg, 0x40);
    Write_Reg(ModeReg, 0x3D);

    Write_Reg(TxModeReg, 0x00);
    Write_Reg(RxModeReg, 0x00);
    Write_Reg(BitFramingReg, 0x00);
    Write_Reg(CollReg, 0x80);

    Antenna_On();
}

uint8_t MFRC522_ReadVersion(void)
{
    return Read_Reg(VersionReg);
}

static uint8_t ToCard(uint8_t command, uint8_t *sendData, uint8_t sendLen,
                      uint8_t *backData, uint16_t *backLen)
{
    uint8_t irqEn = 0x00;
    uint8_t waitIRq = 0x00;
    uint8_t n;
    uint8_t lastBits;
    uint16_t i;

    if (command == PCD_TRANSCEIVE)
    {
        irqEn = 0x77;
        waitIRq = 0x30;
    }

    Write_Reg(ComIEnReg, irqEn | 0x80);
    Write_Reg(ComIrqReg, 0x7F);
    Write_Reg(FIFOLevelReg, 0x80);
    Write_Reg(CommandReg, PCD_IDLE);

    for (i = 0; i < sendLen; i++)
    {
        Write_Reg(FIFODataReg, sendData[i]);
    }

    Write_Reg(CommandReg, command);

    if (command == PCD_TRANSCEIVE)
    {
        Set_Bit_Mask(BitFramingReg, 0x80);
    }

    i = 2000;
    do
    {
        n = Read_Reg(ComIrqReg);
        i--;
    } while ((i != 0) && !(n & 0x01) && !(n & waitIRq));

    Clear_Bit_Mask(BitFramingReg, 0x80);

    if (i != 0)
    {
        if (!(Read_Reg(ErrorReg) & 0x1B))
        {
            n = Read_Reg(FIFOLevelReg);
            lastBits = Read_Reg(ControlReg) & 0x07;

            if (lastBits)
                *backLen = (n - 1) * 8 + lastBits;
            else
                *backLen = n * 8;

            if (n == 0)
                n = 1;
            if (n > 16)
                n = 16;

            for (i = 0; i < n; i++)
            {
                backData[i] = Read_Reg(FIFODataReg);
            }

            return 1;
        }
    }

    return 0;
}

uint8_t MFRC522_Check(uint8_t *uid)
{
    uint8_t status;
    uint16_t backLen;
    uint8_t buffer[2];
    uint8_t backData[16];
    uint8_t i;
    uint8_t serNumCheck = 0;

    for (i = 0; i < 4; i++)
        uid[i] = 0x00;

    Write_Reg(CommandReg, PCD_IDLE);
    Write_Reg(ComIrqReg, 0x7F);
    Write_Reg(FIFOLevelReg, 0x80);
    Write_Reg(BitFramingReg, 0x07);
    HAL_Delay(2);

    buffer[0] = PICC_REQIDL;
    status = ToCard(PCD_TRANSCEIVE, buffer, 1, backData, &backLen);
    if (!status)
        return 0;

    if (backLen != 0x10)
        return 0;

    Write_Reg(BitFramingReg, 0x00);
    HAL_Delay(2);

    buffer[0] = PICC_ANTICOLL;
    buffer[1] = 0x20;

    status = ToCard(PCD_TRANSCEIVE, buffer, 2, backData, &backLen);
    if (!status)
        return 0;

    if (backLen != 40)
        return 0;

    if ((backData[0] == 0x00 && backData[1] == 0x00 &&
         backData[2] == 0x00 && backData[3] == 0x00) ||
        (backData[0] == 0xFF && backData[1] == 0xFF &&
         backData[2] == 0xFF && backData[3] == 0xFF))
    {
        return 0;
    }

    for (i = 0; i < 4; i++)
    {
        uid[i] = backData[i];
        serNumCheck ^= backData[i];
    }

    if (serNumCheck != backData[4])
        return 0;

    return 1;
}

void MFRC522_Halt(void)
{
    uint8_t halt_cmd[4];
    uint16_t backLen;
    uint8_t backData[16];

    halt_cmd[0] = PICC_HALT;
    halt_cmd[1] = 0x00;
    CalculateCRC(halt_cmd, 2, &halt_cmd[2]);

    ToCard(PCD_TRANSCEIVE, halt_cmd, 4, backData, &backLen);
}

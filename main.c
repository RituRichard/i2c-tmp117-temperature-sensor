#include <stdio.h>
#include "em_device.h"
#include "em_chip.h"
#include "em_i2c.h"
#include "em_cmu.h"
#include "em_emu.h"
#include "em_gpio.h"

#define I2C_FOLLOWER_ADDRESS 0x48
#define I2C_TXBUFFER_SIZE 1
#define I2C_RXBUFFER_SIZE 2

float currentTemperature;
uint16_t currentDeviceID;
uint8_t lastWrittenConfig = 0;
uint8_t i2c_txBuffer[I2C_TXBUFFER_SIZE];
uint8_t i2c_rxBuffer[I2C_RXBUFFER_SIZE];

void initCMU(void){
    CMU_ClockEnable(cmuClock_I2C0, true);
    CMU_ClockEnable(cmuClock_GPIO, true);
}

void initGPIO(void){
    NVIC_ClearPendingIRQ(GPIO_EVEN_IRQn);
    NVIC_EnableIRQ(GPIO_EVEN_IRQn);
}

void initI2C(void){
    I2C_Init_TypeDef i2cInit = I2C_INIT_DEFAULT;

    GPIO_PinModeSet(gpioPortB, 1, gpioModeWiredAndPullUpFilter, 1);
    GPIO_PinModeSet(gpioPortB, 0, gpioModeWiredAndPullUpFilter, 1);

    GPIO->I2CROUTE[0].SDAROUTE = (GPIO->I2CROUTE[0].SDAROUTE & ~_GPIO_I2C_SDAROUTE_MASK)
        | (gpioPortB << _GPIO_I2C_SDAROUTE_PORT_SHIFT
        | (1 << _GPIO_I2C_SDAROUTE_PIN_SHIFT));
    GPIO->I2CROUTE[0].SCLROUTE = (GPIO->I2CROUTE[0].SCLROUTE & ~_GPIO_I2C_SCLROUTE_MASK)
        | (gpioPortB << _GPIO_I2C_SCLROUTE_PORT_SHIFT
        | (0 << _GPIO_I2C_SCLROUTE_PIN_SHIFT));
    GPIO->I2CROUTE[0].ROUTEEN = GPIO_I2C_ROUTEEN_SDAPEN | GPIO_I2C_ROUTEEN_SCLPEN;

    I2C_Init(I2C0, &i2cInit);
    I2C0->CTRL = I2C_CTRL_AUTOSN;
}

bool I2C_LeaderRead(uint16_t followerAddress, uint8_t targetAddress, uint8_t *rxBuff, uint8_t numBytes){
    I2C_TransferSeq_TypeDef i2cTransfer;
    I2C_TransferReturn_TypeDef result;
    i2cTransfer.addr = followerAddress << 1;
    i2cTransfer.flags = I2C_FLAG_WRITE_READ;
    i2cTransfer.buf[0].data = &targetAddress;
    i2cTransfer.buf[0].len = 1;
    i2cTransfer.buf[1].data = rxBuff;
    i2cTransfer.buf[1].len = numBytes;

    result = I2C_TransferInit(I2C0, &i2cTransfer);

    while (result == i2cTransferInProgress) {
        result = I2C_Transfer(I2C0);
    }
    return (result == i2cTransferDone);
}

void I2C_LeaderWrite(uint16_t followerAddress, uint8_t targetAddress, uint8_t *txBuff, uint8_t numBytes){
    I2C_TransferSeq_TypeDef i2cTransfer;
    I2C_TransferReturn_TypeDef result;
    uint8_t txBuffer[I2C_TXBUFFER_SIZE + 1];
    txBuffer[0] = targetAddress;
    for(int i = 0; i < numBytes; i++){
        txBuffer[i + 1] = txBuff[i];
    }
    i2cTransfer.addr = followerAddress << 1;
    i2cTransfer.flags = I2C_FLAG_WRITE;
    i2cTransfer.buf[0].data = txBuffer;
    i2cTransfer.buf[0].len = numBytes + 1;
    i2cTransfer.buf[1].data = NULL;
    i2cTransfer.buf[1].len = 0;
    result = I2C_TransferInit(I2C0, &i2cTransfer);
    while (result == i2cTransferInProgress) {
        result = I2C_Transfer(I2C0);
    }
    if (result != i2cTransferDone) {
        while(1);
    }
}

uint16_t ReadDeviceID(void){
    I2C_LeaderRead(I2C_FOLLOWER_ADDRESS, 0x0F, i2c_rxBuffer, I2C_RXBUFFER_SIZE);
    return (i2c_rxBuffer[0] << 8) | i2c_rxBuffer[1];
}

uint16_t ReadTemperatureRaw(void){
    I2C_LeaderRead(I2C_FOLLOWER_ADDRESS, 0x00, i2c_rxBuffer, I2C_RXBUFFER_SIZE);
    return (i2c_rxBuffer[0] << 8) | i2c_rxBuffer[1];
}

float GetTemperatureC(void){
    uint16_t raw = ReadTemperatureRaw();
    int16_t temp_signed = (int16_t)raw;
    float tempC = temp_signed * 0.0078125f;
    return tempC;
}

int main(void){
    CHIP_Init();
    initCMU();
    initGPIO();
    initI2C();

    currentDeviceID = ReadDeviceID();
    lastWrittenConfig = 0xAB;
    I2C_LeaderWrite(I2C_FOLLOWER_ADDRESS, 0x01, &lastWrittenConfig, 1);

    while (1){
        currentTemperature = GetTemperatureC();
        for (volatile int i = 0; i < 1000000; i++);
    }
}

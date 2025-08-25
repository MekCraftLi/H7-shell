/**
 *******************************************************************************
 * @file    app-file.cpp
 * @brief   简要描述
 *******************************************************************************
 * @attention
 *
 * none
 *
 *******************************************************************************
 * @note
 *
 * none
 *
 *******************************************************************************
 * @author  MekLi
 * @date    2025/8/22
 * @version 1.0
 *******************************************************************************
 */




/* ------- define --------------------------------------------------------------------------------------------------*/





/* ------- include ---------------------------------------------------------------------------------------------------*/

#include "../../Drivers/Devices/w25qxx.h"
#include "../../Drivers/Peripheral/GPIO/gpio-intf.hpp"
#include "../../Drivers/Services/serv-time.h"
#include "app-intf.h"
#include "octospi.h"
#include "semphr.h"

/* ------- class prototypes-----------------------------------------------------------------------------------------*/

class FileThread : public AppThreadBase {

  public:
    FileThread() : AppThreadBase("File", 512, 4), _flash(&hospi1) {}

    void init() override;

    void run() override;

    void putQueue();

  private:
    W25Qxx _flash;
    xQueueHandle _sAssignQ;
    xSemaphoreHandle _waitForReceiveLock;
    xSemaphoreHandle _waitForTransmitLock;
    friend void HAL_OSPI_RxCpltCallback(OSPI_HandleTypeDef*);
    friend void HAL_OSPI_CmdCpltCallback(OSPI_HandleTypeDef*);
    friend void HAL_OSPI_TxCpltCallback(OSPI_HandleTypeDef*);
    friend void waitForRxCplt();
};




/* ------- macro -----------------------------------------------------------------------------------------------------*/





/* ------- variables -------------------------------------------------------------------------------------------------*/

FileThread fileThread;
AppThreadBase* pFileThread = &fileThread;
uint8_t txBuff[256];
uint8_t rxBuff[256];




/* ------- function implement ----------------------------------------------------------------------------------------*/

void FileThread::init() {

    for (uint16_t i = 0; i < sizeof(txBuff); i++) {
        txBuff[i] = (i * 3 + 7) ^ 6;
    }

    _waitForReceiveLock  = xSemaphoreCreateBinary();
    _waitForTransmitLock = xSemaphoreCreateBinary();
    configASSERT(_waitForReceiveLock);
    configASSERT(_waitForTransmitLock);
    //
    // _flash.enquireJedecIdAsync();
    //
    // if (xSemaphoreTake(_waitForReceiveLock, portMAX_DELAY) == pdTRUE) {
    //     const auto r = fileThread._flash.asyncRxCallback();
    //     (void)r;
    // }
    //
    // _flash.enquireDeviceIdAsync();
    //
    // if (xSemaphoreTake(_waitForReceiveLock, portMAX_DELAY) == pdTRUE) {
    //     const auto r = fileThread._flash.asyncRxCallback();
    //     (void)r;
    // }
    //
    // _flash.enquireUniqueIdAsync();
    //
    // if (xSemaphoreTake(_waitForReceiveLock, portMAX_DELAY) == pdTRUE) {
    //     const auto r = fileThread._flash.asyncRxCallback();
    //     (void)r;
    // }
    //
    // _flash.enquireStatusRegisterAsync(W25QxxRegisterEnum::STATUS_REGISTER_1);
    //
    // if (xSemaphoreTake(_waitForReceiveLock, portMAX_DELAY) == pdTRUE) {
    //     const auto r = fileThread._flash.asyncRxCallback();
    //     (void)r;
    // }
    //
    // _flash.enquireStatusRegisterAsync(W25QxxRegisterEnum::STATUS_REGISTER_1);
    //
    //
    // if (xSemaphoreTake(_waitForReceiveLock, portMAX_DELAY) == pdTRUE) {
    //     const auto r = fileThread._flash.asyncRxCallback();
    //     (void)r;
    // }
    //
    // _flash.enquireStatusRegisterAsync(W25QxxRegisterEnum::STATUS_REGISTER_2);
    //
    // if (xSemaphoreTake(_waitForReceiveLock, portMAX_DELAY) == pdTRUE) {
    //     const auto r = fileThread._flash.asyncRxCallback();
    //     (void)r;
    // }
    //
    // _flash.writeEnable();
    //
    // if (xSemaphoreTake(_waitForTransmitLock, portMAX_DELAY) == pdTRUE) {
    //     _flash.pageProgram(0x00, txBuff, 256);
    // }
    //
    // if (xSemaphoreTake(_waitForTransmitLock, portMAX_DELAY) == pdTRUE) {
    //     _flash.enquireStatusRegisterAsync(W25QxxRegisterEnum::STATUS_REGISTER_1);
    // }
    //
    // if (xSemaphoreTake(_waitForReceiveLock, portMAX_DELAY) == pdTRUE) {
    //     _flash.asyncRxCallback();
    // }
    //
    // while (_flash.getBusyBit()) {
    //     _flash.enquireStatusRegisterAsync(W25QxxRegisterEnum::STATUS_REGISTER_1);
    //     if (xSemaphoreTake(_waitForReceiveLock, portMAX_DELAY) == pdTRUE) {
    //         _flash.asyncRxCallback();
    //     }
    //
    //     osDelay(100);
    // }

    vTaskDelay(2);
    _flash.fastReadData(0xF0, rxBuff, 32);


}

void FileThread::run() {
    float time = timeServ.getGlobalTimeUs();
    (void)time;
}



void HAL_OSPI_RxCpltCallback(OSPI_HandleTypeDef* hospi) {

    if (hospi == &hospi1) {
        BaseType_t xHigherPriorityTaskWoken = pdFALSE;
        xSemaphoreGiveFromISR(fileThread._waitForReceiveLock, &xHigherPriorityTaskWoken);
        portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
    }
}

void HAL_OSPI_CmdCpltCallback(OSPI_HandleTypeDef* hospi) {

    if (hospi == &hospi1) {
        BaseType_t xHigherPriorityTaskWoken = pdFALSE;
        xSemaphoreGiveFromISR(fileThread._waitForTransmitLock, &xHigherPriorityTaskWoken);
        portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
    }
}

void HAL_OSPI_TxCpltCallback(OSPI_HandleTypeDef* hospi) {

    if (hospi == &hospi1) {
        BaseType_t xHigherPriorityTaskWoken = pdFALSE;
        xSemaphoreGiveFromISR(fileThread._waitForTransmitLock, &xHigherPriorityTaskWoken);
        portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
    }
}

void waitForRxCplt() { xSemaphoreTake(fileThread._waitForReceiveLock, portMAX_DELAY); }
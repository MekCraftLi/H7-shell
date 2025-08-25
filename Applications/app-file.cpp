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
#include "../../Drivers/Services/serv-time.h"
#include "../Adapters/adapter-lfs.h"
#include "../Middlewares/Third_Party/LittleFs/lfs.h"
#include "FreeRTOS.h"
#include "app-intf.h"
#include "octospi.h"
#include "semphr.h"

/* ------- class prototypes-----------------------------------------------------------------------------------------*/

class FileThread : public AppThreadBase {

  public:
    FileThread() : AppThreadBase("File", 1024, 4), _flash(&hospi1) {}

    void init() override;

    void run() override;

    void putQueue();

  private:
    W25Qxx _flash;
    xQueueHandle _sAssignQ;
    xSemaphoreHandle _waitForReceiveLock;
    xSemaphoreHandle _waitForTransmitLock;
    xSemaphoreHandle _waitForCmd;
    xSemaphoreHandle _waitForFlag;
    LfsAdapter _lfsAdapter;
    float _runTime;
    friend void HAL_OSPI_RxCpltCallback(OSPI_HandleTypeDef*);
    friend void HAL_OSPI_CmdCpltCallback(OSPI_HandleTypeDef*);
    friend void HAL_OSPI_TxCpltCallback(OSPI_HandleTypeDef*);
    friend void HAL_OSPI_StatusMatchCallback(OSPI_HandleTypeDef* hospi);
    friend void waitForRxCplt();
};




/* ------- macro -----------------------------------------------------------------------------------------------------*/





/* ------- variables -------------------------------------------------------------------------------------------------*/

FileThread fileThread;
AppThreadBase* pFileThread = &fileThread;
static uint8_t lfs_read_buf[LFS_CACHE_SIZE];
static uint8_t lfs_prog_buf[LFS_CACHE_SIZE];
static uint8_t lfs_lookahead_buf[LFS_LOOKAHEAD_SIZE];

uint32_t debugFlag = 0;
uint32_t debugCnt  = 0;

static lfs_t lfs;
static lfs_file_t file;

struct lfs_config fcfg{
    .read             = LfsAdapter::read,
    .prog             = LfsAdapter::program,
    .erase            = LfsAdapter::erase,
    .sync             = LfsAdapter::sync,


    .read_size        = LFS_READ_SIZE,
    .prog_size        = LFS_PROG_SIZE,
    .block_size       = LFS_BLOCK_SIZE,
    .block_count      = LFS_BLOCK_COUNT,
    .block_cycles     = LFS_BLOCK_CYCLES,
    .cache_size       = LFS_CACHE_SIZE,
    .lookahead_size   = LFS_LOOKAHEAD_SIZE,


    .read_buffer      = lfs_read_buf,
    .prog_buffer      = lfs_prog_buf,
    .lookahead_buffer = lfs_lookahead_buf,
};

/* ------- function implement ----------------------------------------------------------------------------------------*/

void FileThread::init() {


    _waitForReceiveLock  = xSemaphoreCreateBinary();
    _waitForTransmitLock = xSemaphoreCreateBinary();
    _waitForFlag         = xSemaphoreCreateBinary();
    _waitForCmd          = xSemaphoreCreateBinary();
    configASSERT(_waitForReceiveLock);
    configASSERT(_waitForTransmitLock);
    configASSERT(_waitForFlag);

    _lfsAdapter.config(&_flash, _waitForFlag, _waitForTransmitLock, _waitForCmd, _waitForReceiveLock);




}

void FileThread::run() {
    vTaskDelay(1000);

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
        xSemaphoreGiveFromISR(fileThread._waitForCmd, &xHigherPriorityTaskWoken);
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

void HAL_OSPI_StatusMatchCallback(OSPI_HandleTypeDef* hospi) {
    if (hospi == &hospi1) {
        BaseType_t xHigherPriorityTaskWoken = pdFALSE;
        xSemaphoreGiveFromISR(fileThread._waitForFlag, &xHigherPriorityTaskWoken);
        portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
    }
}
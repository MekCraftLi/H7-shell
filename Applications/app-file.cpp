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

#include "app-file.h"

/* ------- class prototypes-----------------------------------------------------------------------------------------*/

int16_t fileHandle(FsOptEnum opt, const char* path, uint8_t* buff, uint16_t buffLen);
int16_t dirHandle(FsOptEnum opt, const char* path, uint8_t* buff, uint16_t buffLen);




/* ------- macro -----------------------------------------------------------------------------------------------------*/





/* ------- variables -------------------------------------------------------------------------------------------------*/

FileThread fileThread;
AppThreadBase* pFileThread = &fileThread;
static uint8_t lfs_read_buf[LFS_CACHE_SIZE];
static uint8_t lfs_prog_buf[LFS_CACHE_SIZE];
static uint8_t lfs_lookahead_buf[LFS_LOOKAHEAD_SIZE];

static lfs_t lfs;
static lfs_file_t file;
static lfs_dir_t dir;
static lfs_info info;

static struct lfs_config fcfg{
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
    _request             = xQueueCreate(10, sizeof(FsMsg*));
    configASSERT(_waitForReceiveLock);
    configASSERT(_waitForTransmitLock);
    configASSERT(_waitForFlag);
    configASSERT(_waitForCmd);
    configASSERT(_request);

    _lfsAdapter.config(&_flash, _waitForFlag, _waitForTransmitLock, _waitForCmd, _waitForReceiveLock);


    console.waitInit();

    _flash.enquireSfdpRegisterAsync();
    xSemaphoreTake(_waitForReceiveLock, portMAX_DELAY);
    _flash.asyncRxCallback();
    uint16_t sfdpHH = _flash.getSFDP() >> 48;
    uint16_t sfdpHL = _flash.getSFDP() >> 32;
    uint16_t sfdpLH = _flash.getSFDP() >> 16;
    uint16_t sfdpLL = _flash.getSFDP();
    console.println("[W25Q64] Initialization infomation:\r\nSFDP Register\t\t: %04X %04X %04X %04X", sfdpHH, sfdpHL,
                    sfdpLH, sfdpLL);


    _flash.writeEnable();
    auto r = _flash.writeRegister(W25QxxRegisterEnum::STATUS_REGISTER_1, 0x00);
    (void)r;
    vTaskDelay(10);

    _flash.writeEnable();
    r = _flash.writeRegister(W25QxxRegisterEnum::STATUS_REGISTER_2, 0x02);
    (void)r;
    vTaskDelay(10);

    _flash.writeEnable();
    r = _flash.writeRegister(W25QxxRegisterEnum::STATUS_REGISTER_3, 0x00);
    (void)r;
    vTaskDelay(10);

    _flash.enquireStatusRegisterAsync(W25QxxRegisterEnum::STATUS_REGISTER_1);
    xSemaphoreTake(_waitForReceiveLock, 50);
    _flash.asyncRxCallback();

    _flash.enquireStatusRegisterAsync(W25QxxRegisterEnum::STATUS_REGISTER_2);
    xSemaphoreTake(_waitForReceiveLock, 50);
    _flash.asyncRxCallback();

    _flash.enquireStatusRegisterAsync(W25QxxRegisterEnum::STATUS_REGISTER_3);
    xSemaphoreTake(_waitForReceiveLock, 50);
    _flash.asyncRxCallback();

    console.println("Status Register 1\t: 0x%02X\r\nStatus Register 2\t: 0x%02X\r\nStatus Register 3\t: 0x%02X",
                    _flash.getSR1(), _flash.getSR2(), _flash.getSR3());

    _flash.enquireJedecIdAsync();
    xSemaphoreTake(_waitForReceiveLock, 50);
    _flash.asyncRxCallback();
    console.println("Manufacturer ID\t\t: %02X\r\nDevice ID - 16\t\t: %04X", _flash.getMID(), _flash.getDevID16());

    _flash.enquireDeviceIdAsync();
    xSemaphoreTake(_waitForReceiveLock, 50);
    _flash.asyncRxCallback();
    console.println("Device ID - 8\t\t: %02X", _flash.getDevID8());

    _flash.enquireUniqueIdAsync();
    xSemaphoreTake(_waitForReceiveLock, 50);
    _flash.asyncRxCallback();
    uint64_t uniqueID   = _flash.getUniqueID();
    uint16_t uniqueIDHH = uniqueID >> 48;
    uint16_t uniqueIDHL = uniqueID >> 32;
    uint16_t uniqueIDLH = uniqueID >> 16;
    uint16_t uniqueIDLL = uniqueID;
    console.println("Unique ID\t\t: %04X %04X %04X %04X\r\n", uniqueIDHH, uniqueIDHL, uniqueIDLH, uniqueIDLL);



    auto err = lfs_mount(&lfs, &fcfg);

    if (err) {
        console.println("Failed to mount LFS. Format the flash.");
        lfs_format(&lfs, &fcfg);

        err = lfs_mount(&lfs, &fcfg);

        if (err) {
            logError("Failed to format the flash. Error Code: %d", err);
            vTaskDelete(NULL);
        }
    }

    console.println("Mount LFS successfully");
}

void FileThread::run() {
    FsMsg* pMsg;
    xQueueReceive(_request, &pMsg, portMAX_DELAY);

    switch (pMsg->getOpt()) {
        case FsOptEnum::FILE_READ:
        case FsOptEnum::FILE_WRITE_AND_MAKE:
        case FsOptEnum::FILE_APPEND:
        case FsOptEnum::FILE_CREATE_NEW:
        case FsOptEnum::FILE_WRITE:
            pMsg->setReturn(fileHandle(pMsg->getOpt(), pMsg->getPath(), pMsg->getBuffer(), pMsg->getBufferLen()));
            pMsg->finishReply();
            break;
        case FsOptEnum::DIR_READ:
            pMsg->setReturn(dirHandle(pMsg->getOpt(), pMsg->getPath(), pMsg->getBuffer(), pMsg->getBufferLen()));
            pMsg->finishReply();
            break;

        default:
            logError("File option undefined!");
            pMsg->finishReply();
    }
}

int16_t FileThread::readFile(const char* path, uint8_t* buffer, uint16_t bufferLen) const {
    FsMsg msg(FsOptEnum::FILE_READ, path, buffer, bufferLen);
    FsMsg* pMsg = &msg;
    xQueueSend(_request, &pMsg, portMAX_DELAY);
    msg.waitForReply();

    return msg.getReturn();
}

int16_t fileHandle(FsOptEnum opt, const char* path, uint8_t* buffer, uint16_t buffLen) {
    int8_t ret   = 0;
    int16_t len  = 0;

    uint32_t flg = 0;

    switch (opt) {
        case FsOptEnum::FILE_READ:
            flg = LFS_O_RDONLY;
            break;
        case FsOptEnum::FILE_WRITE:
            flg = LFS_O_WRONLY | LFS_O_TRUNC;
            break;
        case FsOptEnum::FILE_WRITE_AND_MAKE:
            flg = LFS_O_WRONLY | LFS_O_CREAT | LFS_O_TRUNC;
            break;
        case FsOptEnum::FILE_APPEND:
            flg = LFS_O_WRONLY | LFS_O_APPEND;
            break;
        case FsOptEnum::FILE_CREATE_NEW:
            flg = LFS_O_CREAT | LFS_O_EXCL | LFS_O_TRUNC;
            break;
        default:
            logError("File option undefined!");
    }

    ret = lfs_file_open(&lfs, &file, path, flg);
    if (ret < 0) {
        logError("Open file failed :\"%s\"", path);
        return ret;
    }

    if (opt == FsOptEnum::FILE_READ) {

        len = lfs_file_read(&lfs, &file, buffer, buffLen);

        if (len < 0) {
            logError("Read file failed :\"%s\"", path);
            return len;
        }
    } else if (opt != FsOptEnum::FILE_CREATE_NEW) {
        len = lfs_file_write(&lfs, &file, buffer, buffLen);

        if (len < 0) {
            logError("Write file failed :\"%s\"", path);
            return len;
        }
    }

    ret = lfs_file_close(&lfs, &file);

    if (ret < 0) {
        logError("Close file failed :\"%s\"", path);

        // close failed clear the file structure.
        memset(&file, 0, sizeof(file));
        return ret;
    }
    return len;
}

int16_t dirHandle(FsOptEnum opt, const char* path, uint8_t* buff, uint16_t buffLen) {
    auto ret = lfs_dir_open(&lfs, &dir, path);
    if (ret < 0) {
        logError("Failed to open path :%s", path);
        return ret;
    }

    uint16_t size   = 0;
    uint16_t offset = 0;

    while (true) {
        int res = lfs_dir_read(&lfs, &dir, &info);
        if (res < 0) {
            logError("Failed to read dir info");
            break;
        }

        if (res == 0) {
            break;
        }

        uint16_t len = 0;

        const char* typeStr = (info.type == LFS_TYPE_REG) ? "file" : "dir ";
        len = sprintf(reinterpret_cast<char*>(buff + offset),
                      "%-5s %-24s %ld\r\n", // type 最宽5字符，name 宽24字符
                      typeStr, info.name, info.size);

        offset += len;

        if (offset >= buffLen) {
            logError("Buffer overflow");
            return 0;
        }


        size++;
    }

    lfs_dir_close(&lfs, &dir);

    memset(&info, 0, sizeof(info));

    return size;
}


int8_t FileThread::writeAndMakeFile(const char* path, const char* buffer, uint16_t bufferLen) const {
    FsMsg msg(FsOptEnum::FILE_WRITE_AND_MAKE, path, (uint8_t*)buffer, bufferLen);
    FsMsg* pMsg = &msg;
    xQueueSend(_request, &pMsg, portMAX_DELAY);
    if (!msg.waitForReply()) {
        logError("Error while send request to the file thread");
        return -1;
    }

    return msg.getReturn();
}

int8_t FileThread::appendFile(const char* path, const char* buffer, uint16_t bufferLen) const {
    FsMsg msg(FsOptEnum::FILE_APPEND, path, (uint8_t*)buffer, bufferLen);
    FsMsg* pMsg = &msg;
    xQueueSend(_request, &pMsg, portMAX_DELAY);
    if (!msg.waitForReply()) {
        logError("Error while send request to the file thread");
        return -1;
    }

    return msg.getReturn();
}

int8_t FileThread::createFile(const char* path) const {
    FsMsg msg(FsOptEnum::FILE_CREATE_NEW, path);
    FsMsg* pMsg = &msg;
    xQueueSend(_request, &pMsg, portMAX_DELAY);
    if (!msg.waitForReply()) {
        logError("Error while send request to the file thread");
        return -1;
    }

    return msg.getReturn();
}

int8_t FileThread::readDir(const char* path, uint8_t* buffer, uint16_t bufferLen) const {
    FsMsg msg(FsOptEnum::DIR_READ, path, buffer, bufferLen);
    FsMsg* pMsg = &msg;

    xQueueSend(_request, &pMsg, portMAX_DELAY);
    if (!msg.waitForReply()) {
        logError("Error while send request to the file thread");
        return -1;
    }

    return msg.getReturn();
}


int8_t FileThread::writeFile(const char* path, const char* buffer, uint16_t bufferLen) const {
    FsMsg msg(FsOptEnum::FILE_WRITE, path, (uint8_t*)buffer, bufferLen);
    FsMsg* pMsg = &msg;
    xQueueSend(_request, &pMsg, portMAX_DELAY);
    msg.waitForReply();

    return msg.getReturn();
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
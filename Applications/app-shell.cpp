/**
 *******************************************************************************
 * @file    app-shell.cpp
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
 * @date    2025/8/21
 * @version 1.0
 *******************************************************************************
 */




/* ------- define --------------------------------------------------------------------------------------------------*/

#define SHELL_GET_TICK() xTaskGetTickCount()


/* ------- include ---------------------------------------------------------------------------------------------------*/

#include "app-shell.h"
#include "../../Middlewares/Third_Party/LetterShell/log/log.h"
#include "../../Middlewares/Third_Party/LetterShell/shell_cpp.h"
#include "../Drivers/Communications/comm-intf.h"
#include "../Drivers/Peripheral/GPIO/gpio-intf.hpp"
#include <cstdarg>
#include <string.h>




/* ------- class prototypes-----------------------------------------------------------------------------------------*/




/* ------- macro -----------------------------------------------------------------------------------------------------*/





/* ------- variables -------------------------------------------------------------------------------------------------*/

ShellThread shellThread;
AppThreadBase* _p_shell_thread = &shellThread;

ConsoleThread console;
AppThreadBase* pConsoleThread = &console;


static char shellBuffer[512];
static char logBuffer[512];
static uint8_t rxBuffer[64];
static uint16_t rxLen;

extern "C" {
void cdcReceiveCallback(uint8_t* data, uint16_t len);
}



/* ------- function implement ----------------------------------------------------------------------------------------*/


void ShellThread::init() {

    /* driver object initialize */
    _comm        = p_cdc_fcty->produce();




    _shell.write = _shellWrite;
    shellInit(&_shell, shellBuffer, 512);

    _log.write  = _logWrite;
    _log.active = true;
    _log.level  = LOG_DEBUG;

    logRegister(&_log, &_shell);

    _rxQueue = xQueueCreate(1, sizeof(uint16_t));
    shellHandler(&_shell, rxBuffer[0]);
    console.start();
}



void ShellThread::run() {
    uint16_t rx;
    if (osMessageQueueGet(_rxQueue, &rx, nullptr, portMAX_DELAY) == osOK) {
        uint16_t charNum = rxLen;
        for (uint16_t i = 0; i < charNum; i++) {
            shellHandler(&_shell, rxBuffer[i]);
        }
    }
}

void ConsoleThread::init() {

    _logQueue = xQueueCreate(1, sizeof(LogMsg));
    configASSERT(_logQueue);
}


void ConsoleThread::run() {

    LogMsg logMsg;

    if (xQueueReceive(_logQueue, &logMsg, portMAX_DELAY) == pdPASS){
        shellThread.getLog().write(reinterpret_cast<char *>((uint8_t*)logMsg.getMsg()), logMsg.getMsgLen());
    }

}

void ConsoleThread::println(const char *fmt, ...) const {

    va_list vargs;
    int len;

    va_start(vargs, fmt);
    len = vsnprintf(logBuffer, LOG_BUFFER_SIZE - 1, fmt, vargs);
    va_end(vargs);

    if (len > LOG_BUFFER_SIZE)
    {
        len = LOG_BUFFER_SIZE;
    }

    logBuffer[len++] = '\r';
    logBuffer[len++] = '\n';
    logBuffer[len] = 0;
    LogMsg logMsg(logBuffer, len);
    xQueueSend(_logQueue, &logMsg, portMAX_DELAY);

}

void ConsoleThread::error(const char *fmt, ...) const {

    va_list vargs;
    int len;

    va_start(vargs, fmt);
    memcpy(logBuffer, "ERROR: ", 7);
    len = vsnprintf(logBuffer + 7, LOG_BUFFER_SIZE - 1, fmt, vargs) + 7;
    va_end(vargs);

    if (len > LOG_BUFFER_SIZE)
    {
        len = LOG_BUFFER_SIZE;
    }

    logBuffer[len++] = '\r';
    logBuffer[len++] = '\n';
    logBuffer[len] = 0;
    LogMsg logMsg(logBuffer, len);
    xQueueSend(_logQueue, &logMsg, portMAX_DELAY);
}


int16_t ShellThread::_shellWrite(char* data, const uint16_t len) {
    shellThread._comm->transmit(reinterpret_cast<uint8_t*>(data), len);
    return len;
}

void ShellThread::_logWrite(char* data, int16_t len) {
    if (shellThread._log.shell) {
        shellWriteEndLine(shellThread._log.shell, data, len);
    } else {
        shellThread._comm->transmit(reinterpret_cast<uint8_t*>(data), len);
    }
}

void ShellThread::putQueue() { osMessageQueuePut(_rxQueue, &rxLen, 0, 0); }

void cdcReceiveCallback(uint8_t* data, uint16_t len) {
    rxLen = len;
    memcpy(rxBuffer, data, len);
    shellThread.putQueue();
}

void lsHandle();

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

#include "app-intf.h"

#include "../Drivers/Communications/comm-intf.h"
#include "../Drivers/Peripheral/GPIO/gpio-intf.hpp"
#include "../../Middlewares/Third_Party/LetterShell/shell_cpp.h"
#include "../../Middlewares/Third_Party/LetterShell/log/log.h"
#include "tim.h"
#include <string.h>




/* ------- class prototypes-----------------------------------------------------------------------------------------*/

class ShellThread : public AppThreadBase {
public :
  ShellThread() : AppThreadBase("Shell", 512, 4){}

  void init() override;

  void run() override;

  void putQueue();
private:
  IComm* _comm;
  IGpio* _g;
  Shell _shell;
  Log _log;

  osMessageQueueId_t rxQueue;

  static int16_t _shellWrite(char* data, uint16_t len);
  static void _logWrite(char* data, int16_t len);
};




/* ------- macro -----------------------------------------------------------------------------------------------------*/





/* ------- variables -------------------------------------------------------------------------------------------------*/

ShellThread shellThread;

AppThreadBase* _p_shell_thread = &shellThread;

char shellBuffer[512];
uint8_t rxBuffer[64];

osThreadId_t shellTaskHandle;
const osThreadAttr_t shellTask_attributes = {
  .name = "defaultTask",
  .stack_size = 512,
  .priority = (osPriority_t) osPriorityBelowNormal,
};

extern "C" {
  void cdcReceiveCallback(uint8_t*data, uint16_t len);
}

static uint16_t rxLen;

/* ------- function implement ----------------------------------------------------------------------------------------*/


void ShellThread::init() {
  osDelay(5000);

  /* driver object initialize */

  auto r = p_gpio_reg_fcty->produce(GpioPortEnum::GPIO_PORT_C, GpioPinEnum::GPIO_PIN_0_, GpioModeEnum::GPIO_MODE_OUTPUT_PP_);
  _g = std::get<IGpio*>(r);
  _comm = p_cdc_fcty->produce();



  _shell.write = _shellWrite;
  shellInit(&_shell, shellBuffer, 512);

  _log.write = _logWrite;
  _log.active = true;
  _log.level = LOG_DEBUG;

  logRegister(&_log, &_shell);


  _g->enable();


  rxQueue = osMessageQueueNew(1, 2, NULL);
  shellHandler(&_shell, rxBuffer[0]);




}



void ShellThread::run() {
  uint16_t rx;
  if (osMessageQueueGet(rxQueue, &rx, 0, 700) == osOK) {
    uint16_t charNum = rxLen;
    for (uint16_t i = 0; i < charNum; i++) {
      shellHandler(&_shell, rxBuffer[i]);
    }
  }
  //
  // char statsBuf[512];
  // vTaskGetRunTimeStats(statsBuf);
  // logPrintln("%s\n===================================================\n", statsBuf);

}









int16_t ShellThread::_shellWrite(char* data, const uint16_t len) {
  shellThread._comm->transmit(reinterpret_cast<uint8_t*>(data), len);
  return len;
}

void ShellThread::_logWrite(char *data, int16_t len){
  if (shellThread._log.shell) {
    shellWriteEndLine(shellThread._log.shell, data, len);
  } else {
    shellThread._comm->transmit(reinterpret_cast<uint8_t*>(data), len);
  }
}

void ShellThread::putQueue() {
  osMessageQueuePut(rxQueue, &rxLen, 0, 0);
}

void cdcReceiveCallback(uint8_t*data, uint16_t len){
  rxLen = len;
  memcpy(rxBuffer, data, len);
  shellThread.putQueue();
}


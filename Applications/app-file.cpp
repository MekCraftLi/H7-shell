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

#include "../../Drivers/Peripheral/GPIO/gpio-intf.hpp"
#include "../../Drivers/Services/serv-time.h"
#include "../Drivers/Communications/comm-intf.h"
#include "app-intf.h"
#include "octospi.h"

/* ------- class prototypes-----------------------------------------------------------------------------------------*/

class FileThread : public AppThreadBase {
  public:
    FileThread() : AppThreadBase("File", 512, 4) {}

    void init() override;

    void run() override;

    void putQueue();

  private:
    IComm* _comm;
    IGpio* _gpio;
    osMessageQueueId_t rxQueue;
};




/* ------- macro -----------------------------------------------------------------------------------------------------*/





/* ------- variables -------------------------------------------------------------------------------------------------*/

FileThread fileThread;
AppThreadBase* pFileThread = &fileThread;




/* ------- function implement ----------------------------------------------------------------------------------------*/

void FileThread::init() {
    _comm = pCommOspiFcty->produce(&hospi1);
    auto r =
        p_gpio_lib_fcty->produce(GpioPortEnum::GPIO_PORT_B, GpioPinEnum::GPIO_PIN_10_, GpioModeEnum::GPIO_MODE_AF_PP_);

    _gpio = std::get<IGpio*>(r);


    uint8_t buf[] = {0xA8, 0x66, 0xC8};

    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_10, GPIO_PIN_RESET);
    _comm->transmit(buf, 3);
    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_10, GPIO_PIN_SET);
}

void FileThread::run() { float time = timeServ.getGlobalTimeUs(); }

void FileThread::putQueue() {}
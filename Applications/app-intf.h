/**
*******************************************************************************
* @file    app-intf.h
* @brief
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

#pragma once



/* ------- define ----------------------------------------------------------------------------------------------------*/





/* ------- include ---------------------------------------------------------------------------------------------------*/

#include "FreeRTOS.h"
#include "task.h"
#include <cstdint>
#include <string>




/* ------- class prototypes-------------------------------------------------------------------------------------------*/

class IAppThread {
  public:
    virtual ~IAppThread(){};
    /**
     * @brief 初始化线程所需资源
     */
    virtual void init()                                          = 0;

    /**
     * @brief 线程主循环函数，由FreeRTOS Task调用
     */
    virtual void run()                                           = 0;

    /**
     * @brief 启动线程
     */
    virtual bool start()                                         = 0;

    /**
     * @brief 获取线程信息
     */
    [[nodiscard]] virtual const char* getName() const            = 0;
    [[nodiscard]] virtual uint32_t getRunTime() const            = 0;
    [[nodiscard]] virtual uint32_t getCpuUsage() const           = 0;
    [[nodiscard]] virtual uint32_t getStackHighWaterMark() const = 0;
    [[nodiscard]] virtual TaskHandle_t getTaskHandle() const     = 0;
};




class AppThreadBase : public IAppThread {
  public:
    AppThreadBase(const char* name, uint16_t stackSize, UBaseType_t priority)
        : _name(name), _stackSize(stackSize), _priority(priority), _handle(nullptr) {}

    // 统一启动线程
    bool start() override {
        BaseType_t result = xTaskCreate(taskEntry, // 静态入口函数
                                        _name, _stackSize,
                                        this, // 将对象指针传给任务
                                        _priority, &_handle);
        return result == pdPASS;
    }

    [[nodiscard]] const char* getName() const override { return _name; }


    [[nodiscard]] uint32_t getStackHighWaterMark() const override {
        if (_handle)
            return uxTaskGetStackHighWaterMark(_handle);
        return 0;
    }


    [[nodiscard]] TaskHandle_t getTaskHandle() const override { return _handle; }

    // 可选实现：CPU占用率和运行时间
    [[nodiscard]] uint32_t getRunTime() const override { return 0; }


    [[nodiscard]] uint32_t getCpuUsage() const override { return 0; }

  protected:
    const char* _name;
    uint16_t _stackSize;
    UBaseType_t _priority;
    TaskHandle_t _handle;

    // 静态任务入口，用于FreeRTOS
    [[noreturn]] static void taskEntry(void* pvParameters) {
        auto* threadObj = static_cast<AppThreadBase*>(pvParameters);
        threadObj->init();
        while (true) {
            threadObj->run();
        }
    }
};


/* ------- macro -----------------------------------------------------------------------------------------------------*/





/* ------- variables -------------------------------------------------------------------------------------------------*/

extern AppThreadBase* _p_shell_thread;




/* ------- function implement ----------------------------------------------------------------------------------------*/

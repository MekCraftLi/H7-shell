/**
 *******************************************************************************
 * @file    app-shell.h
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
 * @date    2025/8/26
 * @version 1.0
 *******************************************************************************
 */


/* Define to prevent recursive inclusion -----------------------------------------------------------------------------*/

#pragma once




/*-------- 1. includes and imports -----------------------------------------------------------------------------------*/

#include <cstring>

#include "../../Drivers/Communications/comm-intf.h"
#include "../../Middlewares/Third_Party/LetterShell/log/log.h"
#include "../../Middlewares/Third_Party/LetterShell/shell_cpp.h"
#include "app-intf.h"
#include "queue.h"




/*-------- 2. enum ---------------------------------------------------------------------------------------------------*/


/*-------- 3. interface ---------------------------------------------------------------------------------------------*/



class LogMsg {
  public:
    LogMsg() {}

    LogMsg(const std::string& str) : _len(str.length()) {
        strcpy(_msg, str.c_str());
    }

    LogMsg(const char* str, uint16_t len) : _len(len) {
        strcpy(_msg, str);
    }

    const char* getMsg() { return _msg; }

    [[nodiscard]] uint8_t getMsgLen() const { return _len; }

  private:
    char _msg[256];
    uint16_t _len;
};


class ShellThread final : public AppThreadBase {
  public:
    ShellThread() : AppThreadBase("Shell", 512, 4) {}

    void init() override;

    void run() override;

    void putQueue();

    Shell* getShell() { return &_shell; }
    Log& getLog() { return _log; }

  private:
    IComm* _comm;
    Shell _shell;
    Log _log;

    osMessageQueueId_t _rxQueue;

    static int16_t _shellWrite(char* data, uint16_t len);
    static void _logWrite(char* data, int16_t len);

    friend class Console;
};




class ConsoleThread final : public AppThreadBase {
  public:
    ConsoleThread() : AppThreadBase("Console", 256, 4) {}

    void init() override;

    void run() override;

    void println(const char* fmt, ...) const;
    void error(const char* fmt, ...) const;
    void output(char* data, uint16_t len) const {}

  private:
    xQueueHandle _logQueue;
};
extern ConsoleThread console;
extern ShellThread shellThread;

/*-------- 4. decorator ----------------------------------------------------------------------------------------------*/





/*-------- 5. factories ----------------------------------------------------------------------------------------------*/

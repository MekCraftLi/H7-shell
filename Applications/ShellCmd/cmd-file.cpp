/**
 *******************************************************************************
 * @file    cmd-file.cpp
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


/* ------- define --------------------------------------------------------------------------------------------------*/


/* ------- include ---------------------------------------------------------------------------------------------------*/

#include "cmd-file.h"

/* ------- class prototypes-----------------------------------------------------------------------------------------*/


/* ------- macro -----------------------------------------------------------------------------------------------------*/


/* ------- variables -------------------------------------------------------------------------------------------------*/

static uint8_t fileBuffer[1024];
static uint8_t currentPath[64] = "/";

/* ------- function implement ----------------------------------------------------------------------------------------*/

#include "../app-intf.h"

int cat(int argc, char* argv[]) {
    if (argc < 2) {
        logPrintln("ERROR: Too few arguments");
        return -1;
    }

    memset(fileBuffer, 0, sizeof(fileBuffer));
    auto len = fileThread.readFile(argv[1], fileBuffer, 1024);

    if (len > 0) {
        fileBuffer[len + 1] = 0;
        logPrintln("%s", fileBuffer);
    }
    return len;
}

int echo(int argc, char* argv[]) {
    if (argc < 4) {
        logPrintln("ERROR: Too few arguments");
        return -1;
    }

    if (argv[2][0] != '>') {
        logPrintln("ERROR: param wrong");
    }

    uint16_t len;
    for (int i = 0; argv[1][i] != '\0'; i++) {
        len = i + 1;
    }
    logPrintln("len: %d", len);

    if (argv[2][1] == '>') {
        return fileThread.appendFile(argv[3], argv[1], len);
    }
    return fileThread.writeFile(argv[3], argv[1], len);
}

int touch(int argc, char* argv[]) {
    if (argc < 2) {
        logPrintln("ERROR: Too few arguments");
        return -1;
    }
    return fileThread.createFile(argv[1]);
}

int ls(int argc, char* argv[]) {
    memset(fileBuffer, 0, sizeof(fileBuffer));
    uint8_t* path = nullptr;

    if (argc < 2) {
        path = currentPath;

    } else {
        path = reinterpret_cast<uint8_t*>(argv[1]);
    }

    auto ret = fileThread.readDir(reinterpret_cast<const char*>(path), fileBuffer, 1024);

    if (ret == -1) {
        return ret;
    }

    logPrintln("\r\n\r\n Direction: %s \r\n\r\ntype  name                     size\r\n====  =================        "
               "========\r\n%s\r\n",
               path, fileBuffer);

    return 0;
}
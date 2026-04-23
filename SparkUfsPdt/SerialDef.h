#pragma once
#include "pch.h"

#define WM_FACTORY_CMD_BASE            (WM_USER + 0x1000)
#define FACOTRY_CMD_DOWNLOAD           (WM_FACTORY_CMD_BASE + 0)
#define FACOTRY_CMD_START_TEST         (WM_FACTORY_CMD_BASE + 1)
#define FACOTRY_CMD_TEST_DONE          (WM_FACTORY_CMD_BASE + 2)
#define FACOTRY_CMD_TIME_OUT           (WM_FACTORY_CMD_BASE + 3)
#define FACOTRY_CMD_UNKNOW_CMD         (WM_FACTORY_CMD_BASE + 4)
#define FACOTRY_CMD_IDX(c)             ((c) - WM_FACTORY_CMD_BASE)

enum FactoryCmdType
{
	AUTO_DOWNLOAD = 0,
	START_TEST,
	TIME_OUT
};



static char* FACTORY_CMD_STR[] =
{
	"AUTO_DOWNLOAD",
	"START_TEST",
	"TIME_OUT",
	"NOOP"
};

static char* FACTORY_CMD_RESPONE[] =
{
	"DOWNLOADOK",
	"TEST_DONE",
	"TIME_OUT",
	"NOOP"
};

typedef struct SerialPortRecvHead
{
	HWND NotifyWnd;
	int nUM_RECVDATA;
}SERIALPORTRECVHEAD, * PSERIALPORTRECVHEAD;

#define UM_RECVDATA                 (WM_USER+0x801)

#define MACHINE_DEVICE_CNT  (8)
typedef struct FactoryCmd
{
	BYTE group;
	BYTE device[MACHINE_DEVICE_CNT];
	size_t cnt;
}FACTORYCMD, * PFACTORYCMD;
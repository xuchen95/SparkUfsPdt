#pragma once
#include "pch.h"

// Message base for factory commands
static constexpr UINT WM_FACTORY_CMD_BASE = (WM_USER + 0x1000);
static constexpr UINT FACOTRY_CMD_DOWNLOAD = (WM_FACTORY_CMD_BASE + 0);
static constexpr UINT FACOTRY_CMD_START_TEST = (WM_FACTORY_CMD_BASE + 1);
static constexpr UINT FACOTRY_CMD_TEST_DONE = (WM_FACTORY_CMD_BASE + 2);
static constexpr UINT FACOTRY_CMD_TIME_OUT = (WM_FACTORY_CMD_BASE + 3);
static constexpr UINT FACOTRY_CMD_UNKNOW_CMD = (WM_FACTORY_CMD_BASE + 4);

// Helper to compute index from message id
static constexpr int FACOTRY_CMD_IDX(UINT c) { return static_cast<int>(c - WM_FACTORY_CMD_BASE); }

enum FactoryCmdType
{
	AUTO_DOWNLOAD = 0,
	START_TEST,
	TIME_OUT,
	FACTORY_CMD_MAX
};


static const char* FACTORY_CMD_STR[] =
{
	"AUTO_DOWNLOAD",
	"START_TEST",
	"TIME_OUT",
	"NOOP"
};

static const char* FACTORY_CMD_RESPONE[] =
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

static constexpr UINT UM_RECVDATA = (WM_USER + 0x801);

static constexpr int MACHINE_DEVICE_CNT = 8;
static constexpr size_t FACTORY_PATH_MAX = 1024;

typedef struct FactoryCmd
{
	BYTE group = 0xFF;
	BYTE device[MACHINE_DEVICE_CNT] = { 0 };
	size_t cnt = 0;
	char filePath[FACTORY_PATH_MAX] = { 0 };
}FACTORYCMD, * PFACTORYCMD;

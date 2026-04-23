#pragma once
#include <serial/serial.h>
#include <thread>
#include <atomic>
#include "SerialDef.h"




class CSerialPort
{
public:
    CSerialPort();
    CSerialPort(SERIALPORTRECVHEAD stRecvHead);
    ~CSerialPort();

    bool Open(CString& strPort, UINT nBaudRate = 9600, UINT nByteSize = 8, UINT nParity = 2, UINT nStopBits = 1);
    void Close();
    size_t WriteData(const std::string& data);
    bool IsOpen() const;
    std::vector<serial::PortInfo> listPorts();
    std::vector<serial::PortInfo> EnumDetailSerialPorts();

    void SetNotifyWnd(HWND hWnd) { m_stRecvHead.NotifyWnd = hWnd; }

    void ReadThreadProc();

    serial::Serial* m_pSerial;
    std::thread m_readThread;
    std::atomic<bool> m_bRunning{ false };
    //HWND m_hNotifyWnd{ nullptr };
    SERIALPORTRECVHEAD m_stRecvHead;
public:
    void FactorySendResponse(UINT cmd, CString group = "", CString arg = "");

    void FactoryCmdParser(std::string command_str);
    void FactoryCmdHandler(UINT cmd);
    CList< FactoryCmd, FactoryCmd&> m_SerialCmdList;
    CSemaphore m_cmd_data_mutex;

};

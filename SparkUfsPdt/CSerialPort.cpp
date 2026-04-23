#include "pch.h"
#include "CSerialPort.h"

CSerialPort::CSerialPort() {
    m_bRunning = false;
    m_pSerial = nullptr;
}

CSerialPort::CSerialPort(SERIALPORTRECVHEAD stRecvHead)
{
    m_bRunning = false;
    m_stRecvHead.NotifyWnd = stRecvHead.NotifyWnd;
    m_stRecvHead.nUM_RECVDATA = stRecvHead.nUM_RECVDATA;
    m_pSerial = nullptr;


}

CSerialPort::~CSerialPort() {
    Close();
    delete m_pSerial;
}

bool CSerialPort::Open(CString& strPort, UINT nBaudRate/*=9600*/, UINT nByteSize/*=8*/, UINT nParity/*=0*/, UINT nStopBits/*=1*/)
{
    try
    {
        // 处理COM端口号大于9的情况
        CStringA strPortA;
        if (strPort.GetLength() <= 4)
        {
            strPortA = CT2A(strPort);
        }
        else
        {
            strPortA.Format("\\\\.\\%s", strPort);
        }
        if (m_pSerial != nullptr)
        {
            m_pSerial->close();
            delete m_pSerial;
            m_pSerial = nullptr;
        }
        m_pSerial = new serial::Serial(strPortA.GetBuffer(), nBaudRate, serial::Timeout(1500, 1500, 1500, 1500, 1500),
            (serial::bytesize_t)nByteSize, (serial::parity_t)nParity, (serial::stopbits_t)nStopBits);

        //m_pSerial->open();  构造函数时打开串口，不需要显式打开串口
        if (m_pSerial->isOpen())
        {
            m_bRunning = true;
            m_readThread = std::thread(&CSerialPort::ReadThreadProc, this);
            return true;
        }

    }
    catch (const std::exception& e)
    {
        CString strErr;
        strErr.Format(_T("Open Error: %S"), e.what());
        //AfxMessageBox(strErr);
        return false;
    }
    return false;
}

void CSerialPort::Close()
{
    m_bRunning = false;
    if (m_readThread.joinable())
    {
        m_readThread.join();
    }
    if (m_pSerial)
    {
        if (m_pSerial->isOpen())
        {
            m_pSerial->close();
        }
    }
}

size_t CSerialPort::WriteData(const std::string& data)
{
    try
    {
        if (m_pSerial->isOpen())
        {
            return m_pSerial->write(data);
        }
        return 0;
    }
    catch (const std::exception& e) {
        CString strErr;
        strErr.Format(_T("Write Error: %S"), e.what());
        AfxMessageBox(strErr);
    }
}

bool CSerialPort::IsOpen() const
{
    if (m_pSerial)
        return m_pSerial->isOpen();
    else
        return false;
}

std::vector<serial::PortInfo> CSerialPort::listPorts()
{
    return serial::list_ports();
}

std::vector<serial::PortInfo> CSerialPort::EnumDetailSerialPorts()
{
    return serial::EnumDetailSerialPorts();
}

void CSerialPort::ReadThreadProc()
{
    while (m_bRunning && m_pSerial->isOpen())
    {
        try {
            // 读取数据
            int len = m_pSerial->available();
            if (len)
            {
                Sleep(100);
                len = m_pSerial->available();
                std::string data = m_pSerial->readline(65536, "+");
                if (!data.empty() && m_stRecvHead.NotifyWnd)
                {
                    FactoryCmdParser(data);
                    /*::PostMessage(m_stRecvHead.NotifyWnd,m_stRecvHead.nUM_RECVDATA,(WPARAM)new std::string(data),0);*/
                }
            }
            Sleep(100);

        }
        catch (serial::IOException& e) {
            break;
            CString strErr;
            strErr.Format(_T("Read loop exited, Please check. error_info: %S"), e.what());
            AfxMessageBox(strErr);
        }
    }
}

void CSerialPort::FactorySendResponse(UINT cmd, CString group, CString arg)
{
    CString strResult;
    strResult.Format("@%s", FACTORY_CMD_RESPONE[cmd]);
    if (group != "" && arg != "")
    {
        strResult.AppendFormat("%s", " ");
        strResult.AppendFormat("%s", group);
        strResult.AppendFormat("%s", arg);
    }
    strResult.AppendFormat("+");
    std::string data;
    
    data.append(strResult.GetBuffer(), strResult.GetLength());
    WriteData(data);
}

void CSerialPort::FactoryCmdParser(std::string command_str)
{
    size_t pos;
    FactoryCmd stCmd;
    ZeroMemory(&stCmd, sizeof(FactoryCmd));
    std::string command, group, args_str;
    CSingleLock slock(&m_cmd_data_mutex);

    slock.Lock();
    if (slock.IsLocked())
    {
        // Parse the command
        if (command_str.at(0) != '@' ||
            command_str.at(command_str.length() - 1) != '+')
        {
            return;
        }

        command_str = command_str.substr(1, command_str.length() - 2);
        pos = command_str.find(" ");
        if (pos == std::string::npos)
        {
            command = command_str;
            args_str = "";
        }
        else
        {
            command = command_str.substr(0, pos);
            command_str = command_str.substr(pos + 1, command_str.length() - 2);
            pos = command_str.find(" ");
            if (pos == std::string::npos)
            {
                //unknown cmd
                group = "ff";
                args_str = "";
            }
            else
            {
                group = command_str.substr(0, pos);
                args_str = command_str.substr(pos + 1, command_str.length() - 2);
                stCmd.group = strtoul(group.c_str(), nullptr, 16);
                for (int i = 0; i < MACHINE_DEVICE_CNT; ++i)
                {
                    stCmd.device[i] = args_str.at(i) - '0';
                    if (stCmd.device[i])
                    {
                        stCmd.cnt++;
                    }
                }

                m_SerialCmdList.AddTail(stCmd);
            }
        }
        pos = 0;
        while (strcmp(FACTORY_CMD_STR[pos], "NOOP"))
        {
            if (command == FACTORY_CMD_STR[pos])
            {
                slock.Unlock();
                return FactoryCmdHandler(WM_FACTORY_CMD_BASE + pos);
            }
            pos++;
        }
        slock.Unlock();
        return FactoryCmdHandler(FACOTRY_CMD_UNKNOW_CMD);
    }
}

void CSerialPort::FactoryCmdHandler(UINT cmd)
{
    switch (cmd)
    {
    case FACOTRY_CMD_DOWNLOAD:
        ::SendMessage(m_stRecvHead.NotifyWnd, FACOTRY_CMD_DOWNLOAD, 0, 0);
        break;
    case FACOTRY_CMD_START_TEST:
        ::SendMessage(m_stRecvHead.NotifyWnd, FACOTRY_CMD_START_TEST, 0, 0);
        break;
    case FACOTRY_CMD_TEST_DONE:
        break;
    case FACOTRY_CMD_TIME_OUT:
        break;
    case FACOTRY_CMD_UNKNOW_CMD:
    default:
        break;
    }
}

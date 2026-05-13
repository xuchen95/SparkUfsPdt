#include "pch.h"
#include "CSerialPort.h"
#include <sstream>

namespace
{
    void TrimRight(std::string& s)
    {
        while (!s.empty() && (s.back() == '\r' || s.back() == '\n' || s.back() == ' ' || s.back() == '\t'))
        {
            s.pop_back();
        }
    }

    std::vector<std::string> SplitBySpace(const std::string& s)
    {
        std::vector<std::string> out;
        std::istringstream iss(s);
        std::string t;
        while (iss >> t)
        {
            out.push_back(t);
        }
        return out;
    }
}

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

CString CSerialPort::GetCmdLogFilePath() const
{
    TCHAR currentDirectory[MAX_PATH] = {};
    GetCurrentDirectory(MAX_PATH, currentDirectory);
    CString path;
    path.Format(_T("%s\\FactorySerialCmd.log"), currentDirectory);
    return path;
}

bool CSerialPort::IsCmdLogEnabled() const
{
    TCHAR currentDirectory[MAX_PATH] = {};
    GetCurrentDirectory(MAX_PATH, currentDirectory);
    CString iniPath;
    iniPath.Format(_T("%s\\BoostSetting.ini"), currentDirectory);

    // default enabled
    UINT enabled = GetPrivateProfileInt(_T("Base"), _T("EnableFactoryCmdLog"), 1, iniPath);
    return enabled != 0;
}

void CSerialPort::AppendCmdLog(const CString& direction, const CStringA& command)
{
    if (!IsCmdLogEnabled())
    {
        return;
    }

    CSingleLock lock(&m_log_file_mutex);
    lock.Lock();
    if (!lock.IsLocked())
    {
        return;
    }

    CTime now = CTime::GetCurrentTime();
    CStringA ts(now.Format(_T("%Y-%m-%d %H:%M:%S")));
    CStringA line;
    line.Format("[%s] [%s] %s\r\n", ts.GetString(), direction.GetString(), command.GetString());

    CFile file;
    CString path = GetCmdLogFilePath();
    DWORD flags = CFile::modeCreate | CFile::modeNoTruncate | CFile::modeWrite | CFile::shareDenyNone;
    if (file.Open(path, flags))
    {
        file.SeekToEnd();
        file.Write(line.GetString(), static_cast<UINT>(line.GetLength()));
        file.Close();
    }
}

bool CSerialPort::Open(CString& strPort, UINT nBaudRate/*=9600*/, UINT nByteSize/*=8*/, UINT nParity/*=0*/, UINT nStopBits/*=1*/)
{
    try
    {
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

        if (m_pSerial->isOpen())
        {
            m_bRunning = true;
            m_readThread = std::thread(&CSerialPort::ReadThreadProc, this);
            return true;
        }
        else
        {
            CString strErr;
            strErr.Format(_T("无法打开串口：%s (波特率：%u，数据位：%u，奇偶校验：%u，停止位：%u)"),
                strPort, nBaudRate, nByteSize, nParity, nStopBits);
            AfxMessageBox(strErr, MB_ICONERROR | MB_OK);
            return false;
        }
    }
    catch (const std::exception& e)
    {
        CString strErr;
        strErr.Format(_T("打开串口时发生异常 %s: %S"), strPort, e.what());
        AfxMessageBox(strErr, MB_ICONERROR | MB_OK);
        return false;
    }
    catch (...)
    {
        CString strErr;
        strErr.Format(_T("打开串口时发生未知错误：%s"), strPort);
        AfxMessageBox(strErr, MB_ICONERROR | MB_OK);
        return false;
    }
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
        if (m_pSerial && m_pSerial->isOpen())
        {
            return m_pSerial->write(data);
        }
        return 0;
    }
    catch (const std::exception& e) {
        CString strErr;
        strErr.Format(_T("写入错误: %S"), e.what());
        AfxMessageBox(strErr);
    }
    return 0;
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

void CSerialPort::DiagnosticCheckSerialPorts()
{
    OutputDebugString(_T("[Diagnostic] Checking available serial ports...\n"));
    
    try
    {
        std::vector<serial::PortInfo> ports = EnumDetailSerialPorts();
        
        CString msg;
        msg.Format(_T("[Diagnostic] Found %d serial port(s)\n"), static_cast<int>(ports.size()));
        OutputDebugString(msg);
        
        if (ports.empty())
        {
            OutputDebugString(_T("[Diagnostic] ERROR: No serial ports found!\n"));
            OutputDebugString(_T("[Diagnostic] Possible reasons:\n"));
            OutputDebugString(_T("  1. No USB-to-Serial adapter connected\n"));
            OutputDebugString(_T("  2. Serial port driver not installed\n"));
            OutputDebugString(_T("  3. Device Manager doesn't show COM port\n"));
            OutputDebugString(_T("  4. Device disabled or not recognized\n"));
            return;
        }
        
        for (size_t i = 0; i < ports.size(); ++i)
        {
            const auto& p = ports[i];
            CString portName(p.port.c_str());
            CString description(p.description.c_str());
            CString hardwareId(p.hardware_id.c_str());
            
            CString portDetail;
            portDetail.Format(_T("[Diagnostic] Port %d: %s\n"), i + 1, portName);
            OutputDebugString(portDetail);
            
            CString descDetail;
            descDetail.Format(_T("  Description: %s\n"), description);
            OutputDebugString(descDetail);
            
            CString hwDetail;
            hwDetail.Format(_T("  Hardware ID: %s\n"), hardwareId);
            OutputDebugString(hwDetail);
        }
    }
    catch (const std::exception& e)
    {
        CString errMsg;
        errMsg.Format(_T("[Diagnostic] Exception during port enumeration: %S\n"), e.what());
        OutputDebugString(errMsg);
    }
    catch (...)
    {
        OutputDebugString(_T("[Diagnostic] Unknown exception during port enumeration\n"));
    }
}

void CSerialPort::ReadThreadProc()
{
    while (m_bRunning && m_pSerial && m_pSerial->isOpen())
    {
        try {
            int len = static_cast<int>(m_pSerial->available());
            if (len)
            {
                Sleep(50);
                try
                {
                    std::string data = m_pSerial->readline(65536, "+");
                    if (!data.empty() && m_stRecvHead.NotifyWnd)
                    {
                        if (data.back() != '+')
                        {
                            data.push_back('+');
                        }
                        AppendCmdLog(_T("RX"), CStringA(data.c_str()));
                        
                        // Add exception handling for FactoryCmdParser
                        try
                        {
                            FactoryCmdParser(data);
                        }
                        catch (const std::exception& e)
                        {
                            CStringA errMsg;
                            errMsg.Format("[ERROR] FactoryCmdParser exception: %s\n", e.what());
                            AppendCmdLog(_T("ERROR"), errMsg);
                        }
                        catch (...)
                        {
                            AppendCmdLog(_T("ERROR"), CStringA("[ERROR] FactoryCmdParser unknown exception\n"));
                        }
                    }
                }
                catch (const std::exception& e)
                {
                    CStringA errMsg;
                    errMsg.Format("[ERROR] readline exception: %s\n", e.what());
                    OutputDebugString(CA2T(errMsg));
                    break;
                }
                catch (...)
                {
                    OutputDebugString(_T("[ERROR] readline unknown exception\n"));
                    break;
                }
            }
            Sleep(20);
        }
        catch (serial::IOException&) {
            OutputDebugString(_T("[ReadThread] Serial IO Exception - breaking read loop\n"));
            break;
        }
        catch (const std::exception& e) {
            CString errMsg;
            errMsg.Format(_T("[ReadThread] Exception: %S\n"), e.what());
            OutputDebugString(errMsg);
            break;
        }
        catch (...) {
            OutputDebugString(_T("[ReadThread] Unknown exception - breaking read loop\n"));
            break;
        }
    }
}

void CSerialPort::FactorySendResponse(UINT cmd, CString group, CString arg)
{
    int idx = FACOTRY_CMD_IDX(cmd);
    if (idx < AUTO_DOWNLOAD || idx >= FACTORY_CMD_MAX)
    {
        return;
    }

    CString strResult;
    strResult.Format(_T("@%S"), FACTORY_CMD_RESPONE[idx]);

    group.Trim();
    arg.Trim();

    if (!group.IsEmpty())
    {
        strResult.Append(_T(" "));
        strResult.Append(group);
    }
    if (!arg.IsEmpty())
    {
        strResult.Append(_T(" "));
        strResult.Append(arg);
    }

    strResult.Append(_T("+"));

    CStringA outA(strResult);
    AppendCmdLog(_T("TX"), outA);
    WriteData(std::string(outA.GetString()));
}

void CSerialPort::SendGroupTestDone(BYTE group, const WORD resultCode[MACHINE_DEVICE_CNT])
{
    CString groupStr;
    groupStr.Format(_T("%02X"), group);

    CString arg;
    for (int i = 0; i < MACHINE_DEVICE_CNT; ++i)
    {
        CString one;
        one.Format(_T("%04X"), resultCode[i]);
        if (!arg.IsEmpty())
        {
            arg.Append(_T(" "));
        }
        arg.Append(one);
    }

    FactorySendResponse(FACOTRY_CMD_TEST_DONE, groupStr, arg);
}

bool CSerialPort::PopFactoryCmd(FACTORYCMD& outCmd)
{
    CSingleLock lock(&m_cmd_data_mutex);
    lock.Lock();
    if (!lock.IsLocked() || m_SerialCmdList.IsEmpty())
    {
        return false;
    }

    outCmd = m_SerialCmdList.RemoveHead();
    return true;
}

void CSerialPort::FactoryCmdParser(std::string command_str)
{
    try
    {
        TrimRight(command_str);
        if (command_str.size() < 3)
        {
            FactoryCmdHandler(FACOTRY_CMD_UNKNOW_CMD);
            return;
        }

        if (command_str.front() != '@' || command_str.back() != '+')
        {
            FactoryCmdHandler(FACOTRY_CMD_UNKNOW_CMD);
            return;
        }

        const std::string body = command_str.substr(1, command_str.length() - 2);
        const std::vector<std::string> tokens = SplitBySpace(body);
        if (tokens.empty())
        {
            FactoryCmdHandler(FACOTRY_CMD_UNKNOW_CMD);
            return;
        }

        FactoryCmd stCmd;
        ZeroMemory(&stCmd, sizeof(stCmd));

        const std::string& command = tokens[0];

        if (command == "AUTO_DOWNLOAD")
        {
            const std::string prefix = "AUTO_DOWNLOAD ";
            if (body.size() <= prefix.size())
            {
                FactoryCmdHandler(FACOTRY_CMD_UNKNOW_CMD);
                return;
            }

            std::string path = body.substr(prefix.size());
            if (path.empty())
            {
                FactoryCmdHandler(FACOTRY_CMD_UNKNOW_CMD);
                return;
            }
            
            if (path.size() >= FACTORY_PATH_MAX)
            {
                OutputDebugString(_T("[FactoryCmdParser] WARNING: Download path exceeds maximum length\n"));
                path.resize(FACTORY_PATH_MAX - 1);
            }
            
            memcpy(stCmd.filePath, path.c_str(), path.size());
            stCmd.filePath[path.size()] = '\0';

            CSingleLock lock(&m_cmd_data_mutex);
            lock.Lock();
            if (lock.IsLocked())
            {
                m_SerialCmdList.AddTail(stCmd);
            }
            else
            {
                OutputDebugString(_T("[FactoryCmdParser] WARNING: Failed to acquire command list lock\n"));
            }

            FactoryCmdHandler(FACOTRY_CMD_DOWNLOAD);
            return;
        }

        if (command == "START_TEST")
        {
            if (tokens.size() < 3)
            {
                FactoryCmdHandler(FACOTRY_CMD_UNKNOW_CMD);
                return;
            }

            try
            {
                stCmd.group = static_cast<BYTE>(strtoul(tokens[1].c_str(), nullptr, 16));
            }
            catch (...)
            {
                OutputDebugString(_T("[FactoryCmdParser] ERROR: Failed to parse group value\n"));
                FactoryCmdHandler(FACOTRY_CMD_UNKNOW_CMD);
                return;
            }

            const std::string& bitmap = tokens[2];
            if (bitmap.size() < MACHINE_DEVICE_CNT)
            {
                FactoryCmdHandler(FACOTRY_CMD_UNKNOW_CMD);
                return;
            }

            for (int i = 0; i < MACHINE_DEVICE_CNT; ++i)
            {
                stCmd.device[i] = (bitmap[i] == '1') ? 1 : 0;
                if (stCmd.device[i])
                {
                    stCmd.cnt++;
                }
            }

            CSingleLock lock(&m_cmd_data_mutex);
            lock.Lock();
            if (lock.IsLocked())
            {
                m_SerialCmdList.AddTail(stCmd);
            }
            else
            {
                OutputDebugString(_T("[FactoryCmdParser] WARNING: Failed to acquire command list lock\n"));
            }

            FactoryCmdHandler(FACOTRY_CMD_START_TEST);
            return;
        }

        if (command == "TIME_OUT")
        {
            FactoryCmdHandler(FACOTRY_CMD_TIME_OUT);
            return;
        }

        FactoryCmdHandler(FACOTRY_CMD_UNKNOW_CMD);
    }
    catch (const std::exception& e)
    {
        CStringA errMsg;
        errMsg.Format("[FactoryCmdParser] Exception: %s\n", e.what());
        OutputDebugString(CA2T(errMsg));
        FactoryCmdHandler(FACOTRY_CMD_UNKNOW_CMD);
    }
    catch (...)
    {
        OutputDebugString(_T("[FactoryCmdParser] Unknown exception\n"));
        FactoryCmdHandler(FACOTRY_CMD_UNKNOW_CMD);
    }
}

void CSerialPort::FactoryCmdHandler(UINT cmd)
{
    try
    {
        if (!m_stRecvHead.NotifyWnd)
        {
            OutputDebugString(_T("[FactoryCmdHandler] WARNING: NotifyWnd is null\n"));
            return;
        }

        // Validate window handle
        if (!IsWindow(m_stRecvHead.NotifyWnd))
        {
            OutputDebugString(_T("[FactoryCmdHandler] ERROR: NotifyWnd is not a valid window\n"));
            return;
        }

        switch (cmd)
        {
        case FACOTRY_CMD_DOWNLOAD:
        case FACOTRY_CMD_START_TEST:
        case FACOTRY_CMD_TIME_OUT:
        case FACOTRY_CMD_UNKNOW_CMD:
            if (!::PostMessage(m_stRecvHead.NotifyWnd, cmd, 0, 0))
            {
                OutputDebugString(_T("[FactoryCmdHandler] ERROR: Failed to post message\n"));
            }
            break;
        case FACOTRY_CMD_TEST_DONE:
        default:
            break;
        }
    }
    catch (const std::exception& e)
    {
        CStringA errMsg;
        errMsg.Format("[FactoryCmdHandler] Exception: %s\n", e.what());
        OutputDebugString(CA2T(errMsg));
    }
    catch (...)
    {
        OutputDebugString(_T("[FactoryCmdHandler] Unknown exception\n"));
    }
}

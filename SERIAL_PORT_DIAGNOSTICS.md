# 串口诊断指南

## 问题：串口扫描没有端口

当程序启动时，串口下拉框为空，说明系统中没有检测到 COM 端口。

---

## 诊断步骤

### 第 1 步：查看诊断输出

1. 在 Visual Studio 中打开**输出**窗口：
   - 菜单 → View → Output (或按 Ctrl+Alt+O)

2. 程序启动时会输出完整的诊断报告，格式如下：

```
=== [SERIAL PORT DIAGNOSTIC REPORT] ===

[1. CONFIGURATION]
  COM Port: COM1
  BaudRate: 9600
  ByteSize: 8
  Parity: 0
  StopBits: 0

[2. AVAILABLE PORTS]
  WARNING: No COM ports detected!

  Troubleshooting steps:
  1. Check Device Manager (devmgmt.msc):
     - Look for 'Ports (COM & LPT)'
     - Should show COM port (e.g., COM3)
     - If not present, device may not be installed
  2. Verify USB adapter:
     - Physical connection established
     - Not disabled by BIOS
  3. Install drivers:
     - Download from device manufacturer
     - Restart after installation
  4. Check INI configuration:
     - Edit BoostSetting.ini [Base] section
     - Set COM to matching port from Device Manager

[3. CONNECTION STATUS]
  Connected: No
  Serial Port Open: No

[4. DROPDOWN STATUS]
  Items in dropdown: 1
  Currently selected: COM1

=== [END DIAGNOSTIC REPORT] ===
```

---

### 第 2 步：检查设备管理器

#### Windows 10/11：
1. 按 `Windows + X` 打开菜单
2. 选择 **设备管理器** (Device Manager)
3. 展开 **端口 (COM 和 LPT)** (Ports)

#### 预期看到：
- ✅ 一个或多个 COM 端口，如：
  - COM3 - USB Serial Port
  - COM4 - CH340 USB UART

#### 如果看不到：
- ❌ 端口列表为空
- ❌ 设备显示在"未知设备"下
- ❌ 设备显示黄色感叹号（驱动错误）

---

### 第 3 步：物理连接检查

如果是 USB 转串口适配器：

```
步骤 1：检查 USB 连接
  ✓ USB 电缆完好无损
  ✓ 连接到计算机的 USB 端口
  ✓ USB 端口有电（试试连接其他 USB 设备）
  ✓ 适配器指示灯闪烁（如果有的话）

步骤 2：重新插拔
  1. 拔下 USB 电缆
  2. 等待 5 秒
  3. 重新插入
  4. 打开设备管理器刷新（F5）

步骤 3：尝试不同的 USB 端口
  - 如果有 USB 2.0 和 USB 3.0，试试另一个
  - 避免使用 USB 集线器
```

---

### 第 4 步：驱动程序检查

#### 常见的 USB 转串口芯片：

| 芯片型号 | 驱动程序 | 下载地址 |
|---------|--------|--------|
| CH340 | CH340 驱动 | https://sparksys.com/product/USB-TTL-CH340/ |
| PL2303 | ProlificUSB2COM | https://www.prolific.com.tw/US/ShowProduct.aspx?p_id=229 |
| FT232 | FTDI 驱动 | https://ftdichip.com/drivers/ |
| CP2102 | CP210x 驱动 | https://www.silabs.com/developers/usb-to-uart-bridge-vcp-drivers |

#### 安装驱动程序步骤：

1. **确定芯片型号**
   - 查看适配器上的标注
   - 或在设备管理器中找到未知设备的硬件 ID

2. **下载驱动**
   - 从上表中的链接下载
   - 下载 Windows 版本的驱动

3. **安装驱动**
   - 解压驱动文件
   - 运行 setup.exe
   - 按照安装向导进行
   - **重启计算机**

4. **验证安装**
   - 打开设备管理器
   - 查看 COM 端口是否出现

---

### 第 5 步：BIOS 设置检查

如果使用的是计算机的内置 COM 端口（不是 USB 适配器）：

1. 重启计算机，进入 BIOS（按 Del、F2 或 F10，取决于制造商）
2. 找到 **Integrated Peripherals** 或 **Onboard Devices**
3. 确保 **Serial Port** 或 **COM Port** 为 **Enabled**
4. 检查分配的 I/O 地址和 IRQ：
   - 标准设置：COM1 = 3F8h, IRQ 4
5. 保存设置并重启

---

### 第 6 步：INI 配置验证

检查 `BoostSetting.ini` 文件：

```ini
[Base]
COM=COM1
BaudRate=9600
ByteSize=8
Parity=0
StopBits=0
EnableFactoryCmdLog=1
LastSettingPath=...
```

**关键点：**
- `COM=COM1` 必须与设备管理器中的端口名称一致
- 如果设备管理器显示 COM5，则改为 `COM=COM5`
- 保存文件后重启程序

---

## 故障排除流程

```
程序启动
  ↓
[诊断报告] 检查输出窗口
  ↓
Found 0 port(s)?
  ├─ YES → 未检测到硬件
  │   ↓
  │   打开设备管理器检查
  │   ├─ 看不到 COM 端口
  │   │   ↓
  │   │   → 驱动程序丢失或损坏
  │   │   → 安装/重新安装驱动程序
  │   │   → 重启计算机
  │   │   → 重新检查设备管理器
  │   │
  │   └─ 看到 COM 端口但程序找不到
  │       ↓
  │       → 更新 BoostSetting.ini 中的 COM 值
  │       → 重启程序
  │
  └─ NO → 找到 COM 端口
      ↓
      下拉框中有端口?
      ├─ YES → 正常，尝试连接
      │   ↓
      │   → 从下拉框选择端口
      │   → 检查状态灯是否变绿
      │
      └─ NO → 下拉框初始化问题
          ↓
          → 重启程序
          → 清除 BoostSetting.ini
          → 让程序重新创建配置文件
```

---

## 高级诊断

### 在 Visual Studio 中调用诊断函数

如果需要在程序运行时重新扫描：

1. 打开**立即窗口**（Debug → Windows → Immediate）
2. 在调试状态下输入（如果有公开方法）：
   ```
   m_pMainDlg->DiagnosticSerialPortStatus()
   ```

输出会显示完整的诊断报告。

---

## 常见问题解答

### Q1: "Found 0 port(s)" - 如何修复？

**A:** 按以下优先级尝试：
1. ✓ 检查 USB 电缆连接
2. ✓ 在设备管理器中查找 COM 端口
3. ✓ 安装/更新驱动程序
4. ✓ 重启计算机
5. ✓ 尝试不同的 USB 端口

---

### Q2: 设备管理器中有 COM 端口，但程序找不到？

**A:** 
1. 记下设备管理器中显示的 COM 端口号（如 COM5）
2. 编辑 `BoostSetting.ini`
3. 将 `COM=COM1` 改为 `COM=COM5`
4. 保存并重启程序

---

### Q3: 端口显示但无法连接？

**A:** 检查以下几点：
1. 下拉框中的端口选择正确
2. 串口参数正确：
   - 波特率：通常 9600, 115200
   - 数据位：8
   - 奇偶校验：None (0)
   - 停止位：1 (0)
3. 另一个程序没有占用该端口
4. 重新插拔 USB 连接

---

### Q4: 诊断报告中的黄色感叹号是什么意思？

**A:** 表示驱动程序有问题：
1. 在设备管理器中右键点击设备
2. 选择 **更新驱动程序**
3. 选择 **浏览计算机以查找驱动**
4. 指向驱动程序文件夹
5. 重启计算机

---

## 日志文件位置

诊断信息保存在以下位置：

```
Visual Studio 输出窗口
  ↓
  [Diagnostic] ... 开头的所有行
  [Factory Serial] ... 开头的所有行
  [Serial Port Scan] ... 开头的所有行

串口命令日志（如果启用）
  ↓
  FactorySerialCmd.log （在程序工作目录）
```

---

## 联系技术支持

如果按照以上步骤仍然无法解决，请收集以下信息：

1. 完整的诊断报告（Output 窗口的全部内容）
2. 设备管理器的截图
3. BoostSetting.ini 文件内容
4. 操作系统版本和 USB 适配器型号
5. 重现问题的具体步骤

---

## 相关文档

- [串口配置参数说明](./SERIAL_CONFIG.md)
- [程序启动流程](./STARTUP_FLOW.md)
- [错误代码参考](./ERROR_CODES.md)

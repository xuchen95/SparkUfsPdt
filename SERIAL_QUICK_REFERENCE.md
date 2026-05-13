# 串口诊断 - 快速参考卡片

## 症状 → 解决方案

### 症状 1：下拉框没有端口

```
输出窗口看到：
[Serial Port Scan] Found 0 port(s)
```

**快速诊断：**
1. 打开设备管理器 → 端口 (COM & LPT)
2. 看不到 COM 端口？→ 转到 [安装驱动程序]
3. 看到 COM 口（如 COM5）？→ 编辑 BoostSetting.ini，改 COM=COM5

**关键文件：**
- `BoostSetting.ini` - 检查 `[Base]` 段中的 `COM=` 设置
- 设备管理器 - 查看实际的 COM 端口号

---

### 症状 2：端口在下拉框中，但连接失败

```
输出窗口看到：
Failed to connect to COM5
```

**快速诊断：**
1. 检查波特率和参数
   - 输出窗口应该显示尝试的参数
   - 通常：9600, 8, 0, 0
2. 重新插拔 USB 连接
3. 尝试另一个 USB 端口
4. 重启程序

**检查清单：**
- [ ] USB 电缆完好
- [ ] USB 端口有电
- [ ] 设备指示灯闪烁
- [ ] 没有其他程序占用端口

---

### 症状 3：驱动程序错误

```
设备管理器中看到：
- 黄色感叹号
- "未知设备"
- "需要驱动程序"
```

**快速修复：**
1. 确定 USB 芯片型号（看适配器或设备信息）
2. 从表格中找到对应驱动
3. 下载并安装
4. **重启计算机**
5. 检查设备管理器

**常见芯片和驱动：**
| 芯片 | 驱动下载 |
|------|--------|
| CH340 | https://sparksys.com/ |
| PL2303 | https://www.prolific.com.tw/ |
| FT232 | https://ftdichip.com/drivers/ |
| CP2102 | https://www.silabs.com/ |

---

## 诊断输出读取指南

### 完整诊断报告示例

```
=== [SERIAL PORT DIAGNOSTIC REPORT] ===

[1. CONFIGURATION]
  COM Port: COM1          ← INI 中配置的端口
  BaudRate: 9600         ← 波特率
  ByteSize: 8            ← 数据位
  Parity: 0              ← 奇偶校验 (0=None)
  StopBits: 0            ← 停止位 (0=1bit)

[2. AVAILABLE PORTS]
  Found 2 port(s):
  [1] COM3 - USB Serial Port
  [2] COM5 - Serial Port

[3. CONNECTION STATUS]
  Connected: Yes         ← 连接成功
  Serial Port Open: Yes

[4. DROPDOWN STATUS]
  Items in dropdown: 2   ← 下拉框中有 2 个项
  Currently selected: COM3
```

### 诊断前置条件

如果看到以下警告：

```
WARNING: No COM ports detected!
  Troubleshooting steps:
  1. Check Device Manager (devmgmt.msc)
     - Look for 'Ports (COM & LPT)'
  2. Verify USB adapter
     - Physical connection established
  3. Install drivers
     - Download from device manufacturer
  4. Check INI configuration
```

这意味着：
- ❌ 系统中没有识别的 COM 端口
- ❌ USB 适配器可能未连接或驱动丢失
- ✅ 按照提示的 4 个步骤逐一检查

---

## BoostSetting.ini 参考

### [Base] 段 - 串口参数

```ini
[Base]
COM=COM1                  # 默认连接的端口
BaudRate=9600             # 波特率 (300-921600)
ByteSize=8                # 数据位 (5-8)
Parity=0                  # 奇偶校验: 0=None, 1=Odd, 2=Even
StopBits=0                # 停止位: 0=1, 1=1.5, 2=2
```

### 常见波特率

- 2400, 4800, 9600, 19200, 38400, 57600, **115200**

### 常用参数组合

| 应用 | BaudRate | ByteSize | Parity | StopBits |
|------|----------|----------|--------|----------|
| 标准 | 9600 | 8 | 0 | 0 |
| 高速 | 115200 | 8 | 0 | 0 |
| 工业 | 19200 | 8 | 2 | 0 |

---

## 诊断命令速查

### Visual Studio 输出窗口

打开方式：
- 菜单：Debug → Windows → Output
- 快捷键：Ctrl + Alt + O
- 菜单：View → Output

### 过滤诊断信息

在输出窗口顶部的搜索框输入：
- `[Diagnostic]` - 仅显示诊断检查
- `[Serial Port Scan]` - 仅显示扫描结果
- `[Factory Serial]` - 仅显示工厂串口信息
- `[` - 显示所有带前缀的诊断信息

---

## 故障排除决策树

```
程序启动
  ↓
查看输出窗口诊断报告
  ↓
  Found N port(s)?
  ├─ 0: 转 → 没有端口检测到
  └─ >0: 转 → 检查连接状态

【没有端口检测到】
  ↓
设备管理器中看到 COM 端口?
  ├─ 否: 转 → 安装驱动程序
  └─ 是: 转 → 检查 INI 配置

【安装驱动程序】
  ↓
识别 USB 芯片型号
  ↓
从上表下载驱动
  ↓
安装并重启计算机
  ↓
回到 → 没有端口检测到

【检查 INI 配置】
  ↓
编辑 BoostSetting.ini
  ↓
将 COM= 改为设备管理器显示的端口
  ↓
保存并重启程序
  ↓
转 → 检查连接状态

【检查连接状态】
  ↓
诊断报告中 Connected = Yes?
  ├─ 是: ✅ 正常工作
  └─ 否: 转 → 连接失败排查

【连接失败排查】
  ↓
输出是否显示异常信息?
  ├─ 是: 查看错误信息，可能需要更新驱动
  └─ 否: 
      ├─ 重新插拔 USB
      ├─ 尝试另一个 USB 端口
      ├─ 检查波特率和参数
      └─ 重启程序
```

---

## 日志文件

### 位置

程序工作目录中：
- `FactorySerialCmd.log` - 所有串口收发命令
- `BoostSetting.ini` - 配置文件

### 清空诊断日志

如需从头诊断，删除此文件：
- `FactorySerialCmd.log`

重启程序后会自动创建新文件。

---

## 常见参数错误

### ❌ 错误 1：波特率为 0
```ini
BaudRate=0              # 无效！
```
✅ 改为：
```ini
BaudRate=9600           # 或其他标准波特率
```

### ❌ 错误 2：数据位为 0
```ini
ByteSize=0              # 无效！
```
✅ 改为：
```ini
ByteSize=8              # 标准 8 位数据
```

### ❌ 错误 3：COM 口名不匹配
```ini
COM=COM1                # INI 中配置
```
但设备管理器显示：COM5
✅ 改为：
```ini
COM=COM5                # 与设备管理器一致
```

---

## 快速修复命令

### 重置配置到默认值

编辑 BoostSetting.ini，在 [Base] 段中设置：

```ini
[Base]
COM=COM1
BaudRate=9600
ByteSize=8
Parity=0
StopBits=0
PortBaseSel=0
PortMappingSel=0
ForceRomMode=0
SnSeparateIni=0
EnableFactoryCmdLog=1
```

保存并重启程序。

---

## 需要帮助？

1. **检查诊断输出** - 复制完整的诊断报告
2. **收集信息**：
   - 操作系统版本
   - USB 适配器型号
   - 设备管理器截图
   - 诊断报告全文
3. **联系技术支持** - 提供上述信息

---

## 版本历史

- **v1.0** (2024-XX-XX) 
  - 初始诊断系统
  - 自动检测和报告
  - 详细故障排除指南


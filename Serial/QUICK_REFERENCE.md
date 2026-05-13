# CSerialPort Windows 修复快速参考指南

## 已修复项

- ✅ 互斥量创建失败未检查
- ✅ 构造阶段 open() 失败时资源清理
- ✅ `reconfigurePort()` 失败后句柄/状态不同步
- ✅ `read()`/`write()` 缺少线程锁保护
- ✅ `waitReadable()` / `waitByteTimes()` 未实现

## 关键行为变化

### 1) 构造函数更安全
- `CreateMutex` 失败会立即抛异常
- 第二个互斥量失败会清理第一个
- 自动 `open()` 失败会回收互斥量

### 2) 读写线程安全
- `read()` 内部自动 `readLock()/readUnlock()`
- `write()` 内部自动 `writeLock()/writeUnlock()`
- 异常路径也会尝试释放锁，降低死锁风险

### 3) 端口重配置失败状态一致
- `SetCommState/SetCommTimeouts` 失败后：
  - `fd_ = INVALID_HANDLE_VALUE`
  - `is_open_ = false`

### 4) Windows 等待函数可用
- `waitReadable(timeout)`：轮询输入队列并支持超时
- `waitByteTimes(count)`：按波特率估算等待时长

## 使用建议

- 多线程环境可直接调用 `read()/write()`，无需外部再包锁
- 遇到串口异常建议关闭后重开
- 如需更低 CPU 占用，可后续改为事件驱动等待模型

## 验证状态

- ✅ 全工程已编译通过

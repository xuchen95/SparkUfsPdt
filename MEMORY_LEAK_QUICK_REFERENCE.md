# 📋 内存泄漏快速参考卡

## 🎯 5分钟快速定位泄漏

### 步骤1: 运行并收集输出
```
F5 →【完整执行程序】→ 关闭 →【输出窗口】
```

### 步骤2: 从输出提取信息
查找这样的行：
```
{块号} block类型 at 地址, 大小 bytes long.
       文件(行号)
```

### 步骤3: 设置BreakAlloc
编辑 `SparkUfsPdt.cpp` InitInstance():
```cpp
_CrtSetBreakAlloc(块号);  // 用实际块号替换
```

### 步骤4: 重新运行
```
Ctrl+Shift+B → F5 → 【自动中断】
```

### 步骤5: 查看调用栈
```
Ctrl+Alt+C → 【点击相关行看源代码】
```

---

## 🔍 快速识别块号

### 输出格式
```
{12345} normal block at 0x00A1B2C3, 256 bytes long.
 ↑块号                                     ↑大小
        D:\Project\CDialogBase.cpp(52)
        ↑文件                    ↑行号
```

### 快速提取
- 块号: `{` 和 `}` 之间的数字
- 大小: `at 0x...` 前面的数字  
- 文件: 路径的最后部分
- 行号: `(` 和 `)` 之间的数字

---

## ⚡ 批量处理清单

| # | 块号 | 大小 | 文件 | 状态 | BreakAlloc |
|---|------|------|------|------|-----------|
| 1 | {__} | __ | ____ | ☐修复 | _CrtSetBreakAlloc(__); |
| 2 | {__} | __ | ____ | ☐修复 | _CrtSetBreakAlloc(__); |
| 3 | {__} | __ | ____ | ☐修复 | _CrtSetBreakAlloc(__); |

---

## 🔧 常见修复模式

### 模式1: new/delete不匹配
```cpp
// ❌ 泄漏
MyClass* obj = new MyClass();
// 缺少 delete

// ✅ 修复
MyClass* obj = new MyClass();
// ...
delete obj;

// ✅ 更好
auto obj = std::make_unique<MyClass>();
```

### 模式2: 资源未释放
```cpp
// ❌ 泄漏
HANDLE h = CreateFile(...);
// 缺少 CloseHandle

// ✅ 修复
HANDLE h = CreateFile(...);
// ...
CloseHandle(h);
```

### 模式3: 数组new/delete
```cpp
// ❌ 泄漏
MyClass* arr = new MyClass[10];
delete arr;  // ← 错！应该是delete[]

// ✅ 修复
MyClass* arr = new MyClass[10];
delete[] arr;  // ← 正确

// ✅ 最好
auto arr = std::make_unique<MyClass[]>(10);
```

---

## 📊 泄漏优先级

```
大小 > 1MB      🔴 关键   【立即处理】
100KB - 1MB    🟠 高     【今天处理】
10KB - 100KB   🟡 中     【本周处理】
1KB - 10KB     🟢 低     【可暂时忽略】
< 1KB          🔵 微     【优化后处理】
```

---

## 🎯 工具命令速查

| 操作 | 快捷键 | 菜单路径 |
|------|--------|---------|
| 输出窗口 | Ctrl+Alt+O | 调试 > 窗口 > 输出 |
| 调用栈 | Ctrl+Alt+C | 调试 > 窗口 > 调用堆栈 |
| 编译 | Ctrl+Shift+B | 生成 > 生成解决方案 |
| 运行 | F5 | 调试 > 开始调试 |
| 搜索 | Ctrl+F | 编辑 > 查找和替换 |
| 替换 | Ctrl+H | 编辑 > 查找和替换 |

---

## ✅ 完成清单

- [ ] 运行了Debug版本
- [ ] 完整执行了程序
- [ ] 复制了调试输出
- [ ] 提取了块号
- [ ] 记录在清单上
- [ ] 按大小排序
- [ ] 添加了BreakAlloc
- [ ] 重新编译运行
- [ ] 查看了调用栈
- [ ] 找到了源代码
- [ ] 识别了泄漏原因
- [ ] 应用了修复
- [ ] 重新验证

---

## 💡 常见问题

### Q: BreakAlloc后还是没中断？
**A:** 检查块号是否正确，确认格式是 `_CrtSetBreakAlloc(12345);`

### Q: 看到 malloc, operator new，看不到我的代码？
**A:** 向上点击调用栈中的其他行，找您的源文件

### Q: 找不到释放代码，是泄漏吗？
**A:** 可能是MFC自动管理，查看类的析构函数

### Q: 很多来自系统库的泄漏？
**A:** 正常现象，系统库在程序结束时统一释放

---

## 🚀 立即行动

```
【现在】F5 → 完整执行 → 关闭
【然后】Ctrl+Alt+O → 复制输出
【接着】填写上面的清单表格
【最后】按步骤逐个修复
```

---

## 📞 需要帮助？

**使用详细文档:**

| 任务 | 查看文档 |
|------|---------|
| 获取完整输出 | MEMORY_LEAK_LOCK_GUIDE.md |
| 分析输出信息 | MEMORY_LEAK_DIAGNOSIS.md |
| 使用BreakAlloc | MEMORY_LEAK_OUTPUT_ANALYSIS.md |
| 填写分析表格 | MEMORY_LEAK_ANALYSIS_TEMPLATE.md |

---

**现在就开始！从F5运行程序开始！** 🎯

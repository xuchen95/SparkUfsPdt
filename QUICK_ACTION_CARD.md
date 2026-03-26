# ⚡ 快速行动卡 - 立即开始！

## 📍 您现在的代码

```cpp
int blockToBreak = 845;      // ← 改这个数字
_CrtSetBreakAlloc(blockToBreak);
```

✅ **位置完全正确！**

---

## 🚀 开始追踪（现在就做）

### 30秒快速开始

```
F5 → 【等待中断】→ Ctrl+Alt+C 【查看调用栈】→ 【点击进源代码】
```

### 块号序列

```
要追踪: 845 → 844 → 443 → 437 → 78 → 77 → 76
```

---

## 📋 每个块的流程（15分钟/块）

```
1. 改数字: blockToBreak = ???;
2. F5运行
3. 等待中断
4. Ctrl+Alt+C看调用栈
5. 点击源代码行
6. 记录信息
7. 找释放代码（或添加）
8. 修复
9. Ctrl+Shift+B编译
10. 进入下一块
```

---

## 🎯 修复模板（3种）

### 类型1：缺少delete
```cpp
// ❌ 原代码
MyClass* obj = new MyClass();
obj->DoSomething();
// 没有delete

// ✅ 修复方案1：添加delete
delete obj;

// ✅ 修复方案2：改用智能指针（推荐）
auto obj = std::make_unique<MyClass>();
// 不需要delete
```

### 类型2：delete[]改成delete
```cpp
// ❌ 原代码
MyClass* arr = new MyClass[10];
delete arr;  // ← 错

// ✅ 修复
delete[] arr;  // ← 改成delete[]
```

### 类型3：缺少Close/Destroy
```cpp
// ❌ 原代码
HANDLE h = CreateFile(...);
// 缺少CloseHandle

// ✅ 修复
CloseHandle(h);
```

---

## 💡 关键快捷键

| 动作 | 快捷键 |
|------|--------|
| 运行 | F5 |
| 编译 | Ctrl+Shift+B |
| 调用栈 | Ctrl+Alt+C |
| 搜索 | Ctrl+F |
| 替换 | Ctrl+H |

---

## ✅ 进度表

```
块号  状态     下次操作
────────────────────────
845  🔴 待追踪  F5运行
844  ⬜ 待追踪  改blockToBreak=844
443  ⬜ 待追踪  改blockToBreak=443
437  ⬜ 待追踪  改blockToBreak=437
78   ⬜ 待追踪  改blockToBreak=78
77   ⬜ 待追踪  改blockToBreak=77
76   ⬜ 待追踪  改blockToBreak=76
```

修复后标记为 ✅

---

## 📞 遇到问题？

```
不知道怎么做？
  → 读 BREAKALLOC_OPERATION_GUIDE.md

不知道怎么修？
  → 看 LEAK_BLOCK_FIX_TEMPLATE.md

调用栈看不懂？
  → 查 MEMORY_LEAK_LOCK_GUIDE.md

忘记快捷键？
  → 看本卡片的【关键快捷键】
```

---

## 🎉 完成标志

```
✅ 所有块都修复
✅ 重新运行无泄漏
✅ 输出: Normal块数 = 0
✅ 任务完成！
```

---

## ⏱️ 预计时间

```
第1块: 15分钟
第2块: 10分钟（更快）
第3-7块: 5分钟/块

总计: 60-70分钟
```

---

## 🚀 现在就开始！

```
      F5！
```

**下一步**: 
1. 按 F5
2. 等待中断（应该会在块845处）
3. 按 Ctrl+Alt+C 看调用栈
4. 按照模板修复
5. 改数字追踪下一个块

**预计5分钟内就能完成第一块的定位！**

---

*保存本卡片，随时参考！*

# 🔧 泄漏块修复模板

## 块号 845 - 第一个

### 步骤1：定位信息

```
块号:    845
文件:    ________________
行号:    ________________
类型:    new/malloc/Create
大小:    ________________
```

### 步骤2：定位源代码

运行时中断后，从调用栈找到您的代码文件：
```
D:\Project\某某.cpp(123)  ← 点这行
```

### 步骤3：记录代码位置

```cpp
// 在这里找到了创建代码：
// 文件: __________________________
// 行号: __________________________
// 代码样式:
//
// 例1: new方式
//   MyClass* obj = new MyClass();
//
// 例2: malloc方式
//   char* buf = malloc(1024);
//
// 例3: Create方式
//   HANDLE h = CreateFile(...);
```

### 步骤4：找释放代码

搜索对应的释放：

```
Ctrl+F  搜索: delete / free / CloseHandle

【如果找到了】
  delete obj;
  free(buf);
  CloseHandle(h);
  ↓
  跳到【步骤5-2：修复不完整的释放】

【如果没找到】
  跳到【步骤5-1：添加缺失的释放】
```

### 步骤5：修复

#### 5-1：添加缺失的释放

```cpp
// ❌ 原代码
MyClass* obj = new MyClass();
obj->DoSomething();
// 缺少释放！

// ✅ 修复1：添加delete
MyClass* obj = new MyClass();
obj->DoSomething();
delete obj;  // ← 添加这行

// ✅ 修复2：使用智能指针（更推荐）
#include <memory>
auto obj = std::make_unique<MyClass>();
obj->DoSomething();
// 不需要delete，自动释放
```

#### 5-2：修复不完整的释放

```cpp
// ❌ 原代码
MyClass* arr = new MyClass[10];  // new[]
delete arr;  // ← 错！应该是delete[]

// ✅ 修复1：改成delete[]
MyClass* arr = new MyClass[10];
delete[] arr;  // ← 改成delete[]

// ✅ 修复2：使用智能指针（更推荐）
auto arr = std::make_unique<MyClass[]>(10);
// 自动释放，无需手动delete[]
```

#### 5-3：释放在错误的地方

```cpp
// ❌ 原代码
void Func() {
    MyClass* obj = new MyClass();
    obj->Init();
    delete obj;
}
// 问题：obj在其他地方还被使用！

// ✅ 修复：在正确的地方释放
class Manager {
    MyClass* m_obj;  // 成员变量
    
    void Create() {
        m_obj = new MyClass();
    }
    
    ~Manager() {
        delete m_obj;  // ← 在析构函数释放
    }
};
```

### 步骤6：验证修复

```
Ctrl+Shift+B  编译
F5            运行
（观察：是否不再中断块845？）
```

✅ **如果不再中断** → 块845修复完成！  
❌ **如果还中断** → 修复不完整，重复步骤2-5

### 步骤7：进入下一个块

修改代码：
```cpp
int blockToBreak = 844;  // ← 改为844
```

重复整个流程...

---

## 块号 844 - 第二个

### 步骤1：定位信息
```
块号:    844
文件:    ________________
行号:    ________________
```

### 步骤2-7：重复步骤1中的步骤2-7

---

## 块号 443 - 第三个

### 步骤1：定位信息
```
块号:    443
文件:    ________________
行号:    ________________
```

### 步骤2-7：重复步骤1中的步骤2-7

---

## 块号 437 - 第四个

### 步骤1：定位信息
```
块号:    437
文件:    ________________
行号:    ________________
```

### 步骤2-7：重复步骤1中的步骤2-7

---

## 块号 78 - 第五个

### 步骤1：定位信息
```
块号:    78
文件:    ________________
行号:    ________________
```

### 步骤2-7：重复步骤1中的步骤2-7

---

## 块号 77 - 第六个

### 步骤1：定位信息
```
块号:    77
文件:    ________________
行号:    ________________
```

### 步骤2-7：重复步骤1中的步骤2-7

---

## 块号 76 - 第七个

### 步骤1：定位信息
```
块号:    76
文件:    ________________
行号:    ________________
```

### 步骤2-7：重复步骤1中的步骤2-7

---

## 🎯 快速修复对照表

### new/delete

```cpp
// ❌ 泄漏
new                  delete ✅
new[]                delete[] ❌ 应该是delete[]
```

### Create/Destroy

```cpp
// ❌ 泄漏
CreateFile           CloseHandle ✅
CreateWindow         DestroyWindow ✅
CreateDialog         ❌ MFC自动销毁
malloc               free ✅
calloc               free ✅
```

### MFC对象

```cpp
// ✅ 不需要手动释放（MFC自动管理）
CString
CArray
CList
CDialog (在DoModal中)
CWnd (在WM_DESTROY中)
```

---

## 📋 修复进度

```
块号  文件位置        修复方案              状态
────────────────────────────────────────────────
845  待定位          待修复                🔴
844  待定位          待修复                ⬜
443  待定位          待修复                ⬜
437  待定位          待修复                ⬜
78   待定位          待修复                ⬜
77   待定位          待修复                ⬜
76   待定位          待修复                ⬜
```

### 更新进度（修复后）：

```
845  CDialogBase.cpp(52)   添加 delete obj;    ✅
844  ________________      ________________    🔴
443  ________________      ________________    ⬜
...
```

---

## 💡 快速技巧

### 技巧1：快速识别修复方案
```
看到      → 应该               → 修复
new       → delete             → 添加delete或改智能指针
new[]     → delete[]           → 改成delete[]或改智能指针
malloc    → free               → 添加free或改malloc_ptr
```

### 技巧2：使用智能指针（最简单）
```cpp
#include <memory>

// 原来
MyClass* obj = new MyClass();
// ... 使用 ...
delete obj;

// 改成
auto obj = std::make_unique<MyClass>();
// ... 使用 ...
// 自动释放，无需delete
```

### 技巧3：在异常安全的地方释放
```cpp
// ❌ 可能泄漏（如果DoSomething抛异常）
MyClass* obj = new MyClass();
obj->DoSomething();  // 如果这里抛异常，delete不会执行
delete obj;

// ✅ 异常安全
auto obj = std::make_unique<MyClass>();
obj->DoSomething();  // 即使抛异常，也会自动释放
```

---

## ✅ 完成标志

每修复一个块，检查：
- [ ] 代码已修改
- [ ] 编译成功
- [ ] 运行不再中断该块
- [ ] 进入下一个块

所有7个块都修复后：
- [ ] 输出窗口无泄漏信息
- [ ] 内存统计：Normal块数 = 0
- [ ] 任务完成！

---

**现在就开始修复第一个块！** 🚀

F5 → 中断 → Ctrl+Alt+C → 查看调用栈 → 修复 → 验证

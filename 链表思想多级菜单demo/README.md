# 链表思想 · 多级菜单（STM32 + OLED 模板）

一个基于 **链表思想** 实现的 STM32 多级菜单模板。菜单之间的父子关系完全靠 **指针地址** 串起来，而不是用 `switch/case` 状态机。

> 适用于 OLED / LCD 屏幕 + 按键交互的嵌入式项目，可作为菜单系统的通用起点直接裁剪使用。

## ✨ 特点

- **数据驱动**：整棵菜单树就是一份数据，增删菜单只改「数据」，不动导航 / 显示代码。
- **一套导航函数通吃所有层级**：`上 / 下 / 进入 / 返回 / 确认` 五个操作无需为任意层级写分支。
- **树结构与逻辑天然同步**：不会出现「菜单表」和「处理逻辑」对不上的问题。
- **滚动窗口显示**：同级子菜单超过可见行数时，选中项始终保持在屏上。
- **防御性设计**：空指针、下标越界、顶层越界返回均有保护。

## 🧠 核心思想

```
        first
        /    \
      set    start
     /   \      \
  color sound  game
```

菜单节点用结构体描述，父菜单通过「子菜单地址数组」持有子节点指针：

```c
typedef struct MenuItem_t {
    char *id;                       // 菜单项名称
    struct MenuItem_t *parent_menu; // 父菜单地址
    struct MenuItem_t **son_menu;   // 子菜单地址数组的首地址
    uint8_t son_menu_count;         // 子菜单数量
    void (*action)(void);           // 该菜单的执行函数
} MenuItem_t;
```

切换菜单就是「改指针」：`current_menu = parent->son_menu[index]`，全程不涉及任何 `case` 分支。

## 📁 目录结构

```
├── main.c      # 主程序：初始化 OLED / 按键 / 菜单，主循环轮询
├── Menu.h      # 菜单模块接口 + 按键编号映射
└── Menu.c      # 菜单树数据定义 + 导航 / 显示逻辑（核心）
```

## 🚀 快速上手

### 依赖

模板本身不依赖硬件外，还需以下模块（需自行提供或适配为你的平台）：

| 模块      | 作用         |
| --------- | ------------ |
| `OLED.h`  | OLED 显示驱动 |
| `Key.h`   | 按键读取      |
| `Delay.h` | 延时函数      |
| `stm32f10x.h` | 设备头文件 |

### 使用步骤

1. 将 `main.c`、`Menu.c`、`Menu.h` 加入你的工程。
2. 在 [Menu.h](Menu.h) 中把 `KEY_UP / KEY_DOWN / KEY_ENTER / KEY_BACK / KEY_OK` 改成你板子上 `Key_GetNum()` 实际返回的按键编号。
3. 按需替换 [Menu.c](Menu.c) 中各动作函数里 `OLED_ShowString` 显示的内容。

### 新增一个菜单（只改数据）

例如给 `first` 增加一个 `Help` 子菜单：

```c
// 1. 定义节点
MenuItem_t help = {
    .id = "Help",
    .parent_menu = &first,
    .son_menu = NULL,
    .son_menu_count = 0,
    .action = show_help
};

// 2. 在父菜单的子菜单地址数组里加一行地址
MenuItem_t *son_menus1[] = { &set, &start, &help };
```

导航、显示代码一行都不用改。

## 🎮 按键定义

| 按键     | 功能         |
| -------- | ------------ |
| `KEY_UP` | 上一个菜单项 |
| `KEY_DOWN` | 下一个菜单项 |
| `KEY_ENTER` | 进入子菜单 |
| `KEY_BACK`  | 返回父菜单 |
| `KEY_OK`    | 确认（执行当前菜单动作） |

## 📄 许可

本项目仅作学习与模板用途，可自由使用、修改与分发。

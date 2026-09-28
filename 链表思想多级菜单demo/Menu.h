#ifndef __MENU_H
#define __MENU_H

/*==================== 按键编号映射 ====================*/
/* 根据你板子 Key_GetNum() 的返回值，把下面 5 个值改成对应的按键编号 */
#define KEY_UP     1   // 上一个菜单项
#define KEY_DOWN   2   // 下一个菜单项
#define KEY_ENTER  3   // 进入子菜单
#define KEY_BACK   4   // 返回父菜单
#define KEY_OK     5   // 确认（执行当前菜单项的动作）

/* 初始化菜单：选中根菜单并显示，主函数里初始化 OLED、按键后调用一次 */
void Menu_Init(void);

/* 菜单主循环：轮询按键并处理，放在 main 的 while(1) 里持续调用 */
void Menu_Loop(void);

#endif

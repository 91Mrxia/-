#include "stm32f10x.h"                  // 设备头文件
#include "OLED.h"                       // OLED 驱动
#include "Key.h"                        // 按键驱动
#include "Menu.h"                       // 菜单模块

int main(void)
{
    OLED_Init();    // 初始化 OLED
    Key_Init();     // 初始化按键
    Menu_Init();    // 初始化菜单并显示根菜单

    while (1)
    {
        Menu_Loop();    // 轮询按键并处理菜单
    }
}

#include "stm32f10x.h"                  // 设备头文件
#include "OLED.h"                       // OLED 显示驱动
#include "Key.h"                        // 按键驱动
#include "Delay.h"                      // 延时函数
#include "Menu.h"

/* =====================================================================
 *  多级菜单 —— 链表思想实现
 *
 *  【核心思想】菜单之间的"父子关系"完全靠"地址"串起来，不靠 switch/case，
 *  【相比 switch/case 状态机的优点】
 *   1. 增删菜单只改"数据"：加一个节点 + 在父菜单数组里加一行地址，
 *      导航 / 显示代码一行都不用动。例如要新增一个 "Help" 菜单：
 *        MenuItem_t help = { .id = "Help", .parent_menu = &first,
 *                            .son_menu = NULL, .son_menu_count = 0, ... };
 *        MenuItem_t *son_menus1[] = {&set, &start, &help};  // 数组加一行地址即可
 *   2. 一套导航函数通吃所有层级，菜单再深也不用加任何分支；
 *   3. 树结构就是数据本身，不会出现"菜单表"和"逻辑"不同步的问题。
 * ===================================================================== */

/*==================== 一、通用的菜单模板 ====================*/
typedef struct MenuItem_t {
    char *id;                       // 菜单项名称
    struct MenuItem_t *parent_menu; // 父菜单地址（上一个节点）
    struct MenuItem_t **son_menu;   // 子菜单地址数组的首地址（下一个节点集合）
    uint8_t son_menu_count;         // 子菜单数量（即地址数组的长度）
    void (*action)(void);           // 该菜单的执行函数
} MenuItem_t;

#define MENU_VISIBLE_MAX    3

/*==================== 二、前向声明 ====================*/
/* 子菜单地址数组：定义在节点之后，但节点初始化时要用到，故先声明 */
extern MenuItem_t *son_menus1[];    // first 的所有子菜单地址
extern MenuItem_t *son_menus2[];    // set 的所有子菜单地址
extern MenuItem_t *son_menus3[];    // start 的所有子菜单地址

/* 各菜单的动作函数：定义在下面，节点初始化时要用到，故先声明 */
static void show_first(void);
static void show_set(void);
static void show_start(void);
static void show_color(void);
static void show_sound(void);
static void show_game(void);


/*==================== 三、定义所有菜单节点（对应菜单树）====================*/
/*
 *        first
 *        /    \
 *      set    start
 *     /   \      \
 *  color sound  game
 */
MenuItem_t first = {
    .id = "First",
    .parent_menu = NULL,            // 顶层菜单，没有父节点
    .son_menu = son_menus1,
    .son_menu_count = 2,
    .action = show_first
};

MenuItem_t set = {
    .id = "Set",
    .parent_menu = &first,
    .son_menu = son_menus2,
    .son_menu_count = 2,
    .action = show_set
};

MenuItem_t start = {
    .id = "Start",
    .parent_menu = &first,
    .son_menu = son_menus3,
    .son_menu_count = 1,
    .action = show_start
};

MenuItem_t color = {
    .id = "Color",
    .parent_menu = &set,
    .son_menu = NULL,
    .son_menu_count = 0,
    .action = show_color
};

MenuItem_t sound = {
    .id = "Sound",
    .parent_menu = &set,
    .son_menu = NULL,
    .son_menu_count = 0,
    .action = show_sound
};

MenuItem_t game = {
    .id = "Game",
    .parent_menu = &start,
    .son_menu = NULL,
    .son_menu_count = 0,
    .action = show_game
};

/*==================== 四、各菜单的"子菜单地址数组" ====================*/
/* 数组里存的不是菜单内容，而是指向子菜单节点的"地址"。
 * 链表的思想就体现在这里：靠这些地址把整棵树串起来。 */
MenuItem_t *son_menus1[] = {&set, &start};      // first 的所有子菜单地址
MenuItem_t *son_menus2[] = {&color, &sound};    // set 的所有子菜单地址
MenuItem_t *son_menus3[] = {&game};             // start 的所有子菜单地址

/*==================== 五、当前选中菜单的"指针 + 下标" ====================*/
static MenuItem_t *current_menu = &first;  // 始终指向树里某个节点的地址
static uint8_t current_index = 0;          // 该节点在其父菜单数组里的下标

/*==================== 六、核心寻址操作 ====================*/
/* 通过"数组寻址"切换菜单：parent 是父菜单，index 是子菜单下标。
 * parent->son_menu[index] 取出来的正是"第 index 个子菜单的地址"，
 * 把它赋给 current_menu 就完成了切换 —— 全程只改指针。 */
static void select_child(MenuItem_t *parent, uint8_t index)
{
    /* 防御：父节点 / 子菜单数组为空，或下标越界时直接忽略 */
    if (parent == NULL || parent->son_menu == NULL || index >= parent->son_menu_count) {
        return;
    }
    current_menu = parent->son_menu[index]; // 数组里存的正是子菜单的地址
    current_index = index;                  // 记住下标，供上下切换时继续寻址
}
/* 返回 menu 在其父菜单数组里的下标；找不到（如哨兵节点）返回 -1 */
static int get_index_in_parent(MenuItem_t *menu)
{
    int i;
    if (menu->parent_menu == NULL) {
        return -1;
    }
    for (i = 0; i < menu->parent_menu->son_menu_count; i++) {
        if (menu->parent_menu->son_menu[i] == menu) {   // 比较的是地址
            return i;
        }
    }
    return -1;
}

/*==================== 七、叶子菜单的动作函数 ====================*/
/* 各菜单动作只负责"画自己的一屏"，退出靠下一次按键触发 menu_back + menu_show。
 * 叶子动作（Color/Sound/Game）显示套路相同，抽成 show_action 共用。
 * 具体内容按你实际需求改 name/value 即可。 */
static void show_action(char *name, char *value)
{
    OLED_Clear();
    OLED_ShowString(1, 1, name);            // 第 1 行：菜单名
    OLED_ShowString(2, 1, value);           // 第 2 行：具体内容
    OLED_ShowString(4, 1, "Press BACK");    // 第 4 行：操作提示
}

static void show_first(void) { show_action("First", "Top level"); }
static void show_set(void)   { show_action("Set",   "Settings"); }
static void show_start(void) { show_action("Start", "Starting..."); }
static void show_color(void) { show_action("Color", "Red"); }
static void show_sound(void) { show_action("Sound", "On"); }
static void show_game(void)  { show_action("Game",  "Snake"); }

/*==================== 八、导航函数（全部基于"改指针 / 寻址"）====================*/

/* 上一个兄弟菜单：下标减 1（越界则循环到末尾），再按新下标寻址。 */
static void menu_up(void)
{
    MenuItem_t *parent = current_menu->parent_menu;
    if (parent == NULL) {
        return;                                     // 顶层无兄弟菜单，不能切换
    }
    select_child(parent, (current_index - 1 + parent->son_menu_count) % parent->son_menu_count);
}

/* 下一个兄弟菜单：下标加 1（越界则循环到开头），再按新下标寻址 */
static void menu_down(void)
{
    MenuItem_t *parent = current_menu->parent_menu;
    if (parent == NULL) {
        return;                                     // 顶层无兄弟菜单，不能切换
    }
    select_child(parent, (current_index + 1) % parent->son_menu_count);
}

/* 进入子菜单：把 current_menu 指向自己的第一个子菜单地址 */
static void menu_enter(void)
{
    if (current_menu->son_menu_count > 0) {
        select_child(current_menu, 0);
    }
}

/* 返回父菜单：把 current_menu 指向 parent_menu 里存的父菜单地址。
 * 父菜单为 NULL 时（已经在顶层）不允许再退。 */
static void menu_back(void)
{
    MenuItem_t *parent = current_menu->parent_menu;
    if (parent == NULL) {
        return;                                     // 已在顶层，不能再退
    }
    current_menu = parent;                          // 顺着 parent_menu 地址回到上一层
    /* 根节点没有父级下标，直接置 0；其余恢复它在父级里的下标。
     * （get_index_in_parent 对根节点返回 -1，不能直接赋给 uint8_t） */
    current_index = (parent->parent_menu == NULL) ? 0
                                                  : (uint8_t)get_index_in_parent(parent);
}

/* 确认：执行当前菜单项的动作 */
static void menu_ok(void)
{
    if (current_menu->action != NULL) {
        current_menu->action();
    }
}

/*==================== 九、刷新 OLED 显示 ====================*/
/* 顶层菜单没有父节点，单独显示标题 "Menu" 和唯一顶层项；
 * 其余层级：第一行显示父菜单名，下面显示同级菜单项（即父菜单的地址数组）。 */
static void menu_show(void)
{
    MenuItem_t *parent = current_menu->parent_menu;
    uint8_t i, start = 0;

    OLED_Clear();
    if (parent == NULL) {                           // 顶层：无父菜单，单独处理
        OLED_ShowString(1, 1, "Menu");
        OLED_ShowChar(2, 1, '>');
        OLED_ShowString(2, 2, current_menu->id);
        return;
    }
    OLED_ShowString(1, 1, parent->id);

    /* 滚动窗口：子菜单多于可见行数时，保证选中项始终显示在屏上 */
    if (current_index >= MENU_VISIBLE_MAX) {
        start = current_index - MENU_VISIBLE_MAX + 1;
    }

    for (i = 0; i < parent->son_menu_count && i < MENU_VISIBLE_MAX; i++) {
        uint8_t item = start + i;
        /* 靠比较地址判断哪一项是当前选中，而不是比较字符串 */
        OLED_ShowChar(2 + i, 1, (parent->son_menu[item] == current_menu) ? '>' : ' ');
        OLED_ShowString(2 + i, 2, parent->son_menu[item]->id);
    }
}

/*==================== 十、对外接口 ====================*/

/* 初始化菜单：选中根菜单并显示 */
void Menu_Init(void)
{
    current_menu = &first;
    current_index = 0;
    menu_show();
}

/* 菜单主循环：轮询按键并处理 */
void Menu_Loop(void)
{
    uint8_t key = Key_GetNum();
    switch (key) {
    case KEY_UP:    menu_up();    menu_show(); break;
    case KEY_DOWN:  menu_down();  menu_show(); break;
    case KEY_ENTER: menu_enter(); menu_show(); break;
    case KEY_BACK:  menu_back();  menu_show(); break;
    case KEY_OK:    menu_ok();    break;      // 动作函数内部会自行恢复显示
    default:        break;                    // key == 0（无按键）或未映射的按键
    }
}

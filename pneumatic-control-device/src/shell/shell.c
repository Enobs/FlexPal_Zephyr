/*
 * @Author: wang,yongjing
 * @Date: 2024-06-03 17:45:17
 * @LastEditTime: 2024-06-04 20:05:34
 * @LastEditors: wang,yongjing
 * @Description: 
 * @FilePath: /temperature-control/pneumatic-control-device/src/shell/shell.c
 * 
 */
#include "motor.h"
#include "shell.h"


static int cmd_set_fan(const struct shell *sh, size_t argc, char **argv)
{
    set_fan_percent(atoi(argv[2]), atof(argv[3]));

	shell_print(sh, "test wyj shell");

	return 0;
}

/* 注册一个根命令fan，执行根命令fan时会调用cmd_set_fan,无命令子集，必选命令为4个，可选命令为0个 */
/* such as： fan set 1 50  
                    1:channal
                    50:pwm
 */
SHELL_CMD_ARG_REGISTER(fan, NULL, "set pwm to ctrl fan", cmd_set_fan, 4, 0);
Build Process:
1. The zephyr development environment was successfully set up
2. cd ./patch 
3. ./apply_patch.sh  # applate patch
4. cp ./compile.sh ../
5. ./compile all

Hardware Description:
    Hardware    interface   Function
     USART0       Shell      printk LOG_INF LOG_ERR... 
     USART1       ROS        printf

PWM1    PE9     TIMER0_CH0
PWM2    PB8     TIMER3_CH2
PWM3    PB1     TIMER2_CH3
PWM4    PB9     TIMER3_CH3
PWM5    PB0     TIMER2_CH2
PWM6    PE11     TIMER0_CH1
PWM7    PA11     TIMER0_CH3
PWM8    PE13     TIMER0_CH2
PWM9    PB6     TIMER3_CH0
PWM10    PA7     TIMER2_CH1
PWM11    PB7     TIMER3_CH1
PWM12    PA6     TIMER2_CH0
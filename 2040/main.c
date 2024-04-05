#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/pio.h"
#include "hardware/clocks.h"
#include "hardware/irq.h"
#include "step_ctrl.pio.h"
#include <math.h>
#include "hardware/uart.h"
#include "pin_def.h"
// 全局变量
float speed_factor = 1.0f;
float accel_factor = 1.0f;
int finger_pos[2] = {FINGER_OFFSET_INIT, FINGER_OFFSET_INIT};

static void error_code(int x)
{
    gpio_put(ENN, 1);
    while (1)
    {
        for (int i = 3; i >= 0; i--)
        {
            gpio_put(LED, 1);
            if (x & (1 << i))
            {
                sleep_ms(800);
            }
            else
            {
                sleep_ms(100);
            }
            gpio_put(LED, 0);
            sleep_ms(500);
        }
        sleep_ms(3000);
    }
}

///////////////////////////////////////////////////////////////////////////////////
// ---------------------------------- TMC2209 ---------------------------------- //
///////////////////////////////////////////////////////////////////////////////////
static void swuart_calcCRC(uint8_t *datagram, uint8_t datagramLength)
{
    int i, j;
    uint8_t *crc = datagram + (datagramLength - 1); // CRC located in last byte of message
    uint8_t currentByte;
    *crc = 0;
    for (i = 0; i < (datagramLength - 1); i++)
    {                              // Execute for all bytes of a message
        currentByte = datagram[i]; // Retrieve a byte to be sent from Array
        for (j = 0; j < 8; j++)
        {
            if ((*crc >> 7) ^ (currentByte & 0x01)) // update CRC based result of XOR operation
            {
                *crc = (*crc << 1) ^ 0x07;
            }
            else
            {
                *crc = (*crc << 1);
            }
            currentByte = currentByte >> 1;
        } // for CRC bit
    }
}
static void tmc2209_uart_init(void)
{
    gpio_init(TMC2209_RX);
    gpio_disable_pulls(TMC2209_RX);
    uart_init(TMC2209_UART, 115200);
    uart_set_fifo_enabled(TMC2209_UART, true);
    gpio_set_function(TMC2209_TX, GPIO_FUNC_UART);
    gpio_set_function(TMC2209_RX, GPIO_FUNC_UART);
}
static uint32_t tmc2209_read(uint8_t slave_addr, uint8_t reg_addr)
{
    uint8_t buf[8] = {0x05, slave_addr, reg_addr};
    const uint32_t TMC2209_TIME_OUT = 1000;
    swuart_calcCRC(buf, 4);
    // 断开引脚和UART外设的连接，防止收到TX发出去的数据
    gpio_set_function(TMC2209_RX, GPIO_FUNC_SIO);
    uart_write_blocking(TMC2209_UART, buf, 4);
    uart_tx_wait_blocking(TMC2209_UART);
    gpio_set_function(TMC2209_RX, GPIO_FUNC_UART);
    for (int i = 0;; i++)
    {
        if (uart_is_readable_within_us(TMC2209_UART, TMC2209_TIME_OUT))
        {
            buf[0] = (uint8_t)uart_get_hw(TMC2209_UART)->dr;
            if (0x05 == buf[0])
            {
                break;
            }
        }
        if (i > 16)
        {
            puts("tmc2209_read sync error.");
            error_code(slave_addr);
        }
    }
    for (int i = 1; i < 8; i++)
    {
        if (uart_is_readable_within_us(TMC2209_UART, TMC2209_TIME_OUT))
        {
            buf[i] = (uint8_t)uart_get_hw(TMC2209_UART)->dr;
        }
        else
        {
            puts("tmc2209_read time out.");
            error_code(slave_addr);
        }
    }
    uint8_t crc = buf[7];
    swuart_calcCRC(buf, 8);
    if (buf[7] != crc)
    {
        puts("tmc2209_read crc error.");
        error_code(slave_addr);
    }
    uint32_t val = buf[6] | (buf[5] << 8) | (buf[4] << 16) | (buf[3] << 24);
    //printf("tmc2209_read: slave_addr=0x%x, reg_addr=0x%x, value=0x%x\n",
    //       slave_addr, reg_addr, val);
    sleep_ms(1);
    return val;
}
static void tmc2209_write(uint8_t slave_addr, uint8_t reg_addr, uint32_t val)
{
    uint8_t buf[8] = {0x05, slave_addr, 0x80 | reg_addr,
                      (uint8_t)(val >> 24), (uint8_t)(val >> 16), (uint8_t)(val >> 8), (uint8_t)(val >> 0)};
    swuart_calcCRC(buf, 8);
    // 断开引脚和UART外设的连接，防止收到TX发出去的数据
    gpio_set_function(TMC2209_RX, GPIO_FUNC_SIO);
    uart_write_blocking(TMC2209_UART, buf, 8);
    uart_tx_wait_blocking(TMC2209_UART);
    gpio_set_function(TMC2209_RX, GPIO_FUNC_UART);
}
static void tmc2209_config(uint8_t addr)
{
    uint8_t cnt_start = (uint8_t)tmc2209_read(addr, 0x02);
    // GCONF
    tmc2209_write(addr, 0x00, 0b0111000000);
    // 最大电流(32/32) 1.92A 有效值 2.72A 峰值
    // IHOLDDELAY： IRUN：20/32  IHOLD：10/32
    tmc2209_write(addr, 0x10, (2 << 16) + (IRUN << 8) + IHOLD);
    // TPOWERDOWN: 46 * 2^18 / 12M = 1s 停止运动后，降低工作电流到IHOLD的等待时间
    tmc2209_write(addr, 0x11, 46);
    // TPWMTHRS 静音和高速模式的阈值，数值越大，转速越低，300RPM转速对应47
    tmc2209_write(addr, 0x13, 50);
    // TCOOLTHRS 当转速TSTEP（0x12）介于TPWMTHRS和TCOOLTHRS之间，才可以用无限位归零
    tmc2209_write(addr, 0x14, 10000);
    // SGTHRS 无限位归零阈值，数值越大，越容易触发
    tmc2209_write(addr, 0x40, SGTHRS); 

    // 16细分，其他参数默认（默认0x10000053）
    tmc2209_write(addr, 0x6C, 0x14000053);
    // 确认写入的数据
    uint8_t cnt_stop = (uint8_t)tmc2209_read(addr, 0x02);
    uint8_t diff = cnt_stop - cnt_start;
    if (7 != diff)
    {
        printf("tmc2209_write error, count=%d.\n", diff);
        error_code(addr);
    }
    printf("tmc2209_config done, addr=%d, count=%d.\n", addr, cnt_stop);
}

///////////////////////////////////////////////////////////////////////////////////
// ---------------------------------- 脉冲控制 ---------------------------------- //
///////////////////////////////////////////////////////////////////////////////////

typedef struct step_ctrl_t
{
    int s;     // 加速过程需要的步数
    int i;     // 当前步
    int steps; // 步数
    float vi;  // 当前速度
    float a;   // 加速度（脉冲/s2）
    uint dir;
} step_ctrl;

step_ctrl stepper_ctrl[4] = {0};

// 电机加速控制
// gpio_mask 1<<GPIO编号 steps：步数，v0：初始速度（脉冲/s），v：最高速度（脉冲/s），a：加速度（脉冲/s2）
// steps可以为正数或者负数，符号表示方向，但是不能为0
static void stepper_move(int sm, int steps, float v0, float v, float a)
{
    step_ctrl *p_step_ctrl = &stepper_ctrl[sm];
    if(steps < 0){
        p_step_ctrl->dir = 0x80000000UL;
        p_step_ctrl->steps = -steps;
    }else{
        p_step_ctrl->dir = 0;
        p_step_ctrl->steps = steps;
    }
    p_step_ctrl -> vi = v0 * speed_factor;
    p_step_ctrl -> a = a * accel_factor;
    p_step_ctrl -> s = (int)roundf((v * v - v0 * v0) / (2 * a)); // 计算加速过程需要的步数
    p_step_ctrl -> i = 0;
    pio_set_irq0_source_enabled(pio0, PIO_INTR_SM0_TXNFULL_LSB + sm, true);
}
// 等待电机控制指令执行完毕
static void stepper_move_block(int sm, int steps, float v0, float v, float a)
{
    stepper_move(sm, steps, v0, v, a);
    // 等待中断关闭
    while (pio0->inte0 & (1u << (sm + PIO_INTR_SM0_TXNFULL_LSB)));
    // 等待fifo清空
    while (!pio_sm_is_tx_fifo_empty(pio0, sm));
}
// 等待到输出指定的步数，因为有fifo，所以会提前4个脉冲返回
static void stepper_move_wait(int sm, int step_to_wait)
{
    volatile int *index = &stepper_ctrl[sm].i;
    while (*index <= step_to_wait);
}

static uint32_t stepper_move_calc(step_ctrl *p_step_ctrl)
{
    const float F = 10000000.0f; // PIO频率
    float accel;
    int i = p_step_ctrl->i;
    int s = p_step_ctrl->s;
    int steps = p_step_ctrl->steps;
    float vi = p_step_ctrl->vi;
    float a = p_step_ctrl->a;
    if (i < s && i < steps / 2)
    {
        accel = a;
    }
    else if (i >= steps - s)
    {
        accel = -a;
    }
    else
    {
        accel = 0;
    }
    vi = sqrtf(vi * vi + 2.0f * accel);
    p_step_ctrl->vi = vi;
    p_step_ctrl->i = i + 1;
    return (uint32_t)roundf(F / vi) - 26; // 26的来源参考step_ctrl.pio
}
// PIO FIFO非满中断，每次处理耗时5us左右
void pio_fifo_not_full_handler()
{
    gpio_put(LED, 1);
    step_ctrl *p;
    uint32_t date;
    int sm;
    // 对应通道中断功能开启，且FIFO非满，此时应当填充新的数据
    for (sm = 0; sm < 4; sm++)
    {
        while ((pio0->inte0 & (1u << (sm + PIO_INTR_SM0_TXNFULL_LSB))) && !pio_sm_is_tx_fifo_full(pio0, sm))
        {
            p = &stepper_ctrl[sm];
            date = stepper_move_calc(p) | p->dir;
            pio_sm_put(pio0, sm, date);
            if (p->i >= p->steps)
            {
                pio_set_irq0_source_enabled(pio0, sm + PIO_INTR_SM0_TXNFULL_LSB, false);
                break;
            }
        }
    }
    gpio_put(LED, 0);
}

///////////////////////////////////////////////////////////////////////////////////
// ---------------------------------- 运动控制 ---------------------------------- //
///////////////////////////////////////////////////////////////////////////////////

static void zero_point(void)
{
    gpio_put(ENN, 0);
    sleep_ms(100);
    // stepper 2
    stepper_move      (0, 1, SPEED_HOME, SPEED_HOME, ACCEL_HOME);// 移动一步，由HOLD到RUN电流
    int step = 0;
    for (int i = 0; ;i++)
    {
        stepper_move_block(2, -1, SPEED_HOME, SPEED_HOME, ACCEL_HOME);
        step++;
        if(gpio_get(DIAG2) || i > 500)
        {
            break;
        }
    }
    sleep_ms(100);
    step = 0;
    for (int i = 0; ;i++)
    {
        stepper_move_block(2, 1, SPEED_HOME, SPEED_HOME, ACCEL_HOME);
        step++;
        if(gpio_get(DIAG2))
        {
            printf("finger 2 home done. step=%d\n", step);
            break;
        }
        if (i > 200 * 16)
        {
            puts("finger 2 home error.");
            error_code(ERROR_ZERO_POINT);
        }
    }
    sleep_ms(100);
    stepper_move_block(2, -FINGER_OFFSET_INIT, SPEED_HOME, SPEED_LOW, ACCEL_HOME);
    // stepper 0
    step = 0;
    for (int i = 0; ;i++)
    {
        stepper_move      (0, 1, SPEED_HOME, SPEED_HOME, ACCEL_HOME);
        stepper_move_block(2, 1, SPEED_HOME, SPEED_HOME, ACCEL_HOME);
        step++;
        if(gpio_get(ITR9606_0))
        {
            printf("arm 0 home done. step=%d\n", step);
            break;
        }
        if ( i > 200 * 16 || gpio_get(DIAG0))
        {
            puts("arm 0 home error.");
            error_code(ERROR_ZERO_POINT);
        }
    }
    sleep_ms(100);
    stepper_move      (0, ARM_OFFSET, SPEED_HOME, SPEED_HOME, ACCEL_HOME);
    stepper_move_block(2, ARM_OFFSET, SPEED_HOME, SPEED_HOME, ACCEL_HOME);
    // stepper 3
    stepper_move      (1, 1, SPEED_HOME, SPEED_HOME, ACCEL_HOME);// 移动一步，由HOLD到RUN电流
    step = 0;
    for (int i = 0; ;i++)
    {
        stepper_move_block(3, -1, SPEED_HOME, SPEED_HOME, ACCEL_HOME);
        step++;
        if(gpio_get(DIAG3) || i > 500)
        {
            break;
        }
    }
    sleep_ms(100);
    step = 0;
    for (int i = 0; ;i++)
    {
        stepper_move_block(3, 1, SPEED_HOME, SPEED_HOME, ACCEL_HOME);
        step++;
        if(gpio_get(DIAG3))
        {
            printf("finger 3 home done. step=%d\n", step);
            break;
        }
        if (i > 200 * 16)
        {
            puts("finger 3 home error.");
            error_code(ERROR_ZERO_POINT);
        }
    }
    sleep_ms(100);
    stepper_move_block(3, -FINGER_OFFSET_INIT, SPEED_HOME, SPEED_LOW, ACCEL_HOME);
    // stepper 1
    step = 0;
    for (int i = 0; ;i++)
    {
        stepper_move      (1, 1, SPEED_HOME, SPEED_HOME, ACCEL_HOME);
        stepper_move_block(3, 1, SPEED_HOME, SPEED_HOME, ACCEL_HOME);
        step++;
        if(gpio_get(ITR9606_1))
        {
            printf("arm 1 home done. step=%d\n", step);
            break;
        }
        if ( i > 200 * 16 || gpio_get(DIAG1))
        {
            puts("arm 1 home error.");
            error_code(ERROR_ZERO_POINT);
        }
    }
    sleep_ms(100);
    stepper_move      (1, ARM_OFFSET, SPEED_HOME, SPEED_LOW, ACCEL_HOME);
    stepper_move_block(3, ARM_OFFSET, SPEED_HOME, SPEED_LOW, ACCEL_HOME);
    finger_pos[0] = FINGER_OFFSET_INIT;
    finger_pos[1] = FINGER_OFFSET_INIT;
}

// LEFT 0 RIGHT 1
static void move_finger(int lr, int to, float v0, float v, float a)
{
    stepper_move      (lr + 2, finger_pos[lr] - to, v0, v, a);
    finger_pos[lr] = to;
}
static void move_finger_block(int lr, int to, float v0, float v, float a)
{
    stepper_move_block(lr + 2, finger_pos[lr] - to, v0, v, a);
    finger_pos[lr] = to;
}
static void move_arm_block(int lr, int step, float v0, float v, float a)
{
    stepper_move      (lr, step, v0, v, a);
    stepper_move_block(lr + 2, step, v0, v, a);
}
static void move_arm(int lr, int step, float v0, float v, float a)
{
    stepper_move      (lr, step, v0, v, a);
    stepper_move(lr + 2, step, v0, v, a);
}
// 夹紧魔方
static void move_finger_init_to_default(void)
{
    move_finger      (LEFT, FINGER_OFFSET_SPIN, SPEED_HOME, SPEED_LOW, ACCEL_HOME);
    move_finger_block(RIGHT, FINGER_OFFSET_SPIN, SPEED_HOME, SPEED_LOW, ACCEL_HOME);
}
// 松开魔方
static void move_finger_default_to_init(void)
{
    move_finger      (LEFT, FINGER_OFFSET_INIT, SPEED_HOME, SPEED_LOW, ACCEL_HOME);
    move_finger_block(RIGHT, FINGER_OFFSET_INIT, SPEED_HOME, SPEED_LOW, ACCEL_HOME);
}

// 反复移动手指，改善润滑状态
static void move_finger_repetition()
{
    for(int i=0;i<3; i++)
    {
        move_finger      (LEFT, FINGER_OFFSET_MAX, SPEED_HOME, SPEED_LOW, ACCEL_HOME);
        move_finger_block(RIGHT, FINGER_OFFSET_MAX, SPEED_HOME, SPEED_LOW, ACCEL_HOME);
        sleep_us(DELAY_US_AFTER_FINGER_LOCK);
        move_finger      (LEFT, 32, SPEED_HOME, SPEED_LOW, ACCEL_HOME);
        move_finger_block(RIGHT, 32, SPEED_HOME, SPEED_LOW, ACCEL_HOME);
        sleep_us(DELAY_US_AFTER_FINGER_LOCK);
    }
    for(int i=0;i<3; i++)
    {
        move_arm_block(LEFT, 1600, V_START_ARM_L, V_MAX_ARM_L, A_MAX_ARM_L);
        move_arm_block(RIGHT, 1600, V_START_ARM_L, V_MAX_ARM_L, A_MAX_ARM_L);
    }
}
// 拧180度
// twist_cube_180(LEFT); or twist_cube_180(RIGHT);
static void twist_cube_180(int lr)
{
    move_arm_block(lr, 1600, V_START_ARM_L, V_MAX_ARM_L, A_MAX_ARM_L);
}
// 拧90度
static void twist_cube_90(int lr, int cw_ccw, int skip_back_step)
{
    int dir = cw_ccw ? -1 : 1;
    // 转90度 
    move_arm_block(lr, dir * 800, V_START_ARM_L, V_MAX_ARM_L, A_MAX_ARM_L);
    if(!skip_back_step)
    {
        // 对侧手指锁紧
        move_finger_block(!lr, FINGER_OFFSET_LOCK, V_START_FINGER, V_MAX_FINGER, A_MAX_FINGER);
        sleep_us(DELAY_US_AFTER_FINGER_LOCK);
        // 手指松开
        move_finger_block(lr, FINGER_OFFSET_MAX, V_START_FINGER, V_MAX_FINGER, A_MAX_FINGER);
        // 转90度
        move_arm_block(lr, dir * 800, V_START_ARM, V_MAX_ARM, A_MAX_ARM);
        // 手指缩回
        move_finger_block(lr, FINGER_OFFSET_SPIN, V_START_FINGER, V_MAX_FINGER, A_MAX_FINGER);
        // 对侧手指缩回
        move_finger_block(!lr, FINGER_OFFSET_SPIN, V_START_FINGER, V_MAX_FINGER, A_MAX_FINGER);
    }
}
static void flip_cube_180(int lr)
{
    // 手指锁紧
    move_finger_block(lr, FINGER_OFFSET_LOCK, V_START_FINGER, V_MAX_FINGER, A_MAX_FINGER);
    sleep_us(DELAY_US_AFTER_FINGER_LOCK);
    // 对侧手指缩回
    move_finger_block(!lr, FINGER_OFFSET_MAX, V_START_FINGER, V_MAX_FINGER, A_MAX_FINGER);
    // 转180
    move_arm_block(lr, 1600, V_START_ARM_L, V_MAX_ARM_L, A_MAX_ARM_L);
    // 手指归位
    move_finger_block(!lr, FINGER_OFFSET_SPIN, V_START_FINGER, V_MAX_FINGER, A_MAX_FINGER);
    move_finger_block(lr, FINGER_OFFSET_SPIN, V_START_FINGER, V_MAX_FINGER, A_MAX_FINGER);
}
static void flip_cube_90(int lr, int cw_ccw, int skip_back_step)
{
    int dir = cw_ccw ? -1 : 1;
    // 手指锁紧
    move_finger_block(lr, FINGER_OFFSET_LOCK, V_START_FINGER, V_MAX_FINGER, A_MAX_FINGER);
    sleep_us(DELAY_US_AFTER_FINGER_LOCK);
    // 对侧手指松开
    move_finger_block(!lr, FINGER_OFFSET_MAX, V_START_FINGER, V_MAX_FINGER, A_MAX_FINGER);
    // 转90
    move_arm_block(lr, dir * 800, V_START_ARM_L, V_MAX_ARM_L, A_MAX_ARM_L);
    if(!skip_back_step)
    {
        // 手指归位
        move_finger_block(!lr, FINGER_OFFSET_SPIN, V_START_FINGER, V_MAX_FINGER, A_MAX_FINGER);
        move_finger_block(!lr, FINGER_OFFSET_LOCK, V_START_FINGER, V_MAX_FINGER, A_MAX_FINGER);
        sleep_us(DELAY_US_AFTER_FINGER_LOCK);
        move_finger_block(lr, FINGER_OFFSET_MAX, V_START_FINGER, V_MAX_FINGER, A_MAX_FINGER);
        // 转90度
        move_arm_block(lr, dir * 800, V_START_ARM, V_MAX_ARM, A_MAX_ARM);
    }
    // 手指缩回
    move_finger_block(lr, FINGER_OFFSET_SPIN, V_START_FINGER, V_MAX_FINGER, A_MAX_FINGER);
    // 对侧手指缩回
    move_finger_block(!lr, FINGER_OFFSET_SPIN, V_START_FINGER, V_MAX_FINGER, A_MAX_FINGER);
}

///////////////////////////////////////////////////////////////////////////////////
// ------------------------------ 解析魔方还原步骤 ------------------------------- //
///////////////////////////////////////////////////////////////////////////////////

enum cube_stat {
    STAT_DF=0,STAT_DB,  STAT_DL,  STAT_DR,
    STAT_UF,  STAT_UB,  STAT_UL,  STAT_UR,
    STAT_FL,  STAT_FR,  STAT_FU,  STAT_FD,
    STAT_BL,  STAT_BR,  STAT_BU,  STAT_BD,
    STAT_LF,  STAT_LB,  STAT_LU,  STAT_LD,
    STAT_RF,  STAT_RB,  STAT_RU,  STAT_RD,
};

enum cube_flip {
    FLIP_90_LEFT_CW   = 0,
    FLIP_180_LEFT     = 1,
    FLIP_90_LEFT_CCW  = 2,
    FLIP_90_RIGHT_CW  = 3,
    FLIP_180_RIGHT    = 4,
    FLIP_90_RIGHT_CCW = 5,
    FLIP_NULL         = 7,
};
enum cube_twist {
    TWIST_90_LEFT_CW   = 0,
    TWIST_180_LEFT     = 1,
    TWIST_90_LEFT_CCW  = 2,
    TWIST_90_RIGHT_CW  = 3,
    TWIST_180_RIGHT    = 4,
    TWIST_90_RIGHT_CCW = 5,
    TWIST_NULL         = 7,
};
            
// orig|90,LEFT,CW|180,LEFT|90,LEFT,CCW|90,RIGHT,CW|180,RIGHT|90,RIGHT,CCW
// 使用cube_table.py自动生成
const char cube_status_tab[24][6]={
    // 90,LEFT,CW|180,LEFT|90,LEFT,CCW|90,RIGHT,CW|180,RIGHT|90,RIGHT,CCW
    {STAT_DL, STAT_DB, STAT_DR, STAT_RF, STAT_UF, STAT_LF}, // flip from STAT_DF
    {STAT_DR, STAT_DF, STAT_DL, STAT_LB, STAT_UB, STAT_RB}, // flip from STAT_DB
    {STAT_DB, STAT_DR, STAT_DF, STAT_FL, STAT_UL, STAT_BL}, // flip from STAT_DL
    {STAT_DF, STAT_DL, STAT_DB, STAT_BR, STAT_UR, STAT_FR}, // flip from STAT_DR
    {STAT_UR, STAT_UB, STAT_UL, STAT_LF, STAT_DF, STAT_RF}, // flip from STAT_UF
    {STAT_UL, STAT_UF, STAT_UR, STAT_RB, STAT_DB, STAT_LB}, // flip from STAT_UB
    {STAT_UF, STAT_UR, STAT_UB, STAT_BL, STAT_DL, STAT_FL}, // flip from STAT_UL
    {STAT_UB, STAT_UL, STAT_UF, STAT_FR, STAT_DR, STAT_BR}, // flip from STAT_UR
    {STAT_FD, STAT_FR, STAT_FU, STAT_UL, STAT_BL, STAT_DL}, // flip from STAT_FL
    {STAT_FU, STAT_FL, STAT_FD, STAT_DR, STAT_BR, STAT_UR}, // flip from STAT_FR
    {STAT_FL, STAT_FD, STAT_FR, STAT_RU, STAT_BU, STAT_LU}, // flip from STAT_FU
    {STAT_FR, STAT_FU, STAT_FL, STAT_LD, STAT_BD, STAT_RD}, // flip from STAT_FD
    {STAT_BU, STAT_BR, STAT_BD, STAT_DL, STAT_FL, STAT_UL}, // flip from STAT_BL
    {STAT_BD, STAT_BL, STAT_BU, STAT_UR, STAT_FR, STAT_DR}, // flip from STAT_BR
    {STAT_BR, STAT_BD, STAT_BL, STAT_LU, STAT_FU, STAT_RU}, // flip from STAT_BU
    {STAT_BL, STAT_BU, STAT_BR, STAT_RD, STAT_FD, STAT_LD}, // flip from STAT_BD
    {STAT_LU, STAT_LB, STAT_LD, STAT_DF, STAT_RF, STAT_UF}, // flip from STAT_LF
    {STAT_LD, STAT_LF, STAT_LU, STAT_UB, STAT_RB, STAT_DB}, // flip from STAT_LB
    {STAT_LB, STAT_LD, STAT_LF, STAT_FU, STAT_RU, STAT_BU}, // flip from STAT_LU
    {STAT_LF, STAT_LU, STAT_LB, STAT_BD, STAT_RD, STAT_FD}, // flip from STAT_LD
    {STAT_RD, STAT_RB, STAT_RU, STAT_UF, STAT_LF, STAT_DF}, // flip from STAT_RF
    {STAT_RU, STAT_RF, STAT_RD, STAT_DB, STAT_LB, STAT_UB}, // flip from STAT_RB
    {STAT_RF, STAT_RD, STAT_RB, STAT_BU, STAT_LU, STAT_FU}, // flip from STAT_RU
    {STAT_RB, STAT_RU, STAT_RF, STAT_FD, STAT_LD, STAT_BD}, // flip from STAT_RD
};

// a: 需要进行的操作，取值范围如下
// "U", "R", "F", "D", "L", "B"
// "U'", "R'", "F'", "D'", "L'", "B'"
// "U2", "R2", "F2", "D2", "L2", "B2"
// stat: 魔方的朝向，取值范围如下(0-23)
//    STAT_DF,STAT_DB, STAT_DL, STAT_DR,
//    STAT_UF, STAT_UB, STAT_UL, STAT_UR,
//    STAT_FL, STAT_FR, STAT_FU, STAT_FD,
//    STAT_BL, STAT_BR, STAT_BU, STAT_BD,
//    STAT_LF, STAT_LB, STAT_LU, STAT_LD,
//    STAT_RF, STAT_RB, STAT_RU, STAT_RD,
// last_lr: 最后移动过的机械臂，下次操作时，会尽量选择同侧的
// LEFT  0
// RIGHT 1

// 返回值：魔方的朝向，取值范围和stat一样
static int cube_tweak(int stat, const char *a, int *last_lr, char *flip_twist)
{
    int lr = LEFT;
    int flip = FLIP_NULL;
    int twist = TWIST_NULL;
    const char cube_face_code[6] = {'U', 'R', 'F', 'D', 'L', 'B'};
    const char cube_stat_decode[24] = {
        0x32,0x35,0x34,0x31,
        0x02,0x05,0x04,0x01,
        0x24,0x21,0x20,0x23,
        0x54,0x51,0x50,0x53,
        0x42,0x45,0x40,0x43,
        0x12,0x15,0x10,0x13,
    }; 
    for(int face = 0; face < 6; face ++)
    {
        if(cube_face_code[face] == a[0])
        {
            // 将取值范围0-23的stat转为cube_stat_decode中描述的形式，方便判断可操作的面
            char stat_decode = cube_stat_decode[stat];
            printf("action == %c%c, status == %c%c\n", a[0], a[1], cube_face_code[stat_decode>>4], cube_face_code[stat_decode&0x0f]);
            if((stat_decode & 0xF0) == (face << 4))
            {
                // 拧左边
                lr = LEFT;
            }
            else if((stat_decode & 0x0F) == face)
            {
                // 拧右边
                lr = RIGHT;
            }
            else
            {
                // 需要调整魔方方向
                int start, end, add, i;
                // last_lr: 最后移动过的机械臂，翻转魔方操作时，尽量选择同侧的机械臂
                // 和上次动作同为90度时，可以节省一些操作步骤
                if(*last_lr == LEFT)
                {
                    start = 0;
                    end = 6;
                    add = 1;
                }
                else
                {
                    start = 5;
                    end = -1;
                    add = -1;
                }
                for(i=start; i != end; i+=add)
                {
                    int next = cube_status_tab[stat][i];
                    char next_stat_decode = cube_stat_decode[next];
                    if((next_stat_decode & 0xF0) == (face << 4))
                    {
                        lr = LEFT;
                        flip = i;
                        stat = next;
                        break;
                    }
                    else if((next_stat_decode & 0x0F) == face)
                    {
                        lr = RIGHT;
                        flip = i;
                        stat = next;
                        break;
                    }
                }
            }
            break;
        }
    }
    *last_lr = lr;
    // 拧魔方
    if('\'' == a[1])
    {
        if(LEFT == lr){
            twist = TWIST_90_LEFT_CCW;
        }else{
            twist = TWIST_90_RIGHT_CCW;
        }
    }
    else if('2' == a[1])
    {
        if(LEFT == lr){
            twist = TWIST_180_LEFT;
        }else{
            twist = TWIST_180_RIGHT;
        }
    }
    else
    {
        if(LEFT == lr){
            twist = TWIST_90_LEFT_CW;
        }else{
            twist = TWIST_90_RIGHT_CW;
        }
    }
    flip_twist[0] = (char)flip;
    flip_twist[1] = (char)twist;
    return stat;
};

// 按照求解结果执行动作
static int cube_tweak_str(int stat, const char *str)
{
    const int MAX_STEP = 25;
    const char *p = str;
    int count = 0;// 步骤数量
    int last_lr = LEFT; // 最后移动过的机械臂，下次操作时，会尽量选择同侧的
    char motion_table[MAX_STEP][2];// {flip, twist}
    while(1)
    {
        if(*p == 'U' || *p == 'R' || *p == 'F' || *p == 'D' || *p == 'L' || *p == 'B'){
            stat = cube_tweak(stat, p, &last_lr, motion_table[count]);
            p++;
            count++;
            if(*p == 0 || count >= MAX_STEP){
                break;
            }
        }
        p++;
        if(*p == 0){
            break;
        }
    }
    //printf("count=%d\n", count);
    for(int i=0; i<count-1; i++)
    {
        int twist = motion_table[i][1];
        int flip = motion_table[i+1][0];
        if((flip == FLIP_90_LEFT_CW || flip == FLIP_90_LEFT_CCW) && 
           (twist == TWIST_90_LEFT_CW || twist == TWIST_90_LEFT_CCW))
        {
            motion_table[i][1] |= 0x10;
            motion_table[i+1][0] |= 0x10;
        }
        if((flip == FLIP_90_RIGHT_CW || flip == FLIP_90_RIGHT_CCW) && 
           (twist == TWIST_90_RIGHT_CW || twist == TWIST_90_RIGHT_CCW))
        {
            motion_table[i][1] |= 0x10;
            motion_table[i+1][0] |= 0x10;
        }
    }
    for(int i=0; i<count; i++)
    {
        printf("count = %d\n", i);
        // 翻转魔方动作的编号，例如FLIP_90_LEFT_CW
        int flip = motion_table[i][0] & 0x0F;
        // 拧魔方动作的编号，例如TWIST_90_LEFT_CW
        int twist = motion_table[i][1] & 0x0F;
        // 如果可以跳过手指张开，机械臂旋转90度回零的动作，取值16
        // 如果不能，取值0
        int flip_skip_back_step = motion_table[i][0] & 0x10;
        int twist_skip_back_step = motion_table[i][1] & 0x10;
        switch(flip)
        {
            case FLIP_90_LEFT_CW:
                flip_cube_90(LEFT, CW, flip_skip_back_step);
                break;
            case FLIP_180_LEFT:
                flip_cube_180(LEFT);
                break;
            case FLIP_90_LEFT_CCW:
                flip_cube_90(LEFT, CCW, flip_skip_back_step);
                break;
            case FLIP_90_RIGHT_CW:
                flip_cube_90(RIGHT, CW, flip_skip_back_step);
                break;
            case FLIP_180_RIGHT:
                flip_cube_180(RIGHT);
                break;
            case FLIP_90_RIGHT_CCW:
                flip_cube_90(RIGHT, CCW, flip_skip_back_step);
                break;
            default: // FLIP_NULL
                break;
        }
        switch(twist)
        {
            case TWIST_90_LEFT_CW:
                twist_cube_90(LEFT, CW, twist_skip_back_step);
                break;
            case TWIST_180_LEFT:
                twist_cube_180(LEFT);
                break;
            case TWIST_90_LEFT_CCW:
                twist_cube_90(LEFT, CCW, twist_skip_back_step);
                break;
            case TWIST_90_RIGHT_CW:
                twist_cube_90(RIGHT, CW, twist_skip_back_step);
                break;
            case TWIST_180_RIGHT:
                twist_cube_180(RIGHT);
                break;
            case TWIST_90_RIGHT_CCW:
                twist_cube_90(RIGHT, CCW, twist_skip_back_step);
                break;
            default: // TWIST_NULL
                break;
        }
    }
    return stat;
}


static void gpio_config(void)
{
    gpio_init(LED);
    gpio_set_dir(LED, GPIO_OUT);

    gpio_init(DIAG0);
    gpio_init(DIAG1);
    gpio_init(DIAG2);
    gpio_init(DIAG3);

    gpio_init(ENN);
    gpio_put(ENN, 1);
    gpio_set_dir(ENN, GPIO_OUT);

    gpio_init(BUTTON_0);
    gpio_init(BUTTON_1);
    gpio_pull_up(BUTTON_0);
    gpio_pull_up(BUTTON_1);
    
    gpio_init(ITR9606_0);
    gpio_init(ITR9606_1);
    gpio_disable_pulls(ITR9606_0);
    gpio_disable_pulls(ITR9606_1);
}


int main()
{
    stdio_init_all();
    gpio_config();
    // UART
    tmc2209_uart_init();
    // 初始化PIO
    uint offset = pio_add_program(pio0, &step_ctrl_program);
    //step_ctrl_program_init(pio0, 0, offset, 20, 21); // 20 21 at CN3
    step_ctrl_program_init(pio0, 0, offset, STEP0, DIR0);
    step_ctrl_program_init(pio0, 1, offset, STEP1, DIR1);
    step_ctrl_program_init(pio0, 2, offset, STEP2, DIR2);
    step_ctrl_program_init(pio0, 3, offset, STEP3, DIR3);
    // 开中断
    irq_set_exclusive_handler(PIO0_IRQ_0, pio_fifo_not_full_handler);
    irq_set_enabled(PIO0_IRQ_0, true);
 
    // 等待建立USB连接，如果需要初始化阶段的调试信息，建议改到3000以上
    sleep_ms(3000); 
    puts("USB connect ready.");
    for (int addr = 0; addr < 4; addr++)
    {
       tmc2209_config(addr);
    }
    // 回零点
    // 有个BUG，上电后的第一次回零点，会有较大的声音，而且调不准，复位单片机后正常
    // 采用回零点两次的方式规避这一问题，等后续再分析定位问题原因
    // 这个问题，可能和TMC2209在刚刚上电时，20V供电工作电流比较大，随着执行多次步进电机运动控制，工作电流逐渐减小相关。
    zero_point();
    // 反复多次伸缩手指，改善润滑状态，然后再次回零，提升零点稳定性
    move_finger_repetition();
    zero_point();
    while (1)
    {
        while(1){
            sleep_ms(10);
            if(!gpio_get(BUTTON_1)){
                sleep_ms(10);
                if(!gpio_get(BUTTON_1)){
                    absolute_time_t t_but_1 = get_absolute_time();
                    while(!gpio_get(BUTTON_1));
                    absolute_time_t t_but_2 = get_absolute_time();
                    int time = (int)absolute_time_diff_us(t_but_1, t_but_2);
                    if(time < 500*1000){
                        // 按下时间小于0.5s
                        speed_factor = 1.0f;
                        accel_factor = 1.0f;
                    }else{
                        // 按下时间大于等于0.5s
                        speed_factor = 0.125f;// 降速设置
                        accel_factor = 0.125f * 0.125f;
                    }
                    break;
                }
            }
        }
        
        move_finger_init_to_default();
        //sleep_ms(100000);
        const char scramble_string_a[] = "D R2 U2 L2 B2 D F2 D L2 B2 D L2 F' U B R F2 U L D2 R2";
        const char scramble_string_b[] = "R2 D2 L' U' F2 R' B' U' F L2 D' B2 L2 D' F2 D' B2 L2 U2 R2 D'";
        int stat = STAT_DF;
       
        for(int i=0;i<3;i++)
        {
            stat = cube_tweak_str(stat, scramble_string_a);
            sleep_ms(1000);
            stat = cube_tweak_str(stat, scramble_string_b);
            sleep_ms(1000);
            /*
            flip_cube_90(LEFT, CCW, 0);
            flip_cube_90(RIGHT, CCW, 0);
            flip_cube_90(LEFT, CW, 0);
            flip_cube_90(RIGHT, CW, 0);
            twist_cube_180(LEFT);
            twist_cube_180(RIGHT);
            twist_cube_90(LEFT, CCW, 0);
            twist_cube_90(LEFT, CW, 0);
            twist_cube_90(RIGHT, CCW, 0);
            twist_cube_90(RIGHT, CW, 0);
            flip_cube_180(LEFT);
            flip_cube_180(RIGHT);
            */
        }     
        
        move_finger_default_to_init();
        
    }
    return 0;
}

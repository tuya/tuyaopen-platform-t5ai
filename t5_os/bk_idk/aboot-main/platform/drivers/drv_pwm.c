#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "BK_System.h"
#include "driver_system.h"
#include "driver_gpio.h"
#include "drv_pwm.h"


static void (*p_pwm_isr_handler[CHANNEL_NO])(u8);

#if (CFG_SOC_NAME == SOC_BK7256)
void pwm_icu_configuration(pwm_param_t *pwm_param)
{
    u32 reg;
    u32 prm;
    u32 prm_clksel;

    /* set clock power down of icu module*/
    switch(pwm_param->channel)
    {
    case PWM0:
        prm = PWD_PWM0_CLK_BIT;
        prm_clksel = PWD_PWM0_CLK_SEL_BIT;
        break;

    case PWM1:
        prm = PWD_PWM1_CLK_BIT;
        prm_clksel = PWD_PWM1_CLK_SEL_BIT;
        break;
    default:
        goto exit_icu;
    }

    reg = REG_READ(REG_ICU_PERI_CLK_PWD_ADDR);
    reg |= prm;
    REG_WRITE(REG_ICU_PERI_CLK_PWD_ADDR, reg);

    if(PWM_CLK_32K == pwm_param->cfg.bits.clk)
    {
        reg = REG_READ(REG_ICU_PWM_CLK_MUX_ADDR);
        reg &= ~prm_clksel;
        REG_WRITE(REG_ICU_PWM_CLK_MUX_ADDR, reg);
    }
    else
    {
        reg = REG_READ(REG_ICU_PWM_CLK_MUX_ADDR);
        reg |= prm_clksel;
        REG_WRITE(REG_ICU_PWM_CLK_MUX_ADDR, reg);
    }

    if(PWM_INT_EN == pwm_param->cfg.bits.int_en)
    {
        prm = IRQ_PWM_BIT;

        reg = REG_READ(REG_ICU_INT_ENABLE_ADDR);
        reg |= (prm);
        REG_WRITE(REG_ICU_INT_ENABLE_ADDR, reg);
    }

    return;

exit_icu:
    PWM_WARN("pwm_iconfig_fail\r\n");

    return;
}

#else
void pwm_icu_configuration(pwm_param_t *pwm_param)
{
    u32 reg;
    u32 prm;

    /* set clock power down of icu module*/
    switch(pwm_param->channel)
    {
    case PWM0:
        prm = PWD_PWM0_CLK_BIT;
        break;

    case PWM1:
        prm = PWD_PWM1_CLK_BIT;
        break;

    case PWM2:
        prm = PWD_PWM2_CLK_BIT;
        break;

    case PWM3:
        prm = PWD_PWM3_CLK_BIT;
        break;

#if (CFG_SOC_NAME != SOC_BK7271)
    case PWM4:
        prm = PWD_PWM4_CLK_BIT;
        break;

    case PWM5:
        prm = PWD_PWM5_CLK_BIT;
        break;
#endif


    default:
        goto exit_icu;
    }

    reg = REG_READ(REG_ICU_PERI_CLK_PWD_ADDR);
    reg &= ~(prm);
    REG_WRITE(REG_ICU_PERI_CLK_PWD_ADDR, reg);

    if(PWM_CLK_32K == pwm_param->cfg.bits.clk)
    {
        prm = pwm_param->channel;

        reg = REG_READ(REG_ICU_PWM_CLK_MUX_ADDR);
        reg |= (1 << prm);
        REG_WRITE(REG_ICU_PWM_CLK_MUX_ADDR, reg);
    }
    else
    {
        prm = pwm_param->channel;

        reg = REG_READ(REG_ICU_PWM_CLK_MUX_ADDR);
        reg &= ~(1 << prm);
        REG_WRITE(REG_ICU_PWM_CLK_MUX_ADDR, reg);
    }

    if(PWM_INT_EN == pwm_param->cfg.bits.int_en)
    {
        prm = IRQ_PWM_BIT;

        reg = REG_READ(REG_ICU_INT_ENABLE_ADDR);
        reg |= (prm);
        REG_WRITE(REG_ICU_INT_ENABLE_ADDR, reg);
    }

    return;

exit_icu:
    PWM_WARN("pwm_iconfig_fail\r\n");

    return;
}

#endif


static void init_pwm_param(pwm_param_t *pwm_param)
{
    u32 value;

    if((pwm_param == NULL)
            && (pwm_param->channel >= PWM_COUNT)
            && (pwm_param->duty_cycle > pwm_param->end_value))
    {
        return;
    }

	if(pwm_param->cfg.bits.mode != PMODE_TIMER)
	{
    GPIO_PWM_function_enable(pwm_param->channel);
    }

    value = REG_READ(PWM_CTL);
    value = value & ~(0x0F << (0x04 *  pwm_param->channel))
            | ((pwm_param->cfg.val & 0x0F) << (0x04 * pwm_param->channel));
    REG_WRITE(PWM_CTL, value);

#if (CFG_SOC_NAME == SOC_BK7231)
    value = (((u32)pwm_param->duty_cycle & 0x0000FFFF) << 16)
            + ((u32)pwm_param->end_value & 0x0000FFFF);
    REG_WRITE(REG_APB_BK_PWMn_CNT_ADDR(pwm_param->channel), value);
#else
    value = ((UINT32)pwm_param->end_value);
    REG_WRITE(REG_APB_BK_PWMn_END_ADDR(pwm_param->channel), value);

    value = ((UINT32)pwm_param->duty_cycle);
    REG_WRITE(REG_APB_BK_PWMn_DC_ADDR(pwm_param->channel), value);
#endif

    p_pwm_isr_handler[pwm_param->channel] = pwm_param->p_Int_Handler;

    pwm_icu_configuration(pwm_param);
}

static u16 pwm_capture_value_get(u8 ucChannel)
{
    return REG_READ(REG_APB_BK_PWMn_CAP_ADDR(ucChannel));
}

static void pwm_int_handler_clear(u8 ucChannel)
{
    p_pwm_isr_handler[ucChannel] = NULL;
}


void pwm_exit(void)
{
}

u32 pwm_ctrl(u32 cmd, void *param)
{
    u32 ret = PWM_SUCCESS;
    u32 ucChannel;
    u32 value;
    pwm_param_t *p_param;
    pwm_capture_t *p_capture;

    switch(cmd)
    {
    case CMD_PWM_UNIT_ENABLE:
        ucChannel = (*(u32 *)param);
        if(ucChannel > 5)
        {
            ret = PWM_FAILURE;
            break;
        }
        value = REG_READ(PWM_CTL);
        value |= (1 << (ucChannel * 4));
        REG_WRITE(PWM_CTL, value);
        break;
    case CMD_PWM_UINT_DISABLE:
        ucChannel = (*(u32 *)param);
        if(ucChannel > 5)
        {
            ret = PWM_FAILURE;
            break;
        }
        value = REG_READ(PWM_CTL);
        value &= ~(3 << (ucChannel * 4));
        REG_WRITE(PWM_CTL, value);
        break;
    case CMD_PWM_IR_ENABLE:
        ucChannel = (*(u32 *)param);
        if(ucChannel > 5)
        {
            ret = PWM_FAILURE;
            break;
        }
        value = REG_READ(PWM_CTL);
        value |= (2 << (ucChannel * 4));
        REG_WRITE(PWM_CTL, value);
        break;
    case CMD_PWM_IR_DISABLE:
        ucChannel = (*(u32 *)param);
        if(ucChannel > 5)
        {
            ret = PWM_FAILURE;
            break;
        }
        value = REG_READ(PWM_CTL);
        value &= ~(2 << (ucChannel * 4));
        REG_WRITE(PWM_CTL, value);
        break;
    case CMD_PWM_IR_CLEAR:
        ucChannel = (*(u32 *)param);
        if(ucChannel > 5)
        {
            ret = PWM_FAILURE;
            break;
        }
        pwm_int_handler_clear(ucChannel);
        break;
    case CMD_PWM_INIT_PARAM:
        p_param = (pwm_param_t *)param;
        init_pwm_param(p_param);
        break;
    case CMD_PWM_CAP_GET:
        p_capture = (pwm_capture_t *)param;
        if(p_capture->ucChannel > 5)
        {
            ret = PWM_FAILURE;
            break;
        }
        p_capture->value = pwm_capture_value_get(p_capture->ucChannel);
        break;
    default:
        ret = PWM_FAILURE;
        break;
    }

    return ret;
}

void pwm_isr(void)
{
    int i;
    u32 status;

    status = REG_READ(PWM_INTERRUPT_STATUS);

    for(i = 0; i < CHANNEL_NO; i++)
    {
        if(status & (1 << i))
        {
            if(p_pwm_isr_handler[i])
            {
                p_pwm_isr_handler[i]((u8)i);
                do
                {
                    REG_WRITE(PWM_INTERRUPT_STATUS, (1 << i));
                }
                while(REG_READ(PWM_INTERRUPT_STATUS) & (1 << i));
            }
        }
    }

    do
    {
        REG_WRITE(PWM_INTERRUPT_STATUS, status);
    }
    while(REG_READ(PWM_INTERRUPT_STATUS) & status & 0x3F);
}


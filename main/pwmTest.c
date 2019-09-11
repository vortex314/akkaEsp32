/* MCPWM basic config example

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/

/*
 * This example will show you how to use each submodule of MCPWM unit.
 * The example can't be used without modifying the code first.
 * Edit the macros at the top of mcpwm_example_basic_config.c to enable/disable the submodules which are used in the example.
 */

#include <stdio.h>
#include "string.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "esp_attr.h"
#include "soc/rtc.h"
#include "driver/mcpwm.h"
//#include "soc/mcpwm_periph.h"

#define MCPWM_EN_CARRIER 0   //Make this 1 to test carrier submodule of mcpwm, set high frequency carrier parameters
#define MCPWM_EN_DEADTIME 0  //Make this 1 to test deadtime submodule of mcpwm, set deadtime value and deadtime mode
#define MCPWM_EN_FAULT 0     //Make this 1 to test fault submodule of mcpwm, set action on MCPWM signal on fault occurence like overcurrent, overvoltage, etc
#define MCPWM_EN_SYNC 0      //Make this 1 to test sync submodule of mcpwm, sync timer signals
#define MCPWM_EN_CAPTURE 0   //Make this 1 to test capture submodule of mcpwm, measure time between rising/falling edge of captured signal
#define MCPWM_GPIO_INIT 0    //select which function to use to initialize gpio signals
#define CAP_SIG_NUM 3   //Three capture signals


#define GPIO_PWM0A_OUT 19   //Set GPIO 19 as PWM0A
#define GPIO_PWM0B_OUT 18   //Set GPIO 18 as PWM0B

#define GPIO_PWM1A_OUT 17   //Set GPIO 17 as PWM1A
#define GPIO_PWM1B_OUT 16   //Set GPIO 16 as PWM1B





static void mcpwm_example_gpio_initialize(void)
{
    printf("initializing mcpwm gpio...\n");
    mcpwm_gpio_init(MCPWM_UNIT_0, MCPWM0A, GPIO_PWM0A_OUT);
    mcpwm_gpio_init(MCPWM_UNIT_0, MCPWM0B, GPIO_PWM0B_OUT);

    mcpwm_gpio_init(MCPWM_UNIT_1, MCPWM0A, GPIO_PWM1A_OUT);
    mcpwm_gpio_init(MCPWM_UNIT_1, MCPWM0B, GPIO_PWM1B_OUT);
}

/**
 * @brief Set gpio 12 as our test signal that generates high-low waveform continuously, connect this gpio to capture pin.
 */

uint32_t duty_cycle[]= {0,100};
int32_t delta[]= {-1,1};

void change_pwm(mcpwm_unit_t mcpwm_num)
{
    esp_err_t rc;
    rc = mcpwm_set_signal_low(mcpwm_num, MCPWM_TIMER_0, MCPWM_OPR_B);
    if ( rc != ESP_OK ) {
        printf("mcpwm_set_signal_low()=%d\n",rc);
    }
    rc = mcpwm_set_duty_type( mcpwm_num,MCPWM_TIMER_0, MCPWM_OPR_A,MCPWM_DUTY_MODE_0);
    if ( rc != ESP_OK ) {
        printf("mcpwm_set_duty_type()=%d\n",rc);
    }
    rc = mcpwm_set_duty(mcpwm_num, MCPWM_TIMER_0, MCPWM_OPR_A, duty_cycle[mcpwm_num]);
    if ( rc != ESP_OK ) {
        printf("mcpwm_set_duty_type()=%d\n",rc);
    }
    if ( duty_cycle[mcpwm_num]==0 ) {
        delta[mcpwm_num]=1;
    } else if( duty_cycle[mcpwm_num]==100) {
        delta[mcpwm_num]=-1;
    }
    duty_cycle[mcpwm_num]+= delta[mcpwm_num];
}


static void mod_pwm(void *arg)
{
    while(true) {
        vTaskDelay(10);
        change_pwm(0);
        change_pwm(1);
    }
}





/**
 * @brief Configure whole MCPWM module
 */
static void mcpwm_example_config(void *arg)
{
    //1. mcpwm gpio initialization
    mcpwm_example_gpio_initialize();

    //2. initialize mcpwm configuration
    printf("Configuring Initial Parameters of mcpwm 0 ...\n");
    mcpwm_config_t pwm_config;
    pwm_config.frequency = 1000;    //frequency = 1000Hz
    pwm_config.cmpr_a = 60.0;       //duty cycle of PWMxA = 60.0%
    pwm_config.cmpr_b = 50.0;       //duty cycle of PWMxb = 50.0%
    pwm_config.counter_mode = MCPWM_UP_COUNTER;
    pwm_config.duty_mode = MCPWM_DUTY_MODE_0;
    mcpwm_init(MCPWM_UNIT_0, MCPWM_TIMER_0, &pwm_config);   //Configure PWM0A & PWM0B with above settings

    printf("Configuring Initial Parameters of mcpwm 1 ...\n");
    pwm_config.frequency = 1000;    //frequency = 1000Hz
    pwm_config.cmpr_a = 60.0;       //duty cycle of PWMxA = 60.0%
    pwm_config.cmpr_b = 50.0;       //duty cycle of PWMxb = 50.0%
    pwm_config.counter_mode = MCPWM_UP_COUNTER;
    pwm_config.duty_mode = MCPWM_DUTY_MODE_0;
    mcpwm_init(MCPWM_UNIT_1, MCPWM_TIMER_0, &pwm_config);   //Configure PWM0A & PWM0B with above settings


    vTaskDelete(NULL);
}

void app_main2(void)
{
    printf("Testing MCPWM...\n");
//    cap_queue = xQueueCreate(1, sizeof(capture)); //comment if you don't want to use capture module
//    xTaskCreate(disp_captured_signal, "mcpwm_config", 4096, NULL, 5, NULL);  //comment if you don't want to use capture module
    xTaskCreate(mod_pwm, "mod_pwm", 4096, NULL, 5, NULL);
    xTaskCreate(mcpwm_example_config, "mcpwm_example_config", 4096, NULL, 5, NULL);
}

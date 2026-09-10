/****************************************************************************
 *
 *   Copyright (c) 2013 PX4 Development Team. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 * 3. Neither the name PX4 nor the names of its contributors may be
 *    used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
 * COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS
 * OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
 * AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
 * ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 ****************************************************************************/

#include <px4_platform_common/px4_config.h>

#include <stdbool.h>

#include "chip.h"
#include "stm32_gpio.h"
#include "board_config.h"

#include <nuttx/board.h>
#include <arch/board/board.h>

#include <drivers/drv_hrt.h>

/*
 * Winegard Kestrel has a single green status LED on GPIO PC13, active low.
 * Red LED is hardware-controlled (always on when powered, no software control).
 *
 * This board has no RGB LED and never will. All indices (RED/BLUE/GREEN)
 * alias to the same physical pin -- PX4 core code (commander, led.cpp)
 * calls led_on/off/toggle with an LED_* index, so that signature is kept
 * even though there's only one real LED.
 */

static uint32_t g_ledmap[] = {
	GPIO_nLED_GREEN,   // RED
	GPIO_nLED_GREEN,   // BLUE
	GPIO_nLED_GREEN    // GREEN
};

static bool g_led_inverted[] = { true, true, true };

#define xlat(p) (p)

__EXPORT void led_init(void)
{
	for (size_t l = 0; l < (sizeof(g_ledmap) / sizeof(g_ledmap[0])); l++) {
		if (g_ledmap[l] != 0) {
			stm32_configgpio(g_ledmap[l]);
		}
	}
}

static void phy_set_led(int led, bool state)
{
	if (g_ledmap[led] != 0) {
		stm32_gpiowrite(g_ledmap[led], g_led_inverted[led] ? !state : state);
	}
}

static bool phy_get_led(int led)
{
	if (g_ledmap[led] != 0) {
		bool value = stm32_gpioread(g_ledmap[led]);
		return g_led_inverted[led] ? !value : value;
	}

	return false;
}

__EXPORT void led_on(int led)
{
	phy_set_led(xlat(led), true);
}

__EXPORT void led_off(int led)
{
	phy_set_led(xlat(led), false);
}

__EXPORT void led_toggle(int led)
{
	phy_set_led(xlat(led), !phy_get_led(xlat(led)));
}

/****************************************************************************
 * Heartbeat: blinks the LED at 1Hz to indicate the system is alive,
 * independent of commander/arming/overload state.
 ****************************************************************************/

static struct hrt_call _heartbeat_call;

static void heartbeat_tick(void *arg)
{
	led_toggle(2);
}

__EXPORT void heartbeat_led_init(void)
{
	led_init();
	hrt_call_every(&_heartbeat_call, 0, 500000, heartbeat_tick, 0);
}

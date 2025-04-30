#include <stdint.h>
#include "hardware/gpio.h"
#include "tusb.h"

#define JPORT1		0
#define JPORT2		2
#define JPORT3		4
#define JPORT4		6
#define JPORT5		VCC
#define JPORT6		1
#define JPORT7		3
#define JPORT8		5
#define JPORT9		GND

#define GPIO_DAT1	JPORT1
#define GPIO_DAT2	JPORT2
#define GPIO_DAT3	JPORT3
#define GPIO_DAT4	JPORT4
#define GPIO_BTNL	JPORT6
#define GPIO_BTNR	JPORT7
#define GPIO_STB	JPORT8
#define GPIO_LED	25

#define DIV_MIN		2
#define DIV_MAX		8

static volatile int32_t x = 0;
static volatile int32_t y = 0;

void tuh_hid_mount_cb(uint8_t dev_addr, uint8_t instance, uint8_t const* desc_report, uint16_t desc_len)
{
	if (tuh_hid_interface_protocol(dev_addr, instance) != HID_ITF_PROTOCOL_MOUSE) {
		return;
	}

	gpio_put(GPIO_LED, 1);
	tuh_hid_receive_report(dev_addr, instance);
}

void tuh_hid_umount_cb(uint8_t dev_addr, uint8_t instance)
{
	if (tuh_hid_interface_protocol(dev_addr, instance) != HID_ITF_PROTOCOL_MOUSE) {
		return;
	}

	gpio_put(GPIO_LED, 0);
}

void tuh_hid_report_received_cb(uint8_t dev_addr, uint8_t instance, uint8_t const* report, uint16_t len)
{
	static uint8_t mid = 0;
	static uint8_t div = DIV_MIN;
	int32_t tmp;

	if (tuh_hid_interface_protocol(dev_addr, instance) != HID_ITF_PROTOCOL_MOUSE) {
		return;
	}

	if ((mid == 0) && (((hid_mouse_report_t const *)report)->buttons & 0b100)) {
		div += 2;
		if (div > DIV_MAX) {
			div = DIV_MIN;
		}
	}
	mid = ((hid_mouse_report_t const *)report)->buttons & 0b100;

	gpio_set_dir(GPIO_BTNL, ((hid_mouse_report_t const *)report)->buttons & 0b001);
	gpio_set_dir(GPIO_BTNR, ((hid_mouse_report_t const *)report)->buttons & 0b010);

	gpio_set_irq_enabled(GPIO_STB, GPIO_IRQ_EDGE_FALL | GPIO_IRQ_EDGE_RISE, false);

	tmp = x - ((hid_mouse_report_t const *)report)->x / div;
	tmp = MAX(tmp, -128);
	tmp = MIN(tmp, 127);
	x = tmp;

	tmp = y - ((hid_mouse_report_t const *)report)->y / div;
	tmp = MAX(tmp, -128);
	tmp = MIN(tmp, 127);
	y = tmp;

	gpio_set_irq_enabled(GPIO_STB, GPIO_IRQ_EDGE_FALL | GPIO_IRQ_EDGE_RISE, true);

	tuh_hid_receive_report(dev_addr, instance);
}

static void gpio_callback(uint gpio, uint32_t events)
{
	static uint32_t irqtime = 0;
	static uint8_t phase = 0;
	static int8_t regx = 0;
	static int8_t regy = 0;

	if ((events == GPIO_IRQ_EDGE_RISE) && ((time_us_32() - irqtime) > 300)) {
		phase = 0;
	} else {
		phase = (phase + 1) & 0b11;
	}
	irqtime = time_us_32();

	switch (phase) {
		case 0: /* upper X */
			regx = x; x = 0;
			regy = y; y = 0;
			gpio_set_dir(GPIO_DAT4, !(regx & 0b10000000));
			gpio_set_dir(GPIO_DAT3, !(regx & 0b01000000));
			gpio_set_dir(GPIO_DAT2, !(regx & 0b00100000));
			gpio_set_dir(GPIO_DAT1, !(regx & 0b00010000));
			break;
		case 1: /* lower X */
			gpio_set_dir(GPIO_DAT4, !(regx & 0b00001000));
			gpio_set_dir(GPIO_DAT3, !(regx & 0b00000100));
			gpio_set_dir(GPIO_DAT2, !(regx & 0b00000010));
			gpio_set_dir(GPIO_DAT1, !(regx & 0b00000001));
			break;
		case 2: /* upper Y */
			gpio_set_dir(GPIO_DAT4, !(regy & 0b10000000));
			gpio_set_dir(GPIO_DAT3, !(regy & 0b01000000));
			gpio_set_dir(GPIO_DAT2, !(regy & 0b00100000));
			gpio_set_dir(GPIO_DAT1, !(regy & 0b00010000));
			break;
		case 3: /* lower Y */
			gpio_set_dir(GPIO_DAT4, !(regy & 0b00001000));
			gpio_set_dir(GPIO_DAT3, !(regy & 0b00000100));
			gpio_set_dir(GPIO_DAT2, !(regy & 0b00000010));
			gpio_set_dir(GPIO_DAT1, !(regy & 0b00000001));
			break;
	}
}

int main(void)
{
	gpio_init(JPORT1);
	gpio_init(JPORT2);
	gpio_init(JPORT3);
	gpio_init(JPORT4);
	gpio_init(JPORT6);
	gpio_init(JPORT7);
	gpio_init(JPORT8);

	gpio_pull_up(JPORT1);
	gpio_pull_up(JPORT2);
	gpio_pull_up(JPORT3);
	gpio_pull_up(JPORT4);
	gpio_pull_up(JPORT6);
	gpio_pull_up(JPORT7);
	gpio_pull_up(JPORT8);

	gpio_put(GPIO_DAT1, 0);
	gpio_put(GPIO_DAT2, 0);
	gpio_put(GPIO_DAT3, 0);
	gpio_put(GPIO_DAT4, 0);
	gpio_put(GPIO_BTNL, 0);
	gpio_put(GPIO_BTNR, 0);

	gpio_init(GPIO_LED);
	gpio_set_dir(GPIO_LED, 1);

	gpio_set_irq_enabled_with_callback(GPIO_STB, GPIO_IRQ_EDGE_FALL | GPIO_IRQ_EDGE_RISE, true, &gpio_callback);

	tuh_init(0);

	while (1) {
		tuh_task();
	}
}

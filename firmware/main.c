#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include <limits.h>

#include "hardware/clocks.h"
#include "hardware/pio.h"
#include "pico/stdlib.h"
#include "pico/multicore.h"
#include "pico/critical_section.h"
#include "pio_usb.h"
#include "tusb.h"
#include "ws2812.pio.h"

enum {
    ITF_NUM_CDC = 0,
    ITF_NUM_CDC_DATA,
    ITF_NUM_HID,
    ITF_NUM_TOTAL
};

#define CONFIG_TOTAL_LEN (TUD_CONFIG_DESC_LEN + TUD_CDC_DESC_LEN + TUD_HID_DESC_LEN)
#define EPNUM_CDC_NOTIF  0x81
#define EPNUM_CDC_OUT    0x02
#define EPNUM_CDC_IN     0x82
#define EPNUM_HID_IN     0x83

static uint8_t const hid_report_desc[] = {
    TUD_HID_REPORT_DESC_MOUSE()
};

static uint8_t const desc_configuration[] = {
    TUD_CONFIG_DESCRIPTOR(1, ITF_NUM_TOTAL, 0, CONFIG_TOTAL_LEN,
                          0, 100),
    TUD_CDC_DESCRIPTOR(ITF_NUM_CDC, 4, EPNUM_CDC_NOTIF, 8,
                       EPNUM_CDC_OUT, EPNUM_CDC_IN, 64),
    TUD_HID_DESCRIPTOR(ITF_NUM_HID, 5, HID_ITF_PROTOCOL_MOUSE,
                       sizeof(hid_report_desc), EPNUM_HID_IN, 16, 1)
};

static tusb_desc_device_t const desc_device = {
    .bLength            = sizeof(tusb_desc_device_t),
    .bDescriptorType    = TUSB_DESC_DEVICE,
    .bcdUSB             = 0x0200,
    .bDeviceClass       = TUSB_CLASS_MISC,
    .bDeviceSubClass    = MISC_SUBCLASS_COMMON,
    .bDeviceProtocol    = MISC_PROTOCOL_IAD,
    .bMaxPacketSize0    = CFG_TUD_ENDPOINT0_SIZE,
    .idVendor           = 0xCAFE,
    .idProduct          = 0x4010,
    .bcdDevice          = 0x0100,
    .iManufacturer      = 1,
    .iProduct           = 2,
    .iSerialNumber      = 3,
    .bNumConfigurations = 1
};

uint8_t const *tud_descriptor_device_cb(void) {
    return (uint8_t const *)&desc_device;
}

uint8_t const *tud_descriptor_configuration_cb(uint8_t index) {
    (void)index;
    return desc_configuration;
}

uint8_t const *tud_hid_descriptor_report_cb(uint8_t instance) {
    (void)instance;
    return hid_report_desc;
}

uint16_t tud_hid_get_report_cb(uint8_t instance,
                               uint8_t report_id,
                               hid_report_type_t report_type,
                               uint8_t *buffer,
                               uint16_t reqlen) {
    (void)instance;
    (void)report_id;
    (void)report_type;
    (void)buffer;
    (void)reqlen;
    return 0;
}

void tud_hid_set_report_cb(uint8_t instance,
                           uint8_t report_id,
                           hid_report_type_t report_type,
                           uint8_t const *buffer,
                           uint16_t bufsize) {
    (void)instance;
    (void)report_id;
    (void)report_type;
    (void)buffer;
    (void)bufsize;
}

static char const *string_desc_arr[] = {
    (const char[]) { 0x09, 0x04 },
    "Input Systems",
    "USB Input Bridge",
    "IB-000001",
    "Control Interface",
    "HID Pointer"
};

uint16_t const *tud_descriptor_string_cb(uint8_t index, uint16_t langid) {
    (void)langid;
    static uint16_t desc_string[32];
    uint8_t count;

    if (index == 0) {
        desc_string[1] = 0x0409;
        count = 1;
    } else {
        const size_t arr_count = sizeof(string_desc_arr) / sizeof(string_desc_arr[0]);
        if (index >= arr_count) return NULL;
        const char *str = string_desc_arr[index];
        size_t length = strlen(str);
        if (length > 31) length = 31;
        count = (uint8_t)length;
        for (uint8_t i = 0; i < count; ++i) {
            desc_string[1 + i] = (uint8_t)str[i];
        }
    }

    desc_string[0] = (uint16_t)((TUSB_DESC_STRING << 8) | (2 * count + 2));
    return desc_string;
}

static char command_line[80];
static size_t command_length = 0;
static bool command_overflow = false;

static bool pending_report = false;
static int pending_x = 0;
static int pending_y = 0;
static uint8_t last_sent_buttons = 0;

static critical_section_t mouse_lock;
static int32_t host_dx_accum = 0;
static int32_t host_dy_accum = 0;
static int32_t host_wheel_accum = 0;
static int32_t host_pan_accum = 0;
static uint8_t host_buttons = 0;

#define WS2812_PIN 16u
#define WS2812_BLINK_HALF_PERIOD_US 100000u
#define WS2812_ACTIVITY_TIMEOUT_US 250000u
#define WS2812_RED_LEVEL 32u

static PIO ws2812_pio = pio1;
static uint ws2812_sm = 0;
static uint ws2812_offset = 0;
static bool ws2812_red_is_on = false;
static uint32_t last_host_report_us = 0;

static inline uint32_t ws2812_urgb_u32(uint8_t r, uint8_t g, uint8_t b) {
    return ((uint32_t)r << 8u) | ((uint32_t)g << 16u) | (uint32_t)b;
}

static void ws2812_set_rgb(uint8_t r, uint8_t g, uint8_t b) {
    pio_sm_put_blocking(ws2812_pio, ws2812_sm, ws2812_urgb_u32(r, g, b) << 8u);
}

static void ws2812_init(void) {
    ws2812_sm = pio_claim_unused_sm(ws2812_pio, true);
    ws2812_offset = pio_add_program(ws2812_pio, &ws2812_program);
    ws2812_program_init(ws2812_pio, ws2812_sm, ws2812_offset,
                        WS2812_PIN, 800000.0f, false);
    ws2812_set_rgb(0, 0, 0);
    ws2812_red_is_on = false;
}

static void service_usb_activity_led(void) {
    const uint32_t now = time_us_32();
    const uint32_t last = __atomic_load_n(&last_host_report_us, __ATOMIC_RELAXED);
    const bool active = (last != 0u) &&
        ((uint32_t)(now - last) < WS2812_ACTIVITY_TIMEOUT_US);
    const bool should_be_red = active &&
        (((now / WS2812_BLINK_HALF_PERIOD_US) & 1u) == 0u);

    if (should_be_red != ws2812_red_is_on) {
        if (should_be_red) {
            ws2812_set_rgb(WS2812_RED_LEVEL, 0, 0);
        } else {
            ws2812_set_rgb(0, 0, 0);
        }
        ws2812_red_is_on = should_be_red;
    }
}

static void serial_ack_ok(void) {
    static const char response[] = "OK\n";
    tud_cdc_write(response, sizeof(response) - 1u);
    tud_cdc_write_flush();
}

static int32_t clamp_accumulator(int32_t value) {
    if (value > 16000) return 16000;
    if (value < -16000) return -16000;
    return value;
}

static int8_t clamp_report_value(int32_t value) {
    if (value > 127) return 127;
    if (value < -127) return -127;
    return (int8_t)value;
}

static bool is_space(char c) {
    return c == ' ' || c == '\t';
}

static void skip_spaces(const char **cursor) {
    while (is_space(**cursor)) ++(*cursor);
}

static bool parse_integer(const char **cursor, int *result) {
    const char *p = *cursor;
    bool negative = false;
    uint32_t magnitude = 0;

    skip_spaces(&p);
    if (*p == '-' || *p == '+') {
        negative = (*p == '-');
        ++p;
    }
    if (*p < '0' || *p > '9') return false;

    const uint32_t limit = negative ? ((uint32_t)INT_MAX + 1u) : (uint32_t)INT_MAX;
    do {
        const uint32_t digit = (uint32_t)(*p - '0');
        if (magnitude > (limit - digit) / 10u) return false;
        magnitude = magnitude * 10u + digit;
        ++p;
    } while (*p >= '0' && *p <= '9');

    if (*p != '\0' && !is_space(*p)) return false;

    if (negative) {
        *result = (magnitude == (uint32_t)INT_MAX + 1u) ? INT_MIN : -(int)magnitude;
    } else {
        *result = (int)magnitude;
    }
    *cursor = p;
    return true;
}

static bool at_end(const char **cursor) {
    skip_spaces(cursor);
    return **cursor == '\0';
}

static void parse_command(const char *line) {
    const char *cursor = line;
    int x = 0;
    int y = 0;

    skip_spaces(&cursor);
    if (*cursor == '\0') return;
    const char command = *cursor++;
    if (!is_space(*cursor)) return;

    if (command == 'M') {
        if (!parse_integer(&cursor, &x) ||
            !parse_integer(&cursor, &y) ||
            !at_end(&cursor)) {
            return;
        }
        if (x < -127 || x > 127 || y < -127 || y > 127) return;

        pending_x = x;
        pending_y = y;
        pending_report = true;
        return;
    }
}

static void poll_serial_commands(void) {
    while (!pending_report && tud_cdc_available()) {
        const int value = tud_cdc_read_char();
        if (value < 0) break;

        const char c = (char)value;
        if (c == '\r') continue;

        if (c == '\n') {
            if (!command_overflow) {
                command_line[command_length] = '\0';
                if (command_length > 0u) parse_command(command_line);
            }
            command_length = 0u;
            command_overflow = false;
            continue;
        }

        if ((unsigned char)c < 0x20u && c != '\t') {
            command_overflow = true;
        } else if (!command_overflow) {
            if (command_length + 1u < sizeof(command_line)) {
                command_line[command_length++] = c;
            } else {
                command_overflow = true;
            }
        }
    }
}

static void service_mouse_reports(void) {
    if (!tud_hid_ready()) return;

    critical_section_enter_blocking(&mouse_lock);

    if (pending_report) {
        const uint8_t buttons = host_buttons;
        const int8_t x = (int8_t)pending_x;
        const int8_t y = (int8_t)pending_y;

        bool sent = tud_hid_mouse_report(0, buttons, x, y, 0, 0);
        if (sent) {
            pending_report = false;
            last_sent_buttons = buttons;
        }
        critical_section_exit(&mouse_lock);

        if (sent) serial_ack_ok();
        return;
    }

    const int8_t x = clamp_report_value(host_dx_accum);
    const int8_t y = clamp_report_value(host_dy_accum);
    const int8_t wheel = clamp_report_value(host_wheel_accum);
    const int8_t pan = clamp_report_value(host_pan_accum);
    const uint8_t buttons = host_buttons;

    if (x == 0 && y == 0 && wheel == 0 && pan == 0 && buttons == last_sent_buttons) {
        critical_section_exit(&mouse_lock);
        return;
    }

    bool sent = tud_hid_mouse_report(0, buttons, x, y, wheel, pan);
    if (sent) {
        host_dx_accum -= x;
        host_dy_accum -= y;
        host_wheel_accum -= wheel;
        host_pan_accum -= pan;
        last_sent_buttons = buttons;
    }

    critical_section_exit(&mouse_lock);
}

static void host_core1_main(void) {
    sleep_ms(10);
    pio_usb_configuration_t pio_cfg = PIO_USB_DEFAULT_CONFIG;
    pio_cfg.pin_dp = 0;
    (void)tuh_configure(1, TUH_CFGID_RPI_PIO_USB_CONFIGURATION, &pio_cfg);
    (void)tuh_init(1);

    while (true) {
        tuh_task();
    }
}

void tuh_hid_mount_cb(uint8_t dev_addr, uint8_t instance,
                      uint8_t const *desc_report, uint16_t desc_len) {
    (void)desc_report;
    (void)desc_len;

    if (tuh_hid_interface_protocol(dev_addr, instance) == HID_ITF_PROTOCOL_MOUSE) {
        (void)tuh_hid_receive_report(dev_addr, instance);
    }
}

void tuh_hid_umount_cb(uint8_t dev_addr, uint8_t instance) {
    (void)dev_addr;
    (void)instance;
    critical_section_enter_blocking(&mouse_lock);
    host_dx_accum = 0;
    host_dy_accum = 0;
    host_wheel_accum = 0;
    host_pan_accum = 0;
    host_buttons = 0;
    critical_section_exit(&mouse_lock);
    __atomic_store_n(&last_host_report_us, 0u, __ATOMIC_RELAXED);
}

void tuh_hid_report_received_cb(uint8_t dev_addr, uint8_t instance,
                                uint8_t const *report, uint16_t len) {
    if (tuh_hid_interface_protocol(dev_addr, instance) == HID_ITF_PROTOCOL_MOUSE &&
        report != NULL && len >= 4) {
        __atomic_store_n(&last_host_report_us, time_us_32(), __ATOMIC_RELAXED);
        const hid_mouse_report_t *mouse = (const hid_mouse_report_t *)report;
        critical_section_enter_blocking(&mouse_lock);
        host_dx_accum = clamp_accumulator(host_dx_accum + mouse->x);
        host_dy_accum = clamp_accumulator(host_dy_accum + mouse->y);
        host_wheel_accum = clamp_accumulator(host_wheel_accum + mouse->wheel);
        if (len >= 5) host_pan_accum = clamp_accumulator(host_pan_accum + mouse->pan);
        host_buttons = (uint8_t)(mouse->buttons & 0x07);
        critical_section_exit(&mouse_lock);
    }

    (void)tuh_hid_receive_report(dev_addr, instance);
}

int main(void) {
    set_sys_clock_khz(120000, true);
    sleep_ms(10);

    critical_section_init(&mouse_lock);
    ws2812_init();
    multicore_reset_core1();
    multicore_launch_core1(host_core1_main);

    tud_init(0);

    while (true) {
        tud_task();
        poll_serial_commands();
        service_mouse_reports();
        service_usb_activity_led();
        tight_loop_contents();
    }

    return 0;
}

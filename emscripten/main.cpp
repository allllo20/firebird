#include <errno.h>

#include <emscripten.h>

#include "core/mmu.h"
#include "core/debug.h"
#include "core/emu.h"
#include "core/lcd.h"
#include "core/casplus.h"

void gui_do_stuff(bool wait)
{
}

void do_stuff(int i)
{
}

void gui_debug_printf(const char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);

    gui_debug_vprintf(fmt, ap);

    va_end(ap);
}

void gui_debug_vprintf(const char *fmt, va_list ap)
{
    vprintf(fmt, ap);
}

void gui_status_printf(const char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);

    gui_debug_vprintf(fmt, ap);

    va_end(ap);
}

void gui_perror(const char *msg)
{
    gui_debug_printf("%s: %s\n", msg, strerror(errno));
}

void gui_debugger_entered_or_left(bool entered) {}

void gui_debugger_request_input(debug_input_cb callback)
{
    if(!callback) return;
    char debug_in[20];
    fgets(debug_in, 20, stdin);
    callback(debug_in);
}

void gui_putchar(char c) { putc(c, stdout); }
int gui_getchar() { return -1; }
void gui_set_busy(bool busy) {}
void gui_show_speed(double d) {}
void gui_usblink_changed(bool state) {}
void throttle_timer_off() {}
void throttle_timer_on() {}
void throttle_timer_wait(unsigned int usec) {}

static inline uint32_t rgba_from_565(uint16_t rgb565)
{
    return (((rgb565 & 0xF800) >> 8) | ((rgb565 & 0x07E0) << 5) | ((rgb565 & 0x001F) << 19))
            | 0xFF000000;
}

extern "C" void EMSCRIPTEN_KEEPALIVE paintLCD(uint32_t *dest)
{
    static uint16_t lcdBuffer16[320 * 240];
    static uint8_t lcdBuffer4bpp[240 * 160];

    if (emulate_cx)
    {
        lcd_cx_draw_frame(lcdBuffer16);
        uint16_t *in = lcdBuffer16;
        for (int i = 320 * 240; i--;)
            *dest++ = rgba_from_565(*in++);
        return;
    }

    if (emulate_casplus)
        casplus_lcd_draw_frame((uint8_t (*)[160])lcdBuffer4bpp);
    else
        lcd_draw_frame(lcdBuffer4bpp);

    for (int row = 0; row < 240; ++row)
    {
        uint8_t *in = lcdBuffer4bpp + row * 160;
        for (int x = 0; x < 160; ++x)
        {
            uint8_t packed = *in++;

            uint8_t hi = packed >> 4;
            uint8_t lo = packed & 0x0F;

            // Non-CX framebuffers are 4bpp indexed values; map linearly to grayscale.
            uint8_t g1 = (uint8_t)(hi * 17);
            uint8_t g2 = (uint8_t)(lo * 17);

            *dest++ = 0xFF000000 | (g1 << 16) | (g1 << 8) | g1;
            *dest++ = 0xFF000000 | (g2 << 16) | (g2 << 8) | g2;
        }
    }
}

// For some reason, an extra useless argument has to be used here in
// order to trigger a dynCall_vi codegen instead of an inexistent dynCall_v
void step(void*)
{
    int i = 1000;
    while(i--)
    {
        sched_process_pending_events();
        while (!exiting && cycle_count_delta < 0)
        {
            if (cpu_events & (EVENT_FIQ | EVENT_IRQ)) {
                // Align PC in case the interrupt occurred immediately after a jump
                if (arm.cpsr_low28 & 0x20)
                    arm.reg[15] &= ~1;
                else
                    arm.reg[15] &= ~3;

                if (cpu_events & EVENT_WAITING)
                    arm.reg[15] += 4; // Skip over wait instruction

                arm.reg[15] += 4;
                cpu_exception((cpu_events & EVENT_FIQ) ? EX_FIQ : EX_IRQ);
            }
            cpu_events &= ~EVENT_WAITING;

            if (arm.cpsr_low28 & 0x20)
                cpu_thumb_loop();
            else
                cpu_arm_loop();
        }
    }
}

void emscripten_loop(bool reset)
{
    if(reset)
    {
        memset(mem_areas[1].ptr, 0, mem_areas[1].size);

        memset(&arm, 0, sizeof arm);
        arm.control = 0x00050078;
        arm.cpsr_low28 = MODE_SVC | 0xC0;
        cpu_events &= EVENT_DEBUG_STEP;

        sched_reset();
        sched.items[SCHED_THROTTLE].clock = CLOCK_27M;
        sched.items[SCHED_THROTTLE].proc = do_stuff;

        memory_reset();
    }

    addr_cache_flush();

    sched_update_next_event(0);

    exiting = false;

    emscripten_set_main_loop_arg(step, nullptr, 0, 1);
    return;
}

int main()
{
    path_boot1 = "boot1.img";
    path_flash = "flash.img";

    if(!emu_start(0, 0, NULL))
        return 1;

    EM_ASM(initLCD());

    turbo_mode = true;
    emscripten_loop(true);

    return 0;
}

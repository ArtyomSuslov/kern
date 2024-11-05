/* See COPYRIGHT for copyright information. */

#include <inc/x86.h>
#include <kern/kclock.h>
#include <kern/timer.h>
#include <kern/trap.h>
#include <kern/picirq.h>

/* HINT: Note that selected CMOS
 * register is reset to the first one
 * after first access, i.e. it needs to be selected
 * on every access.
 *
 * Don't forget to disable NMI for the time of
 * operation (look up for the appropriate constant in kern/kclock.h)
 * NOTE: CMOS_CMD is the same port that is used to toggle NMIs,
 * so nmi_disable() cannot be used. And you have to use provided
 * constant.
 *
 * Why it is necessary?
 */

uint8_t
cmos_read8(uint8_t reg) {
    /* MC146818A controller */
    // LAB 4: Your code here

    uint8_t res = 0;

    // Устанавливаем нужный регистр CMOS с отключением NMI
    outb(CMOS_CMD, reg | CMOS_NMI_LOCK);

    // Читаем данные из выбранного регистра
    res = inb(CMOS_DATA);

    // Включаем NMI обратно
    nmi_enable();
    return res;
}

void
cmos_write8(uint8_t reg, uint8_t value) {
    // LAB 4: Your code here

    // Выбор регистра для записи с отключением NMI
    outb(CMOS_CMD, reg | CMOS_NMI_LOCK);

    // Запись данных в выбранный регистр CMOS
    outb(CMOS_DATA, value);

    // Включение NMI обратно
    nmi_enable();
}

uint16_t
cmos_read16(uint8_t reg) {
    return cmos_read8(reg) | (cmos_read8(reg + 1) << 8);
}

static void
rtc_timer_pic_interrupt(void) {
    // LAB 4: Your code here
    // Enable PIC interrupts.
    pic_irq_unmask(IRQ_CLOCK);
}

static void
rtc_timer_pic_handle(void) {
    rtc_check_status();
    pic_send_eoi(IRQ_CLOCK);
}

struct Timer timer_rtc = {
        .timer_name = "rtc",
        .timer_init = rtc_timer_init,
        .enable_interrupts = rtc_timer_pic_interrupt,
        .handle_interrupts = rtc_timer_pic_handle,
};

void
rtc_timer_init(void) {
    // LAB 4: Your code here
    // (use cmos_read8()/cmos_write8())

    // Настройка регистра B для разрешения прерываний
    uint8_t b_reg = cmos_read8(RTC_BREG);
    cmos_write8(RTC_BREG, b_reg | RTC_PIE);

    // Настройка регистра A для изменения частоты прерываний
    uint8_t a_reg = cmos_read8(RTC_AREG);
    cmos_write8(RTC_AREG, RTC_SET_NEW_RATE(a_reg, RTC_500MS_RATE));
}

uint8_t
rtc_check_status(void) {
    // LAB 4: Your code here
    // (use cmos_read8())
    return cmos_read8(RTC_CREG);
}

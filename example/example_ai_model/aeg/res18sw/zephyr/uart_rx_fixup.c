/* Copyright (C) 2026 Advanced Micro Devices, Inc.
 * SPDX-License-Identifier: Apache-2.0
 *
 * SBSA PL011 console RX bring-up for VEK280 (Versal AI Edge, UART0 @0xFF000000).
 *
 * Zephyr's pl011 driver performs no initialization in SBSA mode: it assumes
 * firmware already configured the UART (every RX/UARTEN/FIFO step is gated on
 * !sbsa). On this platform the PLM leaves TX usable but the receiver does not
 * deliver bytes, so printf() works while RX stays dead (FR.RXFE always set).
 *
 * This SYS_INIT brings the receiver up once at boot:
 *   1. enable UARTEN|TXE|RXE + FIFO, clear sticky RSR errors;
 *   2. cycle through internal loopback once ("RX prime") - empirically what makes
 *      the external receiver start latching;
 *   3. fix GICv3 secure SPI routing (see below), then wire interrupt-driven RX:
 *      an ISR drains the FIFO into a queue read by abr_console_rx_getc() (the
 *      app's menu reader). No busy-poll.
 *
 * GICv3 ARE_S fix: Zephyr's gicv3 init runs secure/EL3 here but its secure
 * branch enables Group1S without first setting GICD_CTLR.ARE_S, so the
 * distributor stays in legacy layout - EnableGrp1S is dropped (GICD_CTLR reads
 * 0) and SPI IROUTER routing is inert. PPIs (timer) still work via the
 * redistributor, but SPIs (the UART) are never forwarded to the A72. We set
 * ARE_S + EnableGrp1S directly (GICD is identity-mapped) and re-enable the UART
 * IRQ so its IROUTER is rewritten with routing now active. Kept local instead of
 * patching the Zephyr driver. See UART_RX_DEBUG.md.
 *
 * Lives in the app tree (not the upstream driver) so `west update` can't clobber
 * it; candidate to move into the board/driver in the AMD Zephyr tree.
 */

#include <errno.h>
#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/init.h>
#include <zephyr/irq.h>
#include <zephyr/drivers/uart.h>
#include <zephyr/sys/device_mmio.h>

#if DT_HAS_CHOSEN(zephyr_console)

/* PL011 register offsets */
#define PL011_DR   0x00
#define PL011_RSR  0x04
#define PL011_FR   0x18
#define PL011_LCRH 0x2c
#define PL011_CR   0x30
#define PL011_IMSC 0x38

/* FR / CR / LCRH bits */
#define FR_BUSY    (1u << 3)
#define FR_RXFE    (1u << 4)
#define CR_UARTEN  (1u << 0)
#define CR_LBE     (1u << 7)
#define CR_TXE     (1u << 8)
#define CR_RXE     (1u << 9)
#define CR_DTR     (1u << 10)
#define CR_RTS     (1u << 11)
#define LCRH_FEN   (1u << 4)

/* GICv3 distributor (identity-mapped). ARE_S = affinity routing enable (secure),
 * EN_G1S = enable Group 1 secure interrupts. */
#define GICD_CTLR_ADDR  0xF9000000UL
#define GICD_CTLR_ARE_S (1u << 4)
#define GICD_CTLR_ENG1S (1u << 2)

/* Interrupt-fed RX queue drained by abr_console_rx_getc(). */
K_MSGQ_DEFINE(abr_console_rx_q, 1, 256, 1);

#ifdef CONFIG_UART_INTERRUPT_DRIVEN
static void abr_console_rx_cb(const struct device *dev, void *user_data)
{
	ARG_UNUSED(user_data);
	uint8_t b;

	while (uart_irq_update(dev) && uart_irq_rx_ready(dev)) {
		while (uart_fifo_read(dev, &b, 1) == 1) {
			(void)k_msgq_put(&abr_console_rx_q, &b, K_NO_WAIT); /* drop if full */
		}
	}
}
#endif

/* Block for one byte from the console RX queue. Returns 0..255, or -1 on
 * timeout (timeout_ms < 0 waits forever). C linkage for the C++ menu reader.
 */
int abr_console_rx_getc(int timeout_ms)
{
	uint8_t b;
	k_timeout_t to = (timeout_ms < 0) ? K_FOREVER : K_MSEC(timeout_ms);

	if (k_msgq_get(&abr_console_rx_q, &b, to) != 0) {
		return -1;
	}
	return (int)b;
}

static int versal_sbsa_uart_rx_init(void)
{
	const struct device *dev = DEVICE_DT_GET(DT_CHOSEN(zephyr_console));

	if (!device_is_ready(dev)) {
		return -ENODEV;
	}

	/* UART device init (PRE_KERNEL_1) has already mapped the MMIO region. */
	volatile uint8_t *regs = (volatile uint8_t *)DEVICE_MMIO_GET(dev);
	volatile uint32_t *cr   = (volatile uint32_t *)(regs + PL011_CR);
	volatile uint32_t *fr   = (volatile uint32_t *)(regs + PL011_FR);
	volatile uint32_t *lcrh = (volatile uint32_t *)(regs + PL011_LCRH);
	volatile uint32_t *dr   = (volatile uint32_t *)(regs + PL011_DR);
	int spin;

	/* Enable receiver: PL011 wants UARTEN clear while reprogramming CR. */
	while (*fr & FR_BUSY) {
	}
	*cr &= ~CR_UARTEN;
	*lcrh |= LCRH_FEN;
	*cr |= CR_TXE | CR_RXE | CR_DTR | CR_RTS;
	*cr |= CR_UARTEN;
	*(volatile uint32_t *)(regs + PL011_RSR) = 0;   /* clear sticky RX errors */
	*(volatile uint32_t *)(regs + PL011_IMSC) = 0;  /* mask irqs until RX is wired */

	/* RX prime: enter loopback, send a byte internally, exit loopback. */
	spin = 0;
	while ((*fr & FR_BUSY) && spin++ < 100000) {
	}
	*cr &= ~CR_UARTEN;
	*cr |= CR_LBE;
	*cr |= CR_UARTEN;
	for (int i = 0; i < 64 && !(*fr & FR_RXFE); i++) {
		(void)*dr;
	}
	*dr = 0x5a;
	int lb_spin = 0;
	while ((*fr & FR_RXFE) && lb_spin++ < 2000000) {
	}
	int lb_got = !(*fr & FR_RXFE);
	spin = 0;
	while ((*fr & FR_BUSY) && spin++ < 100000) {
	}
	*cr &= ~CR_UARTEN;
	*cr &= ~CR_LBE;
	*cr |= CR_UARTEN;
	*(volatile uint32_t *)(regs + PL011_RSR) = 0;
	for (int i = 0; i < 64 && !(*fr & FR_RXFE); i++) {
		(void)*dr;
	}

	if (!lb_got) {
		printk("[uart] WARNING: SBSA RX prime self-test failed; "
		       "console input may not work\n");
	}

#ifdef CONFIG_UART_INTERRUPT_DRIVEN
	/* Enable secure affinity routing the upstream gicv3 init omits at EL3, so
	 * the UART SPI is forwarded to the A72. ARE_S must be set before group
	 * enable or EnableGrp1S is dropped. The UART driver already enabled the IRQ
	 * (PRE_KERNEL_1) while ARE_S was 0, so its IROUTER write was inert - toggle
	 * the IRQ to rewrite it now that routing is active.
	 */
	volatile uint32_t *gicd_ctlr = (volatile uint32_t *)GICD_CTLR_ADDR;
	*gicd_ctlr |= GICD_CTLR_ARE_S;
	*gicd_ctlr |= GICD_CTLR_ENG1S;
	unsigned int uart_irqn = DT_IRQN(DT_CHOSEN(zephyr_console));
	irq_disable(uart_irqn);
	irq_enable(uart_irqn);

	/* Wire interrupt-driven RX: the ISR drains bytes into abr_console_rx_q.
	 * uart_irq_rx_enable() sets the RX/RX-timeout bits in IMSC.
	 */
	uart_irq_rx_disable(dev);
	uart_irq_tx_disable(dev);
	uart_irq_callback_user_data_set(dev, abr_console_rx_cb, NULL);
	uart_irq_rx_enable(dev);
#else
#warning "CONFIG_UART_INTERRUPT_DRIVEN not set; console RX queue stays empty"
#endif

	return 0;
}

/* After the UART device (PRE_KERNEL_1), before the application starts. */
SYS_INIT(versal_sbsa_uart_rx_init, POST_KERNEL, CONFIG_KERNEL_INIT_PRIORITY_DEVICE);

#endif /* DT_HAS_CHOSEN(zephyr_console) */

//======================================================================
//
// application_fpga_verilator.cc
// -----------------------------
// Wrapper to allow simulation of the application_fpga using Verilator.
//
//
// SPDX-FileCopyrightText: 2022 Tillitis AB <tillitis.se>
// SPDX-License-Identifier: BSD-2-Clause
//
//======================================================================

#include <pty.h>
#include <termios.h>
#include <unistd.h>
#include <stdio.h>
#include <fcntl.h>
#include <poll.h>
#include <stdint.h>
#include <string.h>
#include <signal.h>
#include <sys/types.h>

#include "Vapplication_fpga_sim.h"
#include "verilated.h"

// Clock: 21 MHz, 62500 bps
// Divisor = 21E6 / 62500 = 336
#define CPU_CLOCK 21000000
#define BAUD_RATE 62500
#define BIT_DIV (CPU_CLOCK/BAUD_RATE)


struct uart {
	int bit_div;
	unsigned int ts;

	unsigned int tx_next_ts;
	unsigned int rx_next_ts;

	int rx_state; /* 0, (idle), 1 (start), 2..9 (data), 10 (stop), 11 (stop end) */
	int tx_state; /* 0, (idle), 1..8 (data), 9 (stop), 10 (stop end) */
	uint8_t tx_data;
	uint8_t rx_data;
	int rx_has_data;
	int tx_has_data;
	uint8_t *tx;
	uint8_t *rx;
};

void uart_init(struct uart *u,	uint8_t *tx, uint8_t *rx, int bit_div);
void uart_tick(struct uart *u);
int uart_can_send(struct uart *u);
int uart_send(struct uart *u, uint8_t data);
int uart_recv(struct uart *u, uint8_t *data);

void uart_init(struct uart *u,	uint8_t *tx, uint8_t *rx, int bit_div)
{
	u->bit_div = bit_div;
	u->ts = 0;
	u->tx_next_ts = 0;
	u->rx_next_ts = 0;
	u->rx_state = 0;
	u->tx_state = 0;
	u->tx_data = 0;
	u->rx_data = 0;
	u->rx_has_data = 0;
	u->tx_has_data = 0;
	u->tx = tx;
	*u->tx = 1;
	u->rx = rx;
}

void uart_rx_tick(struct uart *u)
{
	if (u->rx_state == 0) {
		// Idle
		if (!u->rx_has_data && !*u->rx) { // Active low
			u->rx_next_ts = u->ts + u->bit_div / 2; // sample mid-point
			u->rx_state = 1;
		}
		return;
	}

	if (u->rx_state == 1) {
		// Start
		if (u->ts < u->rx_next_ts)
			return;

		if (*u->rx) {
			u->rx_state = 0; // Back to idle, shouldn't happen
			return;
		}

		u->rx_next_ts += u->bit_div;
		u->rx_data = 0;
		u->rx_state = 2;
		return;
	}

	if (u->rx_state > 1 && u->rx_state < 10) {
		// Data
		if (u->ts < u->rx_next_ts)
			return;

		u->rx_next_ts += u->bit_div;
		u->rx_data |= (!!*u->rx) << (u->rx_state - 2);
		u->rx_state++;
		return;
	}

	if (u->rx_state == 10) {
		// Stop
		if (u->ts < u->rx_next_ts)
			return;

		if (!*u->rx) {
			u->rx_state = 0; // Back to dle, shouldn't happen
			return;
		}

		u->rx_next_ts += u->bit_div/2;
		u->rx_has_data = 1;
		u->rx_state = 11;
		return;
	}

	if (u->rx_state == 11) {
		if (u->ts < u->rx_next_ts)
			return;

		u->rx_state = 0;
		return;
	}
}

int uart_recv(struct uart *u, uint8_t *data)
{
	if (u->rx_has_data && (u->rx_state == 0 || u->rx_state == 11)) {
		*data = u->rx_data;
		u->rx_has_data = 0;
		return 1;
	}
	return 0;
}

void uart_tx_tick(struct uart *u)
{
	if (u->tx_state == 0) {
		// Idle
		if (u->tx_has_data) {
			u->tx_next_ts = u->ts + u->bit_div;
			*u->tx = 0; // Start
			u->tx_state = 1;
		}
		return;
	}

	if (u->tx_state > 0 && u->tx_state < 9) {
		// Data
		if (u->ts < u->tx_next_ts)
			return;

		u->tx_next_ts += u->bit_div;
		*u->tx = (u->tx_data >> (u->tx_state - 1)) & 1;
		u->tx_state++;
		return;
	}

	if (u->tx_state == 9) {
		// Stop
		if (u->ts < u->tx_next_ts)
			return;

		u->tx_next_ts += u->bit_div;
		*u->tx = 1; // Stop
		u->tx_has_data = 0;
		u->tx_state = 10;
		return;
	}

	if (u->tx_state == 10) {
		if (u->ts < u->tx_next_ts)
			return;

		u->tx_state = 0;
		return;
	}
}

int uart_can_send(struct uart *u)
{
	return !u->tx_has_data;
}

int uart_send(struct uart *u, uint8_t data)
{
	if (!uart_can_send(u))
		return 0;

	u->tx_has_data = 1;
	u->tx_data = data;
	return 1;
}

void uart_tick(struct uart *u)
{
	u->ts++;

	uart_rx_tick(u);
	uart_tx_tick(u);
}

struct pty {
	int amaster;
	int aslave;
	char slave[32];
};

int pty_init(struct pty *p);
int pty_can_recv(struct pty *p);
int pty_recv(struct pty *p, uint8_t *data);
void pty_send(struct pty *p, uint8_t data);

int pty_init(struct pty *p)
{
	struct termios tty;
	int flags;

	memset(p, 0, sizeof(*p));

	if (openpty(&p->amaster, &p->aslave, p->slave, NULL, NULL) < 0)
		return -1;

	if (tcgetattr(p->aslave, &tty) < 0)
		return -1;
	cfmakeraw(&tty);
	if (tcsetattr(p->aslave, TCSAFLUSH, &tty) < 0)
		return -1;

	if ((flags = fcntl(p->amaster, F_GETFL, 0) < 0))
		return -1;

	flags |= O_NONBLOCK;
	if (fcntl(p->amaster, F_SETFL, flags) < 0)
		return -1;

	printf("pty: %s\n", p->slave);
	return 0;
}

int pty_can_recv(struct pty *p)
{
	struct pollfd fds = {p->amaster, POLLIN, 0};
	char c;

	return poll(&fds, 1, 0) == 1;
}

int pty_recv(struct pty *p, uint8_t *data)
{
	return read(p->amaster, data, 1) == 1;
}

void pty_send(struct pty *p, uint8_t data)
{
	ssize_t i __attribute__((unused));

	i = write(p->amaster, &data, 1);
}

volatile int touch_cyc = 0;

void sighandler(int)
{
	printf("touched!\n");
	touch_cyc = 1000;
}

void touch(uint8_t *touch_event)
{
	if (touch_cyc > 0) {
		touch_cyc--;
		*touch_event = 1;
	} else {
		*touch_event = 0;
	}
}

vluint64_t main_time = 0;
double sc_time_stamp()
{
	return main_time;
}

int main(int argc, char **argv, char **env)
{
	Verilated::commandArgs(argc, argv);
	int r = 0, g = 0, b = 0;
	Vapplication_fpga_sim top;
	struct uart u;
	struct pty p;
	int err;

	if (signal(SIGUSR1, sighandler) == SIG_ERR)
		return -1;
	printf("cpu clock: %d\n", CPU_CLOCK);
	printf("baud rate: %d\n", BAUD_RATE);
	printf("generate touch event: \"$ kill -USR1 %d\"\n", (int)getpid());

	err = pty_init(&p);
	if (err)
		return -1;

	uart_init(&u, &top.interface_tx, &top.interface_rx, BIT_DIV);

	top.clk = 0;
	top.interface_ch552_cts = 1;

	while (!Verilated::gotFinish()) {
		uint8_t to_host = 0;

		top.clk = !top.clk;

		if (main_time < 10)
			goto skip;

		if (!top.clk) {
			touch(&top.touch_event);
			uart_tick(&u);
		}

		if (pty_can_recv(&p) && uart_can_send(&u)) {
			uint8_t from_host = 0;

			pty_recv(&p, &from_host);
			uart_send(&u, from_host);
		}

		if (uart_recv(&u, &to_host) == 1) {
			pty_send(&p, to_host);
		}
	skip:
		main_time++;
		top.eval();

	}
}

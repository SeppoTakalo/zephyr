/* Copyright (c) 2022 Google, LLC.
 * SPDX-License-Identifier: Apache-2.0
 */
#include <zephyr/kernel.h>
#include <string.h>
#include <zephyr/irq.h>
#include <zephyr/toolchain.h>
#include <irq_ctrl.h>
#if defined(CONFIG_BOARD_NATIVE_SIM)
#include <nsi_cpu_if.h>
#include <nsi_main_semipublic.h>
#else
#error "Platform not supported"
#endif

#include <zephyr/modem/cmux.h>
#include <zephyr/modem/pipe.h>
#include <zephyr/sys/crc.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(fuzz, CONFIG_MODEM_CMUX_LOG_LEVEL);

/* Fuzz input received from LLVM via "interrupt" */
static const uint8_t *fuzz_buf;
static size_t fuzz_sz;


int pipe_api_open(void *data)
{
	struct modem_pipe *pipe = (struct modem_pipe *)data;

	modem_pipe_notify_opened(pipe);
	return 0;
}

int pipe_api_transmit(void *data, const uint8_t *buf, size_t size)
{
	struct modem_pipe *pipe = (struct modem_pipe *)data;

	modem_pipe_notify_transmit_idle(pipe);
	return size;
}

int pipe_api_receive(void *data, uint8_t *buf, size_t size)
{
	struct modem_pipe *pipe = (struct modem_pipe *)data;
	size_t len = size > fuzz_sz ? fuzz_sz : size;

	memcpy(buf, fuzz_buf, len);
	fuzz_sz -= len;
	fuzz_buf += len;

	if (fuzz_sz) {
		modem_pipe_notify_receive_ready(pipe);
	}
	return len;
}

int pipe_api_close(void *data)
{
	struct modem_pipe *pipe = (struct modem_pipe *)data;

	modem_pipe_notify_closed(pipe);
	return 0;
}


struct modem_pipe_api pipe_api = {
	.open = pipe_api_open,
	.transmit = pipe_api_transmit,
	.receive = pipe_api_receive,
	.close = pipe_api_close,
};

void cmux_callback(struct modem_cmux *cmux, enum modem_cmux_event event,
				    void *user_data)
{
	LOG_DBG("cmux_callback: %d", event);	
}

static struct modem_pipe *cmux_setup(void)
{
	static uint8_t rx_buf[MODEM_CMUX_WORK_BUFFER_SIZE];
	static uint8_t tx_buf[MODEM_CMUX_WORK_BUFFER_SIZE];
	static struct modem_pipe pipe;
	static struct modem_cmux cmux; 

	struct modem_cmux_config cmux_config = {
		.callback = cmux_callback,
		.user_data = NULL,
		.receive_buf = rx_buf,
		.receive_buf_size = sizeof(rx_buf),
		.transmit_buf = tx_buf,
		.transmit_buf_size = sizeof(tx_buf),
	};

	modem_cmux_init(&cmux, &cmux_config);
	modem_pipe_init(&pipe, &pipe, &pipe_api);
	modem_pipe_open(&pipe, K_SECONDS(1));
	modem_cmux_attach(&cmux, &pipe);
	cmux.state = MODEM_CMUX_STATE_CONNECTED;
	LOG_DBG("test_modem_cmux_setup(): DONE");
	return &pipe;
}


K_SEM_DEFINE(fuzz_sem, 0, 1);

static void fuzz_isr(const void *arg)
{
	k_sem_give(&fuzz_sem);
}

int main(void)
{
	printk("Hello World! %s\n", CONFIG_BOARD);

	IRQ_CONNECT(CONFIG_ARCH_POSIX_FUZZ_IRQ, 0, fuzz_isr, NULL, 0);
	irq_enable(CONFIG_ARCH_POSIX_FUZZ_IRQ);

	struct modem_pipe *pipe = cmux_setup();

	printk("Start running!\n");

	while (true) {
		k_sem_take(&fuzz_sem, K_FOREVER);
		modem_pipe_notify_receive_ready(pipe);
	}
	return 0;
}

/**
 * Entry point for fuzzing. Works by placing the data
 * into two known symbols, triggering an app-visible interrupt, and
 * then letting the simulator run for a fixed amount of time (intended to be
 * "long enough" to handle the event and reach a quiescent state
 * again)
 */
#if defined(CONFIG_BOARD_NATIVE_SIM)
NATIVE_SIMULATOR_IF /* We expose this function to the final runner link stage*/
#endif
int LLVMFuzzerTestOneInput(const uint8_t *data, size_t sz)
{
	static bool runner_initialized;

	if (!runner_initialized) {
		nsi_init(0, NULL);
		runner_initialized = true;
	}

	/* Provide the fuzz data to the embedded OS as an interrupt, with
	 * "DMA-like" data placed into native_fuzz_buf/sz
	 */
	fuzz_buf = (void *)data;
	fuzz_sz = sz;

	hw_irq_ctrl_set_irq(CONFIG_ARCH_POSIX_FUZZ_IRQ);

	/* Give the OS time to process whatever happened in that
	 * interrupt and reach an idle state.
	 */
	while (fuzz_sz) {
		nsi_exec_for(k_ticks_to_us_ceil64(CONFIG_ARCH_POSIX_FUZZ_TICKS));
	}

	return 0;
}

extern size_t
LLVMFuzzerMutate(uint8_t *Data, size_t Size, size_t MaxSize);

#define MODEM_CMUX_SOF				(0xF9)
#define MODEM_CMUX_FCS_POLYNOMIAL	(0xE0)
#define MODEM_CMUX_FCS_INIT_VALUE	(0xFF)
#define MODEM_CMUX_EA				(0x01)
#define MODEM_CMUX_FRAME_TYPE_UIH   (0xEF)

#ifdef CONFIG_CUSTOM_MUTATOR

NATIVE_SIMULATOR_IF
size_t LLVMFuzzerCustomMutator(uint8_t *Data, size_t Size,
                                          size_t MaxSize, unsigned int Seed) {
	size_t len = Size;
	size_t max = MaxSize;

	if (len < MODEM_CMUX_HEADER_SIZE) {
		len = MODEM_CMUX_HEADER_SIZE;
	}
	if (max > (CONFIG_MODEM_CMUX_MTU + MODEM_CMUX_HEADER_SIZE)) {
		max = CONFIG_MODEM_CMUX_MTU + MODEM_CMUX_HEADER_SIZE;
	}
	if (max < MODEM_CMUX_HEADER_SIZE) {
		return 0;
	}

	len = LLVMFuzzerMutate(Data, len, max);
	if (len < MODEM_CMUX_HEADER_SIZE) {
		len = MODEM_CMUX_HEADER_SIZE;
	}
	/* Fix the random input to look like a correct CMUX frame
	 * by adding the flags, length and correct CRC
	 *
	 * |0xF9|ADDR|CTRL|LEN| (I) |FCS|0xF9|
	 */
	Data[0] = MODEM_CMUX_SOF;
	bool uih = (Data[2] & MODEM_CMUX_FRAME_TYPE_UIH) == MODEM_CMUX_FRAME_TYPE_UIH;
	len = uih ? len : MODEM_CMUX_HEADER_SIZE;
	Data[3] = ((len - MODEM_CMUX_HEADER_SIZE) << 1) | MODEM_CMUX_EA;
	uint8_t fcs = 0xFF - crc8_rohc(MODEM_CMUX_FCS_INIT_VALUE, &Data[1], 3);
	Data[len-2] = fcs;
	Data[len-1] = MODEM_CMUX_SOF;

	if (MaxSize - len > MODEM_CMUX_HEADER_SIZE) {
		return len + LLVMFuzzerCustomMutator(Data + len, 1, MaxSize - len, Seed);
	}

	return len;
}

#endif

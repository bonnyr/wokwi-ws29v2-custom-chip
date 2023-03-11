// Wokwi ws29v2 Custom Chip - For information and examples see:
// https://link.wokwi.com/custom-chips-alpha
//
// SPDX-License-Identifier: MIT
// Copyright (C) 2022 Bonny Rais

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <wchar.h>

#include "ws29v2.h"

// tmp fwds
void on_spi_done_2(void *user_data, uint8_t *buffer, uint32_t count);


void on_timer_event(void *data) {
    ws29v2_ctx_t *chip = (ws29v2_ctx_t *)data;
    SPI_DEBUGF("on_timer_event\n");
}


void on_cs_pin_change(void *user_data, pin_t pin, uint32_t value) {
    ws29v2_ctx_t *chip = (ws29v2_ctx_t *)user_data;
    SPI_PIN_DEBUGF("on_cs_pin_change: pin: %d - value: %d %s\n", pin, value, value ? "⮥" : "⮧" );

    // Handle CS pin logic
    if (pin != chip->cs) {
        return; // TBD
    }

    if (value == LOW) {
        SPI_PIN_DEBUGF("SPI chip selected, starting transaction\n");
        chip->buffer[0] = 0;  // dummy data - should be ignored by MCU
        start_spi_transfer(chip, chip->spi_state, 1);
        return;
    }

    SPI_PIN_DEBUGF("SPI chip deselected - transaction finished\n");
    // since the driver code seems to insist on sending one byte at a time, we rely on the framework to stop the SPI at end of txfer
    // spi_stop(chip->spi);
}

void on_dc_pin_change(void *user_data, pin_t pin, uint32_t value) {
    ws29v2_ctx_t *chip = (ws29v2_ctx_t *)user_data;
    SPI_PIN_DEBUGF("on_dc_pin_change: pin: %d - value: %d %s\n", pin, value, value ? "⮥" : "⮧" );

    chip->mode = value;
}


void chip_reset(ws29v2_ctx_t *chip) {
    GEN_DEBUGF("*** ws29v2 chip resetting...\n");
    pin_write(chip->busy, HIGH);

    chip->cmd = 0;
    chip->data_ndx = 0;

    chip->addr_incr_mode = ADDR_INCR_MODE_POR;
    chip->x_addr = 0;
    chip->y_addr = 0;

    chip->x_min = chip->x_max = 0;
    chip->y_min = chip->y_max = 0;

    chip->disp_upd_seq = DISP_UPDATE_SEQ_POR;

    memset(chip->bw_ram, 0, sizeof(chip->bw_ram));
    memset(chip->red_ram, 0, sizeof(chip->red_ram));

    chip->spi_state = ST_SPI_WAIT_CMD;
    chip->mode = pin_read(chip->dc);

    GEN_DEBUGF("*** ws29v2 reset pin value: %d\n", pin_read(chip->reset));
    chip->reset_time = get_sim_nanos();

    chip->sleeping = false;

    chip->act_ndx = 0;
    chip->act_scan_ndx = 0;
    
    buffer_write(chip->frame_buf, 0, (uint8_t*)chip->black, sizeof(uint32_t) * chip->width * chip->height);

    pin_write(chip->busy, LOW);
    GEN_DEBUGF("*** ws29v2 chip reset done @%lld\n", chip->reset_time);
}

static void chip_init_attrs(ws29v2_ctx_t *chip) {
    uint32_t attr;

    attr = attr_init("debug", 0);
    chip->debug = attr_read(attr) != 0;
    attr = attr_init("debug_mask", 0xFF);
    chip->debug_mask = attr_read(attr);

    attr = attr_init("version", 1);
    chip->version = constrain(attr_read(attr), 1, 2);
    attr = attr_init("act_mode", 0);
    chip->act_mode = constrain(attr_read(attr), 0, 2);

    printf("*** ws29v2 chip attributes\n debug: %d\n debug_mask: %d\n version: %d",
           chip->debug, chip->debug_mask, chip->version);
}

uint32_t *alloc_and_init_fb(size_t sz, uint32_t val) {
    uint32_t *p = malloc(2 * sz * sizeof(uint32_t));
    for (size_t i = 0; i < sz; i++) { *p++ = val;}
    return p;
 }

void chip_init() {
    printf("*** ws29v2 chip initialising...\n");
    ws29v2_ctx_t *chip = calloc(1, sizeof(ws29v2_ctx_t));

    // initialise state machine
    cmd_init_hash();

    // initialise sim attributes
    chip_init_attrs(chip);


    // initialise and check pins
    chip->busy = pin_init( "BUSY", OUTPUT);
    chip->reset = pin_init( "RST", INPUT);
    chip->dc = pin_init( "DC", INPUT);
    chip->cs = pin_init( "CS", INPUT);
    chip->clk = pin_init( "CLK", INPUT);
    chip->sdi = pin_init( "DIN", INPUT);
    chip->vcc = pin_init( "VCC", INPUT);

    pin_watch_config_t watch_config = { .edge = BOTH, .user_data = chip };

    watch_config.pin_change = on_cs_pin_change;
    pin_watch(chip->cs, &watch_config);

    watch_config.pin_change = on_reset_pin_change;
    pin_watch(chip->reset, &watch_config);

    watch_config.pin_change = on_dc_pin_change;
    pin_watch(chip->dc, &watch_config);

    const spi_config_t spi_cfg = {
        .sck = chip->clk,
        .mosi = chip->sdi,
        .miso = NO_PIN,
        .mode = 0,
        .done = on_spi_done_2,
        .user_data = chip,
    };
    chip->spi = spi_init(&spi_cfg);
    printf("*** ws29v2 chip initialising with SPI config with:\n sck: %d, mosi: %d, cs: %d ...\n",
            chip->clk, chip->sdi, chip->cs);

    timer_config_t timer_cfg = { .user_data = chip, };
    timer_cfg.callback = on_timer_event;
    chip->timer = timer_init(&timer_cfg);

    timer_cfg.callback = on_reset_timer;
    chip->reset_timer = timer_init(&timer_cfg);

    timer_cfg.callback = on_activation_step;
    chip->activation_timer = timer_init(&timer_cfg);

    // display
    chip->frame_buf = framebuffer_init(&chip->width, &chip->height);
    chip->black = alloc_and_init_fb(chip->width * chip->height, FB_BLACK);
    chip->white = alloc_and_init_fb(chip->width * chip->height, FB_WHITE);
    chip->act_seq = activation_modes[ chip->version - 1][chip->act_mode];
    // chip->act_seq_len = chip->version == 1 ? v1_act_seq_len : v2_act_seq_len;
    printf("... framebuf config:\n w: %d, h: %d ...\n", chip->width, chip->height);

    chip_reset(chip);
    printf("*** ws29v2 chip initialised (@%p)...\n", chip);
}





void start_spi_transfer(ws29v2_ctx_t *chip, spi_state_t st, uint32_t count) {
    chip->spi_state = st;
    chip->txfer = true;
    spi_start(chip->spi, chip->buffer, count);
}


void on_spi_command_byte(ws29v2_ctx_t *chip) {
    chip->cmd = chip->buffer[0];

    chip->data_ndx = 0;
    SPI_DEBUGF("on_spi_command_byte: rcvd %02X CMD\n", chip->cmd);
    if (cmd_data_bytes[chip->cmd]) {
        SPI_DEBUGF("on_spi_done: starting wait for command data\n");
        start_spi_transfer(chip, ST_SPI_WAIT_DATA, 1);
        return;
    }
    on_command(chip);
}

void on_spi_done(void *user_data, uint8_t *buffer, uint32_t count) {
    ws29v2_ctx_t *chip = (ws29v2_ctx_t *)user_data;

    // if no bytes transferred or CS has gone high, we're done
    if (count == 0) {  //|| pin_read(chip->cs) == HIGH) {
        SPI_DEBUGF("on_spi_done: no bytes received or transaction finished\n");
        chip->spi_state = ST_SPI_WAIT_CMD;
        return;
    }

    bool dc = pin_read(chip->dc);

    if (chip->spi_state == ST_SPI_WAIT_DATA && ! dc) {
        SPI_DEBUGF("on_spi_done: D/C changed to command state. Processing existing command and data\n");
        on_command(chip);
        chip->data_ndx = 0;
        chip->spi_state = ST_SPI_WAIT_CMD;
    }

    SPI_DEBUGF("on_spi_done: count: %d, state: %d\n", count, chip->spi_state);
    switch (chip->spi_state) {
        case ST_SPI_WAIT_CMD:
            on_spi_command_byte(chip);
            break;

        case ST_SPI_WAIT_DATA:
            SPI_DEBUGF("on_spi_done: transferred %d DATA bytes: (%s)\n", count, debugHexStr(chip->buffer, min(count, 16)));

            chip->data[chip->data_ndx] = chip->buffer[0];
            chip->data_ndx++;

            if (cmd_data_bytes[chip->cmd] == 0xFF || chip->data_ndx < cmd_data_bytes[chip->cmd]) {
                break;
            }

            SPI_DEBUGF("on_spi_done: command 0x%2x data received(%s)\n", chip->cmd, debugHexStr(chip->buffer, min(count, 16)));
            chip->spi_state = ST_SPI_WAIT_CMD;
            on_command(chip);
            chip->data_ndx = 0;
            break;

        case ST_SPI_DONE:
            break;
    }
}


void on_spi_done_2(void *user_data, uint8_t *buffer, uint32_t count) {
    ws29v2_ctx_t *chip = (ws29v2_ctx_t *)user_data;

    // command byte
    if (chip->mode == 0) {
        // if this is a new command byte after a variable string of data, execute the previous command first
        if (chip->data_ndx) {
            on_command(chip);
        }
        on_spi_command_byte(chip);
        return;
    }


    // data byte, add to current buffer
    chip->data[chip->data_ndx] = chip->buffer[0];
    chip->data_ndx++;

    // buffer overflow test
    if (chip->data_ndx >= MAX_DATA_TXFER) {
        SPI_ERRF("on_spi_done: command %02x - data transfer too long (%d), resetting\n", chip->cmd, chip->data_ndx);
        chip_reset(chip);
        return;
    }
    
    // if variable data or we're still expecting more bytes, exit
    if (cmd_data_bytes[chip->cmd] == 0xFF || chip->data_ndx < cmd_data_bytes[chip->cmd]) {
        return;
    }

    // all command data received
    SPI_DEBUGF("on_spi_done: command 0x%2x data received(%s)\n", chip->cmd, debugHexStr(chip->buffer, min(chip->data_ndx, 16)));
    on_command(chip);
}

void on_reset_timer(void *user_data) {
    ws29v2_ctx_t *chip = (ws29v2_ctx_t *)user_data;
    SPI_DEBUGF("on_reset_timer\n");
    chip_reset(chip);
}

void on_reset_pin_change(void *user_data, pin_t pin, uint32_t value) {
    ws29v2_ctx_t *chip = (ws29v2_ctx_t *)user_data;
    PIN_DEBUGF("on_reset_pin_change: %s\n", value ? "⮥" : "⮧" );

    uint64_t now = get_sim_nanos();
    uint64_t diff = now - chip->reset_time;

    // rising edge. if pin was low for at least pulse width, reset
    if (value) {
        PIN_DEBUGF("on_reset_pin_change: rising edge after (%lld)ns\n", diff);
        if (diff < RESET_PULSE_MS) {
            PIN_DEBUGF("on_reset_pin_change: rising edge too quick (%lld). NOT resetting\n", diff);
            return;
        }
        chip_reset(chip);

    // falling edge, start reset timer. if too short, return
    // } else { 
    //     if (diff > RESET_PULSE_MS) {
    //         timer_start(chip->reset_timer, RESET_PULSE_MS, false);
    //     }
    }
}



static void on_cmd_ctrl_write(ws29v2_ctx_t *chip) {
    uint32_t count;
    uint8_t *ptr;

    // get number of command data  bytes expected
    // reset is a one byte command - check it first
    if (chip->cmd == CMD_SW_RESET) {
        GEN_DEBUGF("on_cmd_ctrl_write: resetting chip\n");
        // set_sleep_mode(chip);
        chip->spi_state = ST_SPI_WAIT_CMD;
        return;
    }

    SPI_DEBUGF("on_cmd_ctrl_write: Starting transfer for write data\n");
    start_spi_transfer(chip, ST_SPI_WAIT_DATA, 1);
}


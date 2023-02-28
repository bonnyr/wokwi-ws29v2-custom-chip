// Wokwi ws29v2 Custom Chip - For information and examples see:
// https://link.wokwi.com/custom-chips-alpha
//
// SPDX-License-Identifier: MIT
// Copyright (C) 2022 Bonny Rais

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


#include "ws29v2.h"
#include "hashmap.h"

// tmp fwds
void on_spi_done_2(void *user_data, uint8_t *buffer, uint32_t count);


typedef void (*cmd_handler)(ws29v2_ctx_t *chip);
typedef struct {
    uint16_t cmd_key;
    const char *cmd_name;
    cmd_handler handler;
} cmd_entry_t;
typedef HASHMAP(uint16_t, cmd_entry_t) cmd_map_t;
#define CMD_ENTRY(cmd, h) {cmd, #cmd, h}


cmd_entry_t cmd_entries[] = {
CMD_ENTRY(CMD_DRIVER_OUTPUT_CTL, on_cmd_driver_output_ctl ),
CMD_ENTRY(CMD_GATE_OUTPUT_CTL, on_cmd_gate_output_ctl ),
CMD_ENTRY(CMD_SRC_OUTPUT_CTL, on_cmd_src_output_ctl ),
CMD_ENTRY(CMD_DEEP_SLEEP, on_cmd_deep_sleep ),
CMD_ENTRY(CMD_DATA_ENTRY_MODE, on_cmd_data_entry_mode ),
CMD_ENTRY(CMD_SW_RESET, on_cmd_sw_reset ),
CMD_ENTRY(CMD_MASTER_ACTIVATION, on_cmd_master_activation ),
CMD_ENTRY(CMD_DISP_UPD_CTL, on_cmd_disp_upd_ctl ),
CMD_ENTRY(CMD_DISP_UPD_CTL2, on_cmd_disp_upd_ctl2 ),
CMD_ENTRY(CMD_WRITE_RAM_BW, on_cmd_write_ram_bw ),
CMD_ENTRY(CMD_WRITE_RAM_RED, on_cmd_write_ram_red ),
CMD_ENTRY(CMD_VCOM_SENSE, on_cmd_vcom_sense ),
CMD_ENTRY(CMD_VCOM_SENSE_DUR, on_cmd_vcom_sense_dur ),
CMD_ENTRY(CMD_PROG_VCOM_OTP, on_cmd_prog_vcom_otp ),
CMD_ENTRY(CMD_VCOM_REG_CTL, on_cmd_vcom_reg_ctl ),
CMD_ENTRY(CMD_WRITE_VCOM_REG, on_cmd_write_vcom_reg ),
CMD_ENTRY(CMD_READ_OTP_REG, on_cmd_read_otp_reg ),
CMD_ENTRY(CMD_READ_USER_ID, on_cmd_read_user_id ),
CMD_ENTRY(CMD_PROG_WS_OTP, on_cmd_prog_ws_otp ),
CMD_ENTRY(CMD_LOAD_WS_OTP, on_cmd_load_ws_otp ),
CMD_ENTRY(CMD_WRITE_LUT_REG, on_cmd_write_lut_reg ),
CMD_ENTRY(CMD_PROG_OTP_SEL, on_cmd_prog_otp_sel ),
CMD_ENTRY(CMD_WRITE_USER_ID, on_cmd_write_user_id ),
CMD_ENTRY(CMD_PROG_OTP_MODE, on_cmd_prog_otp_mode ),
CMD_ENTRY(CMD_WRITE_RAM_X_COORDS, on_cmd_write_ram_x_coords ),
CMD_ENTRY(CMD_WRITE_RAM_Y_COORDS, on_cmd_write_ram_y_coords ),
CMD_ENTRY(CMD_WRITE_RAM_X_ADDR_CTR, on_cmd_write_ram_x_addr_ctr ),
CMD_ENTRY(CMD_WRITE_RAM_Y_ADDR_CTR, on_cmd_write_ram_y_addr_ctr ),
CMD_ENTRY(CMD_LUT_3F, on_cmd_lut_3f ),
};
static cmd_map_t *cmd_map;

// per command data bytes from datasheet
uint8_t cmd_data_bytes[] = {
        //      x0 x1 x2 x3 x4 x5 x6 x7 x8 x9 xA xB xC xD xE xF
        /*0x*/   0, 3, 0, 1, 3, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        /*1x*/   1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        /*2x*/   0, 2, 1, 0, 255, 0, 255, 0, 0, 1, 0, 2, 1, 11, 10, 0,
        /*3x*/   0, 0, 153, 0, 0, 0, 0, 0, 10, 1, 0, 0, 0, 0, 0, 1,
        /*4x*/   0, 0, 0, 0, 2, 4, 0, 0, 0, 0, 0, 0, 0, 0, 1, 2,
};




int cmd_key_compare(const uint16_t *k1, const uint16_t *k2 ) { return *k1 == *k2 ? 0 : *k1 > *k2 ? 1 : -1; }
size_t cmd_key_hash(const uint16_t *k1 ) { return hashmap_hash_default(k1, sizeof(*k1)); }
uint16_t cmd_to_key(uint16_t state, uint16_t cmd) { return state << 8 | cmd; }
void cmd_init_hash() {
    cmd_map = calloc(1, sizeof(cmd_map_t));

    hashmap_init(cmd_map, cmd_key_hash, cmd_key_compare);
    for( int i= 0; i < sizeof(cmd_entries)/sizeof(cmd_entry_t); i++) {
        cmd_entry_t *e = cmd_entries+i;
        hashmap_put(cmd_map, &e->cmd_key, e);
    }
}


void on_timer_event(void *data) {
    ws29v2_ctx_t *chip = (ws29v2_ctx_t *)data;
    SPI_DEBUGF("on_timer_event\n");
}


void on_cs_pin_change(void *user_data, pin_t pin, uint32_t value) {
    ws29v2_ctx_t *chip = (ws29v2_ctx_t *)user_data;
    SPI_DEBUGF("on_cs_pin_change: pin: %d - value: %d %s\n", pin, value, value ? "⮥" : "⮧" );

    // Handle CS pin logic
    if (pin != chip->cs) {
        return; // TBD
    }

    if (value == LOW) {
        SPI_DEBUGF("SPI chip selected, starting transaction\n");
        chip->buffer[0] = 0;  // dummy data - should be ignored by MCU
        start_spi_transfer(chip, chip->spi_state, 1);
        return;
    }

    SPI_DEBUGF("SPI chip deselected - transaction finished\n");
    // since the driver code seems to insist on sending one byte at a time, we rely on the framework to stop the SPI at end of txfer
    // spi_stop(chip->spi);
}

void on_dc_pin_change(void *user_data, pin_t pin, uint32_t value) {
    ws29v2_ctx_t *chip = (ws29v2_ctx_t *)user_data;
    SPI_DEBUGF("on_dc_pin_change: pin: %d - value: %d %s\n", pin, value, value ? "⮥" : "⮧" );

    chip->mode = value;
}


void chip_reset(ws29v2_ctx_t *chip) {
    GEN_DEBUGF("*** ws29v2 chip resetting...\n");
    pin_write(chip->busy, HIGH);


    chip->addr_incr_mode = ADDR_INCR_MODE_POR;
    chip->disp_upd_seq = DISP_UPDATE_SEQ_POR;

    chip->spi_state = ST_SPI_WAIT_CMD;
    chip->mode = pin_read(chip->dc);

    GEN_DEBUGF("*** ws29v2 reset pin value: %d\n", pin_read(chip->reset));
    chip->reset_time = get_sim_nanos();
    pin_write(chip->busy, LOW);
    GEN_DEBUGF("*** ws29v2 chip reset done @%lld\n", chip->reset_time);

}

static void chip_init_attrs(ws29v2_ctx_t *chip) {
    uint32_t attr;

    attr = attr_init("debug", 0);
    chip->debug = attr_read(attr) != 0;
    attr = attr_init("debug_mask", 0xFF);
    chip->debug_mask = attr_read(attr);

    printf("*** ws29v2 chip attributes\n debug: %d\n debug_mask: %d\n",
           chip->debug, chip->debug_mask);
}

void chip_init() {
    printf("*** ws29v2 chip initialising...\n");
    ws29v2_ctx_t *chip = calloc(1, sizeof(ws29v2_ctx_t));

    // initialise state machine
    cmd_init_hash();

    // initialise sim attributes
    chip_init_attrs(chip);


    // initialise and check pins
    // chip->rese = pin_init( "rese", INPUT);
    // chip->bs1 = pin_init( "bs1", INPUT);
    chip->busy = pin_init( "BUSY", OUTPUT);
    chip->reset = pin_init( "RST", INPUT);
    chip->dc = pin_init( "DC", INPUT);
    chip->cs = pin_init( "CS", INPUT);
    chip->sck = pin_init( "CLK", INPUT);
    chip->sdi = pin_init( "DIN", INPUT);
    chip->vci = pin_init( "VCC", INPUT);
    chip->vss = pin_init( "VCC", INPUT);    

    // if (pin_read(chip->vddio) == LOW) {
    //     if (pin_read(chip->sck) == HIGH || pin_read(chip->sdi) == HIGH ||
    //         /* pin_read(chip->sdo) == HIGH || */ pin_read(chip->cs) == HIGH) {
    //         printf("*** ws29v2 error detected - vddio is low but IO pins are high ...\n");
    //         return;
    //     }
    // }


    pin_watch_config_t watch_config = { .edge = BOTH, .user_data = chip };

    watch_config.pin_change = on_cs_pin_change;
    pin_watch(chip->cs, &watch_config);

    watch_config.pin_change = on_reset_pin_change;
    pin_watch(chip->reset, &watch_config);

    watch_config.pin_change = on_dc_pin_change;
    pin_watch(chip->dc, &watch_config);

    const spi_config_t spi_cfg = {
        .sck = chip->sck,
        .mosi = chip->sdi,
        .miso = NO_PIN,
        .mode = 0,
        .done = on_spi_done_2,
        .user_data = chip,
    };
    chip->spi = spi_init(&spi_cfg);
    printf("*** ws29v2 chip initialising with SPI config with:\n sck: %d, mosi: %d, cs: %d ...\n",
            chip->sck, chip->sdi, chip->cs);

    timer_config_t timer_cfg = {
        .user_data = chip,
        .callback = on_timer_event,
    };
    chip->timer = timer_init(&timer_cfg);

    timer_cfg.callback = on_reset_timer;
    chip->reset_timer = timer_init(&timer_cfg);


    // display
    chip->frame_buf = framebuffer_init(&chip->width, &chip->height);

    chip_reset(chip);
    printf("*** ws29v2 chip initialised...\n");
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
        chip_reset(chip);
        return;
    }
    
    // if variable data or we're still expecting more bytes, exit
    if (cmd_data_bytes[chip->cmd] == 0xFF || chip->data_ndx < cmd_data_bytes[chip->cmd]) {
        return;
    }

    // all command data received
    SPI_DEBUGF("on_spi_done: command 0x%2x data received(%s)\n", chip->cmd, debugHexStr(chip->buffer, min(count, 16)));
    on_command(chip);
}

void on_reset_timer(void *user_data) {
    ws29v2_ctx_t *chip = (ws29v2_ctx_t *)user_data;
    SPI_DEBUGF("on_reset_timer\n");
    chip_reset(chip);
}

void on_reset_pin_change(void *user_data, pin_t pin, uint32_t value) {
    ws29v2_ctx_t *chip = (ws29v2_ctx_t *)user_data;
    SPI_DEBUGF("on_reset_pin_change: %s\n", value ? "⮥" : "⮧" );

    uint64_t now = get_sim_nanos();
    uint64_t diff = now - chip->reset_time;

    // rising edge. if pin was low for at least pulse width, reset
    if (value) {
        SPI_DEBUGF("on_reset_pin_change: rising edge after (%lld)ns\n", diff);
        if (diff < RESET_PULSE_MS) {
            SPI_DEBUGF("on_reset_pin_change: rising edge too quick (%lld). NOT resetting\n", diff);
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


void on_pin_change(void *user_data, pin_t pin, uint32_t value) {
    ws29v2_ctx_t *chip = (ws29v2_ctx_t *)user_data;
    SPI_DEBUGF("on_pin_change:  pin:%d - %s\n", pin, value ? "⮥" : "⮧" );

    // Handle CS pin logic
    if (pin != chip->cs) {
        return;
    }

    if (value == LOW) {
        SPI_DEBUGF("SPI chip selected, starting transaction\n");
        chip->buffer[0] = 0;  // dummy data - should be ignored by MCU
        start_spi_transfer(chip, ST_SPI_WAIT_CMD, 1);
        return;
    }

    SPI_DEBUGF("SPI chip deselected - transaction finished\n");
    spi_stop(chip->spi);
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

void on_command(ws29v2_ctx_t *chip) {
    uint16_t key = chip->cmd;
    cmd_entry_t *e = hashmap_get(cmd_map, &key);
    
    if (e == NULL) {
        DEBUGF("**** command code %02x unknown\n", chip->cmd);
        // chip_reset(chip);
        return;
    }

    GEN_DEBUGF("on_command %s (%02x, %d): %s\n", e->cmd_name, chip->cmd, chip->data_ndx, debugHexStr(chip->data, min(16,chip->data_ndx)));
    e->handler(chip);
    chip->data_ndx = 0;
    chip->cmd = 0;
}
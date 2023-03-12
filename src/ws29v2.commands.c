#include "ws29v2.h"

#include <string.h>

static void display_panel_bw_scanline(ws29v2_ctx_t *chip, uint8_t invert);
static void display_panel_colour_scanline(ws29v2_ctx_t *chip, uint8_t invert);

static int cmd_key_compare(const uint16_t *k1, const uint16_t *k2) { return *k1 == *k2 ? 0 : *k1 > *k2 ? 1 : -1; }
static size_t cmd_key_hash(const uint16_t *k1) { return hashmap_hash_default(k1, sizeof(*k1)); }
static uint16_t cmd_to_key(uint16_t state, uint16_t cmd) { return state << 8 | cmd; }

cmd_entry_t cmd_entries[] = {
    CMD_ENTRY(CMD_DRIVER_OUTPUT_CTL, on_cmd_driver_output_ctl),
    CMD_ENTRY(CMD_GATE_OUTPUT_CTL, on_cmd_gate_output_ctl),
    CMD_ENTRY(CMD_SRC_OUTPUT_CTL, on_cmd_src_output_ctl),
    CMD_ENTRY(CMD_DEEP_SLEEP, on_cmd_deep_sleep),
    CMD_ENTRY(CMD_DATA_ENTRY_MODE, on_cmd_data_entry_mode),
    CMD_ENTRY(CMD_SW_RESET, on_cmd_sw_reset),
    CMD_ENTRY(CMD_MASTER_ACTIVATION, on_cmd_master_activation),
    CMD_ENTRY(CMD_DISP_UPD_CTL, on_cmd_disp_upd_ctl),
    CMD_ENTRY(CMD_DISP_UPD_CTL2, on_cmd_disp_upd_ctl2),
    CMD_ENTRY(CMD_WRITE_RAM_BW, on_cmd_write_ram_bw),
    CMD_ENTRY(CMD_WRITE_RAM_COLOUR, on_cmd_write_ram_colour),
    CMD_ENTRY(CMD_VCOM_SENSE, on_cmd_vcom_sense),
    CMD_ENTRY(CMD_VCOM_SENSE_DUR, on_cmd_vcom_sense_dur),
    CMD_ENTRY(CMD_PROG_VCOM_OTP, on_cmd_prog_vcom_otp),
    CMD_ENTRY(CMD_VCOM_REG_CTL, on_cmd_vcom_reg_ctl),
    CMD_ENTRY(CMD_WRITE_VCOM_REG, on_cmd_write_vcom_reg),
    CMD_ENTRY(CMD_READ_OTP_REG, on_cmd_read_otp_reg),
    CMD_ENTRY(CMD_READ_USER_ID, on_cmd_read_user_id),
    CMD_ENTRY(CMD_PROG_WS_OTP, on_cmd_prog_ws_otp),
    CMD_ENTRY(CMD_LOAD_WS_OTP, on_cmd_load_ws_otp),
    CMD_ENTRY(CMD_WRITE_LUT_REG, on_cmd_write_lut_reg),
    CMD_ENTRY(CMD_PROG_OTP_SEL, on_cmd_prog_otp_sel),
    CMD_ENTRY(CMD_WRITE_OTP_DATA, on_cmd_write_otp_data),
    CMD_ENTRY(CMD_WRITE_USER_ID, on_cmd_write_user_id),
    CMD_ENTRY(CMD_PROG_OTP_MODE, on_cmd_prog_otp_mode),
    CMD_ENTRY(CMD_WRITE_RAM_X_COORDS, on_cmd_write_ram_x_coords),
    CMD_ENTRY(CMD_WRITE_RAM_Y_COORDS, on_cmd_write_ram_y_coords),
    CMD_ENTRY(CMD_WRITE_RAM_X_ADDR_CTR, on_cmd_write_ram_x_addr_ctr),
    CMD_ENTRY(CMD_WRITE_RAM_Y_ADDR_CTR, on_cmd_write_ram_y_addr_ctr),
    CMD_ENTRY(CMD_LUT_3F, on_cmd_lut_3f),
    CMD_ENTRY(CMD_NOP, on_cmd_nop),
    CMD_ENTRY(CMD_BOOSTER_SOFT_START, on_cmd_nop),
    CMD_ENTRY(CMD_DUMMY_LINE_3A, on_cmd_nop),
    CMD_ENTRY(CMD_GATE_WIDTH_3B, on_cmd_nop),
    CMD_ENTRY(CMD_BORDER_WF, on_cmd_nop),
};

cmd_map_t *cmd_map;

// clang-format off
// per command data bytes from datasheet
uint8_t cmd_data_bytes[] = {
    //      x0 x1 x2 x3 x4 x5 x6 x7 x8 x9 xA xB xC xD xE xF
    /*0x*/ 0, 3, 0, 1, 3, 0, 0, 0, 0, 0, 0, 0, 3, 0, 0, 0,
    /*1x*/ 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    /*2x*/ 0, 2, 1, 0, 255, 0, 255, 0, 0, 1, 0, 2, 1, 11, 10, 0,
    /*3x*/ 0, 0, 153, 0, 0, 0, 0, 10, 10, 1, 1, 1, 1, 0, 0, 1, 
    /*4x*/ 0, 0, 0, 0, 2, 4, 0, 0, 0, 0, 0, 0, 0, 0, 1, 2,
};
// clang-format on

#define ACT_SEQ_LEN(a) (sizeof(a) / sizeof(a[0]))
#define AM_IMMEDIATE_UPDATE 1  // the offset of vX_act_seq_immediate
activation_mode_t v1_act_seq_scanline[] = {AM_BLACK, AM_SCAN_LINE_BW, AM_BW};
activation_mode_t v1_act_seq[] = {AM_BLACK, AM_FADE_1, AM_FADE_2, AM_FADE_3, AM_BW, AM_FADE_3, AM_FADE_2, AM_FADE_1, AM_BW, AM_BW, AM_BW};
activation_mode_t v1_act_seq_immediate[] = {AM_BW};
activation_mode_t v1_act_seq_blink[] = {AM_BLACK, AM_WHITE, AM_BW, AM_BW, AM_INV_BW, AM_BW, AM_BW};

activation_mode_t v2_act_seq[] = {AM_BLACK, AM_WHITE, AM_BW, AM_BW, AM_BW, AM_INV_BW, AM_CBW, AM_INV_CBW, AM_CBW, AM_CBW, AM_CBW, AM_CBW};
activation_mode_t v2_act_seq_scanline[] = {AM_BLACK, AM_SCAN_LINE_CBW};
activation_mode_t v2_act_seq_immediate[] = {AM_CBW};

act_mode_len_t activation_modes[2][4] = {
    {
        {v1_act_seq, ACT_SEQ_LEN(v1_act_seq)},
        {v1_act_seq_immediate, ACT_SEQ_LEN(v1_act_seq_immediate)},
        {v1_act_seq_blink, ACT_SEQ_LEN(v1_act_seq_blink)},
        {v1_act_seq_scanline, ACT_SEQ_LEN(v1_act_seq_scanline)},
    },
    {
        {v2_act_seq, ACT_SEQ_LEN(v2_act_seq)},
        {v2_act_seq_immediate, ACT_SEQ_LEN(v2_act_seq_immediate)},
        {v2_act_seq, ACT_SEQ_LEN(v2_act_seq)},
        {v2_act_seq_scanline, ACT_SEQ_LEN(v2_act_seq_scanline)},
    }};

static uint32_t RED = FB_RED;
static uint32_t BLACK = FB_BLACK;
static uint32_t WHITE = FB_WHITE;
static uint32_t YELLOW = FB_YELLOW;

void cmd_init_hash() {
    cmd_map = calloc(1, sizeof(cmd_map_t));

    hashmap_init(cmd_map, cmd_key_hash, cmd_key_compare);
    for (int i = 0; i < sizeof(cmd_entries) / sizeof(cmd_entry_t); i++) {
        cmd_entry_t *e = cmd_entries + i;
        hashmap_put(cmd_map, &e->cmd_key, e);
    }
}

void on_command(ws29v2_ctx_t *chip) {
    uint16_t key = chip->cmd;
    cmd_entry_t *e = hashmap_get(cmd_map, &key);

    if (e == NULL) {
        DEBUGF("**** command code %02x unknown\n", chip->cmd);
        // chip_reset(chip);
        return;
    }

    GEN_DEBUGF("on_command %s (%02x, %d): %s\n", e->cmd_name, chip->cmd, chip->data_ndx, debugHexStr(chip->data, min(16, chip->data_ndx)));
    e->handler(chip);
    chip->data_ndx = 0;
    chip->cmd = 0;
}

void on_cmd_driver_output_ctl(ws29v2_ctx_t *chip) {
    GEN_DEBUGF("on_cmd_driver_output_ctl: Addr: %2x, gd: %x\n", (int)chip->data[1] << 8 | chip->data[0], chip->data[2]);
}
void on_cmd_gate_output_ctl(ws29v2_ctx_t *chip) {
    GEN_DEBUGF("on_cmd_gate_output_ctl: voltage: %2x\n", chip->data[0] & 0x1f);
}
void on_cmd_src_output_ctl(ws29v2_ctx_t *chip) {
    GEN_DEBUGF("on_cmd_src_output_ctl: vgh1: %2x, vgh2: %2x, vsl: %2x, \n", chip->data[0], chip->data[1], chip->data[2]);
}
void on_cmd_deep_sleep(ws29v2_ctx_t *chip) {
    GEN_DEBUGF("on_cmd_deep_sleep: mode: %x\n", chip->data[0] & 0x3);

    chip->sleeping = false;

    // deep sleep requested?
    if ((chip->data[0] & 0x3) == 1) {
        pin_write(chip->busy, HIGH);
        chip->sleeping = true;
    }
}
void on_cmd_data_entry_mode(ws29v2_ctx_t *chip) {
    GEN_DEBUGF("on_cmd_data_entry_mode: mode: %x -> Y: %c, X: %c\n", chip->data[0] & 0x3, chip->data[0] & ADDR_INCR_Y_MASK ? '+' : '-', chip->data[0] & ADDR_INCR_X_MASK ? '+' : '-');
    chip->addr_incr_mode = chip->data[0] & ADDR_INCR_MASK;
}
void on_cmd_sw_reset(ws29v2_ctx_t *chip) {
    GEN_DEBUGF("on_cmd_sw_reset\n");
    chip_reset(chip);
}

void display_panel_bw_frame(ws29v2_ctx_t *chip, uint8_t invert) {
    GEN_DEBUGF("display_panel_bw_frame: chip: %p - invert: %d\n", chip, invert);
    do {
        display_panel_bw_scanline(chip, invert);
    } while (chip->act_scan_ndx != 0);
}

static void display_panel_bw_scanline(ws29v2_ctx_t *chip, uint8_t invert) {
    // GEN_DEBUGF("display_panel_bw_scanline: chip: %p, scanline: %d\n", chip, chip->act_scan_ndx);
    int pixels_bytes = chip->width / 8;
    int row_off = chip->act_scan_ndx * chip->width / 8;
    int x = 0;
    int index = chip->act_scan_ndx * chip->width;
    for (int i = 0; i < pixels_bytes; i++) {
        uint8_t pixb = chip->bw_ram[row_off + i];

        pixb = invert ? ~pixb : pixb;

        for (uint8_t b = 0x80; b; b >>= 1) {
            bool bit = pixb & b;
            // GEN_DEBUGF("display_panel_bw_frame: setting pixel at index: %d: y: %d, x: %d, bit: %d\n", index, y, x, bit);
            buffer_write(chip->frame_buf, (index + x) * sizeof(uint32_t), (uint8_t *)(bit ? &WHITE : &BLACK), sizeof(uint32_t));
            x++;
        }

        if (x >= chip->width) {
            chip->act_scan_ndx++;
            if (chip->act_scan_ndx >= chip->height) {
                chip->act_scan_ndx = 0;
            }
            break;
        }
    }
}

void display_panel_bw_frame_with_fade(ws29v2_ctx_t *chip, uint16_t fade) {
    GEN_DEBUGF("display_panel_bw_frame_with_fade: chip: %p - fade: %d\n", chip, fade);
    int pixels_bytes = chip->width * chip->height / 8;
    int x = 0, y = 0, w = chip->width;
    int c = 0;
    uint32_t bf, wf;
    switch (fade) {
    default:
    case 1:  // 25%
        bf = 0x3F3F3FFF;
        wf = 0xBFBFBFFF;
        break;

    case 2:  // 50%
        bf = 0x7F7F7FFF;
        wf = 0x7F7F7FFF;
        break;

    case 3:  // 75%
        bf = 0xBFBFBFFF;
        wf = 0x3F3F3FFF;
        break;
    }
    for (int i = 0; i < pixels_bytes; i++) {
        uint8_t pixb = chip->bw_ram[i];
        for (uint8_t b = 0x80; b; b >>= 1) {
            int index = y * w + x;
            bool bit = (pixb & b);
            buffer_write(chip->frame_buf, index * sizeof(uint32_t), (uint8_t *)(bit ? &bf : &wf), sizeof(uint32_t));
            c++;
            x++;
        }

        if (x >= chip->width) {
            x = 0;
            y++;
            if (y >= chip->height) {
                y = 0;
            }
        }
    }
}

void display_panel_colour_frame(ws29v2_ctx_t *chip, uint16_t invert) {
    GEN_DEBUGF("display_panel_colour_frame: chip: %p - invert: %d, c: %4x, w:%d, h: %d\n", chip, invert, chip->colour, chip->width, chip->height);
    do {
        display_panel_colour_scanline(chip, invert);
    } while (chip->act_scan_ndx != 0);
}

static void display_panel_colour_scanline(ws29v2_ctx_t *chip, uint8_t invert) {
    if (!chip->act_scan_ndx) GEN_DEBUGF("display_panel_colour_scanline: chip: %p - invert: %d, c: %4x, w:%d, h: %d\n", chip, invert, chip->colour, chip->width, chip->height);
    int pixels_bytes = chip->width / 8;
    int row_off = chip->act_scan_ndx * chip->width / 8;
    int x = 0;
    int index = chip->act_scan_ndx * chip->width;
    for (int i = 0; i < pixels_bytes; i++) {
        uint8_t pixr = chip->colour_ram[row_off + i];
        uint8_t pixbw = chip->bw_ram[row_off + i];

        pixr = invert ? ~pixr : pixr;
        pixbw = invert ? ~pixbw : pixbw;

        for (uint8_t b = 0x80; b; b >>= 1) {
            // if (!chip->act_scan_ndx) GEN_DEBUGF("display_panel_colour_scanline:    inv: %2x, pixr: %2x, pixbw: %2x, pixr&b: %2x, pixr&b^i: %2x, pixr? %s, pixbw&b: %2x, pixbw&b^i: %2x, pixbw? %s, \n", invert, pixr, pixbw, pixr & b, (pixr & b) ^ invert, ((pixr & b) ^ invert) ? "T" : "F", pixbw & b, (pixbw & b) ^ invert, ((pixbw & b) ^ invert) ? "T" : "F");
            uint32_t colour = (pixr & b) ? chip->colour : ((pixbw & b) ? WHITE : BLACK);
            buffer_write(chip->frame_buf, (index + x) * sizeof(uint32_t), (uint8_t *)&colour, sizeof(uint32_t));
            x++;
        }
        if (x >= chip->width) {
            chip->act_scan_ndx++;
            if (chip->act_scan_ndx >= chip->height) {
                chip->act_scan_ndx = 0;
            }
            break;
        }
    }
}

void display_panel_fixed_frame(ws29v2_ctx_t *chip, int pixels, uint32_t value) {
    for (int i = 0; i < pixels; i++) {
        buffer_write(chip->frame_buf, i * sizeof(uint32_t), (uint8_t *)(&value), sizeof(uint32_t));
    }
}

void on_activation_step(void *data) {
    ws29v2_ctx_t *chip = data;

    // if we got to the end, release the busy signal
    if (chip->act_ndx >= chip->cur_act_seq.len) {
        GEN_DEBUGF("on_activation_step: resetting busy\n");

        chip->act_ndx = 0;
        pin_write(chip->busy, LOW);
        return;
    }

    // if we're starting, set BUSY
    if (chip->act_ndx == 0) {
        pin_write(chip->busy, HIGH);
    }

    uint32_t timerPeriod = ACTIVATION_STEP_PERIOD;
    int stepInc = 1;

    if (!chip->act_scan_ndx) GEN_DEBUGF("on_activation_step: step: %d, %d\n", chip->act_ndx, chip->cur_act_seq.seq[chip->act_ndx]);

    // set display based on step
    switch (chip->cur_act_seq.seq[chip->act_ndx]) {
    case AM_BLACK:
        // display_panel_fixed_frame(chip, chip->width * chip->height, FB_BLACK);
        buffer_write(chip->frame_buf, 0, (uint8_t *)chip->black, sizeof(uint32_t) * chip->width * chip->height);
        break;
    case AM_WHITE:
        // display_panel_fixed_frame(chip, chip->width * chip->height, FB_WHITE);
        buffer_write(chip->frame_buf, 0, (uint8_t *)chip->white, sizeof(uint32_t) * chip->width * chip->height);
        break;
    case AM_BW:
        display_panel_bw_frame(chip, 0x0);
        break;
    case AM_INV_BW:
        display_panel_bw_frame(chip, 0xFF);
        break;
    case AM_CBW:
        display_panel_colour_frame(chip, 0x0);
        break;
    case AM_INV_CBW:
        display_panel_colour_frame(chip, 0xFF);
        break;
    case AM_FADE_1:
        display_panel_bw_frame_with_fade(chip, 1);
        break;
    case AM_FADE_2:
        display_panel_bw_frame_with_fade(chip, 2);
        break;
    case AM_FADE_3:
        display_panel_bw_frame_with_fade(chip, 3);
        break;
    case AM_SCAN_LINE_BW:
        display_panel_bw_scanline(chip, 0x0);
        if (chip->act_scan_ndx > 0) {
            timerPeriod = ACTIVATION_SCAN_LINE_PERIOD;
            stepInc = 0;
        }
        break;
    case AM_SCAN_LINE_CBW:
        display_panel_colour_scanline(chip, 0x00);
        if (chip->act_scan_ndx > 0) {
            timerPeriod = ACTIVATION_SCAN_LINE_PERIOD;
            stepInc = 0;
        }
        break;
    default:
        break;
    }

    chip->act_ndx += stepInc;

    // fire timer for next step
    timer_start(chip->activation_timer, timerPeriod, false);
}

void on_cmd_master_activation(ws29v2_ctx_t *chip) {
    chip->act_ndx = 0;
    chip->act_scan_ndx = 0;

    switch (chip->disp_upd_seq) {
    default:
    case 0xC7:
        chip->cur_act_seq = chip->act_seq;
        break;
    case 0xC0:
    case 0x0F:
        chip->cur_act_seq = activation_modes[chip->version - 1][AM_IMMEDIATE_UPDATE];
        break;
    }

    on_activation_step(chip);
}

void on_cmd_disp_upd_ctl(ws29v2_ctx_t *chip) {
}
void on_cmd_disp_upd_ctl2(ws29v2_ctx_t *chip) {
    chip->disp_upd_seq = chip->data[0];
}

void write_ram_byte(ws29v2_ctx_t *chip, uint8_t *buf, int i) {
    int off = (chip->y_addr * chip->width + chip->x_addr) / 8;
    // GEN_DEBUGF("write_ram_byte: writing at offset %p -> %p,  %d (%d) x: %d, y: %d\n",buf, buf + off, off, i, chip->x_addr, chip->y_addr);
    buf[off] = chip->data[i];
    chip->x_addr += 8;

    if (chip->x_addr > chip->x_max) {
        chip->x_addr = chip->x_min;
        chip->y_addr++;
        if (chip->y_addr > chip->y_max) {
            chip->y_addr = chip->y_min;
        }
    }
}

void on_cmd_write_ram_bw(ws29v2_ctx_t *chip) {
    GEN_DEBUGF("on_cmd_write_ram_bw: copying %d bytes to ram ... ", chip->data_ndx);

    for (int i = 0; i < chip->data_ndx; i++) {
        write_ram_byte(chip, chip->bw_ram, i);
    }
    // DBG_BUFFER("on_cmd_write_ram_bw: ram contents ", chip->bw_ram, 4736, 16);
}

void on_cmd_write_ram_colour(ws29v2_ctx_t *chip) {
    GEN_DEBUGF("on_cmd_write_ram_colour: copying %d bytes to ram ... ", chip->data_ndx);

    for (int i = 0; i < chip->data_ndx; i++) {
        write_ram_byte(chip, chip->colour_ram, i);
    }

    GEN_DEBUGF("x_addr: %d, y_addr: %d\n", chip->x_addr, chip->y_addr);
}

void on_cmd_vcom_sense(ws29v2_ctx_t *chip) {
}
void on_cmd_vcom_sense_dur(ws29v2_ctx_t *chip) {
}
void on_cmd_prog_vcom_otp(ws29v2_ctx_t *chip) {
}
void on_cmd_vcom_reg_ctl(ws29v2_ctx_t *chip) {
}
void on_cmd_write_vcom_reg(ws29v2_ctx_t *chip) {
}
void on_cmd_read_otp_reg(ws29v2_ctx_t *chip) {
}
void on_cmd_read_user_id(ws29v2_ctx_t *chip) {
}
void on_cmd_prog_ws_otp(ws29v2_ctx_t *chip) {
}
void on_cmd_load_ws_otp(ws29v2_ctx_t *chip) {
}
void on_cmd_write_lut_reg(ws29v2_ctx_t *chip) {
}
void on_cmd_prog_otp_sel(ws29v2_ctx_t *chip) {
}
void on_cmd_write_otp_data(ws29v2_ctx_t *chip) {
    memcpy(chip->otp, chip->data, chip->data_ndx);
}
void on_cmd_write_user_id(ws29v2_ctx_t *chip) {
}
void on_cmd_prog_otp_mode(ws29v2_ctx_t *chip) {
}
void on_cmd_write_ram_x_coords(ws29v2_ctx_t *chip) {
    chip->x_min = (uint16_t)(chip->data[0]) * 8;
    chip->x_max = (uint16_t)(chip->data[1] + 1) * 8 - 1;
    GEN_DEBUGF("on_cmd_write_ram_x_coords: x_min: %d, x_max: %d\n", chip->x_min, chip->x_max);
}

void on_cmd_write_ram_y_coords(ws29v2_ctx_t *chip) {
    chip->y_min = (uint16_t)(chip->data[1] << 8) | chip->data[0];
    chip->y_max = (uint16_t)(chip->data[3] << 8) | chip->data[2];
    GEN_DEBUGF("on_cmd_write_ram_y_coords: y_min: %d, y_max: %d\n", chip->y_min, chip->y_max);
}
void on_cmd_write_ram_x_addr_ctr(ws29v2_ctx_t *chip) {
    chip->x_addr = (uint16_t)(chip->data[0]) * 8;
    GEN_DEBUGF("on_cmd_write_ram_x_addr_ctr: x_addr: %d\n", chip->x_addr);
}
void on_cmd_write_ram_y_addr_ctr(ws29v2_ctx_t *chip) {
    chip->y_addr = (uint16_t)(chip->data[1] << 8) | chip->data[0];
    GEN_DEBUGF("on_cmd_write_ram_y_addr_ctr: y_addr: %d\n", chip->y_addr);
}

// undocumented commands
void on_cmd_lut_3f(ws29v2_ctx_t *chip) {
}

// undocumented commands
void on_cmd_nop(ws29v2_ctx_t *chip) {
}

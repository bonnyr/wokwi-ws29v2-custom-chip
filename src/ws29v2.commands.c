#include "ws29v2.h"

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
    if ((chip->data[0] & 0x3) == 1 ) {
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
void on_cmd_master_activation(ws29v2_ctx_t *chip) {
    
}
void on_cmd_disp_upd_ctl(ws29v2_ctx_t *chip) {
    
}
void on_cmd_disp_upd_ctl2(ws29v2_ctx_t *chip) {
    
}


void on_cmd_write_ram_bw(ws29v2_ctx_t *chip) {
    uint32_t BLACK = 0x000000FF;
    uint32_t WHITE = 0xFFFFFFFF;

  for (int i = 0; i < chip->data_ndx; i++) {
    for (uint8_t b = 0x80; b; b >>=1 ) {
        int index = chip->y_addr * chip->width + chip->x_addr;
        buffer_write(chip->frame_buf, index * sizeof(uint32_t), (uint8_t *)(chip->data[i]&b ? &WHITE : &BLACK), sizeof(uint32_t));
        chip->x_addr++;
    }

    if (chip->x_addr > chip->x_max) {
        chip->x_addr = chip->x_min;
        chip->y_addr ++;
        if (chip->y_addr > chip->y_max) {
            chip->y_addr = chip->y_min;
        }
    }
  }
}

void on_cmd_write_ram_red(ws29v2_ctx_t *chip) {

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
void on_cmd_write_user_id(ws29v2_ctx_t *chip) {
    
}
void on_cmd_prog_otp_mode(ws29v2_ctx_t *chip) {
    
}
void on_cmd_write_ram_x_coords(ws29v2_ctx_t *chip) {
    chip->x_min = (uint16_t)(chip->data[0]) * 8;
    chip->x_max = (uint16_t)(chip->data[1]) * 8;
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

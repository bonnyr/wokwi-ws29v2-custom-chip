#ifndef __WS29V2_H__
#define __WS29V2_H__

#include "wokwi-api.h"

#define DEBUG
#include "debug.h"


#define MAX_DATA_TXFER 8192

typedef enum {
    ST_SPI_WAIT_CMD,
    ST_SPI_WAIT_DATA,
    ST_SPI_DONE
} spi_state_t;

typedef struct {
    bool debug;
    uint8_t debug_mask;

    uint8_t cmd;
    uint16_t data_ndx;
    uint8_t buffer[1];
    uint8_t data[MAX_DATA_TXFER];
    uint8_t mode;       // SPI mode depending on dc pin value

    // pins
    pin_t rese;
    pin_t bs1;
    pin_t busy;
    pin_t reset;
    pin_t dc;
    pin_t cs;
    pin_t sck;
    pin_t sdi;
    pin_t vci;
    pin_t vss;

    bool sleeping;      // are we in deep sleep. SPI disabled when on (!?)

    uint8_t addr_incr_mode;
    uint16_t x_addr;
    uint16_t y_addr;

    uint16_t x_min, x_max;
    uint16_t y_min, y_max;

    uint8_t disp_upd_seq;

    spi_dev_t spi;
    spi_state_t spi_state;
    bool txfer;

    // timers
    timer_t timer;
    timer_t reset_timer;
    uint64_t reset_time;

    // display
    buffer_t frame_buf;
    uint32_t width;
    uint32_t height;


} ws29v2_ctx_t;

typedef enum {
    DA_CLK_EN,
    DA_CLK_DIS,
    DA_ANA_EN,
    DA_ANA_DIS,
    DA_LOAD_LUT_M1,
    DA_LOAD_LUT_M2,
    DA_OSC_EN,
    DA_OSC_DIS,
    DA_DISP_M1,
    DA_DISP_M2,
    DA_LOAD_TEMP,

} ws29v2_disp_act_t;

// timer values
#define RESET_PULSE_MS 10000

// ADDR INCR modes
#define ADDR_INCR_X_MASK 0x1
#define ADDR_INCR_Y_MASK 0x2
#define ADDR_INCR_AXIS_MASK 0x4 // 0 -> incr X addr, 1 -> incr y
#define ADDR_INCR_MASK (ADDR_INCR_X_MASK | ADDR_INCR_Y_MASK|ADDR_INCR_AXIS_MASK)
#define ADDR_INCR_MODE_POR 0x11 // incr addresses +ve and default to x axis

// disp upd seq action

#define DISP_SEQ_CLK_BIT        0x80

#define DISP_UPDATE_SEQ_CLOCK_EN        0x80
#define DISP_UPDATE_SEQ_CLOCK_DIS       0x01
#define DISP_UPDATE_SEQ_CLOCK_EN_ANA    0xC0
#define DISP_UPDATE_SEQ_CLOCK_DIS_ANA   0x03
#define DISP_UPDATE_SEQ_CLOCK_EN    0x80
#define DISP_UPDATE_SEQ_CLOCK_EN    0x80
#define DISP_UPDATE_SEQ_CLOCK_EN    0x80
#define DISP_UPDATE_SEQ_CLOCK_EN    0x80
#define DISP_UPDATE_SEQ_CLOCK_EN    0x80
#define DISP_UPDATE_SEQ_CLOCK_EN    0x80
#define DISP_UPDATE_SEQ_CLOCK_EN    0x80

#define DISP_UPDATE_SEQ_POR    0xFF

// COMMAND CODES
#define CMD_DRIVER_OUTPUT_CTL   0x01
#define CMD_GATE_OUTPUT_CTL   0x03
#define CMD_SRC_OUTPUT_CTL   0x04
#define CMD_DEEP_SLEEP   0x10
#define CMD_DATA_ENTRY_MODE   0x11
#define CMD_SW_RESET   0x12
#define CMD_MASTER_ACTIVATION   0x20
#define CMD_DISP_UPD_CTL   0x21
#define CMD_DISP_UPD_CTL2   0x22
#define CMD_WRITE_RAM_BW   0x24
#define CMD_WRITE_RAM_RED   0x26
#define CMD_VCOM_SENSE   0x28
#define CMD_VCOM_SENSE_DUR   0x29
#define CMD_PROG_VCOM_OTP   0x2A
#define CMD_VCOM_REG_CTL   0x2B
#define CMD_WRITE_VCOM_REG   0x2C
#define CMD_READ_OTP_REG   0x2D
#define CMD_READ_USER_ID   0x2E
#define CMD_PROG_WS_OTP   0x30
#define CMD_LOAD_WS_OTP   0x31
#define CMD_WRITE_LUT_REG   0x32
#define CMD_PROG_OTP_SEL   0x36
#define CMD_WRITE_USER_ID  0x38
#define CMD_PROG_OTP_MODE  0x39
#define CMD_LUT_3F  0x3F
#define CMD_WRITE_RAM_X_COORDS  0x44
#define CMD_WRITE_RAM_Y_COORDS  0x45
#define CMD_WRITE_RAM_X_ADDR_CTR  0x4E
#define CMD_WRITE_RAM_Y_ADDR_CTR  0x4F


// each command has a number of data bytes following it. 




// fwds

void chip_reset(ws29v2_ctx_t *chip);

void on_command(ws29v2_ctx_t *chip);
void on_timer_event(void *data);
void on_reset_timer(void *data);                                        // reset timer expired
void on_pin_change(void *user_data, pin_t pin, uint32_t value);
void on_reset_pin_change(void *user_data, pin_t pin, uint32_t value);   // reset pin has changed
void on_spi_done(void *user_data, uint8_t *buffer, uint32_t count);
void start_spi_transfer(ws29v2_ctx_t *chip, spi_state_t st, uint32_t count);


void on_cmd_driver_output_ctl(ws29v2_ctx_t *chip);
void on_cmd_gate_output_ctl(ws29v2_ctx_t *chip);
void on_cmd_src_output_ctl(ws29v2_ctx_t *chip);
void on_cmd_deep_sleep(ws29v2_ctx_t *chip);
void on_cmd_data_entry_mode(ws29v2_ctx_t *chip);
void on_cmd_sw_reset(ws29v2_ctx_t *chip);
void on_cmd_master_activation(ws29v2_ctx_t *chip);
void on_cmd_disp_upd_ctl(ws29v2_ctx_t *chip);
void on_cmd_disp_upd_ctl2(ws29v2_ctx_t *chip);
void on_cmd_write_ram_bw(ws29v2_ctx_t *chip);
void on_cmd_write_ram_red(ws29v2_ctx_t *chip);
void on_cmd_vcom_sense(ws29v2_ctx_t *chip);
void on_cmd_vcom_sense_dur(ws29v2_ctx_t *chip);
void on_cmd_prog_vcom_otp(ws29v2_ctx_t *chip);
void on_cmd_vcom_reg_ctl(ws29v2_ctx_t *chip);
void on_cmd_write_vcom_reg(ws29v2_ctx_t *chip);
void on_cmd_read_otp_reg(ws29v2_ctx_t *chip);
void on_cmd_read_user_id(ws29v2_ctx_t *chip);
void on_cmd_prog_ws_otp(ws29v2_ctx_t *chip);
void on_cmd_load_ws_otp(ws29v2_ctx_t *chip);
void on_cmd_write_lut_reg(ws29v2_ctx_t *chip);
void on_cmd_prog_otp_sel(ws29v2_ctx_t *chip);
void on_cmd_write_user_id(ws29v2_ctx_t *chip);
void on_cmd_prog_otp_mode(ws29v2_ctx_t *chip);
void on_cmd_write_ram_x_coords(ws29v2_ctx_t *chip);
void on_cmd_write_ram_y_coords(ws29v2_ctx_t *chip);
void on_cmd_write_ram_x_addr_ctr(ws29v2_ctx_t *chip);
void on_cmd_write_ram_y_addr_ctr(ws29v2_ctx_t *chip);

void on_cmd_lut_3f(ws29v2_ctx_t *chip);



#endif // __WS29V2_H__
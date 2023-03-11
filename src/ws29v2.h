#ifndef __WS29V2_H__
#define __WS29V2_H__

#include "wokwi-api.h"

#define DEBUG
#include "debug.h"
#include "hashmap.h"



#define MAX_DATA_TXFER 8192

typedef enum {
    ST_SPI_WAIT_CMD,
    ST_SPI_WAIT_DATA,
    ST_SPI_DONE
} spi_state_t;

typedef enum {
    AM_BLACK,
    AM_WHITE,
    AM_BW,
    AM_INV_BW,
    AM_RBW,
    AM_INV_RBW,
    AM_FADE_1,
    AM_FADE_2,
    AM_FADE_3,
    AM_SCAN_LINE_BW,
    AM_SCAN_LINE_RBW,

} activation_mode_t;

typedef struct {
    activation_mode_t *seq;
    size_t len;
} act_mode_len_t;


typedef struct {
    // attrs
    bool debug;
    uint8_t debug_mask;
    uint32_t version;

    // spi comms
    uint8_t cmd;
    uint16_t data_ndx;
    uint8_t buffer[1];
    uint8_t data[MAX_DATA_TXFER];
    uint8_t mode;       // SPI mode depending on dc pin value

    // pins
    pin_t busy;
    pin_t reset;
    pin_t dc;
    pin_t cs;
    pin_t clk;
    pin_t sdi;
    pin_t vcc;

    bool sleeping;      // are we in deep sleep. SPI disabled when on (!?)

    // display variables
    uint8_t addr_incr_mode;
    uint16_t x_addr;
    uint16_t y_addr;

    uint16_t x_min, x_max;
    uint16_t y_min, y_max;

    uint8_t disp_upd_seq;

    // display RAM
    uint8_t bw_ram[MAX_DATA_TXFER];
    uint8_t red_ram[MAX_DATA_TXFER];     // v2 only

    // activation
    uint32_t act_mode;
    act_mode_len_t act_seq;
    uint16_t act_ndx;
    uint16_t act_scan_ndx;
    uint32_t *black;
    uint32_t *white;

    // otp buffer 
    uint8_t otp[10];

    spi_dev_t spi;
    spi_state_t spi_state;
    bool txfer;

    // timers
    timer_t timer;
    timer_t reset_timer;
    timer_t activation_timer;
    uint64_t reset_time;

    // display
    buffer_t frame_buf;
    uint32_t width;
    uint32_t height;



} ws29v2_ctx_t;


typedef void (*cmd_handler)(ws29v2_ctx_t *chip);
typedef struct {
    uint16_t cmd_key;
    const char *cmd_name;
    cmd_handler handler;
} cmd_entry_t;
typedef HASHMAP(uint16_t, cmd_entry_t) cmd_map_t;
#define CMD_ENTRY(cmd, h) {cmd, #cmd, h}



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
#define ACTIVATION_STEP_PERIOD 500000
#define ACTIVATION_SCAN_LINE_PERIOD 500

// ADDR INCR modes
#define ADDR_INCR_X_MASK 0x1
#define ADDR_INCR_Y_MASK 0x2
#define ADDR_INCR_AXIS_MASK 0x4 // 0 -> incr X addr, 1 -> incr y
#define ADDR_INCR_MASK (ADDR_INCR_X_MASK | ADDR_INCR_Y_MASK|ADDR_INCR_AXIS_MASK)
#define ADDR_INCR_MODE_POR 0x11 // incr addresses +ve and default to x axis

// disp upd seq action
#define DISP_UPDATE_SEQ_POR    0xFF


// colour constants used when displaying the panel colours using the frame buffer
#define FB_BLACK 0x000000FF
#define FB_WHITE 0xFFFFFFFF
#define FB_RED 0xFF0000FF


// COMMAND CODES
#define CMD_DRIVER_OUTPUT_CTL   0x01
#define CMD_GATE_OUTPUT_CTL   0x03
#define CMD_SRC_OUTPUT_CTL   0x04
#define CMD_BOOSTER_SOFT_START   0x0C
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
#define CMD_WRITE_OTP_DATA   0x37
#define CMD_WRITE_USER_ID  0x38
#define CMD_PROG_OTP_MODE  0x39
#define CMD_DUMMY_LINE_3A  0x3A
#define CMD_GATE_WIDTH_3B  0x3B
#define CMD_BORDER_WF  0x3C
#define CMD_LUT_3F  0x3F
#define CMD_WRITE_RAM_X_COORDS  0x44
#define CMD_WRITE_RAM_Y_COORDS  0x45
#define CMD_WRITE_RAM_X_ADDR_CTR  0x4E
#define CMD_WRITE_RAM_Y_ADDR_CTR  0x4F
#define CMD_NOP  0xFF




// each command has a number of data bytes following it. 




// fwds

void chip_reset(ws29v2_ctx_t *chip);

void cmd_init_hash();

void on_command(ws29v2_ctx_t *chip);
void on_timer_event(void *data);
void on_activation_step(void *data);
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
void on_cmd_write_otp_data(ws29v2_ctx_t *chip);
void on_cmd_prog_otp_mode(ws29v2_ctx_t *chip);
void on_cmd_write_ram_x_coords(ws29v2_ctx_t *chip);
void on_cmd_write_ram_y_coords(ws29v2_ctx_t *chip);
void on_cmd_write_ram_x_addr_ctr(ws29v2_ctx_t *chip);
void on_cmd_write_ram_y_addr_ctr(ws29v2_ctx_t *chip);

void on_cmd_lut_3f(ws29v2_ctx_t *chip);
void on_cmd_nop(ws29v2_ctx_t *chip);


extern uint8_t cmd_data_bytes[];
extern act_mode_len_t activation_modes[2][3];


#endif // __WS29V2_H__
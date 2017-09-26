#include "dw1000.h"
#include "dw1000_internal.h"
#include <hal.h>
#include <ch.h>
#include <common/timing.h>
#include <common/uavcan.h>
#include <bswap.h>

#include <string.h>
#include <stdio.h>

static void dw1000_config(struct dw1000_instance_s* instance);
static void dw1000_write(struct dw1000_instance_s* instance, uint8_t regfile, uint16_t reg, uint32_t len, const void* buf);
static void dw1000_read(struct dw1000_instance_s* instance, uint8_t regfile, uint16_t reg, uint32_t len, void* buf);
static void dw1000_hard_reset(struct dw1000_instance_s* instance);
static void dw1000_clear_double_buffered_status_bits_and_optionally_disable_transceiver(struct dw1000_instance_s* instance, bool transceiver_disable);
static void dw1000_clear_double_buffered_status_bits(struct dw1000_instance_s* instance);
static void dw1000_disable_transceiver(struct dw1000_instance_s* instance);
static void dw1000_swap_rx_buffers(struct dw1000_instance_s* instance);
static void dw1000_write8(struct dw1000_instance_s* instance, uint8_t regfile, uint16_t reg, uint8_t val);
static void dw1000_write16(struct dw1000_instance_s* instance, uint8_t regfile, uint16_t reg, uint16_t val);
static void dw1000_write32(struct dw1000_instance_s* instance, uint8_t regfile, uint16_t reg, uint32_t val);
static void dw1000_otp_read(struct dw1000_instance_s* instance, uint16_t otp_addr, uint8_t otp_field_len, void* buf);
static void dw1000_load_ldotune(struct dw1000_instance_s* instance);
static void dw1000_load_lde_microcode(struct dw1000_instance_s* instance);
static void dw1000_clock_force_sys_xti(struct dw1000_instance_s* instance);
static void dw1000_clock_enable_all_seq(struct dw1000_instance_s* instance);

static uint8_t dw1000_get_fs_plltune_val(enum dw1000_channel_t channel) {
    switch(channel) {
        case DW1000_CHANNEL_1:
            return 0x1E;
        case DW1000_CHANNEL_2:
        case DW1000_CHANNEL_4:
            return 0x26;
        case DW1000_CHANNEL_3:
            return 0x56;
        case DW1000_CHANNEL_5:
        case DW1000_CHANNEL_7:
            return 0xBE;
    }
    return 0;
}

static uint8_t dw1000_get_tc_pgdelay_val(enum dw1000_channel_t channel) {
    switch(channel) {
        case DW1000_CHANNEL_1:
            return 0xC9;
        case DW1000_CHANNEL_2:
            return 0xC2;
        case DW1000_CHANNEL_3:
            return 0xC5;
        case DW1000_CHANNEL_4:
            return 0x95;
        case DW1000_CHANNEL_5:
            return 0xC0;
        case DW1000_CHANNEL_7:
            return 0x93;
    }
    return 0;
}

static uint32_t dw1000_get_rf_txctrl_val(enum dw1000_channel_t channel) {
    switch(channel) {
        case DW1000_CHANNEL_1:
            return 0x00005C40;
        case DW1000_CHANNEL_2:
            return 0x00045CA0;
        case DW1000_CHANNEL_3:
            return 0x00086CC0;
        case DW1000_CHANNEL_4:
            return 0x00045C80;
        case DW1000_CHANNEL_5:
            return 0x001E3FE0;
        case DW1000_CHANNEL_7:
            return 0x001E7DE0;
    }
    return 0;
}

static uint32_t dw1000_get_tx_power_val_prf_16(enum dw1000_channel_t channel) {
    switch(channel) {
        case DW1000_CHANNEL_1:
        case DW1000_CHANNEL_2:
            return 0x15355575;
        case DW1000_CHANNEL_3:
            return 0x0F2F4F6F;
        case DW1000_CHANNEL_4:
            return 0x1F1F3F5F;
        case DW1000_CHANNEL_5:
            return 0x0E082848;
        case DW1000_CHANNEL_7:
            return 0x32527292;
    }
    return 0;
}

static uint32_t dw1000_get_tx_power_val_prf_64(enum dw1000_channel_t channel) {
    switch(channel) {
        case DW1000_CHANNEL_1:
        case DW1000_CHANNEL_2:
            return 0x07274767;
        case DW1000_CHANNEL_3:
            return 0x2B4B6B8B;
        case DW1000_CHANNEL_4:
            return 0x3A5A7A9A;
        case DW1000_CHANNEL_5:
            return 0x25456585;
        case DW1000_CHANNEL_7:
            return 0x5171B1D1;
    }
    return 0;
}

static uint32_t dw1000_get_tx_power_val(enum dw1000_prf_t prf, enum dw1000_channel_t channel) {
    switch(prf) {
        case DW1000_PRF_16MHZ:
            return dw1000_get_tx_power_val_prf_16(channel);
        case DW1000_PRF_64MHZ:
            return dw1000_get_tx_power_val_prf_64(channel);
    }
    return 0;
}

static uint32_t dw1000_get_drx_tune2_val_prf_16(enum dw1000_preamble_t preamble) {
    switch(preamble) {
        case DW1000_PREAMBLE_64:
        case DW1000_PREAMBLE_128:
            // PAC size 8
            return 0x311A002D;
        case DW1000_PREAMBLE_256:
        case DW1000_PREAMBLE_512:
            // PAC size 16
            return 0x331A0052;
        case DW1000_PREAMBLE_1024:
            // PAC size 32
            return 0x351A009A;
        case DW1000_PREAMBLE_1536:
        case DW1000_PREAMBLE_2048:
        case DW1000_PREAMBLE_4096:
            // PAC size 64
            return 0x371A011D;
    }
    return 0;
}

static uint16_t dw1000_get_drx_tune1a_val(enum dw1000_prf_t prf) {
    switch(prf) {
        case DW1000_PRF_16MHZ:
            return 0x0087;
        case DW1000_PRF_64MHZ:
            return 0x008D;
    }
    return 0;
}

static uint32_t dw1000_get_drx_tune2_val_prf_64(enum dw1000_preamble_t preamble) {
    switch(preamble) {
        case DW1000_PREAMBLE_64:
        case DW1000_PREAMBLE_128:
            return 0x313B006B;
        case DW1000_PREAMBLE_256:
        case DW1000_PREAMBLE_512:
            return 0x333B00BE;
        case DW1000_PREAMBLE_1024:
            return 0x353B015E;
        case DW1000_PREAMBLE_1536:
        case DW1000_PREAMBLE_2048:
        case DW1000_PREAMBLE_4096:
            return 0x373B0296;
    }
    return 0;
}

static uint32_t dw1000_get_drx_tune2_val(enum dw1000_prf_t prf, enum dw1000_preamble_t preamble) {
    switch(prf) {
        case DW1000_PRF_16MHZ:
            return dw1000_get_drx_tune2_val_prf_16(preamble);
        case DW1000_PRF_64MHZ:
            return dw1000_get_drx_tune2_val_prf_64(preamble);
    }
    return 0;
}

static uint16_t dw1000_get_agc_tune1_val(enum dw1000_prf_t prf) {
    switch(prf) {
        case DW1000_PRF_16MHZ:
            return 0x8870;
        case DW1000_PRF_64MHZ:
            return 0x889B;
    }
    return 0;
}

static uint16_t dw1000_get_lde_cfg2_val(enum dw1000_prf_t prf) {
    switch(prf) {
        case DW1000_PRF_16MHZ:
            return 0x1607;
        case DW1000_PRF_64MHZ:
            return 0x0607;
    }
    return 0;
}

static struct dw1000_chan_ctrl_s dw1000_get_chan_ctrl_val_prf_16(enum dw1000_channel_t channel) {
    struct dw1000_chan_ctrl_s ret;
    memset(&ret, 0, sizeof(ret));
    ret.TX_CHAN = channel;
    ret.RX_CHAN = channel;
    ret.RXPRF = 0b01;

    switch(channel) {
        case DW1000_CHANNEL_1:
            ret.TX_PCODE = 1;
            ret.RX_PCODE = 1;
            break;
        case DW1000_CHANNEL_2:
        case DW1000_CHANNEL_5:
            ret.TX_PCODE = 4;
            ret.RX_PCODE = 4;
            break;
        case DW1000_CHANNEL_3:
            ret.TX_PCODE = 5;
            ret.RX_PCODE = 5;
            break;
        case DW1000_CHANNEL_4:
        case DW1000_CHANNEL_7:
            ret.TX_PCODE = 7;
            ret.RX_PCODE = 7;
            break;
    }
    return ret;
}

static struct dw1000_chan_ctrl_s dw1000_get_chan_ctrl_val_prf_64(enum dw1000_channel_t channel) {
    struct dw1000_chan_ctrl_s ret;
    memset(&ret, 0, sizeof(ret));
    ret.TX_CHAN = channel;
    ret.RX_CHAN = channel;
    ret.RXPRF = 0b10;
    switch(channel) {
        case DW1000_CHANNEL_1:
        case DW1000_CHANNEL_2:
        case DW1000_CHANNEL_3:
        case DW1000_CHANNEL_5:
            ret.TX_PCODE = 9;
            ret.RX_PCODE = 9;
            break;
        case DW1000_CHANNEL_4:
        case DW1000_CHANNEL_7:
            ret.TX_PCODE = 17;
            ret.RX_PCODE = 17;
            break;
    }

    return ret;
}

static struct dw1000_chan_ctrl_s dw1000_get_chan_ctrl_val(enum dw1000_prf_t prf, enum dw1000_channel_t channel) {
    switch (prf) {
        case DW1000_PRF_16MHZ:
            return dw1000_get_chan_ctrl_val_prf_16(channel);
        case DW1000_PRF_64MHZ:
        default:
            return dw1000_get_chan_ctrl_val_prf_64(channel);
    }
}

static struct dw1000_tx_fctrl_s dw1000_get_tx_fctrl_val(enum dw1000_prf_t prf, enum dw1000_preamble_t preamble) {
    struct dw1000_tx_fctrl_s ret;
    memset(&ret, 0, sizeof(ret));
    ret.TXBR = 0b10;
    switch (prf) {
        case DW1000_PRF_16MHZ:
            ret.TXPRF = 0b01;
            break;
        case DW1000_PRF_64MHZ:
            ret.TXPRF = 0b10;
            break;
    }

    switch(preamble) {
        case DW1000_PREAMBLE_64:
            ret.TXPSR = 0b01;
            ret.PE =    0b00;
            break;
        case DW1000_PREAMBLE_128:
            ret.TXPSR = 0b01;
            ret.PE =    0b01;
            break;
        case DW1000_PREAMBLE_256:
            ret.TXPSR = 0b01;
            ret.PE =    0b10;
            break;
        case DW1000_PREAMBLE_512:
            ret.TXPSR = 0b01;
            ret.PE =    0b11;
            break;
        case DW1000_PREAMBLE_1024:
            ret.TXPSR = 0b10;
            ret.PE =    0b00;
            break;
        case DW1000_PREAMBLE_1536:
            ret.TXPSR = 0b10;
            ret.PE =    0b01;
            break;
        case DW1000_PREAMBLE_2048:
            ret.TXPSR = 0b10;
            ret.PE =    0b10;
            break;
        case DW1000_PREAMBLE_4096:
            ret.TXPSR = 0b11;
            ret.PE =    0b00;
            break;
    }

    return ret;
}

void dw1000_init(struct dw1000_instance_s* instance, uint8_t spi_idx, uint32_t select_line, uint32_t reset_line) {
    if (!instance) {
        return;
    }

    // NOTE: Per DW1000 User Manual Section 2.3.2:
    // SPI accesses from an external microcontroller are possible in the INIT state, but these
    // are limited to a SPICLK input frequency of no greater than 3 MHz. Care should be taken
    // not to have an active SPI access in progress at the CLKPLL lock time (i.e. at t = 5 µs) when
    // the automatic switch from the INIT state to the IDLE state is occurring, because the
    // switch-over of clock source can cause bit errors in the SPI transactions.

    // This is handled by sleeping inside of dw1000_hard_reset

    spi_device_init(&instance->spi_dev, spi_idx, select_line, 3000000, 8, 0);
    instance->reset_line = reset_line;
    dw1000_hard_reset(instance);

    // NOTE: Per DW1000 API: this makes OTP read reliable
    dw1000_clock_force_sys_xti(instance);

    // DW1000 User Manual Section 2.5.5.10
    dw1000_load_lde_microcode(instance);

    // DW1000 User Manual Section 2.5.5.11
    dw1000_load_ldotune(instance);

    dw1000_clock_enable_all_seq(instance);

    dw1000_config(instance);
}

static void dw1000_config(struct dw1000_instance_s* instance) {
    if (!instance) {
        return;
    }

    const enum dw1000_prf_t prf = DW1000_PRF_64MHZ;
    const enum dw1000_preamble_t preamble = DW1000_PREAMBLE_128;
    const enum dw1000_channel_t channel = DW1000_CHANNEL_7;

    // TODO accept parameter(s) for channel, power, and other configurations

    // DW1000 User Manual Section 2.5.5.1
    dw1000_write16(instance, 0x23, 0x04, dw1000_get_agc_tune1_val(prf));

    // DW1000 User Manual Section 2.5.5.2
    dw1000_write32(instance, 0x23, 0x0C, 0x2502A907);

    // DW1000 User Manual Section 2.5.5.3
    dw1000_write32(instance, 0x27, 0x08, dw1000_get_drx_tune2_val(prf, preamble));

    // DW1000 User Manual Section 2.5.5.4
    dw1000_write8(instance, 0x2E, 0x0806, 0x0D);

    // DW1000 User Manual Section 2.5.5.5
    dw1000_write16(instance, 0x2E, 0x1806, dw1000_get_lde_cfg2_val(prf));

    // DW1000 User Manual Section 2.5.5.6
    // NOTE: will need to change for channel != 5, refer to Table 19
    dw1000_write32(instance, 0x1E, 0x00, dw1000_get_tx_power_val(prf, channel));

    // DW1000 User Manual Section 2.5.5.7
    // NOTE: will need to change for channel != 5, refer to Table 36
    dw1000_write32(instance, 0x28, 0x0C, dw1000_get_rf_txctrl_val(channel));

    // DW1000 User Manual Section 2.5.5.8
    // NOTE: will need to change for channel != 5, refer to Table 38
    dw1000_write8(instance, 0x2A, 0x0B, dw1000_get_tc_pgdelay_val(channel));

    // DW1000 User Manual Section 2.5.5.9
    // NOTE: will need to change for channel != 5 or 7, refer to Table 42
    dw1000_write8(instance, 0x2B, 0x0B, dw1000_get_fs_plltune_val(channel));

    dw1000_write16(instance, 0x27, 0x04, dw1000_get_drx_tune1a_val(prf));

    // Set channel control parameters
    {
        struct dw1000_chan_ctrl_s chan_ctrl = dw1000_get_chan_ctrl_val(prf, channel);
        dw1000_write(instance, 0x1F, 0x00, sizeof(chan_ctrl), &chan_ctrl);
    }

    // Set xtal trim to mid value
//     dw1000_write8(instance, 0x2B, 0x0E, 0x6F);

    // Enable rx auto-re-enable and rx double-buffering
    {
        struct dw1000_sys_cfg_s sys_cfg;
        memset(&sys_cfg, 0, sizeof(sys_cfg));
        sys_cfg.HIRQ_POL = 1;
        sys_cfg.RXAUTR = 1;
        dw1000_write(instance, DW1000_SYSTEM_CONFIGURATION_FILE, 0, sizeof(sys_cfg), &sys_cfg);
    }

    // Configure tx parameters
    {
        struct dw1000_tx_fctrl_s tx_fctrl = dw1000_get_tx_fctrl_val(prf, preamble);
        dw1000_write(instance, DW1000_TRANSMIT_FRAME_CONTROL_FILE, 0, sizeof(tx_fctrl), &tx_fctrl);
    }
}

void dw1000_rx_enable(struct dw1000_instance_s* instance) {
    if (!instance) {
        return;
    }

    struct dw1000_sys_status_s sys_status;

    // Read SYS_STATUS to check that HSRBP == ICRBP
    dw1000_read(instance, DW1000_SYSTEM_EVENT_STATUS_REGISTER_FILE, 0, sizeof(sys_status), &sys_status);

    if (sys_status.HSRBP != sys_status.ICRBP) {
        // Issue the HRBPT command
        dw1000_swap_rx_buffers(instance);
    }

    // Set RXENAB bit
    struct dw1000_sys_ctrl_s sys_ctrl;
    memset(&sys_ctrl, 0, sizeof(sys_ctrl));
    sys_ctrl.RXENAB = 1;
    dw1000_write(instance, DW1000_SYSTEM_CONTROL_REGISTER_FILE, 0, sizeof(sys_ctrl), &sys_ctrl);
}

struct dw1000_rx_frame_info_s dw1000_receive(struct dw1000_instance_s* instance, uint32_t buf_len, void* buf) {
    struct dw1000_rx_frame_info_s ret;
    memset(&ret,0,sizeof(ret));
    if (!instance || !buf) {
        ret.err_code = DW1000_RX_ERROR_NULLPTR;
        return ret;
    }

    // DW1000 User Manual Figure 14

    struct dw1000_sys_status_s sys_status;
    struct dw1000_rx_finfo_s rx_finfo;

    // Read SYS_STATUS
    dw1000_read(instance, DW1000_SYSTEM_EVENT_STATUS_REGISTER_FILE, 0, sizeof(sys_status), &sys_status);

    // Check if a good frame is in the buffer
    if (!sys_status.RXFCG) {
        ret.err_code = DW1000_RX_ERROR_NO_FRAME_PRESENT;
        return ret;
    }

    // Read RX_FINFO
    dw1000_read(instance, DW1000_RX_FRAME_INFORMATION_REGISTER_FILE, 0, sizeof(rx_finfo), &rx_finfo);

    // Check if the frame fits in the provided buffer
    if (rx_finfo.RXFLEN-2 <= buf_len) {
        // Read RX_BUFFER
        dw1000_read(instance, DW1000_RX_FRAME_BUFFER_FILE, 0, rx_finfo.RXFLEN-2, buf);
        ret.len = rx_finfo.RXFLEN-2;
    } else {
        ret.err_code = DW1000_RX_ERROR_PROVIDED_BUFFER_TOO_SMALL;
    }

    // Read RX_TIME
    dw1000_read(instance, 0x15, 0, 5, &ret.timestamp);
    dw1000_read(instance, 0x13, 0, 4, &ret.rx_ttcki);
    dw1000_read(instance, 0x14, 0, 4, &ret.rx_ttcko);
    ret.rx_ttcko &= ((1<<19)-1);
    if (((ret.rx_ttcko>>18)&1) != 0) {
        // extend the sign bit
        ret.rx_ttcko |= ~((1<<19)-1);
    }

    // Read SYS_STATUS
    dw1000_read(instance, DW1000_SYSTEM_EVENT_STATUS_REGISTER_FILE, 0, sizeof(sys_status), &sys_status);

    // Check RXOVRR
    if (sys_status.RXOVRR) {
        // Frames must be discarded (do not read frames) due to corrupted registers and TRXOFF command issued
        dw1000_disable_transceiver(instance);

        // Receiver must be reset to exit errored state
        dw1000_rx_enable(instance);

        memset(&ret,0,sizeof(ret));
        ret.err_code = DW1000_RX_ERROR_RXOVRR;
        return ret;
    }

    // Check HSRBP==ICRBP
    if (sys_status.HSRBP == sys_status.ICRBP) {
        // Mask, clear and unmask RX event flags in SYS_STATUS reg:0F; bits FCE, FCG, DFR, LDE_DONE
        dw1000_clear_double_buffered_status_bits(instance);
    }

    // Issue the HRBPT command
    dw1000_swap_rx_buffers(instance);

    return ret;
}

void dw1000_transmit(struct dw1000_instance_s* instance, uint32_t buf_len, void* buf) {
    if (!instance) {
        return;
    }

    if (buf_len > 125) {
        return;
    }

    // Write tx data to data buffer
    dw1000_write(instance, DW1000_TRANSMIT_DATA_BUFFER_FILE, 0, buf_len, buf);

    // Configure tx parameters
    struct dw1000_tx_fctrl_s tx_fctrl;
    dw1000_read(instance, DW1000_TRANSMIT_FRAME_CONTROL_FILE, 0, sizeof(tx_fctrl), &tx_fctrl);
    tx_fctrl.reserved = 0;
    tx_fctrl.TFLEN = buf_len+2;
    dw1000_write(instance, DW1000_TRANSMIT_FRAME_CONTROL_FILE, 0, sizeof(tx_fctrl), &tx_fctrl);

    // Start tx
    uint32_t sys_ctrl = 1<<1;
    dw1000_write(instance, DW1000_SYSTEM_CONTROL_REGISTER_FILE, 0, sizeof(sys_ctrl), &sys_ctrl);
}

void dw1000_handle_interrupt(struct dw1000_instance_s* instance) {
    if (!instance) {
        return;
    }

}

static void dw1000_clock_force_sys_xti(struct dw1000_instance_s* instance) {
    if (!instance) {
        return;
    }

    uint8_t reg[2];
    spi_device_set_max_speed_hz(&instance->spi_dev, 3000000);
    dw1000_write8(instance, 0x36, 0, 0x01 | (reg[0]&0xfc));
    dw1000_write8(instance, 0x36, 1, reg[1]);
    chThdSleepMicroseconds(10);
}

static void dw1000_clock_enable_all_seq(struct dw1000_instance_s* instance) {
    if (!instance) {
        return;
    }

    uint8_t reg[2];
    spi_device_set_max_speed_hz(&instance->spi_dev, 3000000);
    dw1000_read(instance, 0x36, 0, 2, reg);
    dw1000_write8(instance, 0x36, 0, 0x00);
    dw1000_write8(instance, 0x36, 1, reg[1]&0xfe);
    spi_device_set_max_speed_hz(&instance->spi_dev, 20000000);
    chThdSleepMicroseconds(10);
}

static void dw1000_load_lde_microcode(struct dw1000_instance_s* instance) {
    if (!instance) {
        return;
    }

    // DW1000 User Manual Section 2.5.5.10
    dw1000_write16(instance, 0x36, 0x00, 0x0301);
    spi_device_set_max_speed_hz(&instance->spi_dev, 3000000);
    dw1000_write16(instance, 0x2D, 0x06, 0x8000);
    chThdSleepMicroseconds(150);
    dw1000_write16(instance, 0x36, 0x00, 0x0200);
    spi_device_set_max_speed_hz(&instance->spi_dev, 20000000);
}

static void dw1000_load_ldotune(struct dw1000_instance_s* instance) {
    if (!instance) {
        return;
    }

    // DW1000 User Manual Section 2.5.5.11
    uint8_t ldotune_cal[5];
    dw1000_otp_read(instance, 0x04, 4, &ldotune_cal[0]);
    dw1000_otp_read(instance, 0x05, 1, &ldotune_cal[4]);
    if (ldotune_cal[0] != 0) {
        dw1000_write16(instance, 0x2C, 0x00, (1<<11)|(1<<12));
        dw1000_write(instance, 0x28, 0x30, sizeof(ldotune_cal), ldotune_cal);
    } else {
        dw1000_write16(instance, 0x2C, 0x00, (1<<11));
    }
}

static void dw1000_clear_double_buffered_status_bits_and_optionally_disable_transceiver(struct dw1000_instance_s* instance, bool transceiver_disable) {
    if (!instance) {
        return;
    }

    struct dw1000_sys_mask_s sys_mask_prev;
    struct dw1000_sys_mask_s sys_mask;
    struct dw1000_sys_ctrl_s sys_ctrl;
    struct dw1000_sys_status_s sys_status;

    // Read SYS_MASK
    dw1000_read(instance, DW1000_SYSTEM_EVENT_MASK_REGISTER_FILE, 0, sizeof(sys_mask), &sys_mask);
    sys_mask_prev = sys_mask;

    // Mask double buffered status bits FCE, FCG, DFR, LDE_DONE
    sys_mask.MRXFCE = 0;
    sys_mask.MRXFCG = 0;
    sys_mask.MRXDFR = 0;
    sys_mask.MLDEDONE = 0;

    // Write SYS_MASK
    dw1000_write(instance, DW1000_SYSTEM_EVENT_MASK_REGISTER_FILE, 0, sizeof(sys_mask), &sys_mask);

    if (transceiver_disable) {
        // Issue TRXOFF
        memset(&sys_ctrl, 0, sizeof(sys_ctrl));
        sys_ctrl.TRXOFF = 1;

        // Write SYS_CTRL
        dw1000_write(instance, DW1000_SYSTEM_CONTROL_REGISTER_FILE, 0, sizeof(sys_ctrl), &sys_ctrl);
    }

    // Clear double buffered status bits FCE, FCG, DFR, LDE_DONE
    memset(&sys_status, 0, sizeof(sys_status));
    sys_status.RXFCE = 1;
    sys_status.RXFCG = 1;
    sys_status.RXDFR = 1;
    sys_status.LDEDONE = 1;

    // Write SYS_STATUS
    dw1000_write(instance, DW1000_SYSTEM_EVENT_STATUS_REGISTER_FILE, 0, sizeof(sys_status), &sys_status);

    // Unmask double buffered status bits FCE, FCG, DFR, LDE_DONE
    sys_mask = sys_mask_prev;

    // Write SYS_MASK
    dw1000_write(instance, DW1000_SYSTEM_EVENT_MASK_REGISTER_FILE, 0, sizeof(sys_mask), &sys_mask);
}

static void dw1000_clear_double_buffered_status_bits(struct dw1000_instance_s* instance) {
    dw1000_clear_double_buffered_status_bits_and_optionally_disable_transceiver(instance, false);
}

static void dw1000_disable_transceiver(struct dw1000_instance_s* instance) {
    dw1000_clear_double_buffered_status_bits_and_optionally_disable_transceiver(instance, true);
}

static void dw1000_swap_rx_buffers(struct dw1000_instance_s* instance) {
    // Issue the HRBPT command
    struct dw1000_sys_ctrl_s sys_ctrl;
    memset(&sys_ctrl,0,sizeof(sys_ctrl));
    sys_ctrl.HRBPT = 1;
    dw1000_write(instance, DW1000_SYSTEM_CONTROL_REGISTER_FILE, 0, sizeof(sys_ctrl), &sys_ctrl);
}

static void dw1000_hard_reset(struct dw1000_instance_s* instance) {
    if (!instance) {
        return;
    }

    palClearLine(instance->reset_line);
    palSetLineMode(instance->reset_line, PAL_MODE_OUTPUT_PUSHPULL | PAL_STM32_OSPEED_HIGHEST);
    chThdSleepMilliseconds(2);
    palSetLineMode(instance->reset_line, 0);
    chThdSleepMilliseconds(2);
}

static void dw1000_write8(struct dw1000_instance_s* instance, uint8_t regfile, uint16_t reg, uint8_t val) {
    dw1000_write(instance, regfile, reg, sizeof(val), &val);
}

static void dw1000_write16(struct dw1000_instance_s* instance, uint8_t regfile, uint16_t reg, uint16_t val) {
    dw1000_write(instance, regfile, reg, sizeof(val), &val);
}

static void dw1000_write32(struct dw1000_instance_s* instance, uint8_t regfile, uint16_t reg, uint32_t val) {
    dw1000_write(instance, regfile, reg, sizeof(val), &val);
}

static void dw1000_write(struct dw1000_instance_s* instance, uint8_t regfile, uint16_t reg, uint32_t len, const void* buf) {
    if (!instance || !buf) {
        return;
    }

    struct __attribute__((packed)) dw1000_transaction_header_s header;
    header.subidx_present = 1;
    header.write = 1;
    header.extended_address = 1;
    header.regfile = regfile;
    header.addressl = reg;
    header.addressh = reg>>7;

    spi_device_begin(&instance->spi_dev);
    spi_device_send(&instance->spi_dev, sizeof(header), &header);
    spi_device_send(&instance->spi_dev, len, buf);
    spi_device_end(&instance->spi_dev);
}

static void dw1000_read(struct dw1000_instance_s* instance, uint8_t regfile, uint16_t reg, uint32_t len, void* buf) {
    if (!instance || !buf) {
        return;
    }

    struct __attribute__((packed)) dw1000_transaction_header_s header;
    header.subidx_present = 1;
    header.write = 0;
    header.extended_address = 1;
    header.regfile = regfile;
    header.addressl = reg;
    header.addressh = reg>>7;

    spi_device_begin(&instance->spi_dev);
    spi_device_send(&instance->spi_dev, sizeof(header), &header);
    spi_device_receive(&instance->spi_dev, len, buf);
    spi_device_end(&instance->spi_dev);
}

static void dw1000_otp_read(struct dw1000_instance_s* instance, uint16_t otp_addr, uint8_t otp_field_len, void* buf) {
    if (!instance || !buf) {
        return;
    }

    // NOTE: Per DW1000 API code, system clock needs to be XTI - this is necessary to make sure the values read from OTP are reliable

    // DW1000 User Manual Section 6.3.3
    dw1000_write16(instance, 0x2D, 0x04, otp_addr);
    dw1000_write8(instance, 0x2D, 0x06, 0x03);
    dw1000_write8(instance, 0x2D, 0x06, 0x01);
    dw1000_read(instance, 0x2A, 0x04, otp_field_len, buf);
    dw1000_write8(instance, 0x2D, 0x06, 0x00);
}

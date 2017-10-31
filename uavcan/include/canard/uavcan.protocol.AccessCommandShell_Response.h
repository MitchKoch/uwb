#pragma once
#include <canard.h>
#include <stdbool.h>
#include <stdint.h>
#include <common/uavcan.h>


#define UAVCAN_PROTOCOL_ACCESSCOMMANDSHELL_RES_MAX_PACK_SIZE 263
#define UAVCAN_PROTOCOL_ACCESSCOMMANDSHELL_RES_DT_SIG 0x59276B5921C9246E
#define UAVCAN_PROTOCOL_ACCESSCOMMANDSHELL_RES_DT_ID 6

#define UAVCAN_PROTOCOL_ACCESSCOMMANDSHELL_RES_FLAG_RUNNING 1
#define UAVCAN_PROTOCOL_ACCESSCOMMANDSHELL_RES_FLAG_SHELL_ERROR 2
#define UAVCAN_PROTOCOL_ACCESSCOMMANDSHELL_RES_FLAG_HAS_PENDING_STDOUT 64
#define UAVCAN_PROTOCOL_ACCESSCOMMANDSHELL_RES_FLAG_HAS_PENDING_STDERR 128

struct uavcan_protocol_AccessCommandShell_res_s {
    int32_t last_exit_status;
    uint8_t flags;
    struct {uint16_t output_len; uint8_t output[256];};
};

extern const struct uavcan_message_descriptor_s uavcan_protocol_AccessCommandShell_res_descriptor;

uint32_t encode_uavcan_protocol_AccessCommandShell_res(struct uavcan_protocol_AccessCommandShell_res_s* msg, uint8_t* buffer);
uint32_t decode_uavcan_protocol_AccessCommandShell_res(const CanardRxTransfer* transfer, struct uavcan_protocol_AccessCommandShell_res_s* msg);
void _encode_uavcan_protocol_AccessCommandShell_res(uint8_t* buffer, uint32_t* bit_ofs, struct uavcan_protocol_AccessCommandShell_res_s* msg, bool tao);
void _decode_uavcan_protocol_AccessCommandShell_res(const CanardRxTransfer* transfer, uint32_t* bit_ofs, struct uavcan_protocol_AccessCommandShell_res_s* msg, bool tao);
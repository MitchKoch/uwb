#include <canard/uavcan.protocol.file.Read_Request.h>
#include <string.h>

static uint32_t encode_func(void* buffer, void* msg) {
    return encode_uavcan_protocol_file_Read_req(buffer, msg);
}

static uint32_t decode_func(CanardRxTransfer* transfer, void* msg) {
    return decode_uavcan_protocol_file_Read_req(transfer, msg);
}

const struct uavcan_message_descriptor_s uavcan_protocol_file_Read_req_descriptor = {
    UAVCAN_PROTOCOL_FILE_READ_REQ_DT_SIG,
    UAVCAN_PROTOCOL_FILE_READ_REQ_DT_ID,
    CanardTransferTypeRequest,
    sizeof(struct uavcan_protocol_file_Read_req_s),
    UAVCAN_PROTOCOL_FILE_READ_REQ_MAX_PACK_SIZE,
    encode_func,
    decode_func
};

uint32_t encode_uavcan_protocol_file_Read_req(struct uavcan_protocol_file_Read_req_s* msg, uint8_t* buffer) {
    uint32_t bit_ofs = 0;
    memset(buffer, 0, UAVCAN_PROTOCOL_FILE_READ_REQ_MAX_PACK_SIZE);
    _encode_uavcan_protocol_file_Read_req(buffer, &bit_ofs, msg, true);
    return (bit_ofs+7)/8;
}

uint32_t decode_uavcan_protocol_file_Read_req(const CanardRxTransfer* transfer, struct uavcan_protocol_file_Read_req_s* msg) {
    uint32_t bit_ofs = 0;
    _decode_uavcan_protocol_file_Read_req(transfer, &bit_ofs, msg, true);
    return (bit_ofs+7)/8;
}

void _encode_uavcan_protocol_file_Read_req(uint8_t* buffer, uint32_t* bit_ofs, struct uavcan_protocol_file_Read_req_s* msg, bool tao) {
    (void)buffer;
    (void)bit_ofs;
    (void)msg;
    (void)tao;

    canardEncodeScalar(buffer, *bit_ofs, 40, &msg->offset);
    *bit_ofs += 40;
    _encode_uavcan_protocol_file_Path(buffer, bit_ofs, &msg->path, tao);
}

void _decode_uavcan_protocol_file_Read_req(const CanardRxTransfer* transfer, uint32_t* bit_ofs, struct uavcan_protocol_file_Read_req_s* msg, bool tao) {
    (void)transfer;
    (void)bit_ofs;
    (void)msg;
    (void)tao;

    canardDecodeScalar(transfer, *bit_ofs, 40, false, &msg->offset);
    *bit_ofs += 40;
    _decode_uavcan_protocol_file_Path(transfer, bit_ofs, &msg->path, tao);
}
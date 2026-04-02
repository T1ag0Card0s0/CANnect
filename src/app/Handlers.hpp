#pragma once

#include "cannect/cants/CanTsProtocol.hpp"

#include <cstddef>
#include <cstdint>
#include <vector>

void printHexBytes(const uint8_t *data, size_t size);
void printHexBytes(const std::vector<uint8_t> &data);
void printAddress(const std::vector<uint8_t> &addr);

bool onTelecommand(uint8_t from, uint8_t ch, uint8_t data[CAN_FRAME_MAX_DATA]);
bool onTelemetry(uint8_t from, uint8_t ch, uint8_t data[CAN_FRAME_MAX_DATA]);
void onUnsolicitedTelemetry(uint8_t from, uint8_t ch, uint8_t data[CAN_FRAME_MAX_DATA]);
void onTimeSync(uint8_t from, uint8_t data[CAN_FRAME_MAX_DATA]);
bool onSetBlock(uint8_t from, uint8_t ch, std::vector<uint8_t> &data);
bool onGetBlock(uint8_t from, uint8_t ch, std::vector<uint8_t> &data);

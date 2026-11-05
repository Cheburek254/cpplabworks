#include "hamming.h"

#include <cstdint>

uint8_t HammingCodec::EncodeNibble(uint8_t nibble) {
  bool d1 = (nibble >> 0) & 1;
  bool d2 = (nibble >> 1) & 1;
  bool d3 = (nibble >> 2) & 1;
  bool d4 = (nibble >> 3) & 1;

  bool p1 = d1 ^ d2 ^ d4;
  bool p2 = d1 ^ d3 ^ d4;
  bool p4 = d2 ^ d3 ^ d4;

  uint8_t encoded = 0;
  if (p1) encoded |= (1 << 0);
  if (p2) encoded |= (1 << 1);
  if (d1) encoded |= (1 << 2);
  if (p4) encoded |= (1 << 3);
  if (d2) encoded |= (1 << 4);
  if (d3) encoded |= (1 << 5);
  if (d4) encoded |= (1 << 6);

  bool p8 = false;
  for (int i = 0; i < 7; ++i) {
    if ((encoded >> i) & 1) p8 = !p8;
  }
  if (p8) encoded |= (1 << 7);

  return encoded;
}

HammingCodec::DecodeResult HammingCodec::DecodeByte(uint8_t byte) {
  bool parity_calc = false;
  for (int i = 0; i < 8; ++i) {
    if ((byte >> i) & 1) parity_calc = !parity_calc;
  }

  bool parity_error = parity_calc;

  bool c1 = (byte >> 0) & 1;
  bool c2 = (byte >> 1) & 1;
  bool c3 = (byte >> 2) & 1;
  bool c4 = (byte >> 3) & 1;
  bool c5 = (byte >> 4) & 1;
  bool c6 = (byte >> 5) & 1;
  bool c7 = (byte >> 6) & 1;

  bool s1 = c1 ^ c3 ^ c5 ^ c7;
  bool s2 = c2 ^ c3 ^ c6 ^ c7;
  bool s3 = c4 ^ c5 ^ c6 ^ c7;

  int syndrome = (s1 ? 1 : 0) + (s2 ? 2 : 0) + (s3 ? 4 : 0);

  DecodeResult res;
  res.data = 0;
  res.status = Status::kNoError;

  if (syndrome == 0) {
    if (parity_error) {
      res.status = Status::kCorrected;
    }
  } else {
    if (parity_error) {
      res.status = Status::kCorrected;
      byte ^= (1 << (syndrome - 1));
    } else {
      res.status = Status::kDoubleError;
    }
  }

  res.data |= ((byte >> 2) & 1) << 0;
  res.data |= ((byte >> 4) & 1) << 1;
  res.data |= ((byte >> 5) & 1) << 2;
  res.data |= ((byte >> 6) & 1) << 3;

  return res;
}

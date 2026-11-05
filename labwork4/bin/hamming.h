#ifndef HAMMING_H
#define HAMMING_H

#include <cstdint>

class HammingCodec {
 public:
  enum class Status { kNoError, kCorrected, kDoubleError };

  struct DecodeResult {
    uint8_t data;
    Status status;
  };

  static uint8_t EncodeNibble(uint8_t nibble);
  static DecodeResult DecodeByte(uint8_t byte);
};

#endif

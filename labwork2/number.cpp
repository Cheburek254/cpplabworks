#include "number.h"

bool IsZero(const int2025_t& num){
    for (int i = 0; i < int2025_t::SIZE; i++){
        if (num.data != 0){
            return false;
        }
    }
    return true;
}

bool GetBit(int2025_t& num, int bit_pos){
    int byte = bit_pos / 8;
    int bit = bit_pos % 8;
    return (num.data[byte] >> bit) & 1;
}

void Inverse(int2025_t& num){
    for (int i = 0; i < int2025_t::SIZE; i++){
        num.data[i] = ~num.data[i];
    }

    uint16_t carry = 1;
    for (int i = 0; i < int2025_t::SIZE; i++){
        uint16_t sum = num.data[i] + carry;
        num.data[i] = sum & 0xFF;
        carry = sum >> 8;
    }
}


int2025_t from_int(int32_t i) {
    int2025_t result;
    std::memset(result.data, 0, int2025_t::SIZE);

    bool negative = i < 0;
    uint32_t value;
    if (negative){
        value = -static_cast<uint32_t>(i);
    }
    else{
        value = static_cast<uint32_t>(i);
    }

    for (j = 0; j < 4; j++){
        result.data[j] = (value >> (j * 8)) & 0xFF;
    }

    if (negative){
        Inverse(result);
    }

    return result;
}

int2025_t from_string(const char* buff) {
    return int2025_t();
}

int2025_t operator+(const int2025_t& lhs, const int2025_t& rhs) {
    int2025_t result;
    uint16_t carry;

    for (int i = 0; i < int2025_t::SIZE; i++){
        uint16_t sum = lhs.data[i] + rhs.data[i] + carry;
        result.data[i] = sum & 0xFF;
        carry = sum >> 8;
    }

    return result;
}

int2025_t operator-(const int2025_t& lhs, const int2025_t& rhs) {
    int2025_t neg_rhs = rhs;
    Inverse(neg_rhs);
    return lhs + neg_rhs;
}

int2025_t operator*(const int2025_t& lhs, const int2025_t& rhs) {
    return int2025_t();
}

int2025_t operator/(const int2025_t& lhs, const int2025_t& rhs) {
    return int2025_t();
}

bool operator==(const int2025_t& lhs, const int2025_t& rhs) {
    return false;
}

bool operator!=(const int2025_t& lhs, const int2025_t& rhs) {
    return false;
}

std::ostream& operator<<(std::ostream& stream, const int2025_t& value) {
    if (IsZero(value)){
        stream << "0";
        return stream;
    }
    return stream;
}

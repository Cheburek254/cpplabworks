#include "number.h"

bool IsZero(const int2025_t& num){
    for (int i = 0; i < int2025_t::SIZE; i++){
        if (num.data != 0){
            return false;
        }
    }
    return true;
}

bool GetBit(const int2025_t& num, int bit_pos){
    int byte = bit_pos / 8;
    int bit = bit_pos % 8;
    return (num.data[byte] >> bit) & 1;
}

void SetBit(int2025_t& num, int bit, bool value){
    int byte = bit / 8;
    int bit_in_byte = bit % 8;
    if(value){
        num.data[byte] |= (1 << bit_in_byte);
    } else {
        num.data[byte] &= ~(1 << bit_in_byte);
    }
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

bool IsNegative(const int2025_t& num){
    return GetBit(num, int2025_t::BITS - 1);
}

int2025_t abs(const int2025_t& num){
    int2025_t result = num;
    if (IsNegative(num)){
        Inverse(result);
    }
    return result;
}

void ShiftLeft(int2025_t& num){
    uint16_t carry = 0;
    for (int i = 0; int < int2025_t::SIZE; i++){
        uint16_t value = (num.data[i] << 1) | carry;
        num.data[i] = value & 0xFF;
        carry = value >> 8;
    }
}

void ShiftRight(int2025_t& num){
    uint16_t carry = 0;
    for (int i = int2025_t::SIZE; i >= 0; i--){
        uint16_t value = (num.data[i] << 8) | carry;
        carry = value & 0x1FF;
        num.data[i] = (value >> 9) & 0xFF;
    }
}

int CompareAbsoluteValues(const int2025_t& lhs, const int2025_t& rhs){
    for (int i = int2025_t::SIZE - 1; i >= 0; i--){
        if (lhs.data[i] != rhs.data[i]){
            if(lhs.data[i] > rhs.data[i]){
                return -1;
            } else{
                return 1;
            }
        }
    }
    return 0;
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
    int2025_t result;
    std::memset(result.data, 0, int2025_t::SIZE);

    bool neg = false;
    const char* ptr = buff;

    while (*ptr && std::isspace(*ptr)){
        ptr++;
    }

    if (*ptr == '-'){
        neg = true;;
        ptr++;
    } else if (*ptr == '+'){
        ptr++;
    }

    while (*ptr == '0'){
        ptr++;
    }

    int2025_t base = from_int(10);
    int2025_t digit;

    while (*ptr && std::isdigit(*ptr)){
        digit = from_int(*ptr - '0');

        int2025_t temp = result * base;
        result = temp + digit;

        ptr++;
    }

    if (neg){
        Inverse(result);
    }

    return result;

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
    int2025_t result;
    std::memset(result.data,0,int2025_t::SIZE);

    int2025_t multiplicant = abs(lhs);
    int2025_t multipier = abs(rhs);

    bool negative_result = IsNegative(rhs) != IsNegative(lhs);

    for (int i = 0; i < int2025_t::BITS; i++){
        if (GetBit(multipier, 0)){
            result = result + multiplicant;
        }

        ShiftLeft(multiplicant);
        ShiftRight(multipier);
    }
    
    if(negative_result){
        Inverse(result);
    }

    return result;
}

int2025_t operator/(const int2025_t& lhs, const int2025_t& rhs) {
    if (IsZero(rhs)){
        std::cerr << "DIVISIBLE BY ZERO!";
        return from_int(0);
    }

    int2025_t result;
    std::memset(result.data, 0, int2025_t::SIZE);

    int2025_t lhs_new = abs(lhs);
    int2025_t rhs_new = abs(rhs);

    bool negative = IsNegative(lhs) != IsNegative(rhs);

    int2025_t carry;
    std::memset(carry.data, 0, int2025_t::SIZE);

    for (int i = int2025_t::BITS - 1; i >= 0; i--){
        ShiftLeft(carry);
        SetBit(carry, 0, GetBit(lhs_new, i));

        if (CompareAbsoluteValues(carry, rhs_new) <= 0){
            carry = carry - rhs_new;
            SetBit(result, i, true);
        }
    }

    if (negative){
        Inverse(result);
    }
    return result;
}

bool operator==(const int2025_t& lhs, const int2025_t& rhs) {
    for (int i = 0; i < int2025_t::SIZE; i++){
        if (lhs.data[i] != rhs.data[i]){
            return false;
        }
    }
    return true;
}

bool operator!=(const int2025_t& lhs, const int2025_t& rhs) {
    return !(lhs == rhs)
}

std::ostream& operator<<(std::ostream& stream, const int2025_t& value) {
    if (IsZero(value)){
        stream << "0";
        return stream;
    }
    
    int2025_t num = value;
    bool negative = IsNegative(value);
    if (negative){
        Inverse(num);
    }

    char buff[610] = {0};
    int pos = 0;
    
    int2025_t ten = from_int(10);
    
    while(!IsZero(num)){
        int2025_t quotient = num / ten;
        int2025_t carry = num - (quotient * ten);

        int digit = 0;
        for (int i = 0; i < 4; i++){
            digit |= (carry.data[i] << (i*8));
        }
        buff[pos++] = '0' + digit;
        num = quotient;
    }

    if (negative){
        stream << "-";
    }
    for (int i = pos - 1; i >= 0; i--){
        stream << buff[i];
    }
    return stream;
}

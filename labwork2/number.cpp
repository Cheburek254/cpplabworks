#include "number.h"

namespace {
    void SetZero(int2025_t& num){
        for (int i = 0; i < int2025_t::SIZE; i++){
            num.data[i] = 0; 
        }
        num.is_negative = false;
    }

    bool IsZero(const int2025_t& num){
        for (int i = 0; i < int2025_t::SIZE; i++){
            if (num.data[i] != 0){
                return false;
            }
        }
        return true;
    }
    void ShiftLeft(int2025_t& num){
        uint16_t carry = 0;
        for (int i = 0; i < int2025_t::SIZE; i++){
            uint16_t value = (num.data[i] << 1) | carry;
            num.data[i] = value & 0xFF;
            carry = value >> 8;
        }
    }

    void ShiftRight(int2025_t& num){
        uint8_t carry = 0;
        for (int i = int2025_t::SIZE - 1; i >= 0; i--){
            uint8_t new_carry = num.data[i] & 1;  // Сохраняем младший бит
            num.data[i] = (num.data[i] >> 1) | (carry << 7);
            carry = new_carry;
        }
    }

    bool GetBit(const int2025_t& num, int bit_pos) {
        int byte = bit_pos / 8;
        int bit_in_byte = bit_pos % 8;
        return (num.data[byte] >> bit_in_byte) & 1;
    }

    void SetBit(int2025_t& num, int bit_pos, bool value) {
        int byte = bit_pos / 8;
        int bit_in_byte = bit_pos % 8;
        if (value) {
            num.data[byte] |= (1 << bit_in_byte);
        } else {
            num.data[byte] &= ~(1 << bit_in_byte);
        }
    }

    int CompareAbsolute(const int2025_t& lhs, const int2025_t& rhs) {
        for (int i = int2025_t::SIZE - 1; i >= 0; i--) {
            if (lhs.data[i] != rhs.data[i]) {
                return lhs.data[i] < rhs.data[i] ? -1 : 1;
            }
        }
        return 0;
    }

    int2025_t AddAbsolute(const int2025_t& lhs, const int2025_t& rhs) {
        int2025_t result;
        SetZero(result);
        
        uint16_t carry = 0;
        for (int i = 0; i < int2025_t::SIZE; i++) {
            uint16_t sum = lhs.data[i] + rhs.data[i] + carry;
            result.data[i] = sum & 0xFF;
            carry = sum >> 8;
        }
        
        return result;
    }

    int2025_t SubtractAbsolute(const int2025_t& lhs, const int2025_t& rhs) {
        int2025_t result;
        SetZero(result);
        
        int16_t borrow = 0;
        for (int i = 0; i < int2025_t::SIZE; i++) {
            int16_t diff = lhs.data[i] - rhs.data[i] - borrow;
            if (diff < 0) {
                diff += 256;
                borrow = 1;
            } else {
                borrow = 0;
            }
            result.data[i] = diff & 0xFF;
        }
        
        return result;
    }
}


int2025_t from_int(int32_t i) {
    int2025_t result;
    SetZero(result);
    
    if (i < 0) {
        result.is_negative = true;
        i = -i; 
    }
    
    
    for (int j = 0; j < 4 && j < int2025_t::SIZE; j++) {
        result.data[j] = (i >> (j * 8)) & 0xFF;
    }
    
    return result;
}

int2025_t from_string(const char* buff) {
    int2025_t result = from_int(0);
    
    const char* ptr = buff;
    
    
    while (*ptr && (*ptr == ' ' || *ptr == '\t')) {
        ptr++;
    }
    
    
    bool negative = false;
    if (*ptr == '-') {
        negative = true;
        ptr++;
    } else if (*ptr == '+') {
        ptr++;
    }
    
    
    while (*ptr == '0') {
        ptr++;
    }

    
    while (*ptr >= '0' && *ptr <= '9') {
        
        int2025_t result_x2 = result;
        ShiftLeft(result_x2); 
        
        int2025_t result_x8 = result;
        for (int i = 0; i < 3; i++) {
            ShiftLeft(result_x8); 
        }
        
        result = AddAbsolute(result_x2, result_x8); 
        
        
        int2025_t digit = from_int(*ptr - '0');
        result = AddAbsolute(result, digit);
        
        ptr++;
    }
    
    result.is_negative = negative;
    return result;
}

int2025_t operator+(const int2025_t& lhs, const int2025_t& rhs) {
    int2025_t result;
    SetZero(result);
    
    
    if (lhs.is_negative == rhs.is_negative) {
        result = AddAbsolute(lhs, rhs);
        result.is_negative = lhs.is_negative;
    } else {
        
        int cmp = CompareAbsolute(lhs, rhs);
        if (cmp >= 0) {
            
            result = SubtractAbsolute(lhs, rhs);
            result.is_negative = lhs.is_negative;
        } else {
            
            result = SubtractAbsolute(rhs, lhs);
            result.is_negative = rhs.is_negative;
        }
    }
    
    if (IsZero(result)) {
        result.is_negative = false;
    }
    
    return result;
}

int2025_t operator-(const int2025_t& lhs, const int2025_t& rhs) {
    int2025_t neg_rhs = rhs;
    neg_rhs.is_negative = !rhs.is_negative;
    return lhs + neg_rhs;
}

int2025_t operator*(const int2025_t& lhs, const int2025_t& rhs) {
    if (IsZero(lhs) || IsZero(rhs)) {
        return from_int(0);
    }


    if (lhs == from_int(1)) return rhs;
    if (rhs == from_int(1)) return lhs;
    if (lhs == from_int(-1)) {
        int2025_t result = rhs;
        result.is_negative = !rhs.is_negative;
        return result;
    }
    if (rhs == from_int(-1)) {
        int2025_t result = lhs;
        result.is_negative = !lhs.is_negative;
        return result;
    }

    

    
    int2025_t result;
    SetZero(result);
    

    uint16_t temp[int2025_t::SIZE * 2] = {0};
    
    
    for (int i = 0; i < int2025_t::SIZE; i++) {
        if (lhs.data[i] == 0) continue; 
        
        uint16_t carry = 0;
        uint8_t lhs_byte = lhs.data[i];
        
        for (int j = 0; j < int2025_t::SIZE; j++) {
            if (rhs.data[j] == 0 && carry == 0) continue; 
            
            uint16_t product = (uint16_t)lhs_byte * rhs.data[j] + temp[i + j] + carry;
            temp[i + j] = product & 0xFF;
            carry = product >> 8;
        }
        
        
        if (carry > 0 && (i + int2025_t::SIZE) < (int2025_t::SIZE * 2)) {
            temp[i + int2025_t::SIZE] += carry;
        }
    }

    for (int i = 0; i < int2025_t::SIZE; i++) {
        result.data[i] = temp[i] & 0xFF;
    }
    
    result.is_negative = (lhs.is_negative != rhs.is_negative);
    return result;
}

int2025_t operator/(const int2025_t& lhs, const int2025_t& rhs) {
    if (IsZero(rhs)){
        return from_int(0);
    }
    if (IsZero(lhs)){
        return from_int(0);
    }

    int2025_t result;
    SetZero(result);
    int2025_t divident = lhs;
    int2025_t divisor = rhs;

    divident.is_negative = false;
    divisor.is_negative = false;

    int2025_t remainder;
    SetZero(remainder);

    for (int i = int2025_t::BITS - 1; i >= 0; i--){
        ShiftLeft(remainder);
        SetBit(remainder, 0, GetBit(divident, i));

        if (CompareAbsolute(remainder, divisor) >= 0){
            remainder = SubtractAbsolute(remainder,divisor);
            SetBit(result, i, true);
        }
    }

    result.is_negative = (lhs.is_negative != rhs.is_negative);
    return result;
}

bool operator==(const int2025_t& lhs, const int2025_t& rhs) {
    if (IsZero(lhs) && IsZero(rhs)){
        return true;
    }
    if(lhs.is_negative != rhs.is_negative){
        return false;
    }
    return CompareAbsolute(lhs,rhs) == 0;
}

bool operator!=(const int2025_t& lhs, const int2025_t& rhs) {
    return !(lhs == rhs);
}

bool operator<(const int2025_t& lhs, const int2025_t& rhs){
    if (lhs.is_negative && !rhs.is_negative){
        return true;
    }
    if (!lhs.is_negative && rhs.is_negative){
        return false;
    }

    if (lhs.is_negative){
        return CompareAbsolute(lhs,rhs) > 0;
    } else {
        return CompareAbsolute(lhs,rhs) < 0;
    }
}

bool operator>(const int2025_t& lhs, const int2025_t& rhs){
    return rhs < lhs;
}

std::ostream& operator<<(std::ostream& stream, const int2025_t& value) {
    if (IsZero(value)){
        stream << "0";
        return stream;
    }
    if (value.is_negative){
        stream << "-";
    }

    int2025_t num = value;
    num.is_negative = false;
    char buff[610];
    for (int i = 0; i < 610; i++){
        buff[i] = 0;
    }
    int pos = 0;
    int2025_t zero = from_int(0);
    int2025_t ten = from_int(10);

    while(!(num == zero)){
        int2025_t quoitent = num / ten;
        int2025_t remainder = num - ( quoitent * ten);

        int digit = 0;
        for (int i = 0; i < 4; i++){
            digit |= (remainder.data[i] << (i*8));
        }
        buff[pos++] = '0' + digit;
        num = quoitent;
        if (pos > 609){
            break;
        }
    }

    for (int i = pos - 1; i >= 0; i--){
        stream << buff[i];
    }
    return stream;
}

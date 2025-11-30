#include "hamming.h"
#include <bitset>
#include <utility>
#include <vector>
#include <cstring>
#include <iostream>
namespace {

std::bitset<7> encode4to7(const std::bitset<4>& data) {
    std::bitset<7> encoded;
    encoded[2] = data[0];
    encoded[4] = data[1];
    encoded[5] = data[2];
    encoded[6] = data[3];
    encoded[0] = encoded[2] ^ encoded[4] ^ encoded[6];
    encoded[1] = encoded[2] ^ encoded[5] ^ encoded[6];
    encoded[3] = encoded[4] ^ encoded[5] ^ encoded[6];
    return encoded;
}

std::bitset<4> decode7to4(const std::bitset<7>& encoded, bool& single_error, bool& double_error) {
    single_error = false;
    double_error = false;
    std::bitset<7> decoded = encoded;

    bool p1 = encoded[0];
    bool p2 = encoded[1];
    bool p4 = encoded[3];
    bool b1 = encoded[2];
    bool b2 = encoded[4];
    bool b3 = encoded[5];
    bool b4 = encoded[6];

    int s1 = p1 ^ b1 ^ b2 ^ b4;
    int s2 = p2 ^ b1 ^ b3 ^ b4;
    int s3 = p4 ^ b2 ^ b3 ^ b4;

    int syndrome = (s3 << 2) | (s2 << 1) | s1;

    if (syndrome != 0) {
        single_error = true;
        int err_pos = syndrome - 1;
        if (err_pos >= 0 && err_pos < 7){
            decoded.flip(err_pos);}
    }

    std::bitset<4> result;
    result[0] = decoded[2];
    result[1] = decoded[4];
    result[2] = decoded[5];
    result[3] = decoded[6];
    return result;
}

bool CalculateControl(const std::bitset<7>& data) {
    bool parity = false;
    for (int i = 0; i < 7; i++)
        parity ^= data[i];
    return parity;
}

}

namespace HammingCoder {

std::pair<char,char> CodeByte(char input){
    std::bitset<8> bits(input);
    std::bitset<4> left_half;
    std::bitset<4> right_half;

    for (int i = 0; i < 4; i++){
        left_half[i] = bits[i];
        right_half[i] = bits[i+4];
    }

    std::bitset<7> encode_left  = encode4to7(left_half);
    std::bitset<7> encode_right = encode4to7(right_half);

    bool parity_left  = CalculateControl(encode_left);
    bool parity_right = CalculateControl(encode_right);

    char res_left = 0;
    char res_right = 0;

    for (int i = 0; i < 7; i++){
        if (encode_left[i])
            res_left |= (1 << i);
        if (encode_right[i])
            res_right |= (1 << i);
    }

    if (parity_left)  res_left  |= (1 << 7);
    if (parity_right) res_right |= (1 << 7);

    return {res_left, res_right};
}

char DecodeByte(char first, char second, bool& single_error, bool& double_error, bool& parity_error){
    single_error = false;
    double_error = false;
    parity_error = false;

    std::bitset<7> encode_left;
    std::bitset<7> encode_right;

    bool stored_parity_left  = (first  & (1 << 7)) != 0;
    bool stored_parity_right = (second & (1 << 7)) != 0;

    for (int i = 0; i < 7; i++){
        encode_left[i]  = (first  & (1 << i)) != 0;
        encode_right[i] = (second & (1 << i)) != 0;
    }

    bool calc_left  = CalculateControl(encode_left);
    bool calc_right = CalculateControl(encode_right);

    bool mismatch_left  = (calc_left  != stored_parity_left);
    bool mismatch_right = (calc_right != stored_parity_right);

    bool s1=false, s2=false;
    bool d1 = false, d2 = false;
    std::bitset<4> dec_left  = decode7to4(encode_left,  s1, d1);
    std::bitset<4> dec_right = decode7to4(encode_right, s2, d2);

    if (s1 || s2)
        single_error = true;
    if (d1 || d2){
        double_error = true;
    }
    if ((mismatch_left || mismatch_right) && !single_error)
        double_error = true;

    char result = 0;

    for (int i = 0; i < 4; i++){
        if (dec_left[i])
            result |= (1 << i);
        if (dec_right[i])
            result |= (1 << (i+4));
    }

    return result;
}

bool IsValid(const std::pair<char,char>& encoded){
    bool s,d,p;
    DecodeByte(encoded.first, encoded.second, s, d, p);
    return !d;
}

std::vector<char> EncodeData(const std::vector<char>& data){
    std::vector<char> encoded;
    encoded.reserve(data.size()*2);

    for (char byte : data){
        auto p = CodeByte(byte);
        encoded.push_back(p.first);
        encoded.push_back(p.second);
    }
    return encoded;
}

std::vector<char> DecodeData(const std::vector<char>& encoded, int& correct, int& uncorrect){
    std::vector<char> decoded;
    decoded.reserve(encoded.size()/2);

    correct = 0;
    uncorrect = 0;

    for(size_t i = 0; i < encoded.size(); i+=2){
        bool s,d,p;
        char byte = DecodeByte(encoded[i], encoded[i+1], s, d, p);
        decoded.push_back(byte);

        if (s) correct++;
        if (d) uncorrect++;
    }
    return decoded;
}
std::vector<char> EncodeBuffer(const char* data, size_t size) {
    std::vector<char> result;
    result.reserve(size * 2); 
    
    for (size_t i = 0; i < size; ++i) {
        auto encoded_pair = CodeByte(data[i]);
        result.push_back(encoded_pair.first);
        result.push_back(encoded_pair.second);
    }
    
    return result;
}

std::vector<char> DecodeBuffer(const char* encoded_data, size_t encoded_size, int& correct, int& uncorrect) {
    std::vector<char> result;
    if (encoded_size % 2 != 0) return result; 
    
    result.reserve(encoded_size / 2);
    correct = 0;
    uncorrect = 0;
    
    for (size_t i = 0; i < encoded_size; i += 2) {
        bool single_error = false, double_error = false, parity_error = false;
        char decoded_byte = DecodeByte(encoded_data[i], encoded_data[i+1], 
                                      single_error, double_error, parity_error);
        result.push_back(decoded_byte);
        
        if (single_error) correct++;
        if (double_error) uncorrect++;
    }
    
    return result;
}

void EncodeStream(std::istream& input, std::ostream& output, 
                 std::function<void(size_t, size_t)> progress_callback) {
    const size_t BUFFER_SIZE = 64 * 1024; 
    std::vector<char> input_buffer(BUFFER_SIZE);
    std::vector<char> output_buffer;
    output_buffer.reserve(BUFFER_SIZE * 2); 
    
    size_t total_read = 0;
    
    while (input.read(input_buffer.data(), BUFFER_SIZE) || input.gcount() > 0) {
        size_t bytes_read = input.gcount();
        total_read += bytes_read;
        
        
        std::vector<char> block_data(input_buffer.begin(), input_buffer.begin() + bytes_read);
        std::vector<char> encoded_block = EncodeData(block_data);
        
        
        output.write(encoded_block.data(), encoded_block.size());
        
        if (progress_callback) {
            progress_callback(total_read, total_read);
        }
        
        
        output_buffer.clear();
    }
}


void DecodeStream(std::istream& input, std::ostream& output, 
                 int& correct_errors, int& uncorrect_errors,
                 std::function<void(size_t, size_t)> progress_callback) {
    const size_t BUFFER_SIZE = 128 * 1024; 
    std::vector<char> input_buffer(BUFFER_SIZE);
    std::vector<char> output_buffer;
    
    correct_errors = 0;
    uncorrect_errors = 0;
    size_t total_read = 0;
    
    while (input.read(input_buffer.data(), BUFFER_SIZE) || input.gcount() > 0) {
        size_t bytes_read = input.gcount();
        total_read += bytes_read;
        
        
        std::vector<char> encoded_block(input_buffer.begin(), input_buffer.begin() + bytes_read);
        
        int block_correct, block_uncorrect;
        std::vector<char> decoded_block = DecodeData(encoded_block, block_correct, block_uncorrect);
        
        correct_errors += block_correct;
        uncorrect_errors += block_uncorrect;
        
        if (!decoded_block.empty()) {
            output.write(decoded_block.data(), decoded_block.size());
        }
        
        if (progress_callback) {
            progress_callback(total_read, total_read);
        }
    }
}

}

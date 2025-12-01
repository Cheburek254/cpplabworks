#ifndef HAMMING_H
#define HAMMING_H

#include <vector>
#include <bitset>
#include <functional>


namespace hammingcoder {
    std::pair<char,char> CodeByte(char input);
    char DecodeByte(char first, char second,bool& single_error, bool& double_error, bool& parity_error);
    bool IsValid(const std::pair<char,char>& encoded);
    std::vector<char> EncodeData(const std::vector<char>& data);
    std::vector<char> DecodeData(const std::vector<char>& encoded, int& correct, int& uncorrect);
    std::vector<char> EncodeBuffer(const char* data, size_t size);
    std::vector<char> DecodeBuffer(const char* encoded_data, size_t encoded_size, int& correct, int& uncorrect);
    void EncodeStream(std::istream& input, std::ostream& output, std::function<void(size_t, size_t)> progress_callback = nullptr);
    void DecodeStream(std::istream& input, std::ostream& output, 
                     int& correct_errors, int& uncorrect_errors);
}


#endif

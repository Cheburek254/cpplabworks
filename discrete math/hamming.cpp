#include <iostream>
#include <vector>
#include <string>
#include <algorithm>




std::vector<int> GetControlPositions(int n){
    std::vector<int> pos;
    for (int i = 0; i < n; i++){
        if((i+1)&i){
            continue;
        }
        pos.push_back(i);
    }
    return pos;
}

bool PowerTwo(int n){
    return (n & (n-1)) == 0;
}

int CalculateControls(int n){
    int num = 0;
    while ((1<<num) < n + 1){
        num++;
    }
    return num;
}


std::string encode(const std::string& data, int code_len){
    int data_len = data.length();
    std::string coded(code_len, '0');

    int temp = 0;
    for (int i = 0; i < code_len; i++){
        if (!PowerTwo(i+1)){
            coded[i] = data[temp++];
        }
    }

    std::vector<int> control_pos = GetControlPositions(code_len);
    for (int pos: control_pos){
        int control = 0;
        for (int i = pos; i < code_len; i+= (pos + 1) * 2){
            for (int j = 0; j < std::min(i+pos+1, code_len); j++){
                if (j != pos){
                    control ^= (coded[j] - '0');
                }
            }
        }
        coded[pos] = control + '0';
    }

    return coded;
}

std::string decode(const std::string& code){
    int code_len = code.length();
    std::string decoded = code;

    int error_pos = 0;
    int r = CalculateControls(code_len);

    for (int i = 0; i < r; i++){
        int pos = (1<<i)-1;
        if(pos >= code_len) break;

        int parity = 0;
        for (int i = pos; i < code_len; i += (pos+1)*2){
            for (int j = i; j < std::min(i+pos+1, code_len); j++){
                parity ^= (decoded[j] - '0');
            }
        }

        if (parity != 0){
            error_pos += (pos + 1);
        }
    }

    if (error_pos > 0 && error_pos <= code_len){
        decoded[error_pos - 1] = (decoded[error_pos - 1] == 0) ? '1' : '0';
    }

    std::string res;
    for (int i = 0; i < code_len; i++){
        if (!PowerTwo(i+1)){
            res += decoded[i];
        }
    }

    return res;
}

bool IsValid(const std::string& code){
    int code_len = code.length();
    

    
    int r = CalculateControls(code_len);

    for (int i = 0; i < r; i++){
        int pos = (1<<i)-1;
        if(pos >= code_len) break;

        int parity = 0;
        for (int i = pos; i < code_len; i += (pos+1)*2){
            for (int j = i; j < std::min(i+pos+1, code_len); j++){
                parity ^= (code[j] - '0');
            }
        }

        if (parity != 0){
            return false;
        }
    }
    return true;
}


int main(int argc, char** argv){
    if (std::string(argv[1]) == "is_valid"){
        std::string code = argv[3];
        std::cout << IsValid(code) << std::endl;
    }
    else if ( std::string(argv[1]) == "decode"){
        std::string code = argv[3];
        std::string dec = decode(code);
        std::cout << code.length() -dec.length() << std::endl << dec << std::endl;
    }
    else if ( std::string(argv[1]) == "encode"){
        std::string code = argv[4];
        int len = std::stoi(argv[3]);
        std::cout << encode(code, len);
    }
}

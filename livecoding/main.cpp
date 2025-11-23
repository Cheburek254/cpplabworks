#include <iostream>
#include <fstream>
#include <istream>


char** words;
int count = 0;
int GetLen(char* str){
    int temp = 0;
    while (str[temp] != '\0'){
        temp++;
    }
    return temp;
}


int ReadFile(char* path, int len){
    std::fstream file;
    file.open(path);

    if(!file.is_open()){
        
        return 1;
    }
    char* temp_word ;
    while(file.getline(temp_word, 100, '\n')){
        
        if (GetLen(temp_word) == len){
            words[count] = temp_word;
            count++;
        }   
    }
    if (count == 0){
        file.close();
        return 1;
    }
    return 0;
}

char* GetRandomWord (){
    int random_index = std::rand() % count;
    return words[random_index];
}



char* MakeUpper(char* str, int n){
    char* temp;
    for (int i = 0; i < n; i++){
        temp[i] = str[i] - 32;
    }
    temp[n] = '\0';
    return temp;
}


int CheckSymbol(char* str, char given, int n, int pos){
    int ret = 0;
    if (str[pos] == given){
        ret = 1;
        return ret;
    }

    for (int i = 0; i < n; i++){
        if (str[i] == given){
            ret = -1;
            break;
        }
    }
    return ret;
}


char* CheckSymbols(char* str, char* given, int n){
    char* ans;
    for (int i = 0; i < n; i++){
        int c = CheckSymbol(str, given[i], n, i);
        if (c == -1){
            ans[i] = '=';
        } else if (c == 0){
            ans[i] = '-';
        } else{
            ans[i] = '+';
        }
    }
    ans[n] = '\0';
    return ans;
}
bool Win(char* ans, int n){
    bool win = true;
    for(int i = 0; i < n; i++){
        if (ans[i] != '+'){
            win = false;
            break;
        }
    }
    return win;
}

void PrintArray(char* arr, int n){
    for (int i = 0; i < n; i++){
        std::cout << arr[i];
    }
    std::cout << std::endl;
}

int main(int argc, char** argv){
    char* path = argv[0];
    int len = 0;
    char* temp_len = argv[1];
    int temp = 0;
    while(temp_len[temp] != '\0'){
        len = len*10;
        len += (temp_len[temp] - '0');
        temp++;
    }
    if(ReadFile(path, len) == 1){
        return 1;
    }

    char* word = GetRandomWord();
    char* upper_word = MakeUpper(word, len);
    for (int i = 0; i < 6; i++){
        

        char* user_input;
        std::cin >> user_input;

        char* user_input_upper = MakeUpper(user_input, len);
        
        char* ans;
        ans = CheckSymbols(upper_word, user_input_upper, len);
        PrintArray(ans, len);
        if( Win(ans,len)){
            std::cout << "Win!";
            return 0;
        }

    }
    std::cout << "The word was: ";
    PrintArray(word, len);
    return 0;
}

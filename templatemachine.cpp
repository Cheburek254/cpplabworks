#include <iostream>
#include <fstream>

const int kMaxVariables = 1024;
const int kMaxKeyLength = 100;
const int kMaxValueLength = 100;
const int kMaxLineLength = 4096;

struct DataElement {
    char key[kMaxKeyLength + 1];
    char value[kMaxValueLength + 1];
};

int StringLength(const char* str) {
    int length = 0;
    while (str[length] != '\0') {
        length++;
    }
    return length;
}

void StringCopy(char* destination, const char* source) {
    int i = 0;
    while (source[i] != '\0') {
        destination[i] = source[i];
        i++;
    }
    destination[i] = '\0';
}

bool StringCompare(const char* str1, const char* str2) {
    int i = 0;
    while (str1[i] != '\0' && str2[i] != '\0') {
        if (str1[i] != str2[i]) {
            return false;
        }
        i++;
    }
    return str1[i] == str2[i];
}

bool IsSpace(char c) {
    return c == ' ' || c == '\t'  || c == '\n';
}

bool IsValidSymbol(char c) {
    return (c >= 'a' && c <= 'z') || 
           (c >= 'A' && c <= 'Z') || 
           (c >= '0' && c <= '9') || 
           (c == '_');
}

const char* FindChar(const char* str, char c) {
    int i = 0;
    while (str[i] != '\0') {
        if (str[i] == c) {
            return str + i;
        }
        i++;
    }
    return nullptr;
}

bool CompareStringPrefix(const char* str, const char* prefix) {
    int i = 0;
    while (prefix[i] != '\0') {
        if (str[i] != prefix[i]) {
            return false;
        }
        i++;
    }
    return true;
}

void DeleteSpaces(char* str) {
    if (str == nullptr) {
        return;
    }

    int length = StringLength(str);
    int start = 0;
    int end = length - 1;

    while (start < length && IsSpace(str[start])) {
        start++;
    }

    while (end >= 0 && IsSpace(str[end])) {
        end--;
    }

    if (start > 0 || end < length - 1) {
        int new_length = end - start + 1;
        if (new_length > 0) {
            for (int i = 0; i < new_length; i++) {
                str[i] = str[start + i];
            }
        }
        str[new_length] = '\0';
    }
}

bool CheckCommandLineArguments(int argc, char* argv[], 
                               char*& template_path, 
                               char*& data_path, 
                               char*& output_path) {
    template_path = nullptr;
    data_path = nullptr;
    output_path = nullptr;

    for (int i = 1; i < argc; i++) {
        if (StringCompare(argv[i], "-t")) {
            if (i + 1 < argc) {
                template_path = argv[++i];
            } else {
                return false;
            }
        } else if (StringCompare(argv[i], "-d")) {
            if (i + 1 < argc) {
                data_path = argv[++i];
            } else {
                return false;
            }
        } else if (StringCompare(argv[i], "-o")) {
            if (i + 1 < argc) {
                output_path = argv[++i];
            } else {
                return false;
            }
        } else if (CompareStringPrefix(argv[i], "--template=")) {
            template_path = argv[i] + StringLength("--template=");
        } else if (CompareStringPrefix(argv[i], "--data=")) {
            data_path = argv[i] + StringLength("--data=");
        } else if (CompareStringPrefix(argv[i], "--output=")) {
            output_path = argv[i] + StringLength("--output=");
        } else {
            return false;
        }
    }
    return template_path != nullptr && data_path != nullptr;
}

int LoadData(const char* data_path, DataElement* data, int& data_count) {
    std::ifstream data_file(data_path);
    if (!data_file.is_open()) {
        return 3;
    }

    char line[kMaxLineLength];
    data_count = 0;

    while (data_file.getline(line, kMaxLineLength)) {
        if (line[0] == '\0') {
            continue;
        }

        if (line[0] == '#' || (line[0] == '/' && line[1] == '/')) {
            continue;
        }

        DeleteSpaces(line);

        if (line[0] == '\0') {
            continue;
        }

        const char* equal_sign = FindChar(line, '=');
        if (equal_sign == nullptr) {
            continue;
        }

        int key_length = equal_sign - line;
        if (key_length > kMaxKeyLength) {
            continue;
        }

        char key[kMaxKeyLength + 1];
        for (int i = 0; i < key_length; i++) {
            key[i] = line[i];
        }
        key[key_length] = '\0';
        DeleteSpaces(key);

        const char* value_start = equal_sign + 1;
        int value_length = StringLength(value_start);
        if (value_length > kMaxValueLength) {
            continue;
        }

        char value[kMaxValueLength + 1];
        StringCopy(value, value_start);
        DeleteSpaces(value);

        if (StringLength(key) == 0 || StringLength(value) == 0) {
            continue;
        }

        bool valid_symbols = true;
        for (int i = 0; key[i] != '\0'; i++) {
            if (!IsValidSymbol(key[i])) {
                valid_symbols = false;
                break;
            }
        }
        for (int i = 0; value[i] != '\0'; i++) {
            if (!IsValidSymbol(value[i])) {
                valid_symbols = false;
                break;
            }
        }

        if (!valid_symbols) {
            continue;
        }

        bool key_exists = false;
        for (int i = 0; i < data_count; i++) {
            if (StringCompare(data[i].key, key)) {
                StringCopy(data[i].value, value);
                key_exists = true;
                break;
            }
        }

        if (!key_exists) {
            if (data_count < kMaxVariables) {
                StringCopy(data[data_count].key, key);
                StringCopy(data[data_count].value, value);
                data_count++;
            }
        }
    }

    data_file.close();
    return 0;
}

const char* FindValue(const DataElement* data, int data_count, const char* key) {
    for (int i = 0; i < data_count; i++) {
        if (StringCompare(data[i].key, key)) {
            return data[i].value;
        }
    }
    return nullptr;
}

int MakeTemplate(const char* template_path, 
                    const char* output_path, 
                    const DataElement* data, 
                    int data_count) {
    std::ifstream template_file(template_path);
    if (!template_file.is_open()) {
        return 3;
    }

    std::ofstream output_file;
    std::ostream* output_stream;

    if (output_path != nullptr) {
        output_file.open(output_path);
        if (!output_file.is_open()) {
            template_file.close();
            return 3;
        }
        output_stream = &output_file;
    } else {
        output_stream = &std::cout;
    }

    char current_char;
    int state = 0;
    char key_buffer[kMaxKeyLength + 1];
    int key_position = 0;

    while (template_file.get(current_char)) {
        switch (state) {
            case 0:  
                if (current_char == '{') {
                    state = 1;
                } else {
                    *output_stream << current_char;
                }
                break;

            case 1:  
                if (current_char == '{') {
                    state = 2;
                    key_position = 0;
                } else {
                    *output_stream << '{' << current_char;
                    state = 0;
                }
                break;

            case 2:  
                if (current_char == '}') {
                    state = 1;
                } else if (key_position < kMaxKeyLength) {
                    key_buffer[key_position++] = current_char;
                }
                break;

            case 3:  
                if (current_char == '}') {
                    key_buffer[key_position] = '\0';
                    DeleteSpaces(key_buffer);
                    const char* value = FindValue(data, data_count, key_buffer);

                    if (value == nullptr) {
                        template_file.close();
                        if (output_path != nullptr) {
                            output_file.close();
                        }
                        return 1;
                    }

                    *output_stream << value;
                    state = 0;
                } else {
                    if (key_position < kMaxKeyLength) {
                        key_buffer[key_position++] = '}';
                        key_buffer[key_position++] = current_char;
                    }
                    state = 2;
                }
                break;
        }

        if (state == 1 && current_char == '}') {
            state = 3;
        }
    }

    if (state != 0) {
        template_file.close();
        if (output_path != nullptr) {
            output_file.close();
        }
        return 4;
    }

    template_file.close();
    if (output_path != nullptr) {
        output_file.close();
    }
    return 0;
}

int main(int argc, char* argv[]) {
    char* template_path = nullptr;
    char* data_path = nullptr;
    char* output_path = nullptr;

    if (!CheckCommandLineArguments(argc, argv, template_path, data_path, output_path)) {
        return 2;
    }

    DataElement data[kMaxVariables];
    int data_count = 0;

    int result = LoadData(data_path, data, data_count);
    if (result != 0) {
        return result;
    }

    result = MakeTemplate(template_path, output_path, data, data_count);
    return result;
}

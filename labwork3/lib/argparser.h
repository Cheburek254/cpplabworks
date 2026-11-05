#ifndef LIB_ARGPARSER_H
#define LIB_ARGPARSER_H

#include <stddef.h>
#include <stdbool.h>

namespace nargparse {

typedef struct ArgumentParserImpl* ArgumentParser;

enum Nargs {
    kNargsOptional,
    kNargsRequired,
    kNargsZeroOrMore,
    kNargsOneOrMore
};


ArgumentParser CreateParser(const char* program_name, size_t max_str_len);
void FreeParser(ArgumentParser parser);


bool AddArgument(ArgumentParser parser,
                 const char* short_name,
                 const char* long_name,
                 int* value_ptr,
                 const char* description,
                 enum Nargs nargs = kNargsOptional,
                 bool (*validate)(const int&) = nullptr,
                 const char* error_msg = nullptr);

bool AddArgument(ArgumentParser parser,
                 const char* short_name,
                 const char* long_name,
                 float* value_ptr,
                 const char* description,
                 enum Nargs nargs = kNargsOptional,
                 bool (*validate)(const float&) = nullptr,
                 const char* error_msg = nullptr);

bool AddArgument(ArgumentParser parser,
                 const char* short_name,
                 const char* long_name,
                 void* value_ptr,  
                 const char* description,
                 enum Nargs nargs = kNargsOptional,
                 bool (*validate)(const char* const&) = nullptr,
                 const char* error_msg = nullptr);


bool AddArgument(ArgumentParser parser,
                 int* value_ptr,
                 const char* name,
                 enum Nargs nargs = kNargsRequired,
                 bool (*validate)(const int&) = nullptr,
                 const char* error_msg = nullptr);

bool AddArgument(ArgumentParser parser,
                 float* value_ptr,
                 const char* name,
                 enum Nargs nargs = kNargsRequired,
                 bool (*validate)(const float&) = nullptr,
                 const char* error_msg = nullptr);

bool AddArgument(ArgumentParser parser,
                 void* value_ptr,  
                 const char* name,
                 enum Nargs nargs = kNargsRequired,
                 bool (*validate)(const char* const&) = nullptr,
                 const char* error_msg = nullptr);


bool AddFlag(ArgumentParser parser,
             const char* short_name,
             const char* long_name,
             bool* value_ptr,
             const char* description,
             bool default_value = false);


bool AddHelp(ArgumentParser parser);


bool Parse(ArgumentParser parser, int argc, const char* argv[]);


int GetRepeatedCount(ArgumentParser parser, const char* name);

bool GetRepeated(ArgumentParser parser, const char* name, int index, int* value);
bool GetRepeated(ArgumentParser parser, const char* name, int index, float* value);
bool GetRepeated(ArgumentParser parser, const char* name, int index, const char** value);

} // namespace nargparse

#endif 

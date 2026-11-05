#include "argparser.h"
#include <cstdlib>
#include <cstring>
#include <cerrno>
#include <climits>
#include <cfloat>

namespace nargparse {

enum ArgType {
    ARG_FLAG,
    ARG_INT,
    ARG_FLOAT,
    ARG_STRING
};

struct Option {
    char* short_name;
    char* long_name;
    char* display_name;
    char* description;
    char* error_msg;

    ArgType type;
    enum Nargs nargs;

    void* dest;

    bool (*int_validator)(const int&);
    bool (*float_validator)(const float&);
    bool (*str_validator)(const char* const&);

    bool flag_default;

    int repeated_count;
    int repeated_capacity;
    int* int_values;
    float* float_values;
    char** str_values;

    bool was_set;
};

struct ArgumentParserImpl {
    char* program_name;
    size_t max_str_len;

    Option** options;
    int options_count;
    int options_capacity;

    Option** positionals;
    int positional_count;

    bool help_added;
    bool help_triggered;
};

static char* StrDupOrNull(const char* s) {
    if (!s) return nullptr;
    size_t len = std::strlen(s);
    char* r = (char*)std::malloc(len + 1);
    if (!r) return nullptr;
    std::memcpy(r, s, len + 1);
    return r;
}


ArgumentParser CreateParser(const char* program_name, size_t max_str_len) {
    ArgumentParserImpl* p = (ArgumentParserImpl*)std::malloc(sizeof(ArgumentParserImpl));
    if (!p) return nullptr;
    p->program_name = StrDupOrNull(program_name ? program_name : "");
    p->max_str_len = max_str_len;
    p->options_capacity = 32;
    p->options_count = 0;
    p->options = (Option**)std::malloc(sizeof(Option*) * p->options_capacity);
    p->positionals = (Option**)std::malloc(sizeof(Option*) * p->options_capacity);
    p->positional_count = 0;
    p->help_added = false;
    p->help_triggered = false;
    return p;
}

static void FreeOption(Option* o) {
    if (!o) return;

    
    bool is_help_flag = (o->type == ARG_FLAG && o->long_name && std::strcmp(o->long_name, "--help") == 0);

    std::free(o->short_name);
    std::free(o->long_name);
    std::free(o->display_name);
    std::free(o->description);
    std::free(o->error_msg);
    std::free(o->int_values);
    std::free(o->float_values);
    if (o->str_values) {
        for (int i = 0; i < o->repeated_count; ++i)
            std::free(o->str_values[i]);
    }
    std::free(o->str_values);

    if (is_help_flag) {
        std::free(o->dest);  
    }

    std::free(o);
}

void FreeParser(ArgumentParser parser) {
    if (!parser) return;
    ArgumentParserImpl* p = parser;
    std::free(p->program_name);
    for (int i = 0; i < p->options_count; ++i)
        FreeOption(p->options[i]);
    std::free(p->options);
    std::free(p->positionals);
    std::free(p);
}



static Option* CreateOption() {
    Option* o = (Option*)std::malloc(sizeof(Option));
    if (!o) return nullptr;
    std::memset(o, 0, sizeof(Option));
    return o;
}

static bool AppendOption(ArgumentParserImpl* p, Option* o) {
    if (p->options_count >= p->options_capacity) {
        int nc = p->options_capacity * 2;
        Option** arr = (Option**)std::realloc(p->options, sizeof(Option*) * nc);
        Option** pos = (Option**)std::realloc(p->positionals, sizeof(Option*) * nc);
        if (!arr || !pos) return false;
        p->options = arr;
        p->positionals = pos;
        p->options_capacity = nc;
    }
    p->options[p->options_count++] = o;
    if (!o->short_name && !o->long_name) {
        p->positionals[p->positional_count++] = o;
    }
    return true;
}


bool AddFlag(ArgumentParser parser,
             const char* short_name,
             const char* long_name,
             bool* value_ptr,
             const char* description,
             bool default_value) {
    if (!parser || !value_ptr) return false;
    ArgumentParserImpl* p = parser;
    Option* o = CreateOption();
    if (!o) return false;

    o->type = ARG_FLAG;
    o->short_name = StrDupOrNull(short_name);
    o->long_name = StrDupOrNull(long_name);
    o->display_name = StrDupOrNull(description ? description : "");
    o->description = StrDupOrNull(description ? description : "");
    o->dest = (void*)value_ptr;
    o->flag_default = default_value;
    *(bool*)o->dest = default_value;

    if (!AppendOption(p, o)) {
        FreeOption(o);
        return false;
    }
    return true;
}


static bool AddArgumentCommon(ArgumentParserImpl* p, Option* o) {
    if (!AppendOption(p, o)) {
        FreeOption(o);
        return false;
    }
    return true;
}

bool AddArgument(ArgumentParser parser, const char* short_name, const char* long_name,
                 int* value_ptr, const char* description, enum Nargs nargs,
                 bool (*validate)(const int&), const char* error_msg) {
    if (!parser || !value_ptr) return false;
    ArgumentParserImpl* p = parser;
    Option* o = CreateOption();
    o->type = ARG_INT;
    o->short_name = StrDupOrNull(short_name);
    o->long_name = StrDupOrNull(long_name);
    o->display_name = StrDupOrNull(description ? description : "");
    o->description = StrDupOrNull(description ? description : "");
    o->dest = (void*)value_ptr;
    o->nargs = nargs;
    o->int_validator = validate;
    o->error_msg = StrDupOrNull(error_msg);
    return AddArgumentCommon(p, o);
}


bool AddArgument(ArgumentParser parser, const char* short_name, const char* long_name,
                 float* value_ptr, const char* description, enum Nargs nargs,
                 bool (*validate)(const float&), const char* error_msg) {
    if (!parser || !value_ptr) return false;
    ArgumentParserImpl* p = parser;
    Option* o = CreateOption();
    o->type = ARG_FLOAT;
    o->short_name = StrDupOrNull(short_name);
    o->long_name = StrDupOrNull(long_name);
    o->display_name = StrDupOrNull(description ? description : "");
    o->description = StrDupOrNull(description ? description : "");
    o->dest = (void*)value_ptr;
    o->nargs = nargs;
    o->float_validator = validate;
    o->error_msg = StrDupOrNull(error_msg);
    return AddArgumentCommon(p, o);
}


bool AddArgument(ArgumentParser parser, const char* short_name, const char* long_name,
                 void* value_ptr, const char* description, enum Nargs nargs,
                 bool (*validate)(const char* const&), const char* error_msg) {
    if (!parser || !value_ptr) return false;
    ArgumentParserImpl* p = parser;
    Option* o = CreateOption();
    o->type = ARG_STRING;
    o->short_name = StrDupOrNull(short_name);
    o->long_name = StrDupOrNull(long_name);
    o->display_name = StrDupOrNull(description ? description : "");
    o->description = StrDupOrNull(description ? description : "");
    o->dest = value_ptr;
    o->nargs = nargs;
    o->str_validator = validate;
    o->error_msg = StrDupOrNull(error_msg);
    return AddArgumentCommon(p, o);
}

bool AddArgument(ArgumentParser parser, int* value_ptr, const char* name, enum Nargs nargs,
                 bool (*validate)(const int&), const char* error_msg) {
    if (!parser || !value_ptr) return false;
    ArgumentParserImpl* p = parser;
    Option* o = CreateOption();
    o->type = ARG_INT;
    o->display_name = StrDupOrNull(name);
    o->dest = (void*)value_ptr;
    o->nargs = nargs;
    o->int_validator = validate;
    o->error_msg = StrDupOrNull(error_msg);
    return AddArgumentCommon(p, o);
}


bool AddArgument(ArgumentParser parser, float* value_ptr, const char* name, enum Nargs nargs,
                 bool (*validate)(const float&), const char* error_msg) {
    if (!parser || !value_ptr) return false;
    ArgumentParserImpl* p = parser;
    Option* o = CreateOption();
    o->type = ARG_FLOAT;
    o->display_name = StrDupOrNull(name);
    o->dest = (void*)value_ptr;
    o->nargs = nargs;
    o->float_validator = validate;
    o->error_msg = StrDupOrNull(error_msg);
    return AddArgumentCommon(p, o);
}


bool AddArgument(ArgumentParser parser, void* value_ptr, const char* name, enum Nargs nargs,
                 bool (*validate)(const char* const&), const char* error_msg) {
    if (!parser || !value_ptr) return false;
    ArgumentParserImpl* p = parser;
    Option* o = CreateOption();
    o->type = ARG_STRING;
    o->display_name = StrDupOrNull(name);
    o->dest = value_ptr;
    o->nargs = nargs;
    o->str_validator = validate;
    o->error_msg = StrDupOrNull(error_msg);
    return AddArgumentCommon(p, o);
}



static Option* FindByName(ArgumentParserImpl* p, const char* token, const char** value_out) {
    if (!token) return nullptr;
    const char* eq = std::strchr(token, '=');
    for (int i = 0; i < p->options_count; ++i) {
        Option* o = p->options[i];
        if (o->short_name) {
            size_t sl = std::strlen(o->short_name);
            if (eq) {
                if (std::strncmp(token, o->short_name, sl) == 0 && token[sl] == '=') {
                    if (value_out) *value_out = token + sl + 1;
                    return o;
                }
            } else if (std::strcmp(token, o->short_name) == 0) {
                if (value_out) *value_out = nullptr;
                return o;
            }
        }
        if (o->long_name) {
            size_t ll = std::strlen(o->long_name);
            if (eq) {
                if (std::strncmp(token, o->long_name, ll) == 0 && token[ll] == '=') {
                    if (value_out) *value_out = token + ll + 1;
                    return o;
                }
            } else if (std::strcmp(token, o->long_name) == 0) {
                if (value_out) *value_out = nullptr;
                return o;
            }
        }
    }
    return nullptr;
}

static bool AppendIntValue(Option* o, int v) {
    if (o->repeated_count >= o->repeated_capacity) {
        int nc = o->repeated_capacity ? o->repeated_capacity * 2 : 4;
        int* arr = (int*)std::realloc(o->int_values, sizeof(int) * nc);
        if (!arr) return false;
        o->int_values = arr;
        o->repeated_capacity = nc;
    }
    o->int_values[o->repeated_count++] = v;
    return true;
}

static bool AppendFloatValue(Option* o, float v) {
    if (o->repeated_count >= o->repeated_capacity) {
        int nc = o->repeated_capacity ? o->repeated_capacity * 2 : 4;
        float* arr = (float*)std::realloc(o->float_values, sizeof(float) * nc);
        if (!arr) return false;
        o->float_values = arr;
        o->repeated_capacity = nc;
    }
    o->float_values[o->repeated_count++] = v;
    return true;
}

static bool AppendStrValue(Option* o, const char* s) {
    if (!s) s = "";
    char* copy = StrDupOrNull(s);
    if (!copy) return false;
    if (o->repeated_count >= o->repeated_capacity) {
        int nc = o->repeated_capacity ? o->repeated_capacity * 2 : 4;
        char** arr = (char**)std::realloc(o->str_values, sizeof(char*) * nc);
        if (!arr) { std::free(copy); return false; }
        o->str_values = arr;
        o->repeated_capacity = nc;
    }
    o->str_values[o->repeated_count++] = copy;
    return true;
}



static bool ParseInt(const char* s, int* out) {
    if (!s || !out) return false;
    char* end = nullptr;
    errno = 0;
    long val = std::strtol(s, &end, 10);
    if (end == s) return false;
    if (*end != '\0') return false;
    if ((val == LONG_MIN || val == LONG_MAX) && errno == ERANGE) return false;
    if (val < INT_MIN || val > INT_MAX) return false;
    *out = (int)val;
    return true;
}

static bool ParseFloat(const char* s, float* out) {
    if (!s || !out) return false;
    char* end = nullptr;
    errno = 0;
    float v = std::strtof(s, &end);
    if (end == s) return false;
    if (*end != '\0') return false;
    if (errno == ERANGE) return false;
    *out = v;
    return true;
}

static Option* FindByDisplay(ArgumentParserImpl* p, const char* name) {
    if (!p || !name) return nullptr;
    for (int i = 0; i < p->options_count; ++i) {
        Option* o = p->options[i];
        if (o->display_name && std::strcmp(o->display_name, name) == 0) return o;
    }
    return nullptr;
}



int GetRepeatedCount(ArgumentParser parser, const char* name) {
    if (!parser || !name) return 0;
    ArgumentParserImpl* p = parser;
    Option* o = FindByDisplay(p, name);
    if (!o) return 0;
    return o->repeated_count;
}

bool GetRepeated(ArgumentParser parser, const char* name, int index, int* value) {
    if (!parser || !name || !value) return false;
    ArgumentParserImpl* p = parser;
    Option* o = FindByDisplay(p, name);
    if (!o) return false;
    if (o->type != ARG_INT) return false;
    if (index < 0 || index >= o->repeated_count) return false;
    *value = o->int_values[index];
    return true;
}

bool GetRepeated(ArgumentParser parser, const char* name, int index, float* value) {
    if (!parser || !name || !value) return false;
    ArgumentParserImpl* p = parser;
    Option* o = FindByDisplay(p, name);
    if (!o) return false;
    if (o->type != ARG_FLOAT) return false;
    if (index < 0 || index >= o->repeated_count) return false;
    *value = o->float_values[index];
    return true;
}

bool GetRepeated(ArgumentParser parser, const char* name, int index, const char** value) {
    if (!parser || !name || !value) return false;
    ArgumentParserImpl* p = parser;
    Option* o = FindByDisplay(p, name);
    if (!o) return false;
    if (o->type != ARG_STRING) return false;
    if (index < 0 || index >= o->repeated_count) return false;
    *value = (const char*)o->str_values[index];
    return true;
}



bool Parse(ArgumentParser parser, int argc, const char* argv[]) {
    if (!parser || argc < 1 || !argv) return false;
    ArgumentParserImpl* p = parser;
    
    for (int i = 0; i < p->options_count; ++i) {
        Option* o = p->options[i];
        for (int j = 0; j < o->repeated_count; ++j) {
            if (o->type == ARG_STRING && o->str_values && o->str_values[j]) {
                std::free(o->str_values[j]);
                o->str_values[j] = nullptr;
            }
        }
        o->repeated_count = 0;
        o->repeated_capacity = 0;
        std::free(o->int_values); o->int_values = nullptr;
        std::free(o->float_values); o->float_values = nullptr;
        std::free(o->str_values); o->str_values = nullptr;
        o->was_set = false;
    }

    int positional_index = 0;

    for (int i = 1; i < argc; ++i) {
        const char* tok = argv[i];
        if (!tok) continue;

        if (tok[0] == '-') {
            const char* val_after_eq = nullptr;
            Option* o = FindByName(p, tok, &val_after_eq);
            if (o) {
                if (o->type == ARG_FLAG) {
                    bool* dest = (bool*)o->dest;
                    *dest = true;
                    if (o->long_name && std::strcmp(o->long_name, "--help") == 0) {
                        p->help_triggered = true;
                        return true;
                    }
                    continue;
                }

                const char* value_token = nullptr;
                if (val_after_eq) {
                    value_token = val_after_eq;
                } else {
                    if (i + 1 < argc) {
                        value_token = argv[++i];
                    } else {
                        return false;
                    }
                }

                if (o->type == ARG_INT) {
                    int v;
                    if (!ParseInt(value_token, &v)) return false;
                    if (o->int_validator && !o->int_validator(v)) return false;
                    if (o->nargs == kNargsZeroOrMore || o->nargs == kNargsOneOrMore) {
                        if (!AppendIntValue(o, v)) return false;
                        if (!o->was_set) { *(int*)o->dest = v; o->was_set = true; }
                    } else {
                        if (o->was_set) return false;
                        *(int*)o->dest = v;
                        o->was_set = true;
                    }
                } else if (o->type == ARG_FLOAT) {
                    float v;
                    if (!ParseFloat(value_token, &v)) return false;
                    if (o->float_validator && !o->float_validator(v)) return false;
                    if (o->nargs == kNargsZeroOrMore || o->nargs == kNargsOneOrMore) {
                        if (!AppendFloatValue(o, v)) return false;
                        if (!o->was_set) { *(float*)o->dest = v; o->was_set = true; }
                    } else {
                        if (o->was_set) return false;
                        *(float*)o->dest = v;
                        o->was_set = true;
                    }
                } else { 
                    if (std::strlen(value_token) > p->max_str_len - 1) return false;
                    if (o->str_validator && !o->str_validator(value_token)) return false;
                    if (o->nargs == kNargsZeroOrMore || o->nargs == kNargsOneOrMore) {
                        if (!AppendStrValue(o, value_token)) return false;
                        if (!o->was_set) {
                            char* destbuf = (char*)o->dest;
                            std::memset(destbuf, 0, p->max_str_len);
                            std::strncpy(destbuf, value_token, p->max_str_len - 1);
                            o->was_set = true;
                        }
                    } else {
                        if (o->was_set) return false;
                        char* destbuf = (char*)o->dest;
                        std::memset(destbuf, 0, p->max_str_len);
                        std::strncpy(destbuf, value_token, p->max_str_len - 1);
                        o->was_set = true;
                    }
                }

                continue;
            }
        }

       
        if (positional_index >= p->positional_count) return false;
        Option* pos_o = p->positionals[positional_index];

        if (pos_o->type == ARG_INT) {
            int v;
            if (!ParseInt(tok, &v)) return false;
            if (pos_o->int_validator && !pos_o->int_validator(v)) return false;
            if (pos_o->nargs == kNargsZeroOrMore || pos_o->nargs == kNargsOneOrMore) {
                if (!AppendIntValue(pos_o, v)) return false;
                if (!pos_o->was_set) { *(int*)pos_o->dest = v; pos_o->was_set = true; }
            } else {
                if (pos_o->was_set) return false;
                *(int*)pos_o->dest = v;
                pos_o->was_set = true;
                positional_index++;
            }
        } else if (pos_o->type == ARG_FLOAT) {
            float v;
            if (!ParseFloat(tok, &v)) return false;
            if (pos_o->float_validator && !pos_o->float_validator(v)) return false;
            if (pos_o->nargs == kNargsZeroOrMore || pos_o->nargs == kNargsOneOrMore) {
                if (!AppendFloatValue(pos_o, v)) return false;
                if (!pos_o->was_set) { *(float*)pos_o->dest = v; pos_o->was_set = true; }
            } else {
                if (pos_o->was_set) return false;
                *(float*)pos_o->dest = v;
                pos_o->was_set = true;
                positional_index++;
            }
        } else { 
            if (std::strlen(tok) > p->max_str_len - 1) return false;
            if (pos_o->str_validator && !pos_o->str_validator(tok)) return false;
            if (pos_o->nargs == kNargsZeroOrMore || pos_o->nargs == kNargsOneOrMore) {
                if (!AppendStrValue(pos_o, tok)) return false;
                if (!pos_o->was_set) {
                    std::memset((char*)pos_o->dest, 0, p->max_str_len);
                    std::strncpy((char*)pos_o->dest, tok, p->max_str_len - 1);
                    pos_o->was_set = true;
                }
            } else {
                if (pos_o->was_set) return false;
                std::memset((char*)pos_o->dest, 0, p->max_str_len);
                std::strncpy((char*)pos_o->dest, tok, p->max_str_len - 1);
                pos_o->was_set = true;
                positional_index++;
            }
        }
    }

  
    for (int i = 0; i < p->options_count; ++i) {
        Option* o = p->options[i];
        if (o->type == ARG_FLAG) continue;
        if (o->nargs == kNargsRequired && !o->was_set) return false;
        if (o->nargs == kNargsOneOrMore && o->repeated_count == 0 && !o->was_set) return false;
        if (o->type == ARG_STRING && !o->was_set && o->dest) {
            std::memset((char*)o->dest, 0, p->max_str_len);
        }
    }

    return true;
}



bool AddHelp(ArgumentParser parser) {
    if (!parser) return false;
    ArgumentParserImpl* p = parser;
    bool* dummy = (bool*)std::malloc(sizeof(bool));
    *dummy = false;
    bool r = AddFlag(parser, "-h", "--help", dummy, "Show help", false);
    if (r) p->help_added = true;
    return r;
}

} // namespace nargparse

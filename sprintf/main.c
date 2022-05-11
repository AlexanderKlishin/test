#include <stdio.h>
#include <string.h>
#include <strings.h>
#include <unistd.h>
#include <stdarg.h>

typedef struct State {
    char* str;
    const char* fmt;
} State;

typedef struct Spec {
    char flag;
    char width_star;
    unsigned width;
    char precision_star;
    unsigned precision;
    unsigned length;
    char type;
} Spec;

char get_flag(const char** fmt) {
    char flag = 0;
    switch(**fmt) {
    case '-':
    case '+':
    case ' ':
    case '#':
    case '0':
        flag = **fmt;
        (*fmt)++;
    }
    return flag;
}

char get_width_star(const char** fmt) {
    char res = 0;
    if(**fmt && **fmt == '*') {
        (*fmt)++;
        res = '*';
    }
    return res;
}

unsigned get_width(const char** fmt) {
    unsigned width = 0;
    for(;'0' <= **fmt && **fmt <= '9'; (*fmt)++) {
        char c = **fmt;
        width = width * 10 + (c - '0');
    }
    return width;
}

unsigned get_precision(const char** fmt) {
    unsigned precision = 0;
    for(;'0' <= **fmt && **fmt <= '9'; (*fmt)++) {
        char c = **fmt;
        precision = precision * 10 + (c - '0');
    }
    return precision;
}

unsigned get_length(const char** fmt) {
    char length = 0;
    switch(**fmt) {
    case 'h':
    case 'l':
    case 'L':
        length = **fmt;
        (*fmt)++;
    }
    return length;
}

char get_type(const char** fmt) {
    char type = 0;
    switch(**fmt) {
    case 'c':
    case 'd':
    case 'i':
    case 's':
    case 'x':
        type = **fmt;
        (*fmt)++;
    }
    return type;
}

void parse_spec(Spec* spec, const char** fmt) {
    spec->flag = 0;
    spec->width_star = 0;
    spec->width = 0;
    spec->precision = 0;
    spec->length = 0;
    spec->type = 0;

    spec->flag = get_flag(fmt);
    spec->width_star = get_width_star(fmt);
    if(!spec->width_star)
        spec->width = get_width(fmt);
    if(**fmt == '.') {
        (*fmt)++;
        if(**fmt == '*') {
            (*fmt)++;
            spec->precision_star = '*';
        } else {
            spec->precision = get_precision(fmt);
        }
    }
    spec->length = get_length(fmt);
    spec->type = get_type(fmt);
}

char* hexTable = "0123456789abcdef";

void revert(char* start, char* end) {
    for(; start < end; start++, end--) {
        char c = *start;
        *start = *end;
        *end = c;
    }
}

void print_int(long i, char**str, Spec* spec) {
    if(i < 0) {
        **str = '-';
        (*str)++;
        i = -i;
    }
    if(!i) {
        **str = '0';
        (*str)++;
    } else {
        char* start = *str;
        for(;i; i = i/10) {
            **str = (i%10) + '0';
            (*str)++;
        }
        revert(start, *str - 1);
    }
}

void print(char**str, Spec* spec, va_list arg) {
    switch(spec->type) {
    case 's':
        for(char* s = va_arg(arg, char*);*s; s++, (*str)++)
            **str = *s;
        break;
    case 'd':
    case 'i':
        switch(spec->length) {
        case 'l':
            print_int(va_arg(arg, long int), str, spec);
            break;
        default:
            print_int(va_arg(arg, int), str, spec);
        }
        break;
    case 'x': {
        unsigned i = va_arg(arg, unsigned);
        if(!i) {
            **str = '0';
            (*str)++;
        }
        for(;i; i = i/16) {
            //TODO: revert
            **str = hexTable[(i%16)];
            (*str)++;
        }
        break;
    }
    }
}

void parse_fmt(State* s, va_list arg) {
    ++s->fmt; // skip %
    Spec spec;
    switch(*s->fmt) {
    case 0:
        *s->str++ = '%';
        break;
    case '%':
        *s->str++ = '%';
        ++s->fmt;
        break;
    default:
        parse_spec(&spec, &s->fmt);
        print(&s->str, &spec, arg);
    }
}

int s21_vsprintf(char* str, const char *fmt, va_list arg) {
    State s;
    s.str = str;
    s.fmt = fmt;
    while(*s.fmt) {
        if(*s.fmt == '%')
            parse_fmt(&s, arg);
        else
            *(s.str)++ = *(s.fmt)++;
    }
    *s.str++ = 0;
    return s.str - str;
}

int s21_sprintf(char *str, const char *fmt, ...) {
    va_list arg;

    va_start(arg, fmt);
    int res = s21_vsprintf(str, fmt, arg);
    va_end(arg);

    return res;
}

int main(int argc, char **argv)
{
    char test[1000];
    s21_sprintf(test, "%%");
    printf("result:%s\n", test);

    char* test_str  = "AAA";
    s21_sprintf(test, "%+s", test_str);
    printf("result:%s\n", test);

    s21_sprintf(test, "%s %d", test_str, (int)-123);
    printf("result:%s\n", test);

    s21_sprintf(test, "%ld", (long int)1234);
    printf("result:%s\n", test);

    s21_sprintf(test, "%s %x", test_str, (int)0xabcdef);
    printf("result:%s\n", test);

	return 0;
}

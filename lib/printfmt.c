/* Stripped-down primitive printf-style formatting routines,
 * used in common by printf, sprintf, fprintf, etc.
 * This code is also used by both the kernel and user programs. */

#include <inc/types.h>
#include <inc/stdio.h>
#include <inc/string.h>
#include <inc/stdarg.h>
#include <inc/error.h>

#include <inc/ryu.h>
#include <inc/lib.h>

/*
 * Space or zero padding and a field width are supported for the numeric
 * formats only.
 *
 * The special format %i takes an integer error code
 * and prints a string describing the error.
 * The integer may be positive or negative,
 * so that -E_NO_MEM and E_NO_MEM are equivalent.
 */

static const char *const error_string[MAXERROR] = {
        [E_UNSPECIFIED] = "unspecified error",
        [E_BAD_ENV] = "bad environment",
        [E_INVAL] = "invalid parameter",
        [E_NO_MEM] = "out of memory",
        [E_NO_FREE_ENV] = "out of environments",
        [E_BAD_DWARF] = "corrupted debug info",
        [E_FAULT] = "segmentation fault",
        [E_INVALID_EXE] = "invalid ELF image",
        [E_NO_ENT] = "entry not found",
        [E_NO_SYS] = "no such system call",
        [E_IPC_NOT_RECV] = "env is not recving",
        [E_EOF] = "unexpected end of file",
        [E_NO_DISK] = "no free space on disk",
        [E_MAX_OPEN] = "too many files are open",
        [E_NOT_FOUND] = "file or block not found",
        [E_BAD_PATH] = "invalid path",
        [E_FILE_EXISTS] = "file already exists",
        [E_NOT_EXEC] = "file is not a valid executable",
        [E_NOT_SUPP] = "operation not supported",
};

#ifndef JOS_KERNEL

// Приблизительное значение натурального логарифма числа 10
#define LN_10 2.3025850929940456840179914546843642076011014886288

// Функция для вычисления натурального логарифма (ln(x)) через разложение Тейлора
double ln(double x) {
    if (x <= 0) {
        return -1.0 / 0.0; // Возвращаем -inf для отрицательных или нулевых значений
    }

    // Приводим x в диапазон (0.5, 1.5) для лучшей сходимости
    double result = 0.0;
    int iterations = 50; // Количество итераций разложения Тейлора
    int power = 0;

    // Приведение x в диапазон (0.5, 1.5)
    while (x > 1.5) {
        x /= 2.718281828459045; // Делим на e
        power++;
    }
    while (x < 0.5) {
        x *= 2.718281828459045; // Умножаем на e
        power--;
    }

    // Разложение Тейлора для ln(x) вокруг точки 1
    double y = x - 1;
    double term = y;
    for (int i = 1; i <= iterations; i++) {
        if (i % 2 == 1) {
            result += term / i; // Чётные члены вычитаем
        } else {
            result -= term / i;
        }
        term *= y; // Следующий член ряда
    }

    // Учитываем масштабирование
    result += power;
    return result;
}

// Функция для вычисления log10(x)
double log10(double x) {
    return ln(x) / LN_10;
}

// Функция для проверки, является ли число бесконечностью
bool isinf(double x, uint64_t *sign) {
    // Преобразуем double в uint64_t для анализа битов
    union {
        double d;
        uint64_t bits;
    } u;

    u.d = x;

    // Извлекаем биты знака, порядка и мантиссы
    *sign = (u.bits >> 63) & 0x1;         // Старший бит (знак)
    uint64_t exponent = (u.bits >> 52) & 0x7FF;  // 11 битов порядка
    uint64_t mantissa = u.bits & 0xFFFFFFFFFFFFF; // 52 бита мантиссы

    // Условие для бесконечности: порядок = 0x7FF, мантисса = 0
    if (exponent == 0x7FF && mantissa == 0) {
        return true;
    }
    return false;
}

/**
 * Handles floating-point number formatting for the "g" specifier with width.
 */
static void 
print_float(void (*putch)(int, void *), void *put_arg,
                          double value, int precision, int width, char padc,
                          bool capital, bool hash, char format) {
    char buffer[2000];
    char new_buf[2000];
    uint64_t sign_inf;
    
    int formatted_length; // Length of the formatted string

    if (precision == -1) {
        precision = 6; // Default precision
    } else if (precision == 0) {
        precision = 1; // Precision 0 means 1 significant digit for 'g'
    }
    
    // Special case handling for NaN, infinity, and zero
    if (value != value) {
        strcpy(buffer, capital ? "NAN" : "nan");
    } else if (isinf(value, &sign_inf)) {
        if (capital) {
            strcpy(buffer, sign_inf ? "-INF" : "INF");
        } else {
            strcpy(buffer, sign_inf ? "-inf" : "inf");
        }
    } else if (value == 0.0) {
        // strcpy(buffer, signbit(value) ? "-0" : "0");
        strcpy(buffer, "0");
        if (hash) {
            strcat(buffer, "."); // Add decimal point if '#' is used
            int i;
            for (i = 0; i < precision - 1; ++i) {
                strcat(buffer, "0");
            }
        } else if (format == 'e' || format == 'f') {
            strcat(buffer, ".");
            
            for (int i = 0; i < precision; ++i) {
                strcat(buffer, "0");
            }

            if (format == 'e') {
                strcat(buffer, capital ? "E+00" : "e+00");
            }
        }
    } else {
        // Format the value based on 'g' rules
        if (format == 'f') {
            d2fixed(value, precision, new_buf);
            strcpy(buffer, new_buf);
        } else if (format == 'e') {
            d2exp(value, precision, new_buf);
            if (capital) {
                char *e_char = strchr(buffer, 'e');
                *e_char = 'E';
            }

            strcpy(buffer, new_buf);
        } else {
            int exponent = (int)(log10(value > 0.0 ? value : -value));

            if (precision > exponent && exponent >= -4) {
                // Use 'f' format
                int adjusted_precision = precision - (exponent + 1);
                if (adjusted_precision < 0) {
                    adjusted_precision = 1;
                }
                
                d2fixed(value, adjusted_precision, new_buf);
                strcpy(buffer, new_buf);
            } else {
                // Use 'e' format
                d2exp(value, precision - 1, buffer);
                if (capital) {
                    char *e_char = strchr(buffer, 'e');
                    *e_char = 'E';
                }
            }
        }

        // // Remove trailing zeros and decimal point unless '#' flag is used
        // if (!hash) {
        //     char *dot = strchr(buffer, '.');
        //     if (dot) {
        //         char *end = buffer + strlen(buffer) - 1;
        //         while (end > dot && *end == '0') {
        //             *end-- = '\0';
        //         }
        //         if (*end == '.') {
        //             *end = '\0'; // Remove the decimal point if no fractional part remains
        //         }
        //     }
        // }
    }

    // Get the length of the formatted string
    formatted_length = strlen(buffer);

    // Handle padding based on width
    if (width > formatted_length) {
        int padding_length = width - formatted_length;

        while (padding_length-- > 0) putch(padc, put_arg);
    }
    for (char *p = buffer; *p; ++p) {
        putch(*p, put_arg);
    }
}

#endif

/*
 * Print a number (base <= 16) in reverse order,
 * using specified putch function and associated pointer putdat.
 */
static void
print_num(void (*putch)(int, void *), void *put_arg,
          uintmax_t num, unsigned base, int width, char padc, bool capital) {
    /* First recursively print all preceding (more significant) digits */
    if (num >= base) {
        print_num(putch, put_arg, num / base, base, width - 1, padc, capital);
    } else {
        /* Print any needed pad characters before first digit */
        while (--width > 0) {
            putch(padc, put_arg);
        }
    }

    const char *dig = capital ? "0123456789ABCDEF" : "0123456789abcdef";

    /* Then print this (the least significant) digit */
    putch(dig[num % base], put_arg);
}

/* Get an unsigned int of various possible sizes from a varargs list,
 * depending on the lflag parameter. */
static uintmax_t
get_unsigned(va_list *ap, int lflag, bool zflag) {
    if (zflag) return va_arg(*ap, size_t);

    switch (lflag) {
    case 0:
        return va_arg(*ap, unsigned int);
    case 1:
        return va_arg(*ap, unsigned long);
    default:
        return va_arg(*ap, unsigned long long);
    }
}

/* Same as getuint but signed - can't use getuint
 * because of sign extension */
static intmax_t
get_int(va_list *ap, int lflag, bool zflag) {
    if (zflag) return va_arg(*ap, size_t);

    switch (lflag) {
    case 0:
        return va_arg(*ap, int);
    case 1:
        return va_arg(*ap, long);
    default:
        return va_arg(*ap, long long);
    }
}

/* Main function to format and print a string. */
void printfmt(void (*putch)(int, void *), void *put_arg, const char *fmt, ...);

void
vprintfmt(void (*putch)(int, void *), void *put_arg, const char *fmt, va_list ap) {
    const unsigned char *ufmt = (unsigned char *)fmt;

    va_list aq;
    va_copy(aq, ap);

    for (;;) {
        unsigned char ch;
        while ((ch = *ufmt++) != '%') {
            if (!ch) return;
            putch(ch, put_arg);
        }

        /* Process a %-escape sequence */
        char padc = ' ';
        int width = -1, precision = -1;
        unsigned lflag = 0, base = 10;
        bool altflag = 0, zflag = 0;
        uintmax_t num = 0;
        #ifndef JOS_KERNEL
        double fnum;
        bool capital_flag = 0;
        #endif
    reswitch:

        switch (ch = *ufmt++) {
        case '0': /* '-' flag to pad on the right */
        case '-': /* '0' flag to pad with 0's instead of spaces */
            padc = ch;
            goto reswitch;

        case '*': /* Indirect width field */
            precision = va_arg(aq, int);
            goto process_precision;

        case '1':
        case '2':
        case '3':
        case '4':
        case '5':
        case '6':
        case '7':
        case '8':
        case '9': /* width field */
            for (precision = 0;; ++ufmt) {
                precision = precision * 10 + ch - '0';
                if ((ch = *ufmt) - '0' > 9) break;
            }

        process_precision:
            if (width < 0) {
                width = precision;
                precision = -1;
            }
            goto reswitch;

        case '.':
            width = MAX(0, width);
            goto reswitch;

        case '#':
            altflag = 1;
            goto reswitch;

        case 'l': /* long flag (doubled for long long) */
            lflag++;
            goto reswitch;

        case 'z':
            zflag = 1;
            goto reswitch;

        case 'c': /* character */
            putch(va_arg(aq, int), put_arg);
            break;

        case 'i': /* error message */ {
            int err = va_arg(aq, int);
            const char *strerr;

            if (err < 0) err = -err;

            if (err >= MAXERROR || !(strerr = error_string[err])) {
                printfmt(putch, put_arg, "error %d", err);
            } else {
                printfmt(putch, put_arg, "%s", strerr);
            }
            break;
        }

        case 's': /* string */ {
            const char *ptr = va_arg(aq, char *);
            if (!ptr) ptr = "(null)";

            if (width > 0 && padc != '-') {
                width -= strnlen(ptr, precision);

                while (width-- > 0) putch(padc, put_arg);
            }

            for (; (ch = *ptr++) && (precision < 0 || --precision >= 0); width--) {
                putch(altflag && (ch < ' ' || ch > '~') ? '?' : ch, put_arg);
            }

            while (width-- > 0) putch(' ', put_arg);
            break;
        }

        case 'd': /* (signed) decimal */ {
            intmax_t i = get_int(&aq, lflag, zflag);
            if (i < 0) {
                putch('-', put_arg);
                i = -i;
            }
            num = i;
            /* base = 10; */
            goto number;
        }

        case 'u': /* unsigned decimal */
            num = get_unsigned(&aq, lflag, zflag);
            /* base = 10; */
            goto number;

        case 'o': /* (unsigned) octal */
            // LAB 1: Your code here:
            num = get_unsigned(&aq, lflag, zflag);
            base = 8;
            goto number;

        case 'p': /* pointer */
            putch('0', put_arg);
            putch('x', put_arg);
            num = (uintptr_t)va_arg(aq, void *);
            base = 16;
            goto number;
            
        /* Itask code here */
        #ifndef JOS_KERNEL
        case 'F':
        case 'E':
        case 'G':
            capital_flag = 1;
        case 'f':
        case 'e':
        case 'g':
            fnum = va_arg(aq, double);
            print_float(putch, put_arg, fnum, precision, width, padc,
                        capital_flag, altflag, ch);
            break;
        #endif

        case 'X': /* (unsigned) hexadecimal, uppercase */
        case 'x': /* (unsigned) hexadecimal, lowercase */
            num = get_unsigned(&aq, lflag, zflag);
            base = 16;
        number:
            print_num(putch, put_arg, num, base, width, padc, ch == 'X');
            break;

        case '%': /* escaped '%' character */
            putch(ch, put_arg);
            break;

        default: /* unrecognized escape sequence - just print it literally */
            putch('%', put_arg);
            while ((--ufmt)[-1] != '%') /* nothing */
                ;
        }
    }
}

void
printfmt(void (*putch)(int, void *), void *putdat, const char *fmt, ...) {
    va_list ap;

    va_start(ap, fmt);
    vprintfmt(putch, putdat, fmt, ap);
    va_end(ap);
}

struct sprintbuf {
    char *start;
    char *end;
    int count;
};

static void
sprintputch(int ch, struct sprintbuf *state) {
    state->count++;
    if (state->start < state->end) {
        *state->start++ = ch;
    }
}

int
vsnprintf(char *buf, size_t n, const char *fmt, va_list ap) {
    struct sprintbuf state = {buf, buf + n - 1, 0};

    if (!buf || n < 1) return -E_INVAL;

    /* Print the string to the buffer */
    vprintfmt((void *)sprintputch, &state, fmt, ap);

    /* Null terminate the buffer */
    *state.start = '\0';

    return state.count;
}

int
snprintf(char *buf, size_t n, const char *fmt, ...) {
    va_list ap;

    va_start(ap, fmt);
    int rc = vsnprintf(buf, n, fmt, ap);
    va_end(ap);

    return rc;
}

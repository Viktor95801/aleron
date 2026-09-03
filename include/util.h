#ifndef UTIL_H
#define UTIL_H

#include <__stdarg_va_list.h>
#include <inttypes.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

typedef int8_t i8;
typedef int16_t i16;
typedef int32_t i32;
typedef int64_t i64;

typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;

typedef float f32;
typedef double f64;

typedef u8 byte;

#if __has_attribute(format)
#define ATT_FORMAT(a, b) __attribute__((format(printf, a, b)))
#else
#define ATT_FORMAT(a, b)
#endif

ATT_FORMAT(1, 0) const char *formatv(const char *fmt, va_list arg);
ATT_FORMAT(1, 2) const char *format(const char *fmt, ...);

#endif

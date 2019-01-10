// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#ifndef XLOGGING_H
#define XLOGGING_H

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <inttypes.h>
#include "macro_utils.h"
#include "azure_c_shared_utility/agenttime.h"
#include "azure_c_shared_utility/optimize_size.h"

#if defined(ESP8266_RTOS)
#include "c_types.h"
#endif

#if defined(ARDUINO_ARCH_ESP8266)
#include "esp8266/azcpgmspace.h"
#endif

#ifdef __cplusplus
/* Some compilers do not want to play by the standard, specifically ARM CC */
#ifdef MBED_BUILD_TIMESTAMP
#include <stdio.h>
#else
#include <cstdio>
#endif
extern "C" {
#else
#include <stdio.h>
#endif /* __cplusplus */

#ifdef TIZENRT
#undef LOG_INFO
#endif

typedef enum LOG_CATEGORY_TAG
{
    AZ_LOG_ERROR,
    AZ_LOG_INFO,
    AZ_LOG_TRACE
} LOG_CATEGORY;

#if defined _MSC_VER
#define FUNC_NAME __FUNCDNAME__
#else
#define FUNC_NAME __func__
#endif

typedef void(*LOGGER_LOG)(LOG_CATEGORY log_category, const char* file, const char* func, int line, unsigned int options, const char* format, ...);
typedef void(*LOGGER_LOG_GETLASTERROR)(const char* file, const char* func, int line, const char* format, ...);

#define TEMP_BUFFER_SIZE 1024
#define MESSAGE_BUFFER_SIZE 260

#define LOG_NONE 0x00
#define LOG_LINE 0x01

/*no logging is useful when time and fprintf are mocked*/
#ifdef NO_LOGGING
#define LOG(...)
#define LogInfo(...)
#define LogBinary(...)
#define LogError(...)
#define xlogging_get_log_function() NULL
#define xlogging_set_log_function(...)
#define LogErrorWinHTTPWithGetLastErrorAsString(...)
#define UNUSED(x) (void)(x)
#elif (defined MINIMAL_LOGERROR)
#define LOG(...)
#define LogInfo(...)
#define LogBinary(...)
#define LogError(...) printf("error %s: line %d\n",__FILE__,__LINE__);
#define xlogging_get_log_function() NULL
#define xlogging_set_log_function(...)
#define LogErrorWinHTTPWithGetLastErrorAsString(...)
#define UNUSED(x) (void)(x)

#elif defined(ESP8266_RTOS)
#define LogInfo(FORMAT, ...) do {    \
        static const char flash_str[] ICACHE_RODATA_ATTR STORE_ATTR = FORMAT;  \
        printf(flash_str, ##__VA_ARGS__);   \
        printf("\n");\
    } while((void)0,0)

#define LogError LogInfo
#define LOG(log_category, log_options, FORMAT, ...)  { \
        static const char flash_str[] ICACHE_RODATA_ATTR STORE_ATTR = (FORMAT); \
        printf(flash_str, ##__VA_ARGS__); \
        printf("\r\n"); \
}

#else /* NOT ESP8266_RTOS */

// In order to make sure that the compiler evaluates the arguments and issues an error if they do not conform to printf
// specifications, we call printf with the format and __VA_ARGS__ but the call is behind an if (0) so that it does
// not actually get executed at runtime
#if defined _MSC_VER
// ignore warning C4127 
#define LOG(log_category, log_options, format, ...) \
{ \
    __pragma(warning(suppress: 4127)) \
    if (0) \
    { \
        (void)printf(format, __VA_ARGS__); \
    } \
    { \
        LOGGER_LOG l = xlogging_get_log_function(); \
        if (l != NULL) \
        { \
            l(log_category, __FILE__, FUNC_NAME, __LINE__, log_options, format, __VA_ARGS__); \
        } \
    } \
}
#else
#define LOG(log_category, log_options, format, ...) { if (0) { (void)printf(format, ##__VA_ARGS__); } { LOGGER_LOG l = xlogging_get_log_function(); if (l != NULL) l(log_category, __FILE__, FUNC_NAME, __LINE__, log_options, format, ##__VA_ARGS__); } }
#endif

#if defined _MSC_VER
#define LogInfo(FORMAT, ...) do{LOG(AZ_LOG_INFO, LOG_LINE, FORMAT, __VA_ARGS__); }while((void)0,0)
#else
#define LogInfo(FORMAT, ...) do{LOG(AZ_LOG_INFO, LOG_LINE, FORMAT, ##__VA_ARGS__); }while((void)0,0)
#endif

#define ARG_IN_FUNC_DEFINITION(count, arg_type, arg_name) \
    arg_type arg_name IFCOMMA(count)

#define PRI_int8_t PRId8
#define PRI_int16_t PRId16
#define PRI_int32_t PRId32
#define PRI_int64_t PRId64
#define PRI_uint8_t PRIu8
#define PRI_uint16_t PRIu16
#define PRI_uint32_t PRIu32
#define PRI_uint64_t PRIu64

#define PRINT_FORMAT(arg_type) \
    C2(PRI_, arg_type)

static const char* get_logging_format(const char* type)
{
    (void)type;
    if (strcmp(type, "uint32_t") == 0)
    {
        return "%" PRIu32;
    }
    else
    {
        return "%p";
    }
}

static char* xlogging_vsprintf_char(const char* format, va_list va)
{
    char* result;
    int neededSize = vsnprintf(NULL, 0, format, va);
    if (neededSize < 0)
    {
        (void)printf("failure in vsnprintf\n");
        result = NULL;
    }
    else
    {
        result = malloc(neededSize + 1);
        if (result == NULL)
        {
            (void)printf("failure in malloc\n");
            /*return as is*/
        }
        else
        {
            if (vsnprintf(result, neededSize + 1, format, va) != neededSize)
            {
                (void)printf("inconsistent vsnprintf behavior\n");
                free(result);
                result = NULL;
            }
        }
    }
    return result;
}

#define TEXT_ARG_IN_CONSTRUCT_FORMAT(count, arg_type, arg_name) \
    TOSTRING(arg_type) " " TOSTRING(arg_name) "=%s" IF(DEC(DEC(count)),", ",)

#define ARG_TYPE_IN_CONSTRUCT_FORMAT(count, arg_type, arg_name) \
    , get_logging_format(TOSTRING(arg_type))

#define ARG_AS_PARAM_IN_CALL(count, arg_type, arg_name) \
    arg_name IFCOMMA(count)

#ifdef WIN32
extern void xlogging_LogErrorWinHTTPWithGetLastErrorAsStringFormatter(int errorMessageID);
#endif

#if defined _MSC_VER

#if !defined(WINCE)
extern void xlogging_set_log_function_GetLastError(LOGGER_LOG_GETLASTERROR log_function);
extern LOGGER_LOG_GETLASTERROR xlogging_get_log_function_GetLastError(void);
#define LogLastError(FORMAT, ...) do{ LOGGER_LOG_GETLASTERROR l = xlogging_get_log_function_GetLastError(); if(l!=NULL) l(__FILE__, FUNC_NAME, __LINE__, FORMAT, __VA_ARGS__); }while((void)0,0)
#endif

#define LogError(FORMAT, ...) do{ LOG(AZ_LOG_ERROR, LOG_LINE, FORMAT, __VA_ARGS__); }while((void)0,0)
#define LogErrorWinHTTPWithGetLastErrorAsString(FORMAT, ...) do { \
                int errorMessageID = GetLastError(); \
                LogError(FORMAT, __VA_ARGS__); \
                xlogging_LogErrorWinHTTPWithGetLastErrorAsStringFormatter(errorMessageID); \
            } while((void)0,0)
#else // _MSC_VER
#define LogError(FORMAT, ...) do{ LOG(AZ_LOG_ERROR, LOG_LINE, FORMAT, ##__VA_ARGS__); }while((void)0,0)

#ifdef WIN32
// Included when compiling on Windows but not with MSVC, e.g. with MinGW.
#define LogErrorWinHTTPWithGetLastErrorAsString(FORMAT, ...) do { \
                int errorMessageID = GetLastError(); \
                LogError(FORMAT, ##__VA_ARGS__); \
                xlogging_LogErrorWinHTTPWithGetLastErrorAsStringFormatter(errorMessageID); \
            } while((void)0,0)
#endif // WIN32

#endif // _MSC_VER

typedef void(*LOG_ARGS_FUNC)(va_list va);

#define FUNCTION_DEFINITION(return_type, name, ...) \
static char* C2(name, _construct_log_invalid_args_format_string)(void) \
{ \
    char* result; \
    int needed_length = snprintf(NULL, 0, "Invalid arguments: " FOR_EACH_2_COUNTED(TEXT_ARG_IN_CONSTRUCT_FORMAT, __VA_ARGS__) FOR_EACH_2_COUNTED(ARG_TYPE_IN_CONSTRUCT_FORMAT, __VA_ARGS__)); \
    if (needed_length <= 0) \
    { \
        result = NULL; \
    } \
    else \
    { \
        result = malloc(needed_length + 1); \
        if (result != NULL) \
        { \
            (void)sprintf(result, "Invalid arguments: " FOR_EACH_2_COUNTED(TEXT_ARG_IN_CONSTRUCT_FORMAT, __VA_ARGS__) FOR_EACH_2_COUNTED(ARG_TYPE_IN_CONSTRUCT_FORMAT, __VA_ARGS__)); \
        } \
    } \
    return result; \
} \
static void C2(name, _log_args)(va_list va) \
{ \
    char* log_invalid_args_format = C2(name, _construct_log_invalid_args_format_string)(); \
    if (log_invalid_args_format != NULL) \
    { \
        char* log_text = xlogging_vsprintf_char((const char*)log_invalid_args_format, va); \
        if (log_text != NULL) \
        { \
            LogError("%s", log_text); \
            free(log_text); \
        } \
        free(log_invalid_args_format); \
    } \
} \
return_type C2(name, _code)(LOG_ARGS_FUNC log_args_func, va_list log_args_va, FOR_EACH_2_COUNTED(ARG_IN_FUNC_DEFINITION, __VA_ARGS__)); \
return_type C2(name, _with_dummy)(FOR_EACH_2_COUNTED(ARG_IN_FUNC_DEFINITION, __VA_ARGS__), int dummy, ...) \
{ \
    va_list va; \
    return_type ret; \
    va_start(va, dummy); \
    ret = C2(name, _code)(C2(name, _log_args), va, FOR_EACH_2_COUNTED(ARG_AS_PARAM_IN_CALL, __VA_ARGS__)); \
    va_end(va); \
    return ret; \
} \
return_type name(FOR_EACH_2_COUNTED(ARG_IN_FUNC_DEFINITION, __VA_ARGS__)) \
{ \
    return C2(name, _with_dummy)(FOR_EACH_2_COUNTED(ARG_AS_PARAM_IN_CALL, __VA_ARGS__), 0, FOR_EACH_2_COUNTED(ARG_AS_PARAM_IN_CALL, __VA_ARGS__)); \
} \
return_type C2(name, _code)(LOG_ARGS_FUNC log_args_func, va_list log_args_va, FOR_EACH_2_COUNTED(ARG_IN_FUNC_DEFINITION, __VA_ARGS__))

#define LOG_INVALID_ARGS() \
    log_args_func(log_args_va)

extern void LogBinary(const char* comment, const void* data, size_t size);

extern void xlogging_set_log_function(LOGGER_LOG log_function);
extern LOGGER_LOG xlogging_get_log_function(void);

#endif /* NOT ESP8266_RTOS */

#ifdef __cplusplus
}   // extern "C"
#endif /* __cplusplus */

#endif /* XLOGGING_H */

#ifndef _SYSLOG_H
#define _SYSLOG_H
#include <stdarg.h>
#include <stdint.h>

#define SYSLOG_QUEUE_LENGTH 512
#define MAX_LOG_SIZE 128
#define LOG_QUEUE_WAITING_TIME (0)

#define SYSLOG_ASSERT_ENABLE 1
#define TASK_PRIORITY_SYSLOG 1
#define SYSLOG_TASK_NAME "syslogTask"
#define TASK_PRIORITY_HARD_REALTIME 8
#define SYSLOG_TASK_STACKSIZE 144
typedef enum {
    PRINT_LEVEL_DEBUG,
    PRINT_LEVEL_INFO,
    PRINT_LEVEL_WARNING,
    PRINT_LEVEL_ERROR
} print_level_t;

typedef enum {
    DEBUG_LOG_ON,
    DEBUG_LOG_OFF
} log_switch_t;

typedef struct {
    print_level_t print_level;
    const char *func_name;
    int line_number;
    uint32_t timestamp;
    char message[MAX_LOG_SIZE + 1]; //C string format
} log_message_t;

typedef struct {
    uint32_t occupied;
    uint8_t  buffer[sizeof(log_message_t)];
} syslog_buffer_t;

void syslog_init(void);
void print_module_log(const char *func, int line, print_level_t level, const char *message, ...);

extern void log_error(const char *func, int line, const char *message, ...);
extern void log_warning(const char *func, int line, const char *message, ...);
extern void log_info(const char *func, int line, const char *message, ...);
extern void log_debug(const char *func, int line, const char *message, ...);
extern void (*syslog_assert_hook)(const char* expr, const char* func, uint32_t line);

extern void hex_dump(const char *name, const char *data, int length);

#ifdef SYSLOG_ASSERT_ENABLE
    #define SYSLOG_ASSERT(EXPR)                                                 \
    if (!(EXPR))                                                              \
    {                                                                         \
        if (syslog_assert_hook == NULL) {                                       \
            LOG_I("(%s) has assert failed at %s:%ld.", #EXPR, __FUNCTION__, __LINE__); \
            while (1);                                                        \
        } else {                                                              \
            syslog_assert_hook(#EXPR, __FUNCTION__, __LINE__);                  \
        }                                                                     \
    }
#else
    #define SYSLOG_ASSERT(EXPR)                    ((void)0);
#endif

#define LOG_E(_message,...) log_error(__FUNCTION__, __LINE__, (_message),##__VA_ARGS__)
#define LOG_W(_message,...) log_warning(__FUNCTION__, __LINE__, (_message),##__VA_ARGS__)
#define LOG_I(_message,...) log_info(__FUNCTION__, __LINE__, (_message),##__VA_ARGS__)
#define LOG_D(_message,...) log_debug(__FUNCTION__, __LINE__, (_message),##__VA_ARGS__)

#endif

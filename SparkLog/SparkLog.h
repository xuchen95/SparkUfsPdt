#pragma once
#include <string>
#include <cstdint>

typedef uint32_t UINT32;

typedef struct SparkLog_Config
{
    char func_name[32];
    uint8_t ufs_port; // 0-based port index or port id
    char card_id[32];
    char fw_version[32];
    char app_version[32];
    char tester_version[32];
    uint8_t mid[2];
    uint8_t oid[2];
    char manufacturer[32];
    char product_name[32];
    char serial_number[58];
    char start_date[32];
    char start_time[32];
    char build_time[32];
    char state[32];
    UINT32 error_code;

} pdt_log_config_t, *ppdt_log_config_t;

// Initialize logging subsystem (creates internal lock). Safe to call multiple times.
void SparkLog_Init();

// Append a line to the production log file (thread-safe). Line should not contain trailing newline.
void SparkLog_Append(const std::string& line);

// Close logging subsystem and release resources. Safe to call multiple times.
void SparkLog_Close();

// Enqueue a production log record to be written asynchronously by the log thread.
void SparkLog_EnqueuePdtLog(const pdt_log_config_t& cfg);

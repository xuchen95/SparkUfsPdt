#pragma once
#include <stdint.h>
#include "FsStructs.h"

typedef char16_t ExChar16_t;

bool exFatCmpName(const DirName_t* unicode,
    const char* name, size_t offset, size_t n);
bool exFatCmpName(const DirName_t* unicode,
    const ExChar16_t* name, size_t offset, size_t n);
uint16_t exFatHashName(const char* name, size_t n, uint16_t hash);
uint16_t exFatHashName(const ExChar16_t* name, size_t n, uint16_t hash);
uint16_t toUpcase(uint16_t chr);
uint32_t upcaseChecksum(uint16_t unicode, uint32_t checksum);

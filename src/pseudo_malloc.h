#pragma once

#include <stdio.h>
#include <stdint.h>
#include <math.h>
#include <stdlib.h>

void pseudomalloc_init(void);

void* pseudomalloc(size_t size);

void pseudofree(void* ptr);
#ifndef UNI_LOG_H
#define UNI_LOG_H

#include <stdio.h>

#define UNI_LOG(fmt, ...) printf(fmt "\n", __VA_ARGS__)

#endif // UNI_LOG_H

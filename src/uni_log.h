#ifndef UNI_LOG_H
#define UNI_LOG_H

#include <stdio.h>
#include <stdlib.h>

#define UNI_LOG(fmt, ...) printf(fmt "\n", __VA_ARGS__)

#define UNI_UNREACHABLE() UNI_LOG("UNREACHABLE(): %s:%d", __func__, __LINE__); abort()

#ifdef UNI_DEBUG
#define UNI_DLOG(fmt, ...) UNI_LOG(fmt, __VA_ARGS__)
#else // UNI_DEBUG
#define UNI_DLOG
#endif // !UNI_DEBUG

#endif // UNI_LOG_H

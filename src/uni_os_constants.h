#ifndef UNI_OS_CONSTANTS_H
#define UNI_OS_CONSTANTS_H

#if defined(_WIN32)
#define UNI_OS_WINDOWS
#elif defined(__linux__)
#define UNI_OS_LINUX
#endif // __linux__

#endif // !UNI_OS_CONSTANTS_H

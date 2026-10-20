#define _GNU_SOURCE
#include <stdio.h>
#include <dlfcn.h>
#include <fcntl.h>
#include <stdarg.h>
#include <unistd.h>

int open(const char *pathname, int flags, ...) {
    static int (*original_open)(const char *pathname, int flags, ...); // готовим указатель на оригинальную функцию
    if (!original_open) {
        original_open = dlsym(RTLD_NEXT, "open"); // динамический линкер находит следущую реализацию функции по ее имени из загруженных библиотек
    }
    mode_t mode = 0;
    if (flags & O_CREAT) { // проверка на флаг для создания файла
        // извлекаем mode из аргументов для создания файла
        va_list args;
        va_start(args, flags);
        mode = va_arg(args, mode_t);
        va_end(args);
    }
    int result = original_open(pathname, flags, mode); // выполняем оригинальную функцию
    fprintf(stderr, "[SPY]: open(\"%s\", flags=0x%x) = %d\n", pathname, flags, result); // логируем аргументы и результат
    return result;
}

int openat(int dirfd, const char *pathname, int flags, ...) {
    static int (*original_openat)(int dirfd, const char *pathname, int flags, ...);
    if (!original_openat) {
        original_openat = dlsym(RTLD_NEXT, "openat"); // динамический линкер находит следущую реализацию функции по ее имени из загруженных библиотек
    }
    mode_t mode = 0;
    if (flags & O_CREAT) {  // проверка на флаг для создания файла
        // извлекаем mode из аргументов для создания файла
        va_list args;
        va_start(args, flags);
        mode = va_arg(args, mode_t);
        va_end(args);
    }
    int result = original_openat(dirfd, pathname, flags, mode); // выполняем оригинальную функцию
    const char *str = (dirfd == AT_FDCWD) ? "AT_FDCWD" : "<fd>"; // AT_FDCWD означает, что путь указан относительно текущей рабочей директории программы
    fprintf(stderr, "[SPY]: openat(%s, \"%s\", flags=0x%x) = %d\n",str, pathname, flags, result); // логируем аргументы и результат
    return result;
}

ssize_t read(int fd, void *buf, size_t count) {
    static ssize_t (*original_read)(int fd, void *buf, size_t count);  // готовим указатель на оригинальную функцию
    if (!original_read) {
        original_read = dlsym(RTLD_NEXT, "read"); // динамический линкер находит следущую реализацию функции по ее имени из загруженных библиотек
    }
    ssize_t result = original_read(fd, buf, count);
    fprintf(stderr, "[SPY]: read(fd=%d, buf=%p, count=%zu) = %d\n", fd, buf, count, result); // логируем аргументы и результат
    return result;
}

long int write(int fd, const void* buf, long unsigned count) {
    static long int (*original_write)(int fd, const void* buf, long unsigned count);  // готовим указатель на оригинальную функцию
    if (!original_write) {
        original_write = dlsym(RTLD_NEXT, "write"); // динамический линкер находит следущую реализацию функции по ее имени из загруженных библиотек
    }
    ssize_t result = original_write(fd, buf, count); // выполняем оригинальную функцию
    if (fd != 2) { // проверка на fd != 2 (2 = stderr) чтоб не вызвать рекурсию
        fprintf(stderr, "[SPY] write(fd=%d, buf=%p, count=%zu) = %d\n", fd, buf, count, result); // логируем аргументы и результат
    }
    return result;
}

int close(int fd) {
    static int (*original_close)(int fd); // готовим указатель на оригинальную функцию
    if (!original_close) {
        original_close = dlsym(RTLD_NEXT, "close"); // динамический линкер находит следущую реализацию функции по ее имени из загруженных библиотек
    }
    int result = original_close(fd); // выполняем оригинальную функцию
    fprintf(stderr, "[SPY]: close(fd=%d) = %d\n", fd, result); // логируем аргументы и результат
    return result;
}
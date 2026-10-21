#include <stdio.h>
#include <stdint.h>
#include <x86intrin.h>
#include <unistd.h>
#include <sys/time.h>
#include <fcntl.h>
#include <time.h>

#define ITERATIONS 1000000

// Быстрая функция в userspace
int dummy() {
    return 67;
}

static long long get_ns(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC_RAW, &ts);
    return ts.tv_sec * 1000000000LL + ts.tv_nsec;
}

int main(void) {
    uint64_t start, end;
    long long start_ns, end_ns;

    uint64_t total_dummy = 0, total_getpid = 0, total_open = 0, total_vdso = 0;
    long long ns_dummy = 0, ns_getpid = 0, ns_open = 0, ns_vdso = 0;

    // 1. dummy()
    start_ns = get_ns();
    start = __rdtsc();
    for (int i = 0; i < ITERATIONS; i++) {
        dummy();
    }
    end = __rdtsc();
    end_ns = get_ns();
    ns_dummy = end_ns - start_ns;
    total_dummy = end - start;

    // 2. getpid()
    start_ns = get_ns();
    start = __rdtsc();
    for (int i = 0; i < ITERATIONS; i++) {
        getpid();
    }
    end = __rdtsc();
    end_ns = get_ns();
    ns_getpid = end_ns - start_ns;
    total_getpid = end - start;

    // 3. open() + close()
    int fd_test = open("/tmp/testfile", O_CREAT | O_WRONLY, 0644);
    if (fd_test != -1) close(fd_test); // создаем тестовый файл

    start_ns = get_ns();
    start = __rdtsc();
    for (int i = 0; i < ITERATIONS; i++) {
        int fd = open("/tmp/testfile", O_RDONLY);
        if (fd != -1) close(fd);
    }
    end = __rdtsc();
    end_ns = get_ns();
    ns_open = end_ns - start_ns;
    total_open = end - start;

    // 4. gettimeofday() (VDSO)
    struct timeval tv; // ИСПРАВЛЕНО: Добавлено объявление структуры

    start_ns = get_ns();
    start = __rdtsc();
    for (int i = 0; i < ITERATIONS; i++) {
        gettimeofday(&tv, NULL);
    }
    end = __rdtsc();
    end_ns = get_ns(); // ИСПРАВЛЕНО: Снятие времени в правильном порядке
    ns_vdso = end_ns - start_ns;
    total_vdso = end - start;

    // Считаем среднее количество тактов и наносекунд (ИСПРАВЛЕНО: Расчет ns идет напрямую)
    double avg_ns_dummy  = (double)ns_dummy / ITERATIONS;
    double avg_ns_getpid = (double)ns_getpid / ITERATIONS;
    double avg_ns_open   = (double)ns_open / ITERATIONS;
    double avg_ns_vdso   = (double)ns_vdso / ITERATIONS;

    uint64_t avg_dummy  = total_dummy / ITERATIONS;
    uint64_t avg_getpid = total_getpid / ITERATIONS;
    uint64_t avg_open   = total_open / ITERATIONS;
    uint64_t avg_vdso   = total_vdso / ITERATIONS;

    // Защита от деления на 0, если dummy() выполнился слишком быстро
    if (avg_dummy == 0) avg_dummy = 1;

    char buf_time[32], buf_cycles[32], buf_multiplier[64];

    // Вывод шапки таблицы
    printf("| Операция            | Время (ns) | Циклов CPU | Во сколько раз медленнее userspace |\n");
    printf("|---------------------|------------|------------|------------------------------------|\n");

    // dummy()
    sprintf(buf_time, "%.1f ns", avg_ns_dummy);
    sprintf(buf_cycles, "%lu cycles", avg_dummy);
    printf("| %-19s | %-10s | %-10s | 1x (базовая линия)                 |\n", "dummy()", buf_time, buf_cycles);

    // getpid()
    sprintf(buf_time, "%.1f ns", avg_ns_getpid);
    sprintf(buf_cycles, "%lu cycles", avg_getpid);
    sprintf(buf_multiplier, "%.1fx", (double)avg_getpid / avg_dummy);
    printf("| %-19s | %-10s | %-10s | %-34s |\n", "getpid()", buf_time, buf_cycles, buf_multiplier);

    // open+close
    sprintf(buf_time, "%.1f ns", avg_ns_open);
    sprintf(buf_cycles, "%lu cycles", avg_open);
    sprintf(buf_multiplier, "%.1fx", (double)avg_open / avg_dummy);
    printf("| %-19s | %-10s | %-10s | %-34s |\n", "open+close", buf_time, buf_cycles, buf_multiplier);

    // gettimeofday (VDSO)
    sprintf(buf_time, "%.1f ns", avg_ns_vdso);
    sprintf(buf_cycles, "%lu cycles", avg_vdso);
    sprintf(buf_multiplier, "%.1fx", (double)avg_vdso / avg_dummy);
    printf("| %-19s | %-10s | %-10s | %-34s |\n", "gettimeofday vDSO", buf_time, buf_cycles, buf_multiplier);

    return 0;
}

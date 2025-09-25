// --- High-Resolution C Timer Utility ---
#if defined(_WIN32)
#include <windows.h>
static LARGE_INTEGER timer_freq;
static LARGE_INTEGER timer_start_time;
static const char* timer_name;

void start_timer(const char* name) {
    if (timer_freq.QuadPart == 0) {
        QueryPerformanceFrequency(&timer_freq);
    }
    timer_name = name;
    QueryPerformanceCounter(&timer_start_time);
}

void stop_timer() {
    LARGE_INTEGER end_time;
    QueryPerformanceCounter(&end_time);
    double ms = ((double)(end_time.QuadPart - timer_start_time.QuadPart) / timer_freq.QuadPart) * 1000.0;
    printf("  %s: %.3f ms\n", timer_name, ms);
}

#elif defined(__linux__) || defined(__APPLE__)
#include <time.h>
static struct timespec timer_start_time;
static const char* timer_name;

void start_timer(const char* name) {
    timer_name = name;
    clock_gettime(CLOCK_MONOTONIC, &timer_start_time);
}

void stop_timer() {
    struct timespec end_time;
    clock_gettime(CLOCK_MONOTONIC, &end_time);
    double seconds = (end_time.tv_sec - timer_start_time.tv_sec);
    double nanoseconds = (end_time.tv_nsec - timer_start_time.tv_nsec);
    double ms = (seconds * 1000.0) + (nanoseconds / 1000000.0);
    printf("  %s: %.3f ms\n", timer_name, ms);
}

#else
// Fallback to the original low-resolution timer if no platform is recognized
#include <time.h>
static clock_t timer_start_time;
static const char* timer_name;

void start_timer(const char* name) {
    timer_name = name;
    timer_start_time = clock();
}

void stop_timer() {
    clock_t end_time = clock();
    double ms = ((double)(end_time - timer_start_time) / CLOCKS_PER_SEC) * 1000.0;
    printf("  %s: %.3f ms\n", timer_name, ms);
}
#endif
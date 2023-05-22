#include "Timing.h"
#include "Misc.h"

void foo() {
	Sleep(500);
	int x = 0;
	int y = x + 1;
	int z = y + 2;
	x = z + 3;
}


int base_measure_time(std::function<void()> foo) {
	auto t1 = std::chrono::high_resolution_clock::now();
	foo();
	auto t2 = std::chrono::high_resolution_clock::now();
	auto ms_int = std::chrono::duration_cast<std::chrono::milliseconds>(t2 - t1);
	return ms_int.count();
}

int timer_timeGetTime(std::function<void()> foo) {
	auto t1 = timeGetTime();
	std::thread thr([&]() { foo(); });
	thr.join();
	auto t2 = timeGetTime();
	return t2 - t1;
}

int timer_timegetsystime(std::function<void()> foo) {
	MMTIME mmtime;
	timeGetSystemTime(&mmtime, sizeof(mmtime));
	auto start = mmtime.u.ms;
	foo();
	timeGetSystemTime(&mmtime, sizeof(mmtime));
	auto end = mmtime.u.ms;
	return end - start;
}

int timer_timegetsystimeasfile(std::function<void()> foo) {
	FILETIME lptime;
	SYSTEMTIME systemTime;
	GetSystemTimeAsFileTime(&lptime);
	FileTimeToSystemTime(&lptime, &systemTime);
	auto start = systemTime.wMilliseconds + 1000 * systemTime.wSecond + 60000 * systemTime.wMinute;
	foo();
	GetSystemTimeAsFileTime(&lptime);
	FileTimeToSystemTime(&lptime, &systemTime);
	auto end = systemTime.wMilliseconds + 1000 * systemTime.wSecond + 60000 * systemTime.wMinute;
	return end - start;
}

double PCFreq = 0.0;
__int64 CounterStart = 0;
void StartCounter()
{
	LARGE_INTEGER li;
	QueryPerformanceFrequency(&li);
	PCFreq = double(li.QuadPart) / 1000.0;
	QueryPerformanceCounter(&li);
	CounterStart = li.QuadPart;
}
double GetCounter()
{
	LARGE_INTEGER li;
	QueryPerformanceCounter(&li);
	return double(li.QuadPart - CounterStart) / PCFreq;
}

int timer_QueryPerfomanceCounter(std::function<void()> foo) {
	StartCounter();
	foo();
	return GetCounter();
}

int timer_gettickcount(std::function<void()> foo) {
	auto start = GetTickCount();
	foo();
	return GetTickCount() - start;
}

int timer_gettick64count(std::function<void()> foo) {
	auto start = GetTickCount64();
	foo();
	return GetTickCount64() - start;
}

int timer_getlocaltime(std::function<void()> foo) {
	SYSTEMTIME systemTime;
	GetLocalTime(&systemTime);
	auto start = systemTime.wMilliseconds + 1000 * systemTime.wSecond + 60000 * systemTime.wMinute;
	foo();
	GetLocalTime(&systemTime);
	auto end = systemTime.wMilliseconds + 1000 * systemTime.wSecond + 60000 * systemTime.wMinute;
	return end - start;
}

int timer_rdtsc(std::function<void()> foo) {
	LARGE_INTEGER PerfCountFrequencyResult;
	QueryPerformanceFrequency(&PerfCountFrequencyResult);
	auto PerfCountFrequency = PerfCountFrequencyResult.QuadPart;
	auto start = __rdtsc();
	lfence();
	foo();
	lfence();
	auto end = __rdtsc();
	return (int)((double)(end - start) / PerfCountFrequency * 4.2);
}

bool timer_partstiming() {
	counter t;
	t.setworktime(180);
	std::thread thr([&]() { t.run(); });
	t.set_launch(true);
	foo();
	t.set_finished(true);
	thr.join();
	auto expected = 500 / 180;
	return t.get_count() < expected - 1 || t.get_count() > expected + 1;
}

int timer_usualtiming(std::function<int(std::function<void()>)> timefunction) {
	int time = timefunction([]() { foo(); });
	return time < 500 || time > 600;
}
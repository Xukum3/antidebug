#include<thread>
#include<functional>
#include<atomic>
#include<Windows.h> 
#pragma comment (lib, "winmm.lib")

class counter {
private:
	uint32_t waittime = 10;
	uint32_t worktime = 100;
	int count = 0;
	std::atomic<bool> is_ready = false;
	std::atomic<bool> start_run = false;

public:
	void setworktime(uint32_t time) { 
		worktime = time; 
	}
	void setwaittime(uint32_t time) { 
		waittime = time; 
	}
	void set_launch(bool value) {
		start_run.store(value);
	}
	void set_finished(bool value) {
		is_ready.store(value);
	}
	void run() {
		while (!start_run) {
			Sleep(waittime);
		}
		while (!is_ready) {
			count += 1;
			Sleep(worktime);
		}
		return;
	}

	int get_count() { return count; }
};

#ifdef    NO_LFENCE
#define   lfence()
#else
#include <emmintrin.h>
#define   lfence()  _mm_lfence()
#endif
/*
RPDMC è RDTSC
*/


int base_measure_time(std::function<void()> foo);
int timer_timeGetTime(std::function<void()> foo);
int timer_timegetsystime(std::function<void()> foo);
int timer_timegetsystimeasfile(std::function<void()> foo);
int timer_QueryPerfomanceCounter(std::function<void()> foo);
int timer_gettickcount(std::function<void()> foo);
int timer_gettick64count(std::function<void()> foo);
int timer_getlocaltime(std::function<void()> foo);
int timer_rdtsc(std::function<void()> foo);
bool timer_partstiming();
int timer_usualtiming(std::function<int(std::function<void()>)> timefunction);
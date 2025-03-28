#ifndef TIME_H
#define TIME_H

#include <cstdint>
#include <chrono>

class ElapsedTimer
{
public:
	ElapsedTimer(bool do_start = false)
	{
		if (do_start)
			start();
	}

	void start()
	{
		m_start_point = std::chrono::high_resolution_clock::now();
	}

	uint64_t get_elapsed_ms()
	{
		auto now = std::chrono::high_resolution_clock::now();
		return std::chrono::duration_cast<std::chrono::milliseconds>(now-m_start_point).count();
	}

	uint64_t get_elapsed_us()
	{
		auto now = std::chrono::high_resolution_clock::now();
		return std::chrono::duration_cast<std::chrono::microseconds>(now-m_start_point).count();
	}

private:
	std::chrono::time_point<std::chrono::high_resolution_clock> m_start_point;
};

#endif
#pragma once
#include <chrono>

class Timer
{
	std::chrono::time_point<std::chrono::steady_clock> m_start, m_end;

public:
	Timer()
	{
		m_start = std::chrono::high_resolution_clock::now();
	}

	void elapsed() {
		m_end = std::chrono::high_resolution_clock::now();
		std::chrono::duration<float> duration = m_end - m_start;
		m_start = m_end;

		std::cout << std::fixed << "duration\t" << duration.count() << " sec." << std::endl;
	}

};

#pragma once
#include <cmath>
#include <string>
#include <chrono>
#include <iostream>
class AutoProfiler {
public:
	AutoProfiler(std::string name)
		: m_name(std::move(name)),
		m_beg(std::chrono::high_resolution_clock::now()) { }
	~AutoProfiler() {
		auto end = std::chrono::high_resolution_clock::now();
		auto dur = std::chrono::duration_cast<std::chrono::milliseconds>(end - m_beg);
		std::cout << m_name << " : " << dur.count() << " ms\n" << std::endl;
	}
private:
	std::string m_name;
	std::chrono::time_point<std::chrono::high_resolution_clock> m_beg;
};
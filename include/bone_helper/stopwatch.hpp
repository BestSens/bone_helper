/*
 * stopwatch.hpp
 *
 *  Created on: 13.08.2020
 *	  Author: Markus Jankowski
 */

#ifndef _STOPWATCH_HPP_
#define _STOPWATCH_HPP_

#include <chrono>

namespace bestsens {
	class Stopwatch
	{
	public:
		Stopwatch()
		{
			reset();
		}

		void reset()
		{
			_last = std::chrono::high_resolution_clock::now();
		}

		// return elapsed duration in seconds since last reset
		double elapsed()
		{
			auto now = std::chrono::high_resolution_clock::now();
			return std::chrono::duration<double>(now - _last).count();
		}
	private:
		std::chrono::time_point<std::chrono::high_resolution_clock> _last;
	};
} // namespace bestsens

#endif // _STOPWATCH_HPP_

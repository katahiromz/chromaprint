/*
 * Chromaprint -- Audio fingerprinting toolkit
 * Copyright (C) 2010  Lukas Lalinsky <lalinsky@gmail.com>
 * 
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 * 
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301
 * USA
 */

#ifndef CHROMAPRINT_UTILS_H_
#define CHROMAPRINT_UTILS_H_

#if defined(HAVE_CONFIG_H)
#include <config.h>
#endif

#include <math.h>
#include <stddef.h>
#include <stdint.h>
#include <limits.h>
#include <algorithm>
#include <limits>
#include <iterator>
#include "debug.h"

#ifndef HAVE_ROUND
static inline double round(double x)
{
   if (x >= 0.0)
      return floor(x + 0.5);
   else
      return ceil(x - 0.5);
}
#endif

#define CHROMAPRINT_DISABLE_COPY(ClassName) \
	ClassName(const ClassName &);			\
	void operator=(const ClassName &);

//#include <iostream>

namespace chromaprint
{

	template<class RandomAccessIterator>
	void PrepareHammingWindow(RandomAccessIterator first, RandomAccessIterator last, double scale = 1.0)
	{
		int i = 0;
		const auto size = std::distance(first, last);
		while (first != last) {
			*first++ = scale * (0.54 - 0.46 * cos(i++ * 2.0 * M_PI / (size - 1)));
		}
	}

	template<class InputIterator1, class InputIterator2, class OutputIterator>
	void ApplyWindow(InputIterator1 input, InputIterator2 window, OutputIterator output, int size, double scale)
	{
		while (size--) {
			*output = *input * *window * scale;
			++input;
			++window;
			++output;
		}
	}

	template<class Iterator>
	typename std::iterator_traits<Iterator>::value_type Sum(Iterator first, Iterator last)
	{
		typename std::iterator_traits<Iterator>::value_type sum = 0;
		while (first != last) {
			sum += *first;
			++first;
		}
		return sum;
	}

	template<class Iterator>
	typename std::iterator_traits<Iterator>::value_type EuclideanNorm(Iterator first, Iterator last)
	{
		typename std::iterator_traits<Iterator>::value_type squares = 0;
		while (first != last) {
			squares += *first * *first;
			++first;
		}
		return (squares > 0) ? sqrt(squares) : 0;
	}

	template<class Iterator, class Func>
	void NormalizeVector(Iterator first, Iterator last, Func func, double threshold = 0.01)
	{
		double norm = func(first, last);
		if (norm < threshold) {
			std::fill(first, last, 0.0);
		}
		else {
			while (first != last) {
				*first /= norm;
				++first;
			}
		}
	}

	inline int GrayCode(int i)
	{
		static const unsigned char CODES[] = { 0, 1, 3, 2 };
		return CODES[i];
	}

	inline double IndexToFreq(int i, int frame_size, int sample_rate)
	{
		return double(i) * sample_rate / frame_size;
	}

	inline int FreqToIndex(double freq, int frame_size, int sample_rate)
	{
		return (int)round(frame_size * freq / sample_rate);
	}

	template<class T>
	inline bool IsNaN(T value)
	{
		return value != value;
	}

	inline double FreqToBark(double f)
	{
		double z = (26.81 * f) / (1960.0 + f) - 0.53;

		if (z < 2.0) {
			z = z + 0.15 * (2.0 - z);
		} else if (z > 20.1) {
			z = z + 0.22 * (z - 20.1);
		}

		return z;
	}

	// https://graphics.stanford.edu/~seander/bithacks.html#CountBitsSetParallel
	#define CHROMAPRINT_POPCNT_IMPL(T) \
		v = v - ((v >> 1) & (T)~(T)0/3);                           \
		v = (v & (T)~(T)0/15*3) + ((v >> 2) & (T)~(T)0/15*3);      \
		v = (v + (v >> 4)) & (T)~(T)0/255*15;                      \
		c = (T)(v * ((T)~(T)0/255)) >> (sizeof(T) - 1) * CHAR_BIT; \

//	#define CHROMAPRINT_POPCNT_IMPL_32 CHROMAPRINT_POPCNT_IMPL(uint32_t)
//	#define CHROMAPRINT_POPCNT_IMPL_64 CHROMAPRINT_POPCNT_IMPL(uint64_t)

	#define CHROMAPRINT_POPCNT_IMPL_32 c = __builtin_popcount(v);
	#define CHROMAPRINT_POPCNT_IMPL_64 c = __builtin_popcountll(v);

	template<typename T, int Size, bool IsSigned>
	struct _CountSetBits_Impl {
		static unsigned int Do(T v) {
			return T::not_implemented;
		}
	};

	template<typename T>
	inline unsigned int CountSetBits(T v) {
		return _CountSetBits_Impl<T, sizeof(T), std::numeric_limits<T>::is_signed>::Do(v);
	}

	template<typename T, int Size>
	struct _CountSetBits_Impl<T, Size, true> {
		static unsigned int Do(T v) {
			return CountSetBits(SignedToUnsigned(v));
		}
	};

	template<typename T>
	struct _CountSetBits_Impl<T, 4, false> {
		static unsigned int Do(T v) {
			unsigned int c;
			CHROMAPRINT_POPCNT_IMPL_32;
			return c;
		}
	};

	template<typename T>
	struct _CountSetBits_Impl<T, 8, false> {
		static unsigned int Do(T v) {
			unsigned int c;
			CHROMAPRINT_POPCNT_IMPL_64;
			return c;
		}
	};

	#undef CHROMAPRINT_POPCNT_IMPL
	#undef CHROMAPRINT_POPCNT_IMPL_32
	#undef CHROMAPRINT_POPCNT_IMPL_64

	template<typename T>
	inline unsigned int HammingDistance(T a, T b) {
		return CountSetBits(a ^ b);
	}

};

#endif

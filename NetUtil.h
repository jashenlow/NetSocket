//Copyright(c) 2022 Jashen Low
//
//Permission is hereby granted, free of charge, to any person obtaining
//a copy of this softwareand associated documentation files(the
//	"Software"), to deal in the Software without restriction, including
//	without limitation the rights to use, copy, modify, merge, publish,
//	distribute, sublicense, and /or sell copies of the Software, and to
//	permit persons to whom the Software is furnished to do so, subject to
//	the following conditions :
//
//The above copyright noticeand this permission notice shall be
//included in all copies or substantial portions of the Software.
//
//THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
//EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
//MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
//NONINFRINGEMENT.IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE
//LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION
//OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
//WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

#pragma once
#ifndef _NETUTIL_H_
#define _NETUTIL_H_

#include <type_traits>
#include <string>
#include <algorithm>

/*
* NOTES:
*   - This utility header serves as a platform independent implementation for performing byte-swapping operations.
*/
namespace NetUtil
{
	//Call this to perform byte-swapping on primitive data types.
	template<typename T>
	inline void ByteSwap(T& data)
	{
		if (std::is_fundamental<T>::value && (sizeof(T) > 1))
		{
			char* buff = (char*)&data;			
			std::reverse(buff, buff + sizeof(T));
		}
	}

	//Call this for byte-swapping unicode strings directly from the std container.
	template<typename T>
	inline void ByteSwap(std::basic_string<T>& str)
	{
		//Ignore char type since size == 1.
		if (sizeof(T) > 1)
		{
			for (T& c : str)
			{
				char* buff = (char*)&c;
				std::reverse(buff, buff + sizeof(T));
			}
		}
	}

	template<typename T>
	inline size_t GetStringSizeBytes(const std::basic_string<T>& str)
	{
		return (str.length() + 1) * sizeof(T);
	}
};

#endif

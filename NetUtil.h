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

#include <typeinfo>
#include <string>

/*
* NOTES:
*   - This class serves as a platform independent implementation for performing byte-swapping operations.
*/
class NetUtil
{
public:
	//Call this to perform byte-swapping on primitive data types.
	template<typename T>
	static bool ByteSwap(T& data)
	{
		if (CheckValidType(data))
		{
			char* buff		= (char*)&data;
			uint16_t size	= sizeof(T);
			
			for (uint16_t i = 0; i < (size / 2); i++)
				std::swap(buff[i], buff[size - 1 - i]);

			return true;
		}
		else
			return false;
	}

	//Call this for byte-swapping unicode strings directly from the std container.
	template<typename T>
	static bool ByteSwap(std::basic_string<T>& str)
	{
		//Ignore std::string since it uses the char type.
		if (typeid(T).hash_code() != typeid(char).hash_code())
		{
			char* buff			= (char*)&str[0];
			uint16_t charSize	= sizeof(T);
			
			for (uint16_t i = 0; i < (str.length() * charSize); i += charSize)
			{
				for (uint16_t c = 0; c < (charSize / 2); c++)
					std::swap(buff[i + c], buff[i + charSize - 1 - c]);
			}

			return true;
		}
		else
			return false;
	}

	template<typename T>
	static size_t GetStringSizeBytes(const std::basic_string<T>& str)
	{
		return (str.length() + 1) * sizeof(T);
	}

private:
	NetUtil() {}
	~NetUtil() {}

	template<typename T>
	static inline bool CheckValidType(const T& data)
	{
		size_t type = typeid(T).hash_code();

		return (
			(type == typeid(short).hash_code()) ||
			(type == typeid(unsigned short).hash_code()) ||
			(type == typeid(int).hash_code()) ||
			(type == typeid(unsigned int).hash_code()) ||
			(type == typeid(long).hash_code()) ||
			(type == typeid(unsigned long).hash_code()) ||
			(type == typeid(long long).hash_code()) ||
			(type == typeid(unsigned long long).hash_code()) ||
			(type == typeid(float).hash_code()) ||
			(type == typeid(double).hash_code()) ||
			(type == typeid(long double).hash_code()) ||
			(type == typeid(wchar_t).hash_code()) ||
			(type == typeid(char16_t).hash_code()) ||
			(type == typeid(char32_t).hash_code())
			);
	}
};

#endif

#ifndef STREAMS_37824903284320
#define STREAMS_37824903284320

#include "include/fixed-point/fixed.h"

namespace Streams
{
	// TODO: Change sprintf to snprintf

	namespace Formatters
	{
		namespace details
		{
			// Write format
			template <typename T>
			const char * GetWriteFormatter(T) { return 3.1; /* Don't compile no return*/ }

			template <>
			const char * GetWriteFormatter<double>(double) { return "%lf"; }

			template <>
			const char * GetWriteFormatter<float>(float) { return "%f"; }

			template <>
			const char * GetWriteFormatter<int>(int) { return "%d"; }

			template <>
			const char * GetWriteFormatter<char*>(char*) { return "%s"; }

			template <>
			const char * GetWriteFormatter<char>(char) { return "%c"; }

			// Read format
			template <typename T>
			const char * GetReadFormatter(T) { /* Don't compile no return*/ }

			template <>
			const char * GetReadFormatter<double>(double) { return "%lf%n"; }

			template <>
			const char * GetReadFormatter<float>(float) { return "%f%n"; }

			template <>
			const char * GetReadFormatter<int>(int) { return "%d%n"; }

			template <>
			const char * GetReadFormatter<char*>(char*) { return "%s%n"; }

			// Write Fixed
			template<typename intfmt>
			const char* GetFixedFormat(intfmt&)
			{
				// doesn't compile
				// if the code doesn't compile here add another overload below
			}

			template <>
			const char* GetFixedFormat<long long>(long long&) { return "%s%llu.%.*llu"; }

			template <>
			const char* GetFixedFormat<int>(int&) { return "%s%u.%.*u"; }

			template <>
			const char* GetFixedFormat<short>(short&) { return "%s%u.%.*u"; }

			template <>
			const char* GetFixedFormat<signed char>(signed char&) { return "%s%u.%.*u"; }

			template <typename T>
			struct Formatter
			{
				Formatter(T v, const char* fmt)
					: _v(v),
					_fmt(fmt)
				{}
				T _v;
				char const* _fmt;
			};
		}

		template <typename T>
		details::Formatter<T> Format(T v, const char* fmt)
		{
			return details::Formatter<T>(v, fmt);
		}
	}

	template<size_t length>
	struct ReadStream
	{
		const char* data;
		const char* end;
		bool good;
		int read_count;
		ReadStream(const char* buf)
			: data(buf)
			, end(data + length)
			, good(true)
			, read_count(0)
		{}
		enum{ Length = length };
		inline int get_remaining() const
		{
			return (end - data);
		}

		operator bool()
		{
			return good;
		}
	};

	template<size_t N>
	ReadStream<N> MakeReadStream(const char(&arr)[N])
	{
		return ReadStream<N>(arr);
	}

	template<size_t N, size_t M>
	ReadStream<N>& operator>>(ReadStream<N>& str, char(&buf)[M])
	{
		if (str.good)
		{
			if (str.data <= str.end)
			{
				char* output = buf;

				while ((str.data <= str.end)
					&& (*str.data)
					&& (output < (buf + M))
					&& (*str.data != ' ')
					&& (*str.data != '\r')
					&& (*str.data != '\n')
					&& (*str.data != '\t')
					&& (*str.data != '\v')
					&& (*str.data != '\f'))
				{
					*output = *str.data;
					++output;
					++str.data;
				}

				if (output < buf + M)
				{
					*output = 0; // null terminator
					str.read_count++;
				}
				else
				{
					cout << "operator>> destination buffer is too short...\n";
					str.good = false; // We over flowed the output buffer
				}
				return str;
			}
		}
		return str;

	}

	template<size_t N, typename T>
	ReadStream<N>& operator>>(ReadStream<N>& str, T& t)
	{
		if (str.good)
		{
			if (str.data <= str.end)
			{
				int count = 0;
				int r = sscanf(str.data,
					Formatters::details::GetReadFormatter(t),
					&t, &count);
				str.data += count;
				if (r < 0)
				{
					str.good = false;
				}
				else
				{
					str.read_count++;
				}
			}
			else
			{
				str.good = false;
			}
		}
		return str;
	}

	// Read in 'non-value' format part of the string
	template<size_t N, size_t M>
	ReadStream<N>& operator>>(ReadStream<N>& str, const char(&buf)[M])
	{
		if (str.good)
		{
			// copy data from ReadStream into buf
			const char* p = buf;
			while (
				(str.data <= str.end) && // Dont go beyond the end of the stream
				*p &&			// Stop at null-terminator
				((p-buf) < M))	// Stop at end of char buffer
			{
				if (*str.data != *p)
				{
					str.good = false;
					return str;
				}
				str.data++;
				p++;
			}
		}
		return str;
	}

	template<size_t length>
	struct WriteStream
	{
		char *data;
		char *end;
		WriteStream(char* buf) : data(buf), end(buf) { *buf = 0; }
		enum { Length = length };
		inline size_t get_free() const
		{
			return length - (end - data) - 1; // always make room for terminator
		}
	};

	template<size_t N>
	WriteStream<N> MakeWriteStream(char(&arr)[N])
	{
		return WriteStream<N>(arr);
	}

	template<size_t N>
	WriteStream<N>& operator<<(WriteStream<N>& str, const char* input)
	{
		// append a string
		auto free = str.get_free();
		while (free && *input)
		{
			*str.end++ = *input++;
			free--;
		}
		*str.end = '\0';
		return str;
	}

	template<size_t N, typename T>
	WriteStream<N>&
		PrintToWriteStream(WriteStream<N>& str, const T& val, const char* fmt)
	{
		auto free = str.get_free();

		if (free >= 0)
		{
			// Change sprintf to snprintf to avoid overflows
			int n = sprintf(str.end, fmt, val);
			_ASSERT(n <= (int)free);	// we have over run the array: crash
			str.end += n;
		}
		return str;
	}


	template<size_t N, typename T>
	WriteStream<N>& operator<<(WriteStream<N>& str, T&& input)
	{
		return PrintToWriteStream(str, input,
			Formatters::details::GetWriteFormatter(input));
	}

	template<size_t N, typename T>
	WriteStream<N>& 
		operator<<
		(WriteStream<N>& str, Formatters::details::Formatter<T>&& input)
	{
		return PrintToWriteStream(str, input._v, input._fmt);
	}

	template<size_t N, size_t dps, typename intfmt>
	WriteStream<N>& operator<<(WriteStream<N>& str, FixedPoint::Fixed<dps, intfmt>& val)
	{
		auto free = str.get_free();

		if (free > 0)
		{
			int n = sprintf(str.end,
				Formatters::details::GetFixedFormat(val.m_Value),
				(val.m_Value < 0) ? "-" : "",
				(val.m_Value < 0) ? -val.get_integral() : val.get_integral(), dps,
				(val.m_Value < 0) ? -val.get_fractional() : val.get_fractional());

			_ASSERT(n <= (int)free);	// we have over run the array: crash
			str.end += n;
		}

		return str;
	}

	template<size_t N, size_t dps, typename intfmt>
	WriteStream<N>& operator<<(WriteStream<N>& str, FixedPoint::Fixed<dps, intfmt>&& val)
	{
		return operator<<(str, (FixedPoint::Fixed<dps, intfmt>&)val); // Call the non r-value ref?
	}

	template<size_t N, typename intfmt>
	WriteStream<N>& operator<<(WriteStream<N>& str, FixedPoint::RTFixed<intfmt>& val)
	{
		auto free = str.get_free();

		if (free > 0)
		{
			int n = sprintf(str.end,
				Formatters::details::GetFixedFormat(val.m_Value),
				(val.m_Value < 0) ? "-" : "",
				(val.m_Value < 0) ? -val.get_integral() : val.get_integral(), val.m_dps,
				(val.m_Value < 0) ? -val.get_fractional() : val.get_fractional());

			_ASSERT(n <= (int)free);	// we have over run the array: crash
			str.end += n;
		}

		return str;
	}

	template<size_t N, typename intfmt>
	WriteStream<N>& operator<<(WriteStream<N>& str, FixedPoint::RTFixed<intfmt>&& val)
	{
		return operator<<(str, (FixedPoint::RTFixed<intfmt>&)val); // Call the non r-value ref?
	}
}

#endif

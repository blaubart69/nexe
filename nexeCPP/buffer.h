#pragma once

#include <string>
#include <tuple>

class buffer
{
	const char* m_prefixForEachLine;
	std::string m_buf;

public:
	buffer()					{ m_prefixForEachLine = NULL; }
	buffer(const char* prefix)	{ m_prefixForEachLine = prefix; }
	~buffer()					{ }
	void append(_In_ const char* const textdata, _In_ const size_t len);
	//const char* data()			{ return m_buf.c_str();  }
	//size_t length() const		{ return m_buf.length(); }
	std::tuple<const char*,size_t> data() const { return { m_buf.c_str(), m_buf.length() }; }
};


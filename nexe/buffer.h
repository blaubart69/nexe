#pragma once

#include <string>

class buffer
{
	const char* m_prefixForEachLine;
	std::string m_buf;

public:
	buffer()					{ m_prefixForEachLine = NULL; }
	buffer(const char* prefix)	{ m_prefixForEachLine = prefix; }
	~buffer()					{ }
	void append(const char const* textdata, const size_t len);
	const char* data()			{ return m_buf.c_str();  }
	size_t length() const		{ return m_buf.length(); }
};


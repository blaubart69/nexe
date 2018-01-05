#include "buffer.h"

void buffer::append(const char const* textdata, const size_t len)
{
	bool newLineWritten = false;

	if (m_buf.length() == 0 && len > 0) {
		newLineWritten = true;
	}

	const char* pos = textdata;
	for (int i = 0; i < len; i++, pos++) {
		if (newLineWritten && m_prefixForEachLine != NULL) {
			m_buf.append(m_prefixForEachLine);
			m_buf.push_back('\t');
		}
		m_buf.push_back(*pos);
		newLineWritten = *pos == '\n';
	}
}

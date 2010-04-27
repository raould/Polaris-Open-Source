#pragma once

#include <stdio.h>

class auto_close_file
{
	FILE* m_file;

public:
	auto_close_file(FILE* file) : m_file(file) {}
	~auto_close_file()
	{
		fclose(m_file);
	}
};
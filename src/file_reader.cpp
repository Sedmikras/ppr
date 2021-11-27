#include "filereader.h"
#include "open_cl_default_header.h"

void percentile_finder::reset_filereader(std::ifstream& file)
{
	if (file.is_open()) {
		file.clear();
		file.seekg(std::ios::beg);
	}
}
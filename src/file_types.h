#pragma once

#include "utilities.hpp"

#include <string>
#include <vector>

class FileTypes
{
public:
	void add_type(const std::vector<STRING>&);
	bool high_entropy(FILE*, const STRING&);

private:
	// ext, offset, data


	struct t{
		STRING extension;
		uint64_t offset;
		std::string data;
	};

	std::vector<t> types;

};
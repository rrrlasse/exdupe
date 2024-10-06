module;

#include "utilities.hpp"

#include <string>
#include <vector>

export module FileTypes;

export class FileTypes
{
public:
	void add_type(const std::vector<STRING>& extensions) {
		for (auto e : extensions) {
#ifdef _WIN32
			e = lcase(e);
#endif
			types.push_back({ UNITXT(".") + e, {}, {} });
		}
	}

	bool high_entropy(FILE*, const STRING& filename) {
#ifdef _WIN32
		STRING f = lcase(filename);
#else
		STRING& f = filename;
#endif

		for (auto& t : types) {
			if (f.ends_with(t.extension)) {
				return true;
			}
		}
		return false;
	}

private:
	// ext, offset, data


	struct t {
		STRING extension;
		uint64_t offset;
		std::string data;
	};

	std::vector<t> types;

};


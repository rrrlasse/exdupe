#include "file_types.h"


void FileTypes::add_type(const std::vector<STRING>& extensions) {
	for(auto e : extensions) {
#ifdef _WIN32
		e = lcase(e);
#endif
		types.push_back({ UNITXT(".") + e, {}, {}});
	}
}

bool FileTypes::high_entropy(FILE*, const STRING& filename) {
#ifdef _WIN32
	STRING f = lcase(filename);
#else
	STRING& f = filename;
#endif

	for(auto& t : types) {
		if(f.ends_with(t.extension)) {
			return true;
		}
	}
	return false;
}
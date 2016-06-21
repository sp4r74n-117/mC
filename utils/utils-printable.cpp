#include "utils/utils-printable.h"

namespace utils {

	std::string toString(const Printable& printable) {
		std::stringstream ss;
		printable.printTo(ss);
		return ss.str();
	}

	std::string toString(const Ptr<Printable>& printable) {
		return toString(*printable);
	}
}

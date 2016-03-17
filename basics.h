#pragma once

#include <string>
#include <memory>
#include <typeinfo>

using std::string;
template<class T>
using sptr = std::shared_ptr<T>;

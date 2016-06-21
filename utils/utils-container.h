#pragma once
#include "utils/utils-pointer.h"
#include <vector>
#include <set>
#include <unordered_map>
#include <unordered_set>
#include <map>
#include <type_traits>
#include <algorithm>
#include <iterator>
#include <functional>
#include <cassert>

namespace utils {
	template<typename T>
	using PtrList = std::vector<Ptr<T>>;

	template<typename THead>
  size_t combine_hash(THead head) {
    return head;
  }

  template<typename THead, typename... TArgs>
  size_t combine_hash(THead head, TArgs... args) {
    return head + 31 * combine_hash(args...);
  }

	template<typename T>
	struct target_less {
		bool operator()(const Ptr<T>& lhs, const Ptr<T>& rhs) const {
			return *lhs < *rhs;
		}
	};

	template<typename T>
	struct target_hash {
		size_t operator()(const Ptr<T>& ptr) const {
			return std::hash<T>()(*ptr);
		}
	};

	template<typename T>
	struct target_equal {
		bool operator()(const Ptr<T>& lhs, const Ptr<T>& rhs) const {
			return *lhs == *rhs;
		}
	};

	template<typename T>
	using PtrSet = std::set<Ptr<T>, target_less<T>>;

	template<typename TKey, typename TValue>
	using PtrMap = std::map<Ptr<TKey>, Ptr<TValue>, target_less<TKey>>;

	namespace detail {
		template<typename InputIt1, typename InputIt2>
		bool target_equals(InputIt1 it1, InputIt2 it2, size_t N) {
			for (size_t i = 0; i < N; ++it1, ++it2, ++i) {
				if ((**it1) != (**it2)) return false;
			}
			return true;
		}

		template<typename TContainer>
		bool equals(const TContainer& lhs, const TContainer& rhs) {
			if (lhs.size() != rhs.size()) return false;
			return target_equals(std::begin(lhs), std::begin(rhs), lhs.size());
		}
	}

	template<typename T>
	bool operator ==(const PtrSet<T>& lhs, const PtrSet<T>& rhs) {
		return detail::equals(lhs, rhs);
	}

	template<typename T>
	bool operator !=(const PtrSet<T>& lhs, const PtrSet<T>& rhs) {
		return !(lhs == rhs);
	}

	template<typename T>
	bool operator ==(const PtrList<T>& lhs, const PtrList<T>& rhs) {
		return detail::equals(lhs, rhs);
	}

	template<typename T>
	bool operator !=(const PtrList<T>& lhs, const PtrList<T>& rhs) {
		return !(lhs == rhs);
	}

	namespace detail {
		template<typename THead>
		void toVector(std::vector<THead>& result, THead head) {
			result.push_back(head);
		}

		template<typename THead, typename... TTail>
		void toVector(std::vector<THead>& result, THead head, TTail... tail) {
			result.push_back(head);
			toVector(result, tail...);
		}
	}

	template<typename THead, typename... TTail>
	std::vector<THead> toVector(THead head, TTail... tail) {
		std::vector<THead> result;
		result.reserve(sizeof...(tail) + 1);
		detail::toVector(result, head, tail...);
		return result;
	}

	namespace algorithm {
		template<typename T>
		PtrSet<T> set_union(const PtrSet<T>& lhs, const PtrSet<T>& rhs) {
			PtrSet<T> result;
			std::set_union(std::begin(lhs), std::end(lhs), std::begin(rhs), std::end(rhs),
				std::inserter(result, result.begin()), target_less<T>());
			return result;
		}

		template<typename T>
		PtrSet<T> set_intersection(const PtrSet<T>& lhs, const PtrSet<T>& rhs) {
			PtrSet<T> result;
			std::set_intersection(std::begin(lhs), std::end(lhs), std::begin(rhs), std::end(rhs),
				std::inserter(result, result.begin()), target_less<T>());
			return result;
		}

		template<typename T>
		PtrSet<T> set_difference(const PtrSet<T>& lhs, const PtrSet<T>& rhs) {
			PtrSet<T> result;
			std::set_difference(std::begin(lhs), std::end(lhs), std::begin(rhs), std::end(rhs),
				std::inserter(result, result.begin()), target_less<T>());
			return result;
		}
	}

	template<typename T, typename U>
	void appendAll(T& dst, const U& src) {
		dst.insert(std::end(dst), std::begin(src), std::end(src));
	}

	template<typename T>
	class InstanceManager {
		typedef std::unordered_set<Ptr<T>, target_hash<T>, target_equal<T>> storage_type;
		storage_type storage;
	public:
		template<typename U>
		typename std::enable_if<std::is_base_of<T, U>::value || std::is_same<T, U>::value, Ptr<U>>::type
		add(const Ptr<U>& instance) {
			auto it = storage.find(instance);
			if (it != storage.end()) {
				auto res = dyn_cast<U>(*it);
				assert(res && "invalid instance manager state");
				return res;
			}

			storage.insert(instance);
			return instance;
		}
	};
}

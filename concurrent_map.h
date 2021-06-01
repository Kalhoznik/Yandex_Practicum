#pragma once

#include <cstdlib>
#include <map>
#include <algorithm>
#include <string>
#include <vector>
#include <mutex>
#include <execution>

using namespace std::string_literals;

template <typename Key, typename Value>
class ConcurrentMap {
public:

	struct Backet {
		std::map<Key, Value> map;
		std::mutex mutex;
	};

	static_assert(std::is_integral_v<Key>, "ConcurrentMap supports only integer keys");

	struct Access {
		Access(Backet& backet, Key key)
			:guard(backet.mutex)
			,ref_to_value(backet.map[key])
		{ }

		std::lock_guard<std::mutex> guard;
		Value& ref_to_value;
	};


	explicit ConcurrentMap(size_t bucket_count) : bucket_count_(bucket_count), buckets_(bucket_count) { }


	Access operator[](const Key& key) {
		uint64_t key_ = static_cast<uint64_t>(key);
		uint64_t index = key_ % bucket_count_;
		return Access(buckets_[index], key_);
	}


	std::map<Key, Value> BuildOrdinaryMap() {
		std::map<Key, Value> return_map;
		for (size_t i = 0; i < bucket_count_; ++i) {
			std::lock_guard g(buckets_[i].mutex);
			return_map.insert(buckets_[i].map.begin(), buckets_[i].map.end());
		}
		return return_map;
	}

private:
	size_t bucket_count_;
	std::vector<Backet> buckets_;
};


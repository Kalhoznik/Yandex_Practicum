#pragma once

#include <cstdlib>
#include <map>
#include <algorithm>
#include <mutex>
#include <execution>



template <typename Key, typename Value>
class ConcurrentMap {

	struct Backet {
		std::map<Key, Value> map;
		std::mutex mutex;
	};
	std::vector<Backet> buskets_;

public:
	static_assert(std::is_integral_v<Key>, "ConcurrentMap supports only integer keys");

	   
	struct Access {
	

		Access(Backet& backet, Key key)
			:guard(backet.mutex)
			, ref_to_value(backet.map[std::move(key)])
		{ }

		std::lock_guard<std::mutex> guard;
		Value& ref_to_value;
	};

	explicit ConcurrentMap(size_t bucket_count)
		:buskets_(bucket_count)
	{ }

	Access operator[](const Key& key) {
		uint64_t key_ = static_cast<uint64_t>(key);
		const uint64_t index = key_ % buskets_.size();
		return Access(buskets_[index], key_);
	}

	std::map<Key, Value> BuildOrdinaryMap() {
		std::map<Key, Value> return_map;
		for (auto& basked : buskets_) {
			std::lock_guard g(basked.mutex);
			return_map.insert(basked.map.begin(), basked.map.end());
		}

		return return_map;
	}

	void Erase(const Key& key) {
		uint64_t key_ = static_cast<uint64_t>(key);
		const uint64_t index = key_ % buskets_.size();

		std::lock_guard(buskets_[index].mutex);
		buskets_[index].map.erase(key_);
	}

	
};


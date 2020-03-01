#ifndef HYBRID_MAP_H_INCLUDED
#define HYBRID_MAP_H_INCLUDED

#include "hybrid_cont.h"

#include <unordered_map>
#include <map>
#include <cstring>

namespace aux
{
	template <class Key, class Value>
	class hybrid_map
	{
		typedef std::unordered_map<Key, Value> unordered_map;
		typedef std::map<Key, Value> ordered_map;
		union {
			unordered_map umap;
			ordered_map omap;
		};
		bool ordered;

	public:
		typedef impl::hybrid_iterator<typename unordered_map::iterator, typename ordered_map::iterator> iterator;
		typedef impl::hybrid_iterator<typename unordered_map::const_iterator, typename ordered_map::const_iterator> const_iterator;
		typedef typename impl::assert_same<typename unordered_map::reference, typename ordered_map::reference>::type reference;
		typedef typename impl::assert_same<typename unordered_map::const_reference, typename ordered_map::const_reference>::type const_reference;
		typedef typename impl::assert_same<typename unordered_map::value_type, typename ordered_map::value_type>::type value_type;
		typedef typename impl::assert_same<typename unordered_map::size_type, typename ordered_map::size_type>::type size_type;

		hybrid_map() : umap(), ordered(false)
		{
		
		}

		hybrid_map(bool ordered) : ordered(ordered)
		{
			if(ordered)
			{
				new (&omap) ordered_map();
			}else{
				new (&umap) unordered_map();
			}
		}

		hybrid_map(const unordered_map &map) : umap(map), ordered(false)
		{

		}

		hybrid_map(unordered_map &&map) : umap(std::move(map)), ordered(false)
		{

		}

		hybrid_map(const ordered_map &map) : omap(map), ordered(true)
		{

		}


		hybrid_map(ordered_map &&map) : omap(std::move(map)), ordered(true)
		{

		}

		hybrid_map(const hybrid_map<Key, Value> &map) : ordered(map.ordered)
		{
			if(ordered)
			{
				new (&omap) ordered_map(map.omap);
			}else{
				new (&umap) unordered_map(map.umap);
			}
		}

		hybrid_map(hybrid_map<Key, Value> &&map) : ordered(map.ordered)
		{
			if(ordered)
			{
				new (&omap) ordered_map(std::move(map.omap));
			}else{
				new (&umap) unordered_map(std::move(map.umap));
			}
		}

		hybrid_map<Key, Value> &operator=(const unordered_map &map)
		{
			if(ordered)
			{
				omap.~ordered_map();
				new (&umap) unordered_map(map);
				ordered = false;
			}else{
				umap = map;
			}
			return *this;
		}

		hybrid_map<Key, Value> &operator=(unordered_map &&map)
		{
			if(ordered)
			{
				omap.~ordered_map();
				new (&umap) unordered_map(std::move(map));
				ordered = false;
			}else{
				umap = std::move(map);
			}
			return *this;
		}
	
		hybrid_map<Key, Value> &operator=(const ordered_map &map)
		{
			if(!ordered)
			{
				umap.~unordered_map();
				new (&omap) ordered_map(map);
				ordered = true;
			}else{
				omap = map;
			}
			return *this;
		}

		hybrid_map<Key, Value> &operator=(ordered_map &&map)
		{
			if(!ordered)
			{
				umap.~unordered_map();
				new (&omap) ordered_map(std::move(map));
				ordered = true;
			}else{
				omap = std::move(map);
			}
			return *this;
		}

		hybrid_map<Key, Value> &operator=(const hybrid_map<Key, Value> &map)
		{
			if(ordered && map.ordered)
			{
				omap = map.omap;
			}else if(!ordered && !map.ordered)
			{
				umap = map.umap;
			}else if(ordered)
			{
				omap.~ordered_map();
				new (&umap) unordered_map(map.umap);
				ordered = false;
			}else{
				umap.~unordered_map();
				new (&omap) ordered_map(map.omap);
				ordered = true;
			}
			return *this;
		}

		hybrid_map<Key, Value> &operator=(hybrid_map<Key, Value> &&map)
		{
			if(ordered && map.ordered)
			{
				omap = std::move(map.omap);
			}else if(!ordered && !map.ordered)
			{
				umap = std::move(map.umap);
			}else if(ordered)
			{
				omap.~ordered_map();
				new (&umap) unordered_map(std::move(map.umap));
				ordered = false;
			}else{
				umap.~unordered_map();
				new (&omap) ordered_map(std::move(map.omap));
				ordered = true;
			}
			return *this;
		}

		Value &operator[](const Key &key)
		{
			if(ordered)
			{
				return omap[key];
			}else{
				return umap[key];
			}
		}

		Value &operator[](Key &&key)
		{
			if(ordered)
			{
				return omap[std::move(key)];
			} else {
				return umap[std::move(key)];
			}
		}

		const Value &operator[](const Key &key) const
		{
			if(ordered)
			{
				return omap[key];
			}else{
				return umap[key];
			}
		}

		iterator begin()
		{
			if(ordered)
			{
				return omap.begin();
			}else{
				return umap.begin();
			}
		}
		
		iterator end()
		{
			if(ordered)
			{
				return omap.end();
			}else{
				return umap.end();
			}
		}

		const_iterator begin() const
		{
			if(ordered)
			{
				return omap.begin();
			}else{
				return umap.begin();
			}
		}

		const_iterator end() const
		{
			if(ordered)
			{
				return omap.end();
			}else{
				return umap.end();
			}
		}

		const_iterator cbegin() const
		{
			if(ordered)
			{
				return omap.cbegin();
			}else{
				return umap.cbegin();
			}
		}

		const_iterator cend() const
		{
			if(ordered)
			{
				return omap.cend();
			}else{
				return umap.cend();
			}
		}

		size_type count() const
		{
			if(ordered)
			{
				return omap.count();
			}else{
				return umap.count();
			}
		}

		size_type size() const
		{
			if(ordered)
			{
				return omap.size();
			}else{
				return umap.size();
			}
		}

		size_type capacity() const
		{
			if(ordered)
			{
				return -1;
			}else{
				return static_cast<size_type>(umap.bucket_count() * umap.max_load_factor());
			}
		}

		void reserve(size_type count)
		{
			if(!ordered)
			{
				umap.reserve(count);
			}
		}

		void clear()
		{
			if(ordered)
			{
				omap.clear();
			}else{
				umap.clear();
			}
		}

		iterator find(const Key &key)
		{
			if(ordered)
			{
				return omap.find(key);
			}else{
				return umap.find(key);
			}
		}

		const_iterator find(const Key &key) const
		{
			if(ordered)
			{
				return omap.find(key);
			}else{
				return umap.find(key);
			}
		}

		size_type erase(const Key &key)
		{
			if(ordered)
			{
				return omap.erase(key);
			}else{
				return umap.erase(key);
			}
		}

		iterator erase(iterator it)
		{
			if(ordered)
			{
				return omap.erase(it);
			}else{
				return umap.erase(it);
			}
		}

		std::pair<iterator, bool> insert(value_type &&val)
		{
			if(ordered)
			{
				auto pair = omap.insert(std::move(val));
				return std::make_pair(iterator(pair.first), pair.second);
			}else{
				auto pair = umap.insert(std::move(val));
				return std::make_pair(iterator(pair.first), pair.second);
			}
		}

		template<class... Args>
		std::pair<iterator, bool> emplace(Args&&... args)
		{
			if(ordered)
			{
				auto pair = omap.emplace(std::forward<Args>(args)...);
				return std::make_pair(iterator(pair.first), pair.second);
			}else{
				auto pair = umap.emplace(std::forward<Args>(args)...);
				return std::make_pair(iterator(pair.first), pair.second);
			}
		}

		template <class InputIterator>
		void insert(InputIterator first, InputIterator last)
		{
			if(ordered)
			{
				omap.insert(first, last);
			}else{
				umap.insert(first, last);
			}
		}

		bool is_ordered() const
		{
			return ordered;
		}

		bool set_ordered(bool ordered)
		{
			if(this->ordered != ordered)
			{
				if(this->ordered)
				{
					std::unordered_map<Key, Value> map(std::make_move_iterator(omap.begin()), std::make_move_iterator(omap.end()));
					*this = std::move(map);
				}else{
					std::map<Key, Value> map(std::make_move_iterator(umap.begin()), std::make_move_iterator(umap.end()));
					*this = std::move(map);
				}
				return true;
			}
			return false;
		}

		void swap(hybrid_map<Key, Value> &map)
		{
			if(ordered && map.ordered)
			{
				std::swap(omap, map.omap);
			}else if(!ordered && !map.ordered)
			{
				std::swap(umap, map.umap);
			}else{
				hybrid_map<Key, Value> tmp(std::move(map));
				map = std::move(*this);
				*this = std::move(tmp);
			}
		}

		~hybrid_map()
		{
			if(ordered)
			{
				omap.~ordered_map();
			}else{
				umap.~unordered_map();
			}
		}
	};
}

namespace std
{
	template <class Key, class Value>
	void swap(::aux::hybrid_map<Key, Value> &a, ::aux::hybrid_map<Key, Value> &b)
	{
		a.swap(b);
	}
}

#endif

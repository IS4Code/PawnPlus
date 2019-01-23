#ifndef HYBRID_MAP_H_INCLUDED
#define HYBRID_MAP_H_INCLUDED

#include <unordered_map>
#include <map>
#include <type_traits>
#include <cstring>

#if __GNUG__ && __GNUC__ < 5
#define is_trivially_copyable(T) __has_trivial_copy(T)
#define is_trivially_copy_assignable(T) __has_trivial_assign(T)
#define is_trivially_destructible(T) __has_trivial_destructor(T)
#else
#define is_trivially_copyable(T) std::is_trivially_copyable<T>::value
#define is_trivially_copy_assignable(T) std::is_trivially_copy_assignable<T>::value
#define is_trivially_destructible(T) std::is_trivially_destructible<T>::value
#endif

namespace aux
{
	namespace impl
	{
		template <class T, class U>
		struct assert_same
		{
			typedef typename std::conditional<std::is_same<T, U>::value, T, void>::type type;
		};
	}

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
		template <class unordered_iterator, class ordered_iterator>
		class iterator_base
		{
			union {
				unordered_iterator uiterator;
				ordered_iterator oiterator;
			};
			bool ordered;

		public:
			typedef typename impl::assert_same<typename unordered_iterator::difference_type, typename ordered_iterator::difference_type>::type difference_type;
			typedef typename impl::assert_same<typename unordered_iterator::value_type, typename ordered_iterator::value_type>::type value_type;
			typedef typename impl::assert_same<typename unordered_iterator::pointer, typename ordered_iterator::pointer>::type pointer;
			typedef typename impl::assert_same<typename unordered_iterator::reference, typename ordered_iterator::reference>::type reference;
			typedef std::bidirectional_iterator_tag iterator_category;

			iterator_base() : uiterator(), ordered(false)
			{

			}

			iterator_base(const unordered_iterator &it) : uiterator(it), ordered(false)
			{
			
			}

			iterator_base(const ordered_iterator &it) : oiterator(it), ordered(true)
			{

			}

			iterator_base(const iterator_base<unordered_iterator, ordered_iterator> &it)
			{
				if(is_trivially_copyable(unordered_iterator) && is_trivially_copyable(ordered_iterator))
				{
					std::memcpy(this, &it, sizeof(it));
				}else{
					ordered = it.ordered;
					if(ordered)
					{
						new (&oiterator) ordered_iterator(it.oiterator);
					}else{
						new (&uiterator) unordered_iterator(it.uiterator);
					}
				}
			}

			operator unordered_iterator&()
			{
				return uiterator;
			}

			operator ordered_iterator&()
			{
				return oiterator;
			}

			operator const unordered_iterator&() const
			{
				return uiterator;
			}

			operator const ordered_iterator&() const
			{
				return oiterator;
			}

			iterator_base<unordered_iterator, ordered_iterator> &operator=(const unordered_iterator &it)
			{
				if((!is_trivially_destructible(ordered_iterator) || !is_trivially_copy_assignable(unordered_iterator)) && ordered)
				{
					oiterator.~ordered_iterator();
					new (&uiterator) unordered_iterator(it);
				}else{
					uiterator = it;
				}
				ordered = false;
				return *this;
			}

			iterator_base<unordered_iterator, ordered_iterator> &operator=(const ordered_iterator &it)
			{
				if((!is_trivially_destructible(unordered_iterator) || !is_trivially_copy_assignable(ordered_iterator)) && !ordered)
				{
					uiterator.~unordered_iterator();
					new (&oiterator) ordered_iterator(it);
				}else{
					oiterator = it;
				}
				ordered = true;
				return *this;
			}

			iterator_base<unordered_iterator, ordered_iterator> &operator=(const iterator_base<unordered_iterator, ordered_iterator> &it)
			{
				if(this != &it)
				{
					if(is_trivially_copy_assignable(unordered_iterator) && is_trivially_copy_assignable(ordered_iterator))
					{
						std::memcpy(this, &it, sizeof(it));
					}else{
						if(ordered && it.ordered)
						{
							oiterator = it.oiterator;
						}else if(!ordered && !it.ordered)
						{
							uiterator = it.uiterator;
						}else if(ordered)
						{
							oiterator.~ordered_iterator();
							new (&uiterator) unordered_iterator(it.uiterator);
							ordered = false;
						}else{
							uiterator.~unordered_iterator();
							new (&oiterator) ordered_iterator(it.oiterator);
							ordered = true;
						}
					}
				}
				return *this;
			}

			typename impl::assert_same<decltype(*uiterator), decltype(*uiterator)>::type operator*() const
			{
				if(ordered)
				{
					return *oiterator;
				}else{
					return *uiterator;
				}
			}

			typename impl::assert_same<decltype(&*uiterator), decltype(&*uiterator)>::type operator->() const
			{
				return &**this;
			}

			iterator_base<unordered_iterator, ordered_iterator> &operator++()
			{
				if(ordered)
				{
					++oiterator;
				}else{
					++uiterator;
				}
				return *this;
			}

			iterator_base<unordered_iterator, ordered_iterator> operator++(int)
			{
				auto tmp = *this;
				++*this;
				return tmp;
			}

			iterator_base<unordered_iterator, ordered_iterator> &operator--()
			{
				if(ordered)
				{
					--oiterator;
				}
				return *this;
			}

			iterator_base<unordered_iterator, ordered_iterator> operator--(int)
			{
				auto tmp = *this;
				--*this;
				return tmp;
			}

			bool operator==(const iterator_base<unordered_iterator, ordered_iterator> &obj) const
			{
				if(ordered && obj.ordered)
				{
					return oiterator == obj.oiterator;
				}else if(!ordered && !obj.ordered)
				{
					return uiterator == obj.uiterator;
				}else{
					return false;
				}
			}

			bool operator!=(const iterator_base<unordered_iterator, ordered_iterator> &obj) const
			{
				if(ordered && obj.ordered)
				{
					return oiterator != obj.oiterator;
				}else if(!ordered && !obj.ordered)
				{
					return uiterator != obj.uiterator;
				}else{
					return true;
				}
			}

			~iterator_base()
			{
				if(!is_trivially_destructible(unordered_iterator) || !is_trivially_destructible(ordered_iterator))
				{
					if(ordered)
					{
						oiterator.~ordered_iterator();
					}else{
						uiterator.~unordered_iterator();
					}
				}
			}
		};

		typedef iterator_base<typename unordered_map::iterator, typename ordered_map::iterator> iterator;
		typedef iterator_base<typename unordered_map::const_iterator, typename ordered_map::const_iterator> const_iterator;
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

		void set_ordered(bool ordered)
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

#endif

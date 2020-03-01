#ifndef HYBRID_POOL_H_INCLUDED
#define HYBRID_POOL_H_INCLUDED

#include "hybrid_cont.h"
#include "linked_pool.h"
#include "block_pool.h"

#include <cstring>

namespace aux
{
	template <class Type, size_t BlockSize>
	class hybrid_pool
	{
		typedef aux::linked_pool<Type> linked_pool;
		typedef aux::block_pool<Type, BlockSize> block_pool;
		union {
			linked_pool lpool;
			block_pool bpool;
		};
		bool ordered;

	public:
		typedef impl::hybrid_iterator<typename linked_pool::iterator, typename block_pool::iterator> iterator;
		typedef impl::hybrid_iterator<typename linked_pool::const_iterator, typename block_pool::const_iterator> const_iterator;
		typedef typename impl::assert_same<typename linked_pool::reference, typename block_pool::reference>::type reference;
		typedef typename impl::assert_same<typename linked_pool::const_reference, typename block_pool::const_reference>::type const_reference;
		typedef typename impl::assert_same<typename linked_pool::value_type, typename block_pool::value_type>::type value_type;
		typedef typename impl::assert_same<typename linked_pool::size_type, typename block_pool::size_type>::type size_type;

		hybrid_pool() : lpool(), ordered(false)
		{
		
		}

		hybrid_pool(bool ordered) : ordered(ordered)
		{
			if(ordered)
			{
				new (&bpool) block_pool();
			}else{
				new (&lpool) linked_pool();
			}
		}

		hybrid_pool(const linked_pool &pool) : lpool(pool), ordered(false)
		{

		}

		hybrid_pool(linked_pool &&pool) : lpool(std::move(pool)), ordered(false)
		{

		}

		hybrid_pool(const block_pool &pool) : bpool(pool), ordered(true)
		{

		}


		hybrid_pool(block_pool &&pool) : bpool(std::move(pool)), ordered(true)
		{

		}

		hybrid_pool(const hybrid_pool<Type, BlockSize> &pool) : ordered(pool.ordered)
		{
			if(ordered)
			{
				new (&bpool) block_pool(pool.bpool);
			}else{
				new (&lpool) linked_pool(pool.lpool);
			}
		}

		hybrid_pool(hybrid_pool<Type, BlockSize> &&pool) : ordered(pool.ordered)
		{
			if(ordered)
			{
				new (&bpool) block_pool(std::move(pool.bpool));
			}else{
				new (&lpool) linked_pool(std::move(pool.lpool));
			}
		}

		hybrid_pool<Type, BlockSize> &operator=(const linked_pool &pool)
		{
			if(ordered)
			{
				bpool.~block_pool();
				new (&lpool) linked_pool(pool);
				ordered = false;
			}else{
				lpool = pool;
			}
			return *this;
		}

		hybrid_pool<Type, BlockSize> &operator=(linked_pool &&pool)
		{
			if(ordered)
			{
				bpool.~block_pool();
				new (&lpool) linked_pool(std::move(pool));
				ordered = false;
			}else{
				lpool = std::move(pool);
			}
			return *this;
		}
	
		hybrid_pool<Type, BlockSize> &operator=(const block_pool &pool)
		{
			if(!ordered)
			{
				lpool.~linked_pool();
				new (&bpool) block_pool(pool);
				ordered = true;
			}else{
				bpool = pool;
			}
			return *this;
		}

		hybrid_pool<Type, BlockSize> &operator=(block_pool &&pool)
		{
			if(!ordered)
			{
				lpool.~linked_pool();
				new (&bpool) block_pool(std::move(pool));
				ordered = true;
			}else{
				bpool = std::move(pool);
			}
			return *this;
		}

		hybrid_pool<Type, BlockSize> &operator=(const hybrid_pool<Type, BlockSize> &pool)
		{
			if(ordered && pool.ordered)
			{
				bpool = pool.bpool;
			}else if(!ordered && !pool.ordered)
			{
				lpool = pool.lpool;
			}else if(ordered)
			{
				bpool.~block_pool();
				new (&lpool) linked_pool(pool.lpool);
				ordered = false;
			}else{
				lpool.~linked_pool();
				new (&bpool) block_pool(pool.bpool);
				ordered = true;
			}
			return *this;
		}

		hybrid_pool<Type, BlockSize> &operator=(hybrid_pool<Type, BlockSize> &&pool)
		{
			if(ordered && pool.ordered)
			{
				bpool = std::move(pool.bpool);
			}else if(!ordered && !pool.ordered)
			{
				lpool = std::move(pool.lpool);
			}else if(ordered)
			{
				bpool.~block_pool();
				new (&lpool) linked_pool(std::move(pool.lpool));
				ordered = false;
			}else{
				lpool.~linked_pool();
				new (&bpool) block_pool(std::move(pool.bpool));
				ordered = true;
			}
			return *this;
		}

		iterator begin()
		{
			if(ordered)
			{
				return bpool.begin();
			}else{
				return lpool.begin();
			}
		}

		iterator end()
		{
			if(ordered)
			{
				return bpool.end();
			}else{
				return lpool.end();
			}
		}

		bool push_back(Type &&value)
		{
			if(ordered)
			{
				return bpool.push_back(std::move(value));
			}else{
				return lpool.push_back(std::move(value));
			}
		}

		bool push_back(const Type &value)
		{
			if(ordered)
			{
				return bpool.push_back(value);
			}else{
				return lpool.push_back(value);
			}
		}

		size_t get_last_set() const
		{
			if(ordered)
			{
				return bpool.get_last_set();
			}else{
				return lpool.get_last_set();
			}
		}

		template<class... Args>
		bool emplace_back(Args&&... args)
		{
			if(ordered)
			{
				return bpool.emplace_back(std::forward<Args>(args)...);
			}else{
				return lpool.emplace_back(std::forward<Args>(args)...);
			}
		}

		iterator erase(iterator pos)
		{
			if(ordered)
			{
				return bpool.erase(pos);
			}else{
				return lpool.erase(pos);
			}
		}

		Type &operator[](size_type index)
		{
			if(ordered)
			{
				return bpool[index];
			}else{
				return lpool[index];
			}
		}

		const Type &operator[](size_type index) const
		{
			if(ordered)
			{
				return bpool[index];
			}else{
				return lpool[index];
			}
		}

		iterator find(size_type index)
		{
			if(ordered)
			{
				return bpool.find(index);
			}else{
				return lpool.find(index);
			}
		}

		iterator insert_or_set(size_type index, Type &&value)
		{
			if(ordered)
			{
				return bpool.insert_or_set(index, std::move(value));
			}else{
				return lpool.insert_or_set(index, std::move(value));
			}
		}

		bool resize(size_type newsize)
		{
			if(ordered)
			{
				return bpool.resize(newsize);
			}else{
				return lpool.resize(newsize);
			}
		}

		size_type num_elements() const
		{
			if(ordered)
			{
				return bpool.num_elements();
			}else{
				return lpool.num_elements();
			}
		}

		size_type size() const
		{
			if(ordered)
			{
				return bpool.size();
			}else{
				return lpool.size();
			}
		}

		size_type index_of(iterator it) const
		{
			if(ordered)
			{
				return bpool.index_of(it);
			}else{
				return lpool.index_of(it);
			}
		}

		iterator last_iter()
		{
			if(ordered)
			{
				return bpool.last_iter();
			}else{
				return lpool.last_iter();
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
					aux::linked_pool<Type> pool;
					pool.resize(bpool.size());
					for(auto it = bpool.begin(); it != bpool.end(); it++)
					{
						pool.insert_or_set(bpool.index_of(it), std::move(*it));
					}
					*this = std::move(pool);
				}else{
					aux::block_pool<Type, BlockSize> pool;
					pool.resize(lpool.size());
					for(auto it = lpool.begin(); it != lpool.end(); it++)
					{
						pool.insert_or_set(lpool.index_of(it), std::move(*it));
					}
					*this = std::move(pool);
				}
				return true;
			}
			return false;
		}

		void swap(hybrid_pool<Type, BlockSize> &pool)
		{
			if(ordered && pool.ordered)
			{
				std::swap(bpool, pool.bpool);
			}else if(!ordered && !pool.ordered)
			{
				std::swap(lpool, pool.lpool);
			}else{
				hybrid_pool<Type, BlockSize> tmp(std::move(pool));
				pool = std::move(*this);
				*this = std::move(tmp);
			}
		}

		~hybrid_pool()
		{
			if(ordered)
			{
				bpool.~block_pool();
			}else{
				lpool.~linked_pool();
			}
		}
	};
}

namespace std
{
	template <class Type, size_t BlockSize>
	void swap(::aux::hybrid_pool<Type, BlockSize> &a, ::aux::hybrid_pool<Type, BlockSize> &b)
	{
		a.swap(b);
	}
}

#endif

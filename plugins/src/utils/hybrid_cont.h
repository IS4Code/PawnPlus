#ifndef HYBRID_CONT_H_INCLUDED
#define HYBRID_CONT_H_INCLUDED

#include <type_traits>

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

		template <class first_iterator, class second_iterator>
		class hybrid_iterator
		{
			union {
				first_iterator iterator1;
				second_iterator iterator2;
			};
			bool is_second;

		public:
			typedef typename impl::assert_same<typename first_iterator::difference_type, typename second_iterator::difference_type>::type difference_type;
			typedef typename impl::assert_same<typename first_iterator::value_type, typename second_iterator::value_type>::type value_type;
			typedef typename impl::assert_same<typename first_iterator::pointer, typename second_iterator::pointer>::type pointer;
			typedef typename impl::assert_same<typename first_iterator::reference, typename second_iterator::reference>::type reference;
			typedef std::bidirectional_iterator_tag iterator_category;

			hybrid_iterator() : iterator1(), is_second(false)
			{

			}

			hybrid_iterator(const first_iterator &it) : iterator1(it), is_second(false)
			{
			
			}

			hybrid_iterator(const second_iterator &it) : iterator2(it), is_second(true)
			{

			}

			hybrid_iterator(const hybrid_iterator<first_iterator, second_iterator> &it)
			{
				if(is_trivially_copyable(first_iterator) && is_trivially_copyable(second_iterator))
				{
					std::memcpy(this, &it, sizeof(it));
				}else{
					is_second = it.is_second;
					if(is_second)
					{
						new (&iterator2) second_iterator(it.iterator2);
					}else{
						new (&iterator1) first_iterator(it.iterator1);
					}
				}
			}

			operator first_iterator&()
			{
				return iterator1;
			}

			operator second_iterator&()
			{
				return iterator2;
			}

			operator const first_iterator&() const
			{
				return iterator1;
			}

			operator const second_iterator&() const
			{
				return iterator2;
			}

			hybrid_iterator<first_iterator, second_iterator> &operator=(const first_iterator &it)
			{
				if((!is_trivially_destructible(second_iterator) || !is_trivially_copy_assignable(first_iterator)) && is_second)
				{
					iterator2.~second_iterator();
					new (&iterator1) first_iterator(it);
				}else{
					iterator1 = it;
				}
				is_second = false;
				return *this;
			}

			hybrid_iterator<first_iterator, second_iterator> &operator=(const second_iterator &it)
			{
				if((!is_trivially_destructible(first_iterator) || !is_trivially_copy_assignable(second_iterator)) && !is_second)
				{
					iterator1.~first_iterator();
					new (&iterator2) second_iterator(it);
				}else{
					iterator2 = it;
				}
				is_second = true;
				return *this;
			}

			hybrid_iterator<first_iterator, second_iterator> &operator=(const hybrid_iterator<first_iterator, second_iterator> &it)
			{
				if(this != &it)
				{
					if(is_trivially_copy_assignable(first_iterator) && is_trivially_copy_assignable(second_iterator))
					{
						std::memcpy(this, &it, sizeof(it));
					}else{
						if(is_second && it.is_second)
						{
							iterator2 = it.iterator2;
						}else if(!is_second && !it.is_second)
						{
							iterator1 = it.iterator1;
						}else if(is_second)
						{
							iterator2.~second_iterator();
							new (&iterator1) first_iterator(it.iterator1);
							is_second = false;
						}else{
							iterator1.~first_iterator();
							new (&iterator2) second_iterator(it.iterator2);
							is_second = true;
						}
					}
				}
				return *this;
			}

			typename impl::assert_same<decltype(*iterator1), decltype(*iterator1)>::type operator*() const
			{
				if(is_second)
				{
					return *iterator2;
				}else{
					return *iterator1;
				}
			}

			typename impl::assert_same<decltype(&*iterator1), decltype(&*iterator1)>::type operator->() const
			{
				return &**this;
			}

			hybrid_iterator<first_iterator, second_iterator> &operator++()
			{
				if(is_second)
				{
					++iterator2;
				}else{
					++iterator1;
				}
				return *this;
			}

			hybrid_iterator<first_iterator, second_iterator> operator++(int)
			{
				auto tmp = *this;
				++*this;
				return tmp;
			}

		private:
			void dec_first(std::input_iterator_tag)
			{

			}

			void dec_first(std::bidirectional_iterator_tag)
			{
				--iterator1;
			}

			void dec_second(std::input_iterator_tag)
			{

			}

			void dec_second(std::bidirectional_iterator_tag)
			{
				--iterator2;
			}

		public:
			hybrid_iterator<first_iterator, second_iterator> &operator--()
			{
				if(is_second)
				{
					typedef typename std::iterator_traits<second_iterator>::iterator_category category;
					dec_second(category());
				}else{
					typedef typename std::iterator_traits<first_iterator>::iterator_category category;
					dec_first(category());
				}
				return *this;
			}

			hybrid_iterator<first_iterator, second_iterator> operator--(int)
			{
				auto tmp = *this;
				--*this;
				return tmp;
			}

			bool operator==(const hybrid_iterator<first_iterator, second_iterator> &obj) const
			{
				if(is_second && obj.is_second)
				{
					return iterator2 == obj.iterator2;
				}else if(!is_second && !obj.is_second)
				{
					return iterator1 == obj.iterator1;
				}else{
					return false;
				}
			}

			bool operator!=(const hybrid_iterator<first_iterator, second_iterator> &obj) const
			{
				if(is_second && obj.is_second)
				{
					return iterator2 != obj.iterator2;
				}else if(!is_second && !obj.is_second)
				{
					return iterator1 != obj.iterator1;
				}else{
					return true;
				}
			}

			~hybrid_iterator()
			{
				if(!is_trivially_destructible(first_iterator) || !is_trivially_destructible(second_iterator))
				{
					if(is_second)
					{
						iterator2.~second_iterator();
					}else{
						iterator1.~first_iterator();
					}
				}
			}
		};
	}
}

#endif

#ifndef LINKED_POOL_H_INCLUDED
#define LINKED_POOL_H_INCLUDED

#include <vector>
#include <cstddef>

namespace aux
{
	template <class Type>
	class linked_pool
	{
	public:
		typedef typename std::vector<Type>::size_type size_type;
		typedef Type &reference;
		typedef const Type &const_reference;
		typedef Type value_type;

	private:
		struct element_type;

		std::vector<element_type> data;
		size_type first_set;
		size_type last_set;
		size_type first_unset;
		size_type last_unset;
		size_type count;

		struct element_type
		{
			std::ptrdiff_t next;
			std::ptrdiff_t previous;
			bool assigned;
			union {
				Type value;
			};

			element_type(std::ptrdiff_t previous, std::ptrdiff_t next) : previous(previous), next(next), assigned(false)
			{

			}

			element_type(const element_type &obj) : assigned(obj.assigned), next(obj.next), previous(obj.previous)
			{
				if(assigned)
				{
					new (&value) Type(obj.value);
				}
			}

			element_type(element_type &&obj) noexcept(noexcept(new(nullptr) Type(std::move(obj.value)))) : assigned(obj.assigned), next(obj.next), previous(obj.previous)
			{
				if(assigned)
				{
					new (&value) Type(std::move(obj.value));
				}
			}

			element_type &operator=(const element_type &obj)
			{
				if(this != &obj)
				{
					if(assigned && obj.assigned)
					{
						value = obj.value;
					}else if(assigned)
					{
						value.~Type();
					}else if(obj.assigned)
					{
						new (&value) Type(obj.value);
					}
					assigned = obj.assigned;
					next = obj.next;
					previous = obj.previous;
				}
				return *this;
			}

			element_type &operator=(element_type &&obj)
			{
				if(this != &obj)
				{
					if(assigned && obj.assigned)
					{
						value = std::move(obj.value);
					}else if(assigned)
					{
						value.~Type();
					}else if(obj.assigned)
					{
						new (&value) Type(std::move(obj.value));
					}
					assigned = obj.assigned;
					next = obj.next;
					previous = obj.previous;
				}
				return *this;
			}

			~element_type() noexcept(noexcept(value.~Type()))
			{
				if(assigned)
				{
					value.~Type();
					assigned = false;
				}
			}
		};

	public:
		class iterator
		{
			friend class linked_pool<Type>;
			element_type *elem;

		public:
			typedef std::ptrdiff_t difference_type;
			typedef Type value_type;
			typedef Type *pointer;
			typedef Type &reference;
			typedef std::bidirectional_iterator_tag iterator_category;

			iterator() noexcept : elem(nullptr)
			{

			}

			iterator(element_type *elem) noexcept : elem(elem)
			{

			}

			reference operator*() const noexcept
			{
				return elem->value;
			}

			pointer operator->() const noexcept
			{
				return &elem->value;
			}

			iterator &operator++() noexcept
			{
				if(elem->next == 0)
				{
					elem = nullptr;
				}else{
					elem += elem->next;
				}
				return *this;
			}

			iterator operator++(int) noexcept
			{
				auto tmp = *this;
				++*this;
				return tmp;
			}

			iterator &operator--() noexcept
			{
				if(elem->previous == 0)
				{
					elem = nullptr;
				}else{
					elem += elem->previous;
				}
				return *this;
			}

			iterator operator--(int) noexcept
			{
				auto tmp = *this;
				--*this;
				return tmp;
			}

			bool operator==(const iterator &obj) const noexcept
			{
				return elem == obj.elem;
			}

			bool operator!=(const iterator &obj) const noexcept
			{
				return elem != obj.elem;
			}
		};
		
		class const_iterator
		{
			friend class linked_pool<Type>;
			const element_type *elem;

		public:
			typedef std::ptrdiff_t difference_type;
			typedef const Type value_type;
			typedef const Type *pointer;
			typedef const Type &reference;
			typedef std::bidirectional_iterator_tag iterator_category;

			const_iterator() noexcept : elem(nullptr)
			{

			}

			const_iterator(const element_type *elem) noexcept : elem(elem)
			{

			}

			reference operator*() const noexcept
			{
				return *elem;
			}

			pointer operator->() const noexcept
			{
				return elem;
			}

			const_iterator &operator++() noexcept
			{
				if(elem->next == 0)
				{
					elem = nullptr;
				}else{
					elem += elem->next;
				}
				return *this;
			}

			const_iterator operator++(int) noexcept
			{
				auto tmp = *this;
				++*this;
				return tmp;
			}

			const_iterator &operator--() noexcept
			{
				if(elem->previous == 0)
				{
					elem = nullptr;
				}else{
					elem += elem->previous;
				}
				return *this;
			}

			const_iterator operator--(int) noexcept
			{
				auto tmp = *this;
				--*this;
				return tmp;
			}

			bool operator==(const const_iterator &obj) const noexcept
			{
				return elem == obj.elem;
			}

			bool operator!=(const const_iterator &obj) const noexcept
			{
				return elem != obj.elem;
			}
		};

		Type &allocate(bool &resized)
		{
			if(first_unset == -1) // no more slots
			{
				first_unset = last_unset = data.size();
				resized = data.capacity() == data.size();
				data.emplace_back(0, 0);
			}else{
				resized = false;
			}
			auto &elem = data[first_unset];
			if(last_set != -1) // not empty
			{
				data[last_set].next = -(elem.previous = last_set - first_unset);
				last_set = first_unset;
			}else{
				first_set = last_set = first_unset;
				elem.previous = 0;
			}
			if(first_unset == last_unset) // last slot
			{
				first_unset = last_unset = -1;
			}else{
				first_unset += elem.next;
				data[first_unset].previous = 0;
			}
			elem.next = 0;
			elem.assigned = true;
			++count;
			return elem.value;
		}

	public:
		linked_pool() : first_set(-1), last_set(-1), first_unset(-1), last_unset(-1), count(0)
		{

		}

		linked_pool(const linked_pool &obj) : data(obj.data), first_set(obj.first_set), last_set(obj.last_set), first_unset(obj.first_unset), last_unset(obj.last_unset), count(obj.count)
		{

		}

		linked_pool(linked_pool &&obj) : data(std::move(obj.data)), first_set(obj.first_set), last_set(obj.last_set), first_unset(obj.first_unset), last_unset(obj.last_unset), count(obj.count)
		{
			obj.data.clear();
			obj.first_set = obj.last_set = obj.first_unset = obj.last_unset = -1;
			obj.count = 0;
		}

		linked_pool &operator=(const linked_pool &obj)
		{
			if(this != &obj)
			{
				data = obj.data;
				first_set = obj.first_set;
				last_set = obj.last_set;
				first_unset = obj.first_unset;
				last_unset = obj.last_unset;
				count = obj.count;
			}
			return *this;
		}

		linked_pool &operator=(linked_pool &&obj)
		{
			if(this != &obj)
			{
				data = std::move(obj.data);
				first_set = obj.first_set;
				last_set = obj.last_set;
				first_unset = obj.first_unset;
				last_unset = obj.last_unset;
				count = obj.count;
				obj.data.clear();
				obj.first_set = obj.last_set = obj.first_unset = obj.last_unset = -1;
				obj.count = 0;
			}
			return *this;
		}

		iterator begin()
		{
			if(first_set != -1)
			{
				return iterator(&data[first_set]);
			}else{
				return iterator();
			}
		}

		iterator end()
		{
			return iterator();
		}

		iterator last_iter()
		{
			if(last_set != -1)
			{
				return iterator(&data[last_set]);
			}else{
				return iterator();
			}
		}

		bool push_back(Type &&value)
		{
			bool resized;
			new (&allocate(resized)) Type(std::move(value));
			return resized;
		}

		bool push_back(const Type &value)
		{
			bool resized;
			new (&allocate(resized)) Type(value);
			return resized;
		}

		size_t get_last_set() const
		{
			return last_set;
		}

		template<class... Args>
		bool emplace_back(Args&&... args)
		{
			bool resized;
			new (&allocate(resized)) Type(std::forward<Args>(args)...);
			return resized;
		}

	private:
		template <size_type linked_pool::*First, size_type linked_pool::*Last>
		size_type unlink(element_type &elem)
		{
			size_type index;
			if(elem.previous == 0 && elem.next == 0) // single element
			{
				index = this->*First;
				this->*First = this->*Last = -1;
			}else if(elem.previous == 0) // first element
			{
				index = this->*First;
				this->*First += elem.next;
				data[this->*First].previous = 0;
			}else if(elem.next == 0) // last element
			{
				index = this->*Last;
				this->*Last += elem.previous;
				data[this->*Last].next = 0;
			}else{
				(&elem + elem.previous)->next += elem.next;
				(&elem + elem.next)->previous += elem.previous;
				index = &elem - &data[0];
			}
			return index;
		}

		template <size_type linked_pool::*First, size_type linked_pool::*Last>
		void link(element_type &elem, size_type index)
		{
			if(this->*First == -1) // no slots
			{
				this->*First = this->*Last = index;
				elem.previous = elem.next = 0;
			}else{
				elem.next = 0;
				data[this->*Last].next = -(elem.previous = this->*Last - index);
				this->*Last = index;
			}
		}

	public:
		iterator erase(iterator pos)
		{
			auto next = pos;
			++next;
			element_type &elem = *pos.elem;
			size_t index = unlink<&linked_pool::first_set, &linked_pool::last_set>(elem);
			elem.assigned = false;
			elem.value.~Type();
			link<&linked_pool::first_unset, &linked_pool::last_unset>(elem, index);
			--count;
			return next;
		}

		Type &operator[](size_type index)
		{
			return data[index].value;
		}

		const Type &operator[](size_type index) const
		{
			return data[index].value;
		}

		iterator find(size_type index)
		{
			if(index < data.size())
			{
				auto &elem = data[index];
				if(elem.assigned)
				{
					return iterator(&elem);
				}
			}
			return iterator();
		}

		iterator insert_or_set(size_type index, Type &&value)
		{
			if(index >= data.size())
			{
				resize(index + 1);
			}
			auto &elem = data[index];
			if(elem.assigned)
			{
				elem.value = std::move(value);
			}else{
				++count;
				size_t index = unlink<&linked_pool::first_unset, &linked_pool::last_unset>(elem);
				elem.assigned = true;
				new (&elem.value) Type(std::move(value));
				link<&linked_pool::first_set, &linked_pool::last_set>(elem, index);
			}
			return iterator(&elem);
		}

		bool resize(size_type newsize)
		{
			auto size = data.size();
			bool invalidate = false;
			if(newsize > size)
			{
				auto rem = newsize - size;
				if(data.capacity() < newsize)
				{
					data.reserve(newsize);
					invalidate = true;
				}

				if(last_unset == -1)
				{
					first_unset = size;
					last_unset = newsize - 1;
					if(rem == 1)
					{
						data.emplace_back(0, 0);
					}else for(size_type i = 0; i < rem; i++)
					{
						if(i == rem - 1)
						{
							data.emplace_back(-1, 0);
						}else if(i == 0)
						{
							data.emplace_back(0, 1);
						}else{
							data.emplace_back(-1, 1);
						}
					}
				}else{
					std::ptrdiff_t diff = size - last_unset;
					data[last_unset].next = diff;
					last_unset = newsize - 1;
					if(rem == 1)
					{
						data.emplace_back(-diff, 0);
					}else for(size_type i = 0; i < rem; i++)
					{
						if(i == rem - 1)
						{
							data.emplace_back(-1, 0);
						}else if(i == 0)
						{
							data.emplace_back(-diff, 1);
						}else{
							data.emplace_back(-1, 1);
						}
					}
				}
			}else if(newsize < size)
			{
				if(newsize == 0)
				{
					data.clear();
					invalidate = first_set != -1;
					first_set = last_set = first_unset = last_unset = -1;
					count = 0;
				}else{
					for(size_type i = newsize; i < size; i++)
					{
						auto &elem = data[i];
						if(elem.assigned)
						{
							unlink<&linked_pool::first_set, &linked_pool::last_set>(elem);
							invalidate = true;
							--count;
						}else{
							unlink<&linked_pool::first_unset, &linked_pool::last_unset>(elem);
						}
					}
					data.erase(data.begin() + newsize, data.end());
				}
			}
			return invalidate;
		}

		size_type size() const
		{
			return data.size();
		}

		size_type index_of(iterator it) const
		{
			return it.elem - &data[0];
		}

		size_type num_elements() const
		{
			return count;
		}

		void swap(linked_pool<Type> &obj)
		{
			std::swap(data, obj.data);
			std::swap(first_set, obj.first_set);
			std::swap(last_set, obj.last_set);
			std::swap(first_unset, obj.first_unset);
			std::swap(last_unset, obj.last_unset);
			std::swap(count, obj.count);
		}
	};
}

namespace std
{
	template <class Type>
	void swap(::aux::linked_pool<Type> &a, ::aux::linked_pool<Type> &b)
	{
		a.swap(b);
	}
}

#endif

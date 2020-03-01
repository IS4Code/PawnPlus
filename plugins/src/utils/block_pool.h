#ifndef BLOCK_POOL_H_INCLUDED
#define BLOCK_POOL_H_INCLUDED

#include <vector>
#include <array>
#include <memory>
#include <cmath>

#include "fixes/linux.h"

namespace aux
{
	template <class Type, size_t BlockSize>
	class block_pool
	{
	public:
		typedef typename std::vector<Type>::size_type size_type;
		typedef Type &reference;
		typedef const Type &const_reference;
		typedef Type value_type;

	private:
		struct element_type;
		struct block_info;
		std::vector<element_type> data;
		std::unique_ptr<block_info> block;
		size_t level = 0;
		size_type last_set;

		struct element_type
		{
			bool assigned;
			union {
				Type value;
			};

			element_type() : assigned(false)
			{

			}

			element_type(const element_type &obj) : assigned(obj.assigned)
			{
				if(assigned)
				{
					new (&value) Type(obj.value);
				}
			}

			element_type(element_type &&obj) noexcept(noexcept(new(nullptr) Type(std::move(obj.value)))) : assigned(obj.assigned)
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

		struct block_info
		{
			size_type num_elements = 0;
			std::array<std::unique_ptr<block_info>, BlockSize> blocks;
			block_info *parent = nullptr;

			block_info()
			{

			}

			block_info(block_info &parent) : parent(&parent)
			{

			}

			block_info(std::unique_ptr<block_info> &&initial) : num_elements(initial->num_elements)
			{
				initial->parent = this;
				std::get<0>(blocks) = std::move(initial);
			}
		};

	public:
		class iterator
		{
			friend class block_pool<Type, BlockSize>;
			element_type *elem;
			block_info *block;
			size_t index;

		public:
			typedef std::ptrdiff_t difference_type;
			typedef Type value_type;
			typedef Type *pointer;
			typedef Type &reference;
			typedef std::bidirectional_iterator_tag iterator_category;

			iterator() noexcept : elem(nullptr)
			{

			}

			iterator(element_type *elem, block_info *block, size_t index) noexcept : elem(elem), block(block), index(index)
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
				do{
					++elem;
					if(index == BlockSize - 1)
					{
						size_t level = 1;
						while(level > 0)
						{
							auto parent = block->parent;
							if(!parent)
							{
								elem = nullptr;
								return *this;
							}
							size_t next = -1;
							for(size_t i = 0; i < BlockSize; i++)
							{
								if(parent->blocks[i].get() == block)
								{
									next = i + 1;
									break;
								}
							}
							while(next == BlockSize || !parent->blocks[next] || parent->blocks[next]->num_elements == 0)
							{
								if(next == BlockSize)
								{
									++level;
									block = parent;
									next = -1;
									break;
								}else{
									++next;
									elem += static_cast<size_t>(std::pow(BlockSize, level));
								}
							}
							if(next != -1)
							{
								block = parent->blocks[next].get();
								next = -1;
								--level;
								while(level > 0)
								{
									for(size_t i = 0; i < BlockSize; i++)
									{
										if(block->blocks[i] && block->blocks[i]->num_elements > 0)
										{
											block = block->blocks[i].get();
											--level;
											break;
										}
										elem += static_cast<size_t>(std::pow(BlockSize, level));
									}
								}
								index = 0;
							}
						}
					}else{
						++index;
					}
				}while(!elem->assigned);
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
				do{
					--elem;
					if(index == 0)
					{
						size_t level = 1;
						while(level > 0)
						{
							auto parent = block->parent;
							if(!parent)
							{
								elem = nullptr;
								return *this;
							}
							size_t next = -1;
							for(size_t i = 0; i < BlockSize; i++)
							{
								if(parent->blocks[i].get() == block)
								{
									next = i - 1;
									break;
								}
							}
							while(next == -1 || !parent->blocks[next] || parent->blocks[next]->num_elements == 0)
							{
								if(next == -1)
								{
									++level;
									block = parent;
									break;
								}else{
									--next;
									elem -= static_cast<size_t>(std::pow(BlockSize, level));
								}
							}
							if(next != -1)
							{
								block = parent->blocks[next].get();
								next = -1;
								--level;
								while(level > 0)
								{
									for(size_t i = BlockSize - 1; i != -1; i--)
									{
										if(block->blocks[i] && block->blocks[i]->num_elements > 0)
										{
											block = block->blocks[i].get();
											--level;
											break;
										}
										elem -= static_cast<size_t>(std::pow(BlockSize, level));
									}
								}
								index = BlockSize - 1;
							}
						}
					}else{
						--index;
					}
				}while(!elem->assigned);
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

		typedef iterator const_iterator;

	private:
		Type &allocate_in_block(block_info &block, size_type offset, size_t level)
		{
			block.num_elements++;
			if(level > 0)
			{
				size_t block_size = static_cast<size_t>(std::pow(BlockSize, level));
				for(size_t i = 0; i < BlockSize; i++)
				{
					auto &sub = block.blocks[i];
					if(!sub)
					{
						sub = std::make_unique<block_info>(block);
						return allocate_in_block(*sub, offset + i * block_size, level - 1);
					}else if(sub->num_elements < block_size)
					{
						return allocate_in_block(*sub, offset + i * block_size, level - 1);
					}
				}
			}else{
				for(size_t i = 0; i < BlockSize; i++)
				{
					auto &elem = data[offset + i];
					if(!elem.assigned)
					{
						elem.assigned = true;
						last_set = offset + i;
						return elem.value;
					}
				}
			}
			throw nullptr;
		}
		
		Type &allocate(bool &resized)
		{
			if(block->num_elements == data.size())
			{
				resized = data.size() == data.capacity();
				data.emplace_back();
			}else{
				resized = false;
			}
			size_type pad = BlockSize - data.size() % BlockSize;
			if(pad < BlockSize)
			{
				if(!resized)
				{
					resized = data.size() + pad > data.capacity();
				}
				for(size_type i = 0; i < pad; i++)
				{
					data.emplace_back();
				}
			}
			if(block->num_elements == std::pow(BlockSize, level + 1))
			{
				block = std::make_unique<block_info>(std::move(block));
				++level;
			}
			return allocate_in_block(*block, 0, level);
		}

		iterator find_first(block_info &block, size_type offset, size_t level)
		{
			if(level > 0)
			{
				for(size_t i = 0; i < BlockSize; i++)
				{
					auto &sub = block.blocks[i];
					if(sub && sub->num_elements > 0)
					{
						size_t block_size = static_cast<size_t>(std::pow(BlockSize, level));
						return find_first(*sub, offset + i * block_size, level - 1);
					}
				}
			}else{
				for(size_t i = 0; i < BlockSize; i++)
				{
					auto &elem = data[offset + i];
					if(elem.assigned)
					{
						return iterator(&elem, &block, i);
					}
				}
			}
			throw nullptr;
		}

		iterator find_last(block_info &block, size_type offset, size_t level)
		{
			if(level > 0)
			{
				for(size_t i = BlockSize - 1; i != -1; i--)
				{
					auto &sub = block.blocks[i];
					if(sub && sub->num_elements > 0)
					{
						size_t block_size = static_cast<size_t>(std::pow(BlockSize, level));
						return find_last(*sub, offset + i * block_size, level - 1);
					}
				}
			}else{
				for(size_t i = BlockSize - 1; i != -1; i--)
				{
					auto &elem = data[offset + i];
					if(elem.assigned)
					{
						return iterator(&elem, &block, i);
					}
				}
			}
			throw nullptr;
		}

		iterator iter_for(block_info &block, size_type offset, size_type index, size_t level)
		{
			if(level > 0)
			{
				size_t block_size = static_cast<size_t>(std::pow(BlockSize, level));
				for(size_t i = 0; i < BlockSize; i++)
				{
					if(index < offset + block_size)
					{
						auto &sub = block.blocks[i];
						return iter_for(*sub, offset, index, level - 1);
					}
					offset += block_size;
				}
			}else{
				auto &elem = data[index];
				return iterator(&elem, &block, index - offset);
			}
			throw nullptr;
		}
		
		iterator iter_add(block_info &block, size_type offset, size_type index, size_t level, Type &&value)
		{
			block.num_elements++;
			if(level > 0)
			{
				size_t block_size = static_cast<size_t>(std::pow(BlockSize, level));
				for(size_t i = 0; i < BlockSize; i++)
				{
					if(index < offset + block_size)
					{
						auto &sub = block.blocks[i];
						if(!sub)
						{
							sub = std::make_unique<block_info>(block);
						}
						return iter_add(*sub, offset, index, level - 1, std::move(value));
					}
					offset += block_size;
				}
			}else{
				auto &elem = data[index];
				elem.assigned = true;
				new (&elem.value) dyn_object(std::move(value));
				return iterator(&elem, &block, index - offset);
			}
			throw nullptr;
		}

		size_type clear_blocks(block_info &block, size_type offset, size_type newsize, size_t level)
		{
			size_type removed = 0;
			if(level > 0)
			{
				size_t block_size = static_cast<size_t>(std::pow(BlockSize, level));
				for(size_t i = 0; i < BlockSize; i++)
				{
					auto &sub = block.blocks[i];
					if(sub)
					{
						if(offset >= newsize)
						{
							removed += sub->num_elements;
							sub = nullptr;
						}else if(offset + block_size > newsize)
						{
							removed += clear_blocks(*sub, offset, newsize, level - 1);
						}
					}
					offset += block_size;
				}
			}else{
				for(size_t i = 0; i < BlockSize; i++)
				{
					auto &elem = data[offset + i];
					if(elem.assigned && offset + i >= newsize)
					{
						removed++;
					}
				}
			}
			block.num_elements -= removed;
			return removed;
		}

	public:
		block_pool() : block(std::make_unique<block_info>())
		{

		}

		block_pool(const block_pool &obj) : data(obj.data), block(std::make_unique<block_info>(*obj.block)), level(obj.level), last_set(obj.last_set)
		{

		}

		block_pool(block_pool &&obj) : data(std::move(obj.data)), block(std::move(obj.block)), level(obj.level), last_set(obj.last_set)
		{
			obj.data.clear();
		}

		block_pool &operator=(const block_pool &obj)
		{
			if(this != &obj)
			{
				data = obj.data;
				block = std::make_unique<block_info>(*obj.block);
				level = obj.level;
				last_set = obj.last_set;
			}
			return *this;
		}

		block_pool &operator=(block_pool &&obj)
		{
			if(this != &obj)
			{
				data = std::move(obj.data);
				block = std::move(obj.block);
				obj.data.clear();
				level = obj.level;
				last_set = obj.last_set;
			}
			return *this;
		}

		iterator begin()
		{
			if(block->num_elements == 0)
			{
				return end();
			}
			return find_first(*block, 0, level);
		}

		iterator end()
		{
			return iterator();
		}

		iterator last_iter()
		{
			if(block->num_elements == 0)
			{
				return end();
			}
			return find_last(*block, 0, level);
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

		iterator erase(iterator pos)
		{
			auto next = pos;
			++next;
			auto &elem = *pos.elem;
			elem.assigned = false;
			elem.value.~Type();
			auto block = pos.block;
			while(block != nullptr)
			{
				block->num_elements--;
				block = block->parent;
			}
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
					return iter_for(*block, 0, index, level);
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
				return iter_for(*block, 0, index, level);
			}else{
				auto range = static_cast<size_t>(std::pow(BlockSize, level + 1));
				while(index >= range)
				{
					block = std::make_unique<block_info>(std::move(block));
					++level;
					range *= BlockSize;
				}
				return iter_add(*block, 0, index, level, std::move(value));
			}
		}

		bool resize(size_type newsize)
		{
			auto size = data.size();
			bool invalidate = false;
			if(newsize > size)
			{
				size_type pad = BlockSize - newsize % BlockSize;
				if(pad < BlockSize)
				{
					newsize += pad;
				}
				auto rem = newsize - size;
				if(data.capacity() < newsize)
				{
					data.reserve(newsize);
					invalidate = true;
				}
				for(size_type i = 0; i < rem; i++)
				{
					data.emplace_back();
				}
			}else if(newsize < size)
			{
				if(newsize == 0)
				{
					level = 0;
					invalidate = block->num_elements > 0;
					block = std::make_unique<block_info>();
					data.clear();
				}else{
					invalidate = clear_blocks(*block, 0, newsize, level) > 0;
					data.erase(data.begin() + newsize, data.end());

					size_type pad = BlockSize - data.size() % BlockSize;
					if(pad < BlockSize)
					{
						if(!invalidate)
						{
							invalidate = data.size() + pad > data.capacity();
						}
						for(size_type i = 0; i < pad; i++)
						{
							data.emplace_back();
						}
					}
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
			return block ? block->num_elements : 0;
		}

		void swap(block_pool<Type, BlockSize> &obj)
		{
			std::swap(data, obj.data);
			std::swap(block, obj.block);
			std::swap(level, obj.level);
			std::swap(last_set, obj.last_set);
		}
	};
}

namespace std
{
	template <class Type, size_t BlockSize>
	void swap(::aux::block_pool<Type, BlockSize> &a, ::aux::block_pool<Type, BlockSize> &b)
	{
		a.swap(b);
	}
}

#endif

#ifndef UTILS_MEMORY_H_INCLUDED
#define UTILS_MEMORY_H_INCLUDED

#include <type_traits>
#include <utility>
#include <iterator>
#include <memory>
#include <malloc.h>

namespace aux
{
	template <class Iterator>
	using iterator_value = typename std::remove_const<typename std::iterator_traits<Iterator>::value_type>::type;

	template <class Iterator>
	using is_contiguous_iterator = std::integral_constant<bool,
		std::is_same<Iterator, iterator_value<Iterator>*>::value ||
		std::is_same<Iterator, const iterator_value<Iterator>*>::value ||
		std::is_same<Iterator, typename std::basic_string<iterator_value<Iterator>>::iterator>::value ||
		std::is_same<Iterator, typename std::basic_string<iterator_value<Iterator>>::const_iterator>::value ||
		std::is_same<Iterator, typename std::vector<iterator_value<Iterator>>::iterator>::value ||
		std::is_same<Iterator, typename std::vector<iterator_value<Iterator>>::const_iterator>::value
	>;

	template <class Pointer>
	using const_pointer = const typename std::remove_pointer<Pointer>::type*;

	template <class Iterator>
	using const_iterator_pointer = const iterator_value<Iterator>*;

	template <class Pointer, class Receiver>
	auto make_contiguous(Pointer begin, const_pointer<Pointer> end, Receiver receiver) -> decltype(receiver(end, end))
	{
		return receiver(static_cast<const_pointer<Pointer>>(begin), end);
	}

	template <class Iterator, class Receiver>
	auto make_contiguous(Iterator begin, typename std::enable_if<is_contiguous_iterator<Iterator>::value, Iterator>::type end, Receiver receiver) -> decltype(receiver(std::declval<const_iterator_pointer<Iterator>>(), std::declval<const_iterator_pointer<Iterator>>()))
	{
		const_iterator_pointer<Iterator> ptr = &*begin;
		return receiver(ptr, ptr + (end - begin));
	}

	template <class Iterator, class Receiver>
	auto make_contiguous(Iterator begin, typename std::enable_if<!is_contiguous_iterator<Iterator>::value, Iterator>::type end, Receiver receiver) -> decltype(receiver(std::declval<const_iterator_pointer<Iterator>>(), std::declval<const_iterator_pointer<Iterator>>()))
	{
		std::basic_string<iterator_value<Iterator>> str{begin, end};
		const_iterator_pointer<Iterator> ptr = &str[0];
		return receiver(ptr, ptr + str.size());
	}

	template <class Type, class Receiver>
	auto alloc_inline(std::size_t size, Receiver receiver) -> decltype(receiver(std::declval<Type*>()))
	{
		if(size * sizeof(Type) < 128)
		{
			Type *ptr = static_cast<Type*>(alloca(size * sizeof(Type)));
			return receiver(ptr);
		}
		auto memory = std::make_unique<Type[]>(size);
		Type *ptr = memory.get();
		return receiver(ptr);
	}
}

#endif

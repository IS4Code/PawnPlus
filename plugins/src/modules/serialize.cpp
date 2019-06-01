#include "serialize.h"

#include "modules/containers.h"
#include "modules/variants.h"
#include "modules/strings.h"

bool serialize_value(cell value, tag_ptr tag, void(*binary_writer)(void*, const char*, cell), void *binary_writer_cookie, void(*object_writer)(void*, const void*), void *object_writer_cookie)
{
	switch(tag->find_top_base()->uid)
	{
		case tags::tag_list:
		{
			list_t *ptr;
			if(!list_pool.get_by_id(value, ptr))
			{
				return false;
			}
			binary_writer(binary_writer_cookie, &static_cast<const char&>(1), sizeof(char));
			binary_writer(binary_writer_cookie, reinterpret_cast<const char*>(&static_cast<const cell&>(ptr->size())), sizeof(cell));
			for(const auto &elem : *ptr)
			{
				object_writer(object_writer_cookie, &elem);
			}
			return true;
		}
		case tags::tag_map:
		{
			map_t *ptr;
			if(!map_pool.get_by_id(value, ptr))
			{
				return false;
			}
			binary_writer(binary_writer_cookie, &static_cast<const char&>(1), sizeof(char));
			binary_writer(binary_writer_cookie, reinterpret_cast<const char*>(&static_cast<const cell&>(ptr->size())), sizeof(cell));
			binary_writer(binary_writer_cookie, reinterpret_cast<const char*>(&static_cast<const cell&>(ptr->ordered())), sizeof(cell));
			for(const auto &elem : *ptr)
			{
				object_writer(object_writer_cookie, &elem.first);
				object_writer(object_writer_cookie, &elem.second);
			}
			return true;
		}
		break;
		case tags::tag_variant:
		{
			dyn_object *ptr;
			if(!variants::pool.get_by_id(value, ptr))
			{
				return false;
			}
			binary_writer(binary_writer_cookie, &static_cast<const char&>(1), sizeof(char));
			object_writer(object_writer_cookie, ptr);
			return true;
		}
		break;
		case tags::tag_string:
		{
			strings::cell_string *ptr;
			if(!strings::pool.get_by_id(value, ptr))
			{
				return false;
			}
			binary_writer(binary_writer_cookie, &static_cast<const char&>(1), sizeof(char));
			binary_writer(binary_writer_cookie, reinterpret_cast<const char*>(&static_cast<const cell&>(ptr->size())), sizeof(cell));
			binary_writer(binary_writer_cookie, reinterpret_cast<const char*>(&(*ptr)[0]), ptr->size() * sizeof(cell));
			return true;
		}
		break;
	}
	return false;
}

bool deserialize_value(cell &value, tag_ptr tag, cell(*binary_reader)(void*, char*, cell), void *binary_reader_cookie, void*(*object_reader)(void*), void *object_reader_cookie)
{
	switch(tag->find_top_base()->uid)
	{
		case tags::tag_list:
		{
			char version;
			binary_reader(binary_reader_cookie, &version, sizeof(char));
			if(version == 1)
			{
				cell size;
				binary_reader(binary_reader_cookie, reinterpret_cast<char*>(&size), sizeof(cell));
				auto &ptr = list_pool.add();
				value = list_pool.get_id(ptr);
				for(cell i = 0; i < size; i++)
				{
					auto val = static_cast<dyn_object*>(object_reader(object_reader_cookie));
					ptr->push_back(std::move(*val));
					delete val;
				}
				return true;
			}
		}
		break;
		case tags::tag_map:
		{
			char version;
			binary_reader(binary_reader_cookie, &version, sizeof(char));
			if(version == 1)
			{
				cell size;
				binary_reader(binary_reader_cookie, reinterpret_cast<char*>(&size), sizeof(cell));
				cell ordered;
				binary_reader(binary_reader_cookie, reinterpret_cast<char*>(&ordered), sizeof(cell));
				auto &ptr = map_pool.add();
				value = map_pool.get_id(ptr);
				ptr->set_ordered(ordered);
				for(cell i = 0; i < size; i++)
				{
					auto key = static_cast<dyn_object*>(object_reader(object_reader_cookie));
					auto val = static_cast<dyn_object*>(object_reader(object_reader_cookie));
					ptr->insert(std::move(*key), std::move(*val));
					delete key;
					delete val;
				}
				return true;
			}
		}
		break;
		case tags::tag_variant:
		{
			char version;
			binary_reader(binary_reader_cookie, &version, sizeof(char));
			if(version == 1)
			{
				auto val = static_cast<dyn_object*>(object_reader(object_reader_cookie));
				value = variants::create(std::move(*val));
				delete val;
				return true;
			}
		}
		break;
		case tags::tag_string:
		{
			char version;
			binary_reader(binary_reader_cookie, &version, sizeof(char));
			if(version == 1)
			{
				cell size;
				binary_reader(binary_reader_cookie, reinterpret_cast<char*>(&size), sizeof(cell));
				auto &ptr = strings::pool.add();
				value = strings::pool.get_id(ptr);
				ptr->resize(size);
				binary_reader(binary_reader_cookie, reinterpret_cast<char*>(&(*ptr)[0]), ptr->size() * sizeof(cell));
				return true;
			}
		}
		break;
	}
	return false;
}

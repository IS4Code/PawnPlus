#ifndef SERIALIZE_H_INCLUDED
#define SERIALIZE_H_INCLUDED

#include "sdk/amx/amx.h"
#include "modules/tags.h"

bool serialize_value(cell value, tag_ptr tag, void(*binary_writer)(void*, const char*, cell), void *binary_writer_cookie, void(*object_writer)(void*, const void*), void *object_writer_cookie);
bool deserialize_value(cell &value, tag_ptr tag, cell(*binary_reader)(void*, char*, cell), void *binary_reader_cookie, void*(*object_reader)(void*), void *object_reader_cookie);

#endif

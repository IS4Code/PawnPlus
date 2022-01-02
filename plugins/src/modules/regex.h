#ifndef REGEX_H_INCLUDED
#define REGEX_H_INCLUDED

#include "modules/strings.h"
#include "modules/containers.h"

#include <memory>

namespace strings
{
	bool regex_search(const cell_string &str, const cell *pattern, cell *pos, cell options, std::weak_ptr<void> mem_handle);
	bool regex_search(const cell_string &str, const cell_string &pattern, cell *pos, cell options, std::weak_ptr<void> mem_handle);
	cell regex_extract(const cell_string &str, const cell *pattern, cell *pos, cell options, std::weak_ptr<void> mem_handle);
	cell regex_extract(const cell_string &str, const cell_string &pattern, cell *pos, cell options, std::weak_ptr<void> mem_handle);
	void regex_replace(cell_string &target, const cell_string &str, const cell *pattern, const cell *replacement, cell *pos, cell options, std::weak_ptr<void> mem_handle);
	void regex_replace(cell_string &target, const cell_string &str, const cell_string &pattern, const cell_string &replacement, cell *pos, cell options, std::weak_ptr<void> mem_handle);
	void regex_replace(cell_string &target, const cell_string &str, const cell *pattern, const list_t &replacement, cell *pos, cell options, std::weak_ptr<void> mem_handle);
	void regex_replace(cell_string &target, const cell_string &str, const cell_string &pattern, const list_t &replacement, cell *pos, cell options, std::weak_ptr<void> mem_handle);
	void regex_replace(cell_string &target, const cell_string &str, const cell *pattern, AMX *amx, int replacement_index, cell *pos, cell options, const char *format, cell *params, size_t numargs, std::weak_ptr<void> mem_handle);
	void regex_replace(cell_string &target, const cell_string &str, const cell_string &pattern, AMX *amx, int replacement_index, cell *pos, cell options, const char *format, cell *params, size_t numargs, std::weak_ptr<void> mem_handle);
	void regex_replace(cell_string &target, const cell_string &str, const cell *pattern, AMX *amx, const expression &expr, cell *pos, cell options, std::weak_ptr<void> mem_handle);
	void regex_replace(cell_string &target, const cell_string &str, const cell_string &pattern, AMX *amx, const expression &expr, cell *pos, cell options, std::weak_ptr<void> mem_handle);
}

#endif

#ifndef REGEX_H_INCLUDED
#define REGEX_H_INCLUDED

#include "modules/strings.h"
#include "modules/containers.h"

namespace strings
{
	bool regex_search(const cell_string &str, const cell *pattern, cell *pos, cell options);
	bool regex_search(const cell_string &str, const cell_string &pattern, cell *pos, cell options);
	cell regex_extract(const cell_string &str, const cell *pattern, cell *pos, cell options);
	cell regex_extract(const cell_string &str, const cell_string &pattern, cell *pos, cell options);
	void regex_replace(cell_string &target, const cell_string &str, const cell *pattern, const cell *replacement, cell *pos, cell options);
	void regex_replace(cell_string &target, const cell_string &str, const cell_string &pattern, const cell_string &replacement, cell *pos, cell options);
	void regex_replace(cell_string &target, const cell_string &str, const cell *pattern, const list_t &replacement, cell *pos, cell options);
	void regex_replace(cell_string &target, const cell_string &str, const cell_string &pattern, const list_t &replacement, cell *pos, cell options);
	void regex_replace(cell_string &target, const cell_string &str, const cell *pattern, AMX *amx, int replacement_index, cell *pos, cell options, const char *format, cell *params, size_t numargs);
	void regex_replace(cell_string &target, const cell_string &str, const cell_string &pattern, AMX *amx, int replacement_index, cell *pos, cell options, const char *format, cell *params, size_t numargs);
	void regex_replace(cell_string &target, const cell_string &str, const cell *pattern, AMX *amx, const expression &expr, cell *pos, cell options);
	void regex_replace(cell_string &target, const cell_string &str, const cell_string &pattern, AMX *amx, const expression &expr, cell *pos, cell options);
}

#endif

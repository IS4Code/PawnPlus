#ifndef STORED_PARAM_H_INCLUDED
#define STORED_PARAM_H_INCLUDED

#include "modules/containers.h"
#include "modules/expressions.h"
#include "sdk/amx/amx.h"
#include <string>
#include <memory>

enum class param_type
{
	cell_param, self_id_param, string_param, variant_param, handle_param, expression_param
};

struct stored_param
{
	param_type type;
	union {
		cell cell_value;
		std::basic_string<cell> string_value;
		dyn_object variant_value;
		std::shared_ptr<handle_t> handle_value;
		expression_ptr expr_value;
	};

	static stored_param create(AMX *amx, char format, const cell *args, size_t &argi, size_t numargs);
	void push(AMX *amx, int id) const;

	stored_param() noexcept;
	stored_param(cell val) noexcept;
	stored_param(std::basic_string<cell> &&str) noexcept;
	stored_param(const dyn_object &obj) noexcept;
	stored_param(std::shared_ptr<handle_t> &&handle) noexcept;
	stored_param(expression_ptr &&expr) noexcept;
	stored_param(const stored_param &obj) noexcept;
	stored_param(stored_param &&obj) noexcept;
	~stored_param();
	stored_param &operator=(const stored_param &obj) noexcept;
	stored_param &operator=(stored_param &&obj) noexcept;
};


#endif

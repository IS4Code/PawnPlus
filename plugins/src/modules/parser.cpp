#include "parser.h"
#include <limits>

const std::unordered_map<std::string, expression_ptr> &parser_symbols()
{
	static std::unordered_map<std::string, expression_ptr> data{
		{"null", std::make_shared<constant_expression>(dyn_object())},
		{"true", std::make_shared<constant_expression>(dyn_object(1, tags::find_tag(tags::tag_bool)))},
		{"false", std::make_shared<constant_expression>(dyn_object(0, tags::find_tag(tags::tag_bool)))},
		{"cellmin", std::make_shared<constant_expression>(dyn_object(std::numeric_limits<cell>::min(), tags::find_tag(tags::tag_cell)))},
		{"cellmax", std::make_shared<constant_expression>(dyn_object(std::numeric_limits<cell>::max(), tags::find_tag(tags::tag_cell)))},
		{"cellbits", std::make_shared<constant_expression>(dyn_object(PAWN_CELL_SIZE, tags::find_tag(tags::tag_cell)))}
	};
	return data;
}

static void concat(const expression::exec_info &info, parser_options options, const expression::call_args_type &input, expression::call_args_type &output)
{
	strings::cell_string result;
	for(const auto &arg : input)
	{
		result.append(arg.to_string());
	}
	output.emplace_back(result.c_str(), result.size() + 1, tags::find_tag(tags::tag_char));
}

template <class Iter>
struct parse_base
{
	expression_ptr operator()(Iter begin, Iter end, AMX *amx, parser_options options)
	{
		return expression_parser<Iter>(options).parse_simple(amx, begin, end);
	}
};

static void eval(const expression::exec_info &info, parser_options options, const expression::call_args_type &input, expression::call_args_type &output)
{
	if(input.size() == 0)
	{
		amx_FormalError(errors::not_enough_args, 1, 0);
	}
	const auto &value = input[0];
	if(!value.tag_assignable(tags::find_tag(tags::tag_char)))
	{
		amx_LogicError("argument tag mismatch (%s: required, %s: provided)", tags::find_tag(tags::tag_char)->format_name(), value.get_tag()->format_name());
	}
	expression_ptr expr;
	if(value.is_null())
	{
		expr = expression_parser<cell*>(options).parse_simple(info.amx, nullptr, nullptr);
	}else if(value.is_cell())
	{
		cell c = value.get_cell(0);
		expr = expression_parser<cell*>(options).parse_simple(info.amx, &c, &c + 1);
	}else{
		expr = strings::select_iterator<parse_base>(value.data_begin(), info.amx, options);
	}
	expression::args_type args;
	for(auto it = std::next(input.begin()); it != input.end(); ++it)
	{
		args.push_back(std::cref(*it));
	}
	expr->execute_multi(args, info, output);
}

const std::unordered_map<std::string, intrinsic_function*> &parser_instrinsics()
{
	static std::unordered_map<std::string, intrinsic_function*> data{
		{"concat", concat},
		{"eval", eval}
	};
	return data;
}

#ifndef PARSER_H_INCLUDED
#define PARSER_H_INCLUDED

#include "errors.h"
#include "modules/expressions.h"
#include "amxinfo.h"
#include "sdk/amx/amx.h"
#include "sdk/amx/amxdbg.h"
#include <string>
#include <cstring>
#include <unordered_map>

const std::unordered_map<std::string, expression_ptr> &parser_symbols();

template <class Iter>
class expression_parser
{
	std::string parse_symbol(Iter &begin, Iter end)
	{
		std::string symbol;
		while(begin != end)
		{
			cell c = *begin;
			if(c == '@' || (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9'))
			{
				symbol.push_back(c);
			}else{
				break;
			}
			++begin;
		}
		return symbol;
	}

	cell parse_int(Iter &begin, Iter end)
	{
		cell value = 0;
		while(begin != end)
		{
			cell c = *begin;
			if(c >= '0' && c <= '9')
			{
				value = value * 10 + (c - '0');
			}else{
				break;
			}
			++begin;
		}
		return value;
	}

	expression_ptr parse_num(Iter &begin, Iter end)
	{
		cell value = 0;
		float fvalue = 0.0;
		while(begin != end)
		{
			cell c = *begin;
			if(c >= '0' && c <= '9')
			{
				value = value * 10 + (c - '0');
				fvalue = fvalue * 10.0f + (c - '0');
			}else if(c == '.')
			{
				auto old = begin;
				++begin;
				float base = 0.1f;
				while(begin != end)
				{
					c = *begin;
					if(c >= '0' && c <= '9')
					{
						fvalue += base * (c - '0');
						base *= 0.1f;
					}else{
						break;
					}
					++begin;
				}
				if(base < 0.1f)
				{
					return std::make_shared<constant_expression>(dyn_object(amx_ftoc(fvalue), tags::find_tag(tags::tag_float)));
				}
				begin = old;
			}else{
				break;
			}
			++begin;
		}
		return std::make_shared<constant_expression>(dyn_object(value, tags::find_tag(tags::tag_cell)));
	}

	expression_ptr parse_term(AMX *amx, Iter &begin, Iter end, cell endchar)
	{
		while(begin != end)
		{
			cell c = *begin;
			switch(c)
			{
				case ' ':
				case '\t':
				case '\r':
				case '\n':
				{
					++begin;
				}
				continue;
				case '\'':
					break;
				case '"':
					break;
				case '-':
				{
					++begin;
					auto inner = parse_factor(amx, begin, end, endchar);
					if(!inner)
					{
						amx_FormalError(errors::invalid_expression, "missing expression");
					}
					return std::make_shared<unary_object_expression<&dyn_object::operator- >>(std::move(inner));
				}
				case '!':
				{
					++begin;
					auto inner = parse_factor(amx, begin, end, endchar);
					if(!inner)
					{
						amx_FormalError(errors::invalid_expression, "missing expression");
					}
					return std::make_shared<unary_logic_expression<&dyn_object::operator!>>(std::move(inner));
				}
				case '~':
				{
					++begin;
					auto inner = parse_factor(amx, begin, end, endchar);
					if(!inner)
					{
						amx_FormalError(errors::invalid_expression, "missing expression");
					}
					return std::make_shared<unary_object_expression<&dyn_object::operator~>>(std::move(inner));
				}
				case '*':
				{
					++begin;
					auto inner = parse_factor(amx, begin, end, endchar);
					if(!inner)
					{
						amx_FormalError(errors::invalid_expression, "missing expression");
					}
					return std::make_shared<variant_value_expression>(std::move(inner));
				}
				case '&':
				{
					++begin;
					auto inner = parse_factor(amx, begin, end, endchar);
					if(!inner)
					{
						amx_FormalError(errors::invalid_expression, "missing expression");
					}
					return std::make_shared<variant_expression>(std::move(inner));
				}
				case '(':
				{
					auto old = begin;
					++begin;
					auto inner = parse_outer_expression(amx, begin, end, ')');
					if(!inner)
					{
						begin = old;
						return {};
					}
					++begin;
					return inner;
				}
				case '<':
				{
					++begin;
					auto inner = parse_outer_expression(amx, begin, end, '>');
					if(!inner)
					{
						amx_FormalError(errors::invalid_expression, "missing expression");
					}
					++begin;
					return std::make_shared<quote_expression>(std::move(inner));
				}
				case '{':
				{
					++begin;
					auto args = parse_expressions(amx, begin, end, '}');
					++begin;
					return std::make_shared<array_expression>(std::move(args));
				}
				case '[':
				{
					auto old = begin;
					++begin;
					auto inner = parse_outer_expression(amx, begin, end, ']');
					if(!inner)
					{
						amx_FormalError(errors::invalid_expression, "missing expression");
					}
					++begin;
					while(begin != end && (*begin == ' ' || *begin == '\t' || *begin == '\r' || *begin == '\n'))
					{
						++begin;
					}
					if(begin == end || *begin != '(')
					{
						begin = old;
						return {};
					}
					++begin;
					auto args = parse_expressions(amx, begin, end, ')');
					++begin;
					return std::make_shared<bind_expression>(std::move(inner), std::move(args));
				}
				default:
				{
					if(c == endchar)
					{
						return {};
					}
					if(c == '$')
					{
						if( ++begin == end || *begin != 'a' ||
							++begin == end || *begin != 'r' ||
							++begin == end || *begin != 'g' )
						{
							amx_FormalError(errors::invalid_expression, "invalid argument format");
						}
						++begin;
						auto old = begin;
						cell index = parse_int(begin, end);
						if(old == begin)
						{
							amx_FormalError(errors::invalid_expression, "invalid argument format");
						}
						return std::make_shared<arg_expression>(index);
					}
					if(c == '@' || (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z'))
					{
						auto symbol = parse_symbol(begin, end);
						if(begin != end)
						{
							if(*begin == ':')
							{
								++begin;
								auto inner = parse_factor(amx, begin, end, endchar);
								if(!inner)
								{
									amx_FormalError(errors::invalid_expression, "missing expression");
								}
								return std::make_shared<cast_expression>(std::move(inner), tags::find_tag(symbol.c_str()));
							}
						}
						if(symbol == "tagof")
						{
							auto inner = parse_element(amx, begin, end, endchar);
							if(!inner)
							{
								amx_FormalError(errors::invalid_expression, "missing expression");
							}
							return std::make_shared<tagof_expression>(std::move(inner));
						}else if(symbol == "sizeof")
						{
							auto inner = parse_element(amx, begin, end, endchar);
							if(!inner)
							{
								amx_FormalError(errors::invalid_expression, "missing expression");
							}
							return std::make_shared<sizeof_expression>(std::move(inner), std::vector<expression_ptr>());
						}else if(symbol == "rankof")
						{
							auto inner = parse_element(amx, begin, end, endchar);
							if(!inner)
							{
								amx_FormalError(errors::invalid_expression, "missing expression");
							}
							return std::make_shared<rankof_expression>(std::move(inner));
						}else if(symbol == "addressof")
						{
							auto inner = parse_element(amx, begin, end, endchar);
							if(!inner)
							{
								amx_FormalError(errors::invalid_expression, "missing expression");
							}
							return std::make_shared<addressof_expression>(std::move(inner));
						}else if(symbol == "try")
						{
							auto old = begin;
							while(begin != end && (*begin == ' ' || *begin == '\t' || *begin == '\r' || *begin == '\n'))
							{
								++begin;
							}
							if(begin != end && *begin == '[')
							{
								++begin;
								auto main = parse_outer_expression(amx, begin, end, ']');
								++begin;
								while(begin != end && (*begin == ' ' || *begin == '\t' || *begin == '\r' || *begin == '\n'))
								{
									++begin;
								}
								if(parse_symbol(begin, end) == "catch")
								{
									while(begin != end && (*begin == ' ' || *begin == '\t' || *begin == '\r' || *begin == '\n'))
									{
										++begin;
									}
									if(begin != end && *begin == '[')
									{
										++begin;
										auto fallback = parse_outer_expression(amx, begin, end, ']');
										++begin;
										return std::make_shared<try_expression>(std::move(main), std::move(fallback));
									}else{
										begin = old;
									}
								}else{
									begin = old;
								}
							}else{
								begin = old;
							}
						}else{
							auto it = parser_symbols().find(symbol);
							if(it != parser_symbols().end())
							{
								return it->second;
							}
						}

						AMX_DBG_SYMBOL *minsym = nullptr;
						auto obj = amx::load_lock(amx);
						auto dbg = obj->dbg.get();
						if(dbg)
						{
							ucell cip = amx->cip - 2 * sizeof(cell);;

							ucell mindist;
							for(uint16_t i = 0; i < dbg->hdr->symbols; i++)
							{
								auto sym = dbg->symboltbl[i];
								if((sym->ident == iFUNCTN || (sym->codestart <= cip && sym->codeend > cip)) && !std::strcmp(sym->name, symbol.c_str()))
								{
									if(minsym == nullptr || sym->codeend - sym->codestart < mindist)
									{
										minsym = sym;
										mindist = sym->codeend - sym->codestart;
									}
								}
							}
						}
						if(minsym)
						{
							return std::make_shared<symbol_expression>(obj, dbg, minsym);
						}else{
							auto native = amx::find_native(amx, symbol);
							if(native)
							{
								return std::make_shared<local_native_expression>(amx, native, std::move(symbol));
							}
							return std::make_shared<unknown_expression>(std::move(symbol));
						}
					}else if(c >= '0' && c <= '9')
					{
						return parse_num(begin, end);
					}else{
						return {};
					}
				}
				break;
			}
		}
		return {};
	}
	
	expression_ptr parse_element(AMX *amx, Iter &begin, Iter end, cell endchar)
	{
		expression_ptr result;
		while(begin != end)
		{
			cell c = *begin;
			switch(c)
			{
				case ' ':
				case '\t':
				case '\r':
				case '\n':
				{
					++begin;
				}
				continue;
				case '[':
				{
					if(!result)
					{
						break;
					}
					std::vector<expression_ptr> indices;
					while(true)
					{
						++begin;
						auto inner = parse_outer_expression(amx, begin, end, ']');
						if(!inner)
						{
							amx_FormalError(errors::invalid_expression, "missing expression");
						}
						++begin;
						indices.push_back(std::move(inner));
						while(begin != end && (*begin == ' ' || *begin == '\t' || *begin == '\r' || *begin == '\n'))
						{
							++begin;
						}
						if(begin == end || *begin != '[')
						{
							break;
						}
					}
					result = std::make_shared<index_expression>(std::move(result), std::move(indices));
				}
				continue;
				case '(':
				{
					if(!result)
					{
						break;
					}
					++begin;
					auto args = parse_expressions(amx, begin, end, ')');
					++begin;
					result = std::make_shared<call_expression>(std::move(result), std::move(args));
				}
				continue;
			}
			if(!result)
			{
				result = parse_term(amx, begin, end, endchar);
				if(!result)
				{
					return result;
				}
			}else{
				return result;
			}
		}
		return result;
	}
	
	expression_ptr parse_factor(AMX *amx, Iter &begin, Iter end, cell endchar)
	{
		expression_ptr result;
		while(begin != end)
		{
			cell c = *begin;
			switch(c)
			{
				case ' ':
				case '\t':
				case '\r':
				case '\n':
				{
					++begin;
				}
				continue;
				case '*':
				{
					if(!result)
					{
						break;
					}
					++begin;
					auto inner = parse_element(amx, begin, end, endchar);
					if(!inner)
					{
						amx_FormalError(errors::invalid_expression, "missing expression");
					}
					result = std::make_shared<binary_object_expression<&dyn_object::operator*>>(std::move(result), std::move(inner));
				}
				continue;
				case '/':
				{
					if(!result)
					{
						break;
					}
					++begin;
					auto inner = parse_element(amx, begin, end, endchar);
					if(!inner)
					{
						amx_FormalError(errors::invalid_expression, "missing expression");
					}
					result = std::make_shared<binary_object_expression<&dyn_object::operator/>>(std::move(result), std::move(inner));
				}
				continue;
				case '%':
				{
					if(!result)
					{
						break;
					}
					++begin;
					auto inner = parse_element(amx, begin, end, endchar);
					if(!inner)
					{
						amx_FormalError(errors::invalid_expression, "missing expression");
					}
					result = std::make_shared<binary_object_expression<&dyn_object::operator% >>(std::move(result), std::move(inner));
				}
				continue;
				case '[':
				{
					if(!result)
					{
						break;
					}
					std::vector<expression_ptr> indices;
					while(true)
					{
						++begin;
						auto inner = parse_outer_expression(amx, begin, end, ']');
						if(!inner)
						{
							amx_FormalError(errors::invalid_expression, "missing expression");
						}
						++begin;
						indices.push_back(std::move(inner));
						while(begin != end && (*begin == ' ' || *begin == '\t' || *begin == '\r' || *begin == '\n'))
						{
							++begin;
						}
						if(begin == end || *begin != '[')
						{
							break;
						}
					}
					result = std::make_shared<index_expression>(std::move(result), std::move(indices));
				}
			}
			if(!result)
			{
				result = parse_element(amx, begin, end, endchar);
				if(!result)
				{
					return result;
				}
			}else{
				return result;
			}
		}
		return result;
	}

	expression_ptr parse_operand(AMX *amx, Iter &begin, Iter end, cell endchar)
	{
		expression_ptr result;
		while(begin != end)
		{
			cell c = *begin;
			switch(c)
			{
				case ' ':
				case '\t':
				case '\r':
				case '\n':
				{
					++begin;
				}
				continue;
				case '+':
				{
					if(!result)
					{
						break;
					}
					++begin;
					auto inner = parse_factor(amx, begin, end, endchar);
					if(!inner)
					{
						amx_FormalError(errors::invalid_expression, "missing expression");
					}
					result = std::make_shared<binary_object_expression<&dyn_object::operator+>>(std::move(result), std::move(inner));
				}
				continue;
				case '-':
				{
					if(result)
					{
						++begin;
						auto inner = parse_factor(amx, begin, end, endchar);
						if(!inner)
						{
							amx_FormalError(errors::invalid_expression, "missing expression");
						}
						result = std::make_shared<binary_object_expression<&dyn_object::operator- >>(std::move(result), std::move(inner));
					}else{
						result = parse_factor(amx, begin, end, endchar);
					}
				}
				continue;
			}
			if(!result)
			{
				result = parse_factor(amx, begin, end, endchar);
				if(!result)
				{
					return result;
				}
			}else{
				return result;
			}
		}
		return result;
	}

	expression_ptr parse_condition(AMX *amx, Iter &begin, Iter end, cell endchar)
	{
		expression_ptr result;
		while(begin != end)
		{
			cell c = *begin;
			switch(c)
			{
				case ' ':
				case '\t':
				case '\r':
				case '\n':
				{
					++begin;
				}
				continue;
				case '=':
				{
					if(!result)
					{
						break;
					}
					auto old = begin;
					++begin;
					if(begin == end || *begin != '=')
					{
						begin = old;
						break;
					}
					++begin;
					auto inner = parse_operand(amx, begin, end, endchar);
					if(!inner)
					{
						amx_FormalError(errors::invalid_expression, "missing expression");
					}
					result = std::make_shared<binary_logic_expression<&dyn_object::operator==>>(std::move(result), std::move(inner));
				}
				continue;
				case '!':
				{
					if(!result)
					{
						break;
					}
					auto old = begin;
					++begin;
					if(begin == end || *begin != '=')
					{
						begin = old;
						break;
					}
					++begin;
					auto inner = parse_operand(amx, begin, end, endchar);
					if(!inner)
					{
						amx_FormalError(errors::invalid_expression, "missing expression");
					}
					result = std::make_shared<binary_logic_expression<&dyn_object::operator!=>>(std::move(result), std::move(inner));
				}
				continue;
				case '<':
				{
					if(!result)
					{
						break;
					}
					auto old = begin;
					++begin;
					if(begin != end && *begin == '=')
					{
						++begin;
						auto inner = parse_operand(amx, begin, end, endchar);
						if(!inner)
						{
							amx_FormalError(errors::invalid_expression, "missing expression");
						}
						result = std::make_shared<binary_logic_expression<&dyn_object::operator<=>>(std::move(result), std::move(inner));
					}else if(begin != end && *begin == '<')
					{
						++begin;
						auto inner = parse_operand(amx, begin, end, endchar);
						if(!inner)
						{
							amx_FormalError(errors::invalid_expression, "missing expression");
						}
						result = std::make_shared<binary_object_expression<&dyn_object::operator<<>>(std::move(result), std::move(inner));
					}else{
						auto inner = parse_operand(amx, begin, end, endchar);
						if(!inner)
						{
							begin = old;
							break;
						}
						result = std::make_shared<binary_logic_expression<&dyn_object::operator<>>(std::move(result), std::move(inner));
					}
				}
				continue;
				case '>':
				{
					if(!result)
					{
						break;
					}
					auto old = begin;
					++begin;
					if(begin != end && *begin == '=')
					{
						++begin;
						auto inner = parse_operand(amx, begin, end, endchar);
						if(!inner)
						{
							amx_FormalError(errors::invalid_expression, "missing expression");
						}
						result = std::make_shared<binary_logic_expression<&dyn_object::operator>=>>(std::move(result), std::move(inner));
					}else if(begin != end && *begin == '>')
					{
						++begin;
						auto inner = parse_operand(amx, begin, end, endchar);
						if(!inner)
						{
							amx_FormalError(errors::invalid_expression, "missing expression");
						}
						result = std::make_shared<binary_object_expression<(&dyn_object::operator>>)>>(std::move(result), std::move(inner));
					}else{
						auto inner = parse_operand(amx, begin, end, endchar);
						if(!inner)
						{
							begin = old;
							break;
						}
						result = std::make_shared<binary_logic_expression<(&dyn_object::operator>)>>(std::move(result), std::move(inner));
					}
				}
				continue;
				case '&':
				{
					if(!result)
					{
						break;
					}
					++begin;
					auto inner = parse_operand(amx, begin, end, endchar);
					if(!inner)
					{
						amx_FormalError(errors::invalid_expression, "missing expression");
					}
					result = std::make_shared<binary_object_expression<&dyn_object::operator&>>(std::move(result), std::move(inner));
				}
				continue;
				case '|':
				{
					if(!result)
					{
						break;
					}
					++begin;
					auto inner = parse_operand(amx, begin, end, endchar);
					if(!inner)
					{
						amx_FormalError(errors::invalid_expression, "missing expression");
					}
					result = std::make_shared<binary_object_expression<&dyn_object::operator|>>(std::move(result), std::move(inner));
				}
				continue;
				case '^':
				{
					if(!result)
					{
						break;
					}
					++begin;
					auto inner = parse_operand(amx, begin, end, endchar);
					if(!inner)
					{
						amx_FormalError(errors::invalid_expression, "missing expression");
					}
					result = std::make_shared<binary_object_expression<&dyn_object::operator^>>(std::move(result), std::move(inner));
				}
				continue;
			}
			if(!result)
			{
				result = parse_operand(amx, begin, end, endchar);
				if(!result)
				{
					return result;
				}
			}else{
				return result;
			}
		}
		if(endchar != '\0')
		{
			amx_FormalError(errors::invalid_expression, "unexpected end of string");
		}
		return result;
	}

	expression_ptr parse_expression(AMX *amx, Iter &begin, Iter end, cell endchar)
	{
		expression_ptr result;
		while(begin != end)
		{
			cell c = *begin;
			switch(c)
			{
				case ' ':
				case '\t':
				case '\r':
				case '\n':
				{
					++begin;
				}
				continue;
				case '&':
				{
					if(!result)
					{
						break;
					}
					auto old = begin;
					++begin;
					if(begin == end || *begin != '&')
					{
						break;
					}
					++begin;
					auto inner = parse_condition(amx, begin, end, endchar);
					result = std::make_shared<logic_and_expression>(std::move(result), std::move(inner));
				}
				continue;
				case '|':
				{
					if(!result)
					{
						break;
					}
					auto old = begin;
					++begin;
					if(begin == end || *begin != '|')
					{
						break;
					}
					++begin;
					auto inner = parse_condition(amx, begin, end, endchar);
					result = std::make_shared<logic_or_expression>(std::move(result), std::move(inner));
				}
				continue;
				case '?':
				{
					if(!result)
					{
						break;
					}
					++begin;
					auto inner1 = parse_outer_expression(amx, begin, end, ':');
					++begin;
					auto inner2 = parse_outer_expression(amx, begin, end, endchar);
					result = std::make_shared<conditional_expression>(std::move(result), std::move(inner1), std::move(inner2));
				}
				continue;
			}
			if(!result)
			{
				result = parse_condition(amx, begin, end, endchar);
				if(!result)
				{
					return result;
				}
			}else{
				return result;
			}
		}
		if(endchar != '\0')
		{
			amx_FormalError(errors::invalid_expression, "unexpected end of string");
		}
		return result;
	}

	std::vector<expression_ptr> parse_expressions(AMX *amx, Iter &begin, Iter end, cell endchar)
	{
		std::vector<expression_ptr> result;
		auto inner = parse_expression(amx, begin, end, endchar);
		if(!inner)
		{
			return result;
		}
		result.push_back(std::move(inner));
		while(begin != end)
		{
			cell c = *begin;
			switch(c)
			{
				case ' ':
				case '\t':
				case '\n':
					++begin;
					break;
				case ',':
				{
					++begin;
					inner = parse_expression(amx, begin, end, endchar);
					if(!inner)
					{
						amx_FormalError(errors::invalid_expression, "missing expression");
					}
					result.push_back(std::move(inner));
				}
				break;
				default:
				{
					if(c == endchar)
					{
						return result;
					}
					amx_FormalError(errors::invalid_expression, "unrecognized character");
				}
				break;
			}
		}
		if(endchar != '\0')
		{
			amx_FormalError(errors::invalid_expression, "unexpected end of string");
		}
		return result;
	}

	expression_ptr parse_outer_expression(AMX *amx, Iter &begin, Iter end, cell endchar)
	{
		auto result = parse_expressions(amx, begin, end, endchar);
		if(result.size() == 0)
		{
			return {};
		}
		auto next = result[0];
		for(size_t i = 1; i < result.size(); i++)
		{
			next = std::make_shared<comma_expression>(std::move(next), std::move(result[i]));
		}
		return next;
	}

public:
	expression_ptr parse_simple(AMX *amx, Iter begin, Iter end)
	{
		return parse_outer_expression(amx, begin, end, '\0');
	}

	decltype(expression_pool)::object_ptr parse(AMX *amx, Iter begin, Iter end)
	{
		auto expr = parse_outer_expression(amx, begin, end, '\0');
		return static_cast<const expression_base*>(expr.get())->clone();
	}
};

#endif

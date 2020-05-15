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

enum parser_options
{
	allow_unknown_symbols = 1,
	allow_debug_symbols = 1 << 1,
	allow_natives = 1 << 2,
	allow_publics = 1 << 3,
	allow_pubvars = 1 << 4,
	allow_constructions = 1 << 5,
	allow_ranges = 1 << 6,
	allow_arrays = 1 << 7,
	allow_queries = 1 << 8,
	allow_arguments = 1 << 9,
	allow_environment = 1 << 10,
	allow_casts = 1 << 11,
	allow_literals = 1 << 12,
	allow_intrinsics = 1 << 13,
	allow_strings = 1 << 14,

	all = -1
};

const std::unordered_map<std::string, expression_ptr> &parser_symbols();
typedef void intrinsic_function(const expression::exec_info &info, parser_options options, const expression::call_args_type &input, expression::call_args_type &output);
const std::unordered_map<std::string, intrinsic_function*> &parser_instrinsics();

template <class Iter>
class expression_parser
{
private:
	Iter parse_start;
	parser_options options;

	void amx_ExpressionError(const char *format, ...)
	{
		va_list args;
		va_start(args, format);
		std::string message(vsnprintf(NULL, 0, format, args), '\0');
		vsprintf(&message[0], format, args);
		va_end(args);
		amx_LogicError(errors::invalid_expression, message.c_str());
	}

	void amx_ParserError(const char *message, const Iter &pos, const Iter &end)
	{
		if(pos != end)
		{
			amx_ExpressionError("%s at %d ('%c')", message, pos - parse_start, *pos);
		}else{
			amx_ExpressionError("%s at %d", message, pos - parse_start);
		}
	}

	void skip_whitespace(Iter &begin, Iter end)
	{
		while(begin != end && (*begin == ' ' || *begin == '\t' || *begin == '\r' || *begin == '\n'))
		{
			++begin;
		}
	}

	std::string parse_symbol(Iter &begin, Iter end)
	{
		std::string symbol;
		while(begin != end)
		{
			cell c = *begin;
			if(c == '@' || c == '_' || (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9'))
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

	bool parse_hex_digit(cell c, int &digit)
	{
		if(c >= '0' && c <= '9')
		{
			digit = c - '0';
			return true;
		}else if(c >= 'A' && c <= 'F')
		{
			digit = 10 + (c - 'A');
			return true;
		}else if(c >= 'a' && c <= 'f')
		{
			digit = 10 + (c - 'a');
			return true;
		}else{
			return false;
		}
	}

	expression_ptr parse_hex(Iter &begin, Iter end)
	{
		ucell value = 0;
		while(begin != end)
		{
			int digit;
			if(!parse_hex_digit(*begin, digit))
			{
				break;
			}
			value = value * 16 + digit;
			++begin;
		}
		return std::make_shared<constant_expression>(dyn_object(value, tags::find_tag(tags::tag_cell)));
	}

	expression_ptr parse_num(Iter &begin, Iter end)
	{
		ucell value = 0;
		float fvalue = 0.0;
		bool first = true;
		bool maybe_hex = false;
		while(begin != end)
		{
			ucell c = *begin;
			if(maybe_hex)
			{
				if(c == 'x')
				{
					++begin;
					return parse_hex(begin, end);
				}
				maybe_hex = false;
			}
			if(c >= '0' && c <= '9')
			{
				if(first)
				{
					if(c == '0')
					{
						maybe_hex = true;
					}
					first = false;
				}
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
				break;
			}else{
				break;
			}
			++begin;
		}
		return std::make_shared<constant_expression>(dyn_object(value, tags::find_tag(tags::tag_cell)));
	}

	strings::cell_string parse_string(Iter &begin, Iter end, cell marker)
	{
		strings::cell_string buffer;
		while(begin != end)
		{
			if(*begin == marker)
			{
				return buffer;
			}else if(*begin == '\\')
			{
				++begin;
				if(begin == end)
				{
					break;
				}
				switch(*begin)
				{
					case '\\':
						buffer.push_back('\\');
						++begin;
						continue;
					case '\'':
						buffer.push_back('\'');
						++begin;
						continue;
					case '"':
						buffer.push_back('"');
						++begin;
						continue;
					case '0':
						buffer.push_back('\0');
						++begin;
						continue;
					case 'r':
						buffer.push_back('\r');
						++begin;
						continue;
					case 'n':
						buffer.push_back('\n');
						++begin;
						continue;
					case 'v':
						buffer.push_back('\v');
						++begin;
						continue;
					case 't':
						buffer.push_back('\t');
						++begin;
						continue;
					case 'b':
						buffer.push_back('\b');
						++begin;
						continue;
					case '#':
					case '%':
						buffer.push_back('%');
						++begin;
						continue;
					case 'x':
					case 'u':
						int size = (*begin == 'u') ? 8 : 2;
						cell val = 0;
						for(int i = 0; i < size; i++)
						{
							++begin;
							if(begin == end) break;
							int digit;
							if(!parse_hex_digit(*begin, digit))
							{
								size = 0;
								break;
							}
							val = val * 16 + digit;
						}
						if(size == 0) break;
						buffer.push_back(val);
						++begin;
						continue;
				}
				amx_ParserError("unrecognized escape sequence", begin, end);
			}
			buffer.push_back(*begin);
			++begin;
		}
		amx_ParserError("unexpected end of string", begin, end);
		return buffer;
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
				{
					if(options & parser_options::allow_strings)
					{
						++begin;
						auto str = parse_string(begin, end, c);
						++begin;
						if(str.size() != 1)
						{
							amx_ParserError("character constant must be a single character", begin, end);
						}
						return std::make_shared<constant_expression>(dyn_object(str[0], tags::find_tag(tags::tag_char)));
					}
					return {};
				}
				continue;
				case '"':
				{
					if(options & parser_options::allow_strings)
					{
						++begin;
						auto str = parse_string(begin, end, c);
						++begin;
						return std::make_shared<constant_expression>(dyn_object(str.data(), str.size() + 1, tags::find_tag(tags::tag_char)));
					}
					return {};
				}
				continue;
				case '-':
				{
					++begin;
					auto inner = parse_factor(amx, begin, end, endchar);
					if(!inner)
					{
						amx_ParserError("missing expression", begin, end);
					}
					return std::make_shared<unary_object_expression<&dyn_object::operator- >>(std::move(inner));
				}
				case '+':
				{
					++begin;
					auto inner = parse_factor(amx, begin, end, endchar);
					if(!inner)
					{
						amx_ParserError("missing expression", begin, end);
					}
					return std::make_shared<nested_expression>(std::move(inner));
				}
				case '!':
				{
					++begin;
					auto inner = parse_factor(amx, begin, end, endchar);
					if(!inner)
					{
						amx_ParserError("missing expression", begin, end);
					}
					return std::make_shared<unary_logic_expression<&dyn_object::operator!>>(std::move(inner));
				}
				case '~':
				{
					++begin;
					auto inner = parse_factor(amx, begin, end, endchar);
					if(!inner)
					{
						amx_ParserError("missing expression", begin, end);
					}
					return std::make_shared<unary_object_expression<&dyn_object::operator~>>(std::move(inner));
				}
				case '*':
				{
					++begin;
					auto inner = parse_factor(amx, begin, end, endchar);
					if(!inner)
					{
						amx_ParserError("missing expression", begin, end);
					}
					return std::make_shared<extract_expression>(std::move(inner));
				}
				case '&':
				{
					if(options & parser_options::allow_constructions)
					{
						++begin;
						auto inner = parse_factor(amx, begin, end, endchar);
						if(!inner)
						{
							amx_ParserError("missing expression", begin, end);
						}
						return std::make_shared<variant_expression>(std::move(inner));
					}
					return {};
				}
				case '^':
				{
					++begin;
					auto inner = parse_factor(amx, begin, end, endchar);
					if(!inner)
					{
						amx_ParserError("missing expression", begin, end);
					}
					return std::make_shared<dequote_expression>(std::move(inner));
				}
				case '(':
				{
					auto old = begin;
					++begin;
					auto inner = parse_outer_expression(amx, begin, end, ')');
					++begin;
					return inner;
				}
				case '<':
				{
					if(options & parser_options::allow_constructions)
					{
						++begin;
						auto inner = parse_outer_expression(amx, begin, end, '>');
						++begin;
						return std::make_shared<quote_expression>(std::move(inner));
					}
					return {};
				}
				case '{':
				{
					if(options & parser_options::allow_arrays)
					{
						++begin;
						auto args = parse_expressions(amx, begin, end, '}');
						++begin;
						return std::make_shared<array_expression>(std::move(args));
					}
					return {};
				}
				case '[':
				{
					auto old = begin;
					++begin;
					auto inner = parse_outer_expression(amx, begin, end, ']');
					++begin;
					skip_whitespace(begin, end);
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
						if(++begin != end)
						{
							if((options & parser_options::allow_arguments) && *begin == 'k' && ++begin != end && *begin == 'e' && ++begin != end && *begin == 'y')
							{
								++begin;
								return std::make_shared<arg_expression>(1);
							}else if((options & parser_options::allow_arguments) && *begin == 'v' && ++begin != end && *begin == 'a' && ++begin != end && *begin == 'l' && ++begin != end && *begin == 'u' && ++begin != end && *begin == 'e')
							{
								++begin;
								return std::make_shared<arg_expression>(0);
							}else if((options & parser_options::allow_arguments) && *begin == 'a' && ++begin != end && *begin == 'r' && ++begin != end && *begin == 'g')
							{
								if(++begin != end && *begin == 's')
								{
									++begin;
									auto old = begin;
									cell first = parse_int(begin, end);
									if(old == begin)
									{
										return std::make_shared<arg_pack_expression>(0);
									}
									if(begin != end && *begin == '_')
									{
										++begin;
										old = begin;
										cell last = parse_int(begin, end);
										if(old == begin || last < first)
										{
											amx_ParserError("invalid argument pack format", begin, end);
										}
										return std::make_shared<arg_pack_expression>(first, last);
									}
									return std::make_shared<arg_pack_expression>(first);
								}else{
									auto old = begin;
									cell index = parse_int(begin, end);
									if(old == begin)
									{
										amx_ParserError("invalid argument format", begin, end);
									}
									return std::make_shared<arg_expression>(index);
								}
							}else if((options & parser_options::allow_environment) && *begin == 'e' && ++begin != end && *begin == 'n' && ++begin != end && *begin == 'v')
							{
								++begin;
								return std::make_shared<env_expression>();
							}
							amx_ParserError("invalid special symbol", begin, end);
						}
					}
					if(c == '@' || c == '_' || (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z'))
					{
						auto symbol = parse_symbol(begin, end);
						if(begin != end)
						{
							if(*begin == ':' && (options & parser_options::allow_casts))
							{
								++begin;
								auto inner = parse_factor(amx, begin, end, endchar);
								if(!inner)
								{
									inner = empty_expression::instance;
								}
								if(symbol == "_")
								{
									return std::make_shared<cast_expression>(std::move(inner), tags::find_tag(tags::tag_cell));
								}
								return std::make_shared<cast_expression>(std::move(inner), tags::find_tag(symbol.c_str()));
							}
						}
						if(symbol == "@" && (options & parser_options::allow_constructions))
						{
							auto inner = parse_element(amx, begin, end, endchar);
							if(!inner)
							{
								amx_ParserError("missing expression", begin, end);
							}
							return std::make_shared<string_expression>(std::move(inner));
						}else if(symbol == "tagof")
						{
							auto inner = parse_element(amx, begin, end, endchar);
							if(!inner)
							{
								amx_ParserError("missing expression", begin, end);
							}
							return std::make_shared<tagof_expression>(std::move(inner));
						}else if(symbol == "sizeof")
						{
							auto inner = parse_element(amx, begin, end, endchar);
							if(!inner)
							{
								amx_ParserError("missing expression", begin, end);
							}
							return std::make_shared<sizeof_expression>(std::move(inner), std::vector<expression_ptr>());
						}else if(symbol == "rankof")
						{
							auto inner = parse_element(amx, begin, end, endchar);
							if(!inner)
							{
								amx_ParserError("missing expression", begin, end);
							}
							return std::make_shared<rankof_expression>(std::move(inner));
						}else if(symbol == "addressof")
						{
							auto inner = parse_element(amx, begin, end, endchar);
							if(!inner)
							{
								amx_ParserError("missing expression", begin, end);
							}
							return std::make_shared<addressof_expression>(std::move(inner));
						}else if(symbol == "nameof")
						{
							auto inner = parse_element(amx, begin, end, endchar);
							if(!inner)
							{
								amx_ParserError("missing expression", begin, end);
							}
							return std::make_shared<nameof_expression>(std::move(inner));
						}else if(symbol == "void")
						{
							auto inner = parse_element(amx, begin, end, endchar);
							if(!inner)
							{
								return std::make_shared<void_expression>(empty_expression::instance);
							}
							return std::make_shared<void_expression>(std::move(inner));
						}else if(symbol == "try")
						{
							auto old = begin;
							skip_whitespace(begin, end);
							if(begin != end && *begin == '[')
							{
								++begin;
								auto main = parse_outer_expression(amx, begin, end, ']');
								++begin;
								skip_whitespace(begin, end);
								if(parse_symbol(begin, end) == "catch")
								{
									skip_whitespace(begin, end);
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
							if(options & parser_options::allow_literals)
							{
								auto it = parser_symbols().find(symbol);
								if(it != parser_symbols().end())
								{
									return it->second;
								}
							}
							if(options & parser_options::allow_intrinsics)
							{
								auto it = parser_instrinsics().find(symbol);
								if(it != parser_instrinsics().end())
								{
									return std::make_shared<intrinsic_expression>(reinterpret_cast<void*>(it->second), static_cast<cell>(options), std::move(symbol));
								}
							}
						}

						if(amx)
						{
							if(options & parser_options::allow_debug_symbols)
							{
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
								}
							}
							if(options & parser_options::allow_publics)
							{
								int index;
								if(amx_FindPublicSafe(amx, symbol.c_str(), &index) == AMX_ERR_NONE)
								{
									return std::make_shared<public_expression>(std::move(symbol), index);
								}
							}
							if(options & parser_options::allow_pubvars)
							{
								cell amx_addr;
								if(amx_FindPubVar(amx, symbol.c_str(), &amx_addr) == AMX_ERR_NONE)
								{
									return std::make_shared<pubvar_expression>(std::move(symbol), last_pubvar_index, amx_addr);
								}
							}
							if(options & parser_options::allow_natives)
							{
								auto native = amx::find_native(amx, symbol);
								if(native)
								{
									return std::make_shared<native_expression>(native, std::move(symbol));
								}
							}
						}
						if(options & parser_options::allow_unknown_symbols)
						{
							return std::make_shared<global_expression>(std::move(symbol));
						}
						amx_ParserError("unknown symbol", begin, end);
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
						auto inner = parse_expressions(amx, begin, end, ']');
						if(inner.size() == 0)
						{
							amx_ParserError("missing expression", begin, end);
						}
						++begin;
						indices.insert(indices.end(), std::make_move_iterator(inner.begin()), std::make_move_iterator(inner.end()));
						skip_whitespace(begin, end);
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
				default:
				{
					if(!result)
					{
						break;
					}
					auto old = begin;
					auto symbol = parse_symbol(begin, end);
					if((options & parser_options::allow_queries) && symbol == "select")
					{
						skip_whitespace(begin, end);
						if(begin != end && *begin == '[')
						{
							++begin;
							auto func = parse_outer_expression(amx, begin, end, ']');
							++begin;
							result = std::make_shared<select_expression>(std::move(result), std::move(func));
						}else{
							begin = old;
							break;
						}
					}else if((options & parser_options::allow_queries) && symbol == "where")
					{
						skip_whitespace(begin, end);
						if(begin != end && *begin == '[')
						{
							++begin;
							auto func = parse_outer_expression(amx, begin, end, ']');
							++begin;
							result = std::make_shared<where_expression>(std::move(result), std::move(func));
						}else{
							begin = old;
							break;
						}
					}else{
						begin = old;
						break;
					}
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
						amx_ParserError("missing expression", begin, end);
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
						amx_ParserError("missing expression", begin, end);
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
						amx_ParserError("missing expression", begin, end);
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
						++begin;
						indices.push_back(std::move(inner));
						skip_whitespace(begin, end);
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
						amx_ParserError("missing expression", begin, end);
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
							amx_ParserError("missing expression", begin, end);
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
						amx_ParserError("missing expression", begin, end);
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
						amx_ParserError("missing expression", begin, end);
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
							amx_ParserError("missing expression", begin, end);
						}
						result = std::make_shared<binary_logic_expression<&dyn_object::operator<=>>(std::move(result), std::move(inner));
					}else if(begin != end && *begin == '<')
					{
						++begin;
						auto inner = parse_operand(amx, begin, end, endchar);
						if(!inner)
						{
							amx_ParserError("missing expression", begin, end);
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
							begin = old;
							break;
						}
						result = std::make_shared<binary_logic_expression<&dyn_object::operator>=>>(std::move(result), std::move(inner));
					}else if(begin != end && *begin == '>')
					{
						++begin;
						auto inner = parse_operand(amx, begin, end, endchar);
						if(!inner)
						{
							begin = old;
							break;
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
						amx_ParserError("missing expression", begin, end);
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
						amx_ParserError("missing expression", begin, end);
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
						amx_ParserError("missing expression", begin, end);
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
			amx_ParserError("unexpected end of string", begin, end);
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
				case '=':
				{
					if(!result)
					{
						break;
					}
					auto old = begin;
					++begin;
					auto inner = parse_expression(amx, begin, end, endchar);
					if(!inner)
					{
						begin = old;
						break;
					}
					result = std::make_shared<assign_expression>(std::move(result), std::move(inner));
				}
				continue;
				case '.':
				{
					if(!result)
					{
						break;
					}
					auto old = begin;
					++begin;
					if(begin == end || *begin != '.' || !(options & parser_options::allow_ranges))
					{
						begin = old;
						break;
					}
					++begin;
					auto inner = parse_condition(amx, begin, end, endchar);
					if(!inner)
					{
						begin = old;
						break;
					}
					result = std::make_shared<range_expression>(std::move(result), std::move(inner));
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
			amx_ParserError("unexpected end of string", begin, end);
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
						amx_ParserError("missing expression", begin, end);
					}
					result.push_back(std::move(inner));
				}
				break;
				case ';':
				{
					++begin;
					inner = parse_expression(amx, begin, end, endchar);
					if(inner)
					{
						result.push_back(std::move(inner));
					}
				}
				break;
				default:
				{
					if(c == endchar)
					{
						return result;
					}
					amx_ParserError("unrecognized character", begin, end);
				}
				break;
			}
		}
		if(endchar != '\0')
		{
			amx_ParserError("unexpected end of string", begin, end);
		}
		return result;
	}

	expression_ptr parse_outer_expression(AMX *amx, Iter &begin, Iter end, cell endchar)
	{
		auto result = parse_expressions(amx, begin, end, endchar);
		if(result.size() == 0)
		{
			return empty_expression::instance;
		}
		auto next = result[0];
		for(size_t i = 1; i < result.size(); i++)
		{
			next = std::make_shared<comma_expression>(std::move(next), std::move(result[i]));
		}
		return next;
	}

public:
	expression_parser(parser_options options) : options(options)
	{

	}

	expression_ptr parse_simple(AMX *amx, Iter begin, Iter end)
	{
		parse_start = begin;
		auto expr = parse_outer_expression(amx, begin, end, '\0');
		if(!expr)
		{
			amx_ParserError("missing expression", begin, end);
		}
		if(begin != end)
		{
			amx_ParserError("unrecognized character", begin, end);
		}
		return expr;
	}

	decltype(expression_pool)::object_ptr parse(AMX *amx, Iter begin, Iter end)
	{
		return static_cast<const expression_base*>(parse_simple(amx, begin, end).get())->clone();
	}
};

#endif

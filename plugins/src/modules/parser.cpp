#include "parser.h"
#include <limits>

const std::unordered_map<std::string, expression_ptr> &parser_symbols()
{
	static std::unordered_map<std::string, expression_ptr> data{
		{"true", std::make_shared<constant_expression>(dyn_object(1, tags::find_tag(tags::tag_bool)))},
		{"false", std::make_shared<constant_expression>(dyn_object(0, tags::find_tag(tags::tag_bool)))},
		{"cellmin", std::make_shared<constant_expression>(dyn_object(std::numeric_limits<cell>::min(), tags::find_tag(tags::tag_cell)))},
		{"cellmax", std::make_shared<constant_expression>(dyn_object(std::numeric_limits<cell>::max(), tags::find_tag(tags::tag_cell)))},
		{"cellbits", std::make_shared<constant_expression>(dyn_object(PAWN_CELL_SIZE, tags::find_tag(tags::tag_cell)))}
	};
	return data;
}

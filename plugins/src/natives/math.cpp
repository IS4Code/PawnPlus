#include "natives.h"
#include "main.h"
#include "hooks.h"
#include "exec.h"
#include "context.h"
#include "modules/tasks.h"
#include "modules/strings.h"
#include "modules/variants.h"
#include "modules/guards.h"
#include "modules/containers.h"

#include <limits>
#include <random>
#include <math.h>
#include <cmath>
#include <ctime>
#include <type_traits>

namespace Natives
{
	// native math_iadd({_,signed}:a, {_,signed}:b);
	AMX_DEFINE_NATIVE_TAG(math_iadd, 2, cell)
	{
		return params[1] + params[2];
	}

	// native math_iadd_ovf({_,signed}:a, {_,signed}:b);
	AMX_DEFINE_NATIVE_TAG(math_iadd_ovf, 2, cell)
	{
		cell a = params[1], b = params[2];
		cell res = b + a;
		if((b ^ a) >= 0 && (res ^ b) < 0)
		{
			throw errors::amx_error(AMX_ERR_DOMAIN);
		}
		return res;
	}

	// native math_isub({_,signed}:a, {_,signed}:b);
	AMX_DEFINE_NATIVE_TAG(math_isub, 2, cell)
	{
		return params[1] - params[2];
	}

	// native math_isub_ovf({_,signed}:a, {_,signed}:b);
	AMX_DEFINE_NATIVE_TAG(math_isub_ovf, 2, cell)
	{
		cell a = params[1], b = params[2];
		cell res = a - b;
		if((a ^ b) < 0 && (res ^ a) < 0)
		{
			throw errors::amx_error(AMX_ERR_DOMAIN);
		}
		return res;
	}

	// native math_imul({_,signed}:a, {_,signed}:b);
	AMX_DEFINE_NATIVE_TAG(math_imul, 2, cell)
	{
		return params[1] * params[2];
	}

	// native math_imul_ovf({_,signed}:a, {_,signed}:b);
	AMX_DEFINE_NATIVE_TAG(math_imul_ovf, 2, cell)
	{
		auto res = (int64_t)params[1] * (int64_t)params[2];
		if((cell)res != res)
		{
			throw errors::amx_error(AMX_ERR_DOMAIN);
		}
		return (cell)res;
	}

	// native math_idiv({_,signed}:a, {_,signed}:b);
	AMX_DEFINE_NATIVE_TAG(math_idiv, 2, cell)
	{
		if(params[2] == 0)
		{
			throw errors::amx_error(AMX_ERR_DIVIDE);
		}
		return params[1] / params[2];
	}

	// native math_idiv({_,signed}:a, {_,signed}:b);
	AMX_DEFINE_NATIVE_TAG(math_imod, 2, cell)
	{
		if(params[2] == 0)
		{
			throw errors::amx_error(AMX_ERR_DIVIDE);
		}
		return params[1] % params[2];
	}

	// native math_iinc({_,signed}:a);
	AMX_DEFINE_NATIVE_TAG(math_iinc, 1, cell)
	{
		return params[1] + 1;
	}

	// native math_iinc_ovf({_,signed}:a);
	AMX_DEFINE_NATIVE_TAG(math_iinc_ovf, 1, cell)
	{
		cell a = params[1];
		if(a == std::numeric_limits<cell>::max())
		{
			throw errors::amx_error(AMX_ERR_DOMAIN);
		}
		return a + 1;
	}

	// native math_idec({_,signed}:a);
	AMX_DEFINE_NATIVE_TAG(math_idec, 1, cell)
	{
		return params[1] - 1;
	}

	// native math_idec_ovf({_,signed}:a);
	AMX_DEFINE_NATIVE_TAG(math_idec_ovf, 1, cell)
	{
		cell a = params[1];
		if(a == std::numeric_limits<cell>::min())
		{
			throw errors::amx_error(AMX_ERR_DOMAIN);
		}
		return a - 1;
	}

	// native math_uadd({_,unsigned}:a, {_,unsigned}:b);
	AMX_DEFINE_NATIVE_TAG(math_uadd, 2, cell)
	{
		return (ucell)params[1] + (ucell)params[2];
	}

	// native math_uadd_ovf({_,unsigned}:a, {_,unsigned}:b);
	AMX_DEFINE_NATIVE_TAG(math_uadd_ovf, 2, cell)
	{
		ucell a = params[1], b = params[2];
		ucell res = b + a;
		if(res < b)
		{
			throw errors::amx_error(AMX_ERR_DOMAIN);
		}
		return res;
	}

	// native math_usub({_,unsigned}:a, {_,unsigned}:b);
	AMX_DEFINE_NATIVE_TAG(math_usub, 2, cell)
	{
		return (ucell)params[1] - (ucell)params[2];
	}

	// native math_usub_ovf({_,unsigned}:a, {_,unsigned}:b);
	AMX_DEFINE_NATIVE_TAG(math_usub_ovf, 2, cell)
	{
		ucell a = params[1], b = params[2];
		ucell res = a - b;
		if(res > a)
		{
			throw errors::amx_error(AMX_ERR_DOMAIN);
		}
		return res;
	}

	// native math_umul({_,unsigned}:a, {_,unsigned}:b);
	AMX_DEFINE_NATIVE_TAG(math_umul, 2, cell)
	{
		return (ucell)params[1] * (ucell)params[2];
	}

	// native math_umul_ovf({_,unsigned}:a, {_,unsigned}:b);
	AMX_DEFINE_NATIVE_TAG(math_umul_ovf, 2, cell)
	{
		auto res = (uint64_t)(ucell)params[1] * (uint64_t)(ucell)params[2];
		if(res > 0xffffffff)
		{
			throw errors::amx_error(AMX_ERR_DOMAIN);
		}
		return (cell)(ucell)res;
	}

	// native math_udiv({_,unsigned}:a, {_,unsigned}:b);
	AMX_DEFINE_NATIVE_TAG(math_udiv, 2, cell)
	{
		if((ucell)params[2] == 0)
		{
			throw errors::amx_error(AMX_ERR_DIVIDE);
		}
		return (ucell)params[1] / (ucell)params[2];
	}

	// native math_umod({_,unsigned}:a, {_,unsigned}:b);
	AMX_DEFINE_NATIVE_TAG(math_umod, 2, cell)
	{
		if((ucell)params[2] == 0)
		{
			throw errors::amx_error(AMX_ERR_DIVIDE);
		}
		return (ucell)params[1] % (ucell)params[2];
	}

	// native math_uinc({_,unsigned}:a);
	AMX_DEFINE_NATIVE_TAG(math_uinc, 1, cell)
	{
		return (ucell)params[1] + 1;
	}

	// native math_uinc_ovf({_,unsigned}:a);
	AMX_DEFINE_NATIVE_TAG(math_uinc_ovf, 1, cell)
	{
		ucell a = params[1];
		if(a == std::numeric_limits<ucell>::max())
		{
			throw errors::amx_error(AMX_ERR_DOMAIN);
		}
		return a + 1;
	}

	// native math_udec({_,unsigned}:a);
	AMX_DEFINE_NATIVE_TAG(math_udec, 1, cell)
	{
		return (ucell)params[1] - 1;
	}

	// native math_udec_ovf({_,unsigned}:a);
	AMX_DEFINE_NATIVE_TAG(math_udec_ovf, 1, cell)
	{
		ucell a = params[1];
		if(a == std::numeric_limits<ucell>::min())
		{
			throw errors::amx_error(AMX_ERR_DOMAIN);
		}
		return a - 1;
	}

	// native bool:math_ilt({_,signed}:a, {_,signed}:b);
	AMX_DEFINE_NATIVE_TAG(math_ilt, 2, bool)
	{
		return params[1] < params[2];
	}

	// native bool:math_ilte({_,signed}:a, {_,signed}:b);
	AMX_DEFINE_NATIVE_TAG(math_ilte, 2, bool)
	{
		return params[1] <= params[2];
	}

	// native bool:math_igt({_,signed}:a, {_,signed}:b);
	AMX_DEFINE_NATIVE_TAG(math_igt, 2, bool)
	{
		return params[1] > params[2];
	}

	// native bool:math_igte({_,signed}:a, {_,signed}:b);
	AMX_DEFINE_NATIVE_TAG(math_igte, 2, bool)
	{
		return params[1] >= params[2];
	}

	// native bool:math_ult({_,unsigned}:a, {_,unsigned}:b);
	AMX_DEFINE_NATIVE_TAG(math_ult, 2, bool)
	{
		return (ucell)params[1] < (ucell)params[2];
	}

	// native bool:math_ulte({_,unsigned}:a, {_,unsigned}:b);
	AMX_DEFINE_NATIVE_TAG(math_ulte, 2, bool)
	{
		return (ucell)params[1] <= (ucell)params[2];
	}

	// native bool:math_ugt({_,unsigned}:a, {_,unsigned}:b);
	AMX_DEFINE_NATIVE_TAG(math_ugt, 2, bool)
	{
		return (ucell)params[1] >= (ucell)params[2];
	}

	// native bool:math_ugte({_,unsigned}:a, {_,unsigned}:b);
	AMX_DEFINE_NATIVE_TAG(math_ugte, 2, bool)
	{
		return (ucell)params[1] >= (ucell)params[2];
	}

	static decltype(std::mt19937::default_seed) default_seed()
	{
		static thread_local std::random_device random_device;
		return random_device() ^ static_cast<std::random_device::result_type>(std::time(nullptr));
	}

	thread_local std::mt19937 generator(default_seed());
	
	template <typename Wide, typename Generator, typename UInt>
	static UInt rand_gen(Generator &g, UInt range)
	{
		static_assert(!std::is_signed<UInt>::value && !std::is_signed<Wide>::value, "types must be unsigned");

		auto product = static_cast<Wide>(g()) * static_cast<Wide>(range);
		auto low = static_cast<UInt>(product);
		if(low < range)
		{
			auto threshold = (static_cast<UInt>(0) - range) % range;
			while(low < threshold)
			{
				product = static_cast<Wide>(g()) * static_cast<Wide>(range);
				low = static_cast<UInt>(product);
			}
		}
		return product >> std::numeric_limits<UInt>::digits;
	}

	static ucell rand_cell(ucell range)
	{
		return rand_gen<std::uint_fast64_t>(generator, range);
	}
	
	// native math_random_seed(seed);
	AMX_DEFINE_NATIVE_TAG(math_random_seed, 1, cell)
	{
		generator.seed(params[1]);
		return 1;
	}

	// native math_random(min=0, max=cellmax);
	AMX_DEFINE_NATIVE_TAG(math_random, 0, cell)
	{
		cell min = optparam(1, 0);
		cell max = optparam(2, std::numeric_limits<cell>::max());
		if(max < min)
		{
			amx_LogicError(errors::out_of_range, "max");
		}
		if(min == std::numeric_limits<cell>::min() && max == std::numeric_limits<cell>::max())
		{
			return generator();
		}
		return rand_cell(max - min + 1) + min;
	}

	// native math_random_unsigned(min=0, max=-1);
	AMX_DEFINE_NATIVE_TAG(math_random_unsigned, 0, cell)
	{
		ucell min = optparam(1, 0);
		ucell max = optparam(2, -1);
		if(max < min)
		{
			amx_LogicError(errors::out_of_range, "max");
		}
		if(min == std::numeric_limits<ucell>::min() && max == std::numeric_limits<ucell>::max())
		{
			return generator();
		}
		return rand_cell(max - min + 1) + min;
	}

	// native Float:math_random_float(Float:min=0.0, Float:max=1.0);
	AMX_DEFINE_NATIVE_TAG(math_random_float, 0, float)
	{
		cell cmin = optparam(1, 0);
		cell cmax = optparam(2, 0x3F800000);
		float min = amx_ctof(cmin);
		if(std::isnan(min))
		{
			amx_LogicError(errors::out_of_range, "min");
		}
		float max = amx_ctof(cmax);
		if(std::isnan(max) || max < min)
		{
			amx_LogicError(errors::out_of_range, "max");
		}
		if(cmin == cmax)
		{
			return cmin;
		}
		if(std::isinf(min))
		{
			if(std::isinf(max))
			{
				return (generator() % 2) == 0 ? cmin : cmax;
			}else{
				return cmin;
			}
		}else if(std::isinf(max))
		{
			return cmax;
		}
		float scale = (float)(generator.max() - generator.min()) + 1.0f;
		float result = (generator() - generator.min()) / scale * (max - min) + min;
		return amx_ftoc(result);
	}

	template <float (&Func)(float)>
	cell AMX_NATIVE_CALL math_tointeger(AMX *amx, cell *params)
	{
		float val = amx_ctof(params[1]);
		if(std::isnan(val))
		{
			amx_LogicError(errors::out_of_range, "val");
		}
		val = Func(val);
		if((double)val > (double)std::numeric_limits<cell>::max() || (double)val < (double)std::numeric_limits<cell>::min())
		{
			throw errors::amx_error(AMX_ERR_DOMAIN);
		}
		return static_cast<cell>(val);
	}

	// native math_round(Float:val);
	AMX_DEFINE_NATIVE_TAG(math_round, 1, cell)
	{
		return math_tointeger<roundf>(amx, params);
	}

	// native math_floor(Float:val);
	AMX_DEFINE_NATIVE_TAG(math_floor, 1, cell)
	{
		return math_tointeger<floorf>(amx, params);
	}

	// native math_ceiling(Float:val);
	AMX_DEFINE_NATIVE_TAG(math_ceiling, 1, cell)
	{
		return math_tointeger<ceilf>(amx, params);
	}

	// native math_truncate(Float:val);
	AMX_DEFINE_NATIVE_TAG(math_truncate, 1, cell)
	{
		return math_tointeger<truncf>(amx, params);
	}

	template <float (&Func)(float)>
	cell AMX_NATIVE_CALL math_try_tointeger(AMX *amx, cell *params)
	{
		float val = amx_ctof(params[1]);
		if(std::isnan(val))
		{
			return 0;
		}
		val = Func(val);
		if((double)val > (double)std::numeric_limits<cell>::max() || (double)val < (double)std::numeric_limits<cell>::min())
		{
			return 0;
		}
		cell *addr = amx_GetAddrSafe(amx, params[2]);
		*addr = static_cast<cell>(val);
		return 1;
	}

	// native bool:math_try_round(Float:val, &result);
	AMX_DEFINE_NATIVE_TAG(math_try_round, 1, cell)
	{
		return math_try_tointeger<roundf>(amx, params);
	}

	// native bool:math_try_floor(Float:val, &result);
	AMX_DEFINE_NATIVE_TAG(math_try_floor, 1, cell)
	{
		return math_try_tointeger<floorf>(amx, params);
	}

	// native bool:math_try_ceiling(Float:val, &result);
	AMX_DEFINE_NATIVE_TAG(math_try_ceiling, 1, cell)
	{
		return math_try_tointeger<ceilf>(amx, params);
	}

	// native bool:math_try_truncate(Float:val, &result);
	AMX_DEFINE_NATIVE_TAG(math_try_truncate, 1, cell)
	{
		return math_try_tointeger<truncf>(amx, params);
	}
}

static AMX_NATIVE_INFO native_list[] =
{
	AMX_DECLARE_NATIVE(math_iadd),
	AMX_DECLARE_NATIVE(math_iadd_ovf),
	AMX_DECLARE_NATIVE(math_isub),
	AMX_DECLARE_NATIVE(math_isub_ovf),
	AMX_DECLARE_NATIVE(math_imul),
	AMX_DECLARE_NATIVE(math_imul_ovf),
	AMX_DECLARE_NATIVE(math_idiv),
	AMX_DECLARE_NATIVE(math_imod),
	AMX_DECLARE_NATIVE(math_iinc),
	AMX_DECLARE_NATIVE(math_iinc_ovf),
	AMX_DECLARE_NATIVE(math_idec),
	AMX_DECLARE_NATIVE(math_idec_ovf),
	AMX_DECLARE_NATIVE(math_uadd),
	AMX_DECLARE_NATIVE(math_uadd_ovf),
	AMX_DECLARE_NATIVE(math_usub),
	AMX_DECLARE_NATIVE(math_usub_ovf),
	AMX_DECLARE_NATIVE(math_umul),
	AMX_DECLARE_NATIVE(math_umul_ovf),
	AMX_DECLARE_NATIVE(math_udiv),
	AMX_DECLARE_NATIVE(math_umod),
	AMX_DECLARE_NATIVE(math_uinc),
	AMX_DECLARE_NATIVE(math_uinc_ovf),
	AMX_DECLARE_NATIVE(math_udec),
	AMX_DECLARE_NATIVE(math_udec_ovf),

	AMX_DECLARE_NATIVE(math_ilt),
	AMX_DECLARE_NATIVE(math_ilte),
	AMX_DECLARE_NATIVE(math_igt),
	AMX_DECLARE_NATIVE(math_igte),
	AMX_DECLARE_NATIVE(math_ult),
	AMX_DECLARE_NATIVE(math_ulte),
	AMX_DECLARE_NATIVE(math_ugt),
	AMX_DECLARE_NATIVE(math_ugte),

	AMX_DECLARE_NATIVE(math_random_seed),
	AMX_DECLARE_NATIVE(math_random),
	AMX_DECLARE_NATIVE(math_random_unsigned),
	AMX_DECLARE_NATIVE(math_random_float),
	AMX_DECLARE_NATIVE(math_round),
	AMX_DECLARE_NATIVE(math_floor),
	AMX_DECLARE_NATIVE(math_ceiling),
	AMX_DECLARE_NATIVE(math_truncate),
	AMX_DECLARE_NATIVE(math_try_round),
	AMX_DECLARE_NATIVE(math_try_floor),
	AMX_DECLARE_NATIVE(math_try_ceiling),
	AMX_DECLARE_NATIVE(math_try_truncate),
};

int RegisterMathNatives(AMX *amx)
{
	return amx_Register(amx, native_list, sizeof(native_list) / sizeof(*native_list));
}

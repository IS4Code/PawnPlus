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

namespace Natives
{
	// native math_iadd({_,signed}:a, {_,signed}:b);
	AMX_DEFINE_NATIVE(math_iadd, 2)
	{
		return params[1] + params[2];
	}

	// native math_iadd_ovf({_,signed}:a, {_,signed}:b);
	AMX_DEFINE_NATIVE(math_iadd_ovf, 2)
	{
		cell a = params[1], b = params[2];
		cell res = b + a;
		if((b ^ a) >= 0 && (res ^ b) < 0)
		{
			amx_RaiseError(amx, AMX_ERR_DOMAIN);
			return 0;
		}
		return res;
	}

	// native math_isub({_,signed}:a, {_,signed}:b);
	AMX_DEFINE_NATIVE(math_isub, 2)
	{
		return params[1] - params[2];
	}

	// native math_isub_ovf({_,signed}:a, {_,signed}:b);
	AMX_DEFINE_NATIVE(math_isub_ovf, 2)
	{
		cell a = params[1], b = params[2];
		cell res = b - a;
		if((b ^ a) < 0 && (res ^ b) < 0)
		{
			amx_RaiseError(amx, AMX_ERR_DOMAIN);
			return 0;
		}
		return res;
	}

	// native math_imul({_,signed}:a, {_,signed}:b);
	AMX_DEFINE_NATIVE(math_imul, 2)
	{
		return params[1] * params[2];
	}

	// native math_imul_ovf({_,signed}:a, {_,signed}:b);
	AMX_DEFINE_NATIVE(math_imul_ovf, 2)
	{
		auto res = (int64_t)params[1] * (int64_t)params[2];
		if((cell)res != res)
		{
			amx_RaiseError(amx, AMX_ERR_DOMAIN);
			return 0;
		}
		return (cell)res;
	}

	// native math_idiv({_,signed}:a, {_,signed}:b);
	AMX_DEFINE_NATIVE(math_idiv, 2)
	{
		return params[1] / params[2];
	}

	// native math_idiv({_,signed}:a, {_,signed}:b);
	AMX_DEFINE_NATIVE(math_imod, 2)
	{
		return params[1] % params[2];
	}

	// native math_iinc({_,signed}:a);
	AMX_DEFINE_NATIVE(math_iinc, 1)
	{
		return params[1] + 1;
	}

	// native math_iinc_ovf({_,signed}:a);
	AMX_DEFINE_NATIVE(math_iinc_ovf, 1)
	{
		cell a = params[1];
		if(a == std::numeric_limits<cell>::max())
		{
			amx_RaiseError(amx, AMX_ERR_DOMAIN);
			return 0;
		}
		return a + 1;
	}

	// native math_idec({_,signed}:a);
	AMX_DEFINE_NATIVE(math_idec, 1)
	{
		return params[1] - 1;
	}

	// native math_idec_ovf({_,signed}:a);
	AMX_DEFINE_NATIVE(math_idec_ovf, 1)
	{
		cell a = params[1];
		if(a == std::numeric_limits<cell>::min())
		{
			amx_RaiseError(amx, AMX_ERR_DOMAIN);
			return 0;
		}
		return a - 1;
	}

	// native math_uadd({_,unsigned}:a, {_,unsigned}:b);
	AMX_DEFINE_NATIVE(math_uadd, 2)
	{
		return (ucell)params[1] + (ucell)params[2];
	}

	// native math_uadd_ovf({_,unsigned}:a, {_,unsigned}:b);
	AMX_DEFINE_NATIVE(math_uadd_ovf, 2)
	{
		ucell a = params[1], b = params[2];
		ucell res = b + a;
		if(res < b)
		{
			amx_RaiseError(amx, AMX_ERR_DOMAIN);
			return 0;
		}
		return res;
	}

	// native math_usub({_,unsigned}:a, {_,unsigned}:b);
	AMX_DEFINE_NATIVE(math_usub, 2)
	{
		return (ucell)params[1] - (ucell)params[2];
	}

	// native math_usub_ovf({_,unsigned}:a, {_,unsigned}:b);
	AMX_DEFINE_NATIVE(math_usub_ovf, 2)
	{
		ucell a = params[1], b = params[2];
		ucell res = b - a;
		if(res > b)
		{
			amx_RaiseError(amx, AMX_ERR_DOMAIN);
			return 0;
		}
		return res;
	}

	// native math_umul({_,unsigned}:a, {_,unsigned}:b);
	AMX_DEFINE_NATIVE(math_umul, 2)
	{
		return (ucell)params[1] * (ucell)params[2];
	}

	// native math_umul_ovf({_,unsigned}:a, {_,unsigned}:b);
	AMX_DEFINE_NATIVE(math_umul_ovf, 2)
	{
		auto res = (uint64_t)(ucell)params[1] * (uint64_t)(ucell)params[2];
		if(res > 0xffffffff)
		{
			amx_RaiseError(amx, AMX_ERR_DOMAIN);
			return 0;
		}
		return (cell)(ucell)res;
	}

	// native math_udiv({_,unsigned}:a, {_,unsigned}:b);
	AMX_DEFINE_NATIVE(math_udiv, 2)
	{
		return (ucell)params[1] / (ucell)params[2];
	}

	// native math_umod({_,unsigned}:a, {_,unsigned}:b);
	AMX_DEFINE_NATIVE(math_umod, 2)
	{
		return (ucell)params[1] % (ucell)params[2];
	}

	// native math_uinc({_,unsigned}:a);
	AMX_DEFINE_NATIVE(math_uinc, 1)
	{
		return (ucell)params[1] + 1;
	}

	// native math_uinc_ovf({_,unsigned}:a);
	AMX_DEFINE_NATIVE(math_uinc_ovf, 1)
	{
		ucell a = params[1];
		if(a == std::numeric_limits<ucell>::max())
		{
			amx_RaiseError(amx, AMX_ERR_DOMAIN);
			return 0;
		}
		return a + 1;
	}

	// native math_udec({_,unsigned}:a);
	AMX_DEFINE_NATIVE(math_udec, 1)
	{
		return (ucell)params[1] - 1;
	}

	// native math_udec_ovf({_,unsigned}:a);
	AMX_DEFINE_NATIVE(math_udec_ovf, 1)
	{
		ucell a = params[1];
		if(a == std::numeric_limits<ucell>::min())
		{
			amx_RaiseError(amx, AMX_ERR_DOMAIN);
			return 0;
		}
		return a - 1;
	}

	// native bool:math_ilt({_,signed}:a, {_,signed}:b);
	AMX_DEFINE_NATIVE(math_ilt, 2)
	{
		return params[1] < params[2];
	}

	// native bool:math_ilte({_,signed}:a, {_,signed}:b);
	AMX_DEFINE_NATIVE(math_ilte, 2)
	{
		return params[1] <= params[2];
	}

	// native bool:math_igt({_,signed}:a, {_,signed}:b);
	AMX_DEFINE_NATIVE(math_igt, 2)
	{
		return params[1] > params[2];
	}

	// native bool:math_igte({_,signed}:a, {_,signed}:b);
	AMX_DEFINE_NATIVE(math_igte, 2)
	{
		return params[1] >= params[2];
	}

	// native bool:math_ult({_,unsigned}:a, {_,unsigned}:b);
	AMX_DEFINE_NATIVE(math_ult, 2)
	{
		return (ucell)params[1] < (ucell)params[2];
	}

	// native bool:math_ulte({_,unsigned}:a, {_,unsigned}:b);
	AMX_DEFINE_NATIVE(math_ulte, 2)
	{
		return (ucell)params[1] <= (ucell)params[2];
	}

	// native bool:math_ugt({_,unsigned}:a, {_,unsigned}:b);
	AMX_DEFINE_NATIVE(math_ugt, 2)
	{
		return (ucell)params[1] >= (ucell)params[2];
	}

	// native bool:math_ugte({_,unsigned}:a, {_,unsigned}:b);
	AMX_DEFINE_NATIVE(math_ugte, 2)
	{
		return (ucell)params[1] >= (ucell)params[2];
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
};

int RegisterMathNatives(AMX *amx)
{
	return amx_Register(amx, native_list, sizeof(native_list) / sizeof(*native_list));
}

/** PawnPlus v1.3.1 by IllidanS4 **/
//github.com/IllidanS4/PawnPlus

#if defined _inc_PawnPlus
#undef _inc_PawnPlus
#endif

#if defined _PawnPlus_included
#endinput
#endif
#define _PawnPlus_included

/*                 */
/*      Setup      */
/*                 */

const PP_VERSION = 0x131;

#if defined PP_SYNTAX
#define PP_SYNTAX_AMX_FORKED
#define PP_SYNTAX_@
#define PP_SYNTAX_@@
#define PP_SYNTAX_AWAIT
#define PP_SYNTAX_YIELD
#define PP_SYNTAX_THREADED
#define PP_SYNTAX_FOR_LIST
#define PP_SYNTAX_FOR_MAP
#define PP_SYNTAX_FOR_LINKED_LIST
#define PP_SYNTAX_STRING_OP
#define PP_SYNTAX_GENERIC
#define PP_SYNTAX_AMX_PARALLEL
#define PP_SYNTAX_ASYNC
#define PP_SYNTAX_FOR_POOL
#define PP_SYNTAX_ON_INIT
#define PP_SYNTAX_ON_EXIT
#endif

#define ConstString String@Const
#define ConstVariant Variant@Const

#if defined PP_SYNTAX_GENERIC

#define List<%0> List@%0
#define Map<%0,%1> Map@%0@%1
#define LinkedList<%0> LinkedList@%0
#define Iter<%0> Iter@%0
#define PairIter<%0,%1> Iter@%1@%0
#define Handle<%0> Handle@%0
#define Ref<%0> Ref@%0
#define Task<%0> Task@%0
#define Pool<%0> Pool@%0

#endif

#define StringTags String
#define StringTag {StringTags}
#define ConstStringTags ConstString,StringTags
#define ConstStringTag {ConstStringTags}
#define VariantTags Variant
#define VariantTag {VariantTags}
#define ConstVariantTags ConstVariant,VariantTags
#define ConstVariantTag {ConstVariantTags}
#define IterTags Iter
#define IterTag {IterTags}
#define HandleTags Handle
#define HandleTag {HandleTags}

#define TagTags _,tag_uid
#define TagTag {TagTags}

#if !defined PP_ALL_TAGS
#define PP_ALL_TAGS _,bool,Float,VariantTags,StringTags,List,LinkedList,Map,Pool,IterTags,HandleTags,Task,Expression
#if defined PP_ADDITIONAL_TAGS
#define AnyTag {PP_ALL_TAGS,PP_ADDITIONAL_TAGS}
#else
#define AnyTag {PP_ALL_TAGS}
#endif
#else
#define AnyTag {PP_ALL_TAGS}
#endif

stock const _PP@TRUE = true;

stock _PP@TMP1, _PP@TMP2, _PP@TMP3;
#define _PP@END

#define _PP@TMP2=(%0,%1)_PP@END) _PP@TMP1=(%0)),%1
#define _PP@CAST[%0](%1) (%0:_PP@TMP2=(%1)_PP@END)

#define _PP@TMP3=(%0,%1)[%2] _PP@TMP1=(%0)[%2]
#define _PP@EXPAND:(%0,%1)_PP@END) (%0)),%1
#define _PP@CAST_ARR[%0](%1) ((_PP@TRUE?(%0:0):(%0:_PP@TMP3=(%1)[0])),%0:_PP@EXPAND:(%1)_PP@END)

#define _PP@REMOVE_COLON:(%0:) (%0)

#define _PP@TAGGED_PACK(%0) ArgTag:_PP@TAGGED:_PP@TAGGED_END:%0
#define _PP@TAGGED:_PP@TAGGED_END:%0, pawn_arg_pack(%0),ArgTag:_PP@TAGGED:_PP@TAGGED_END:
#define _PP@TAGGED_END:%0) pawn_arg_pack(%0))

#define _PP@TAGGED1_PACK(%0) ArgTag:_PP@TAGGED1:_PP@TAGGED1_END:%0
#define _PP@TAGGED1:_PP@TAGGED1_END:%0,%1, pawn_arg_pack(%0),%1,ArgTag:_PP@TAGGED1:_PP@TAGGED1_END:
#define _PP@TAGGED1_END:%0,%1) pawn_arg_pack(%0),%1)

#define _PP@TAGGED2_PACK(%0) ArgTag:_PP@TAGGED2:_PP@TAGGED2_END:%0
#define _PP@TAGGED2:_PP@TAGGED2_END:%0,%1, %0,pawn_arg_pack(%1),ArgTag:_PP@TAGGED2:_PP@TAGGED2_END:
#define _PP@TAGGED2_END:%0,%1) %0,pawn_arg_pack(%1))

/*                 */
/*    Adapters     */
/*                 */

native print_s(ConstStringTag:string);


/*                 */
/*  Configuration  */
/*                 */

native pp_version();
native pp_version_string(version[], size=sizeof(version));
native String:pp_version_string_s();

native pp_hook_strlen(bool:hook);
native pp_hook_check_ref_args(bool:hook);
native pp_max_recursion(level);
native pp_public_min_index(index);
native bool:pp_use_funcidx(bool:use);

native pp_tick();
native pp_num_tasks();
native pp_num_local_strings();
native pp_num_global_strings();
native pp_num_local_variants();
native pp_num_global_variants();
native pp_num_lists();
native pp_num_linked_lists();
native pp_num_maps();
native pp_num_pools();
native pp_num_guards();
native pp_num_amx_guards();
native pp_entry(name[], size=sizeof(name));
native String:pp_entry_s();
native pp_num_local_iters();
native pp_num_global_iters();
native pp_num_local_handles();
native pp_num_global_handles();
native pp_num_local_expressions();
native pp_num_global_expressions();
native pp_max_hooked_natives();
native pp_num_hooked_natives();
native pp_collect();
native pp_num_natives();
native pp_module_name(const function[], name[], size=sizeof(name));
native String:pp_module_name_s(const function[]);

enum error_level
{
    error_message = 0,
    error_warning = 1,
    error_logic = 2,
    error_formal = 3,
    error_none = cellmax,
}

native error_level:pp_error_level(error_level:level);
native pp_raise_error(const message[], error_level:level=error_logic);
forward pp_on_error(source[], message[], error_level:level, &retval);

enum locale_category (<<= 1)
{
    locale_none = 0,
    locale_collate = 1,
    locale_ctype,
    locale_monetary,
    locale_numeric,
    locale_time,
    locale_messages,
    locale_all = -1,
}

native pp_locale(const locale[], locale_category:category=locale_all);
native pp_locale_name(locale[], size=sizeof(locale));
native String:pp_locale_name_s();

native pp_format_env_push(Map:env);
native pp_format_env_pop();

native pp_stackspace();


/*                 */
/*      Pawn       */
/*                 */

enum filter_type
{
    filter_in = 0,
    filter_out = 1,
}

enum handler_flags
{
    handler_default = 0,
    handler_return = 1,
    handler_args = 2,
}

const Guard:INVALID_GUARD = Guard:0;
const CallbackHandler:INVALID_CALLBACK_HANDLER = CallbackHandler:0;
const NativeHook:INVALID_NATIVE_HOOK = NativeHook:0;

native bool:pawn_native_exists(const function[]);
native bool:pawn_native_imported(const function[]);
native bool:pawn_public_exists(const function[]);
native pawn_call_native(const function[], const format[], AnyTag:...);
native pawn_call_public(const function[], const format[], AnyTag:...);
native amx_err:pawn_try_call_native(const function[], &result, const format[], AnyTag:...);
native amx_err:pawn_try_call_native_msg(const function[], &result, msg[], msg_size=sizeof(msg), const format[]="", AnyTag:...);
native amx_err:pawn_try_call_native_msg_s(const function[], &result, &StringTag:msg, const format[], AnyTag:...);
native amx_err:pawn_try_call_public(const function[], &result, const format[], AnyTag:...);
native pawn_create_callback(const callback[], Expression:action);
native CallbackHandler:pawn_register_callback(const callback[], const handler[], handler_flags:flags=handler_default, const additional_format[]="", AnyTag:...);
native pawn_unregister_callback(CallbackHandler:id);
native NativeHook:pawn_add_hook(const function[], const format[], const handler[], const additional_format[]="", AnyTag:...);
native NativeHook:pawn_add_filter(const function[], const format[], const handler[], filter_type:type=filter_in, const additional_format[]="", AnyTag:...);
native pawn_remove_hook(NativeHook:id);
native Guard:pawn_guard(AnyTag:value, TagTag:tag_id=tagof(value));
native Guard:pawn_guard_arr(AnyTag:value[], size=sizeof(value), TagTag:tag_id=tagof(value));
native bool:pawn_guard_valid(Guard:guard);
native pawn_guard_free(Guard:guard);
native List:pawn_get_args(const format[], bool:byref=false, level=0);
#define pawn_nameof(%0) ((tagof(%0)),_:_PP@REMOVE_COLON:(#%0))
native ArgTag:[2]pawn_arg_pack(AnyTag:value, tag_id=tagof(value));
#define pawn_cast<%0>(%1) (%0:_PP@TMP2=(%1))
#define pawn_same(%1,%2) (_:(%1)==_:(%2))

#if defined PP_SYNTAX_GENERIC

stock _pp_fake_cast(val)
{
    return val;
}

#define pawn_ref<%0>(%1) (Ref<%0>:_pp_fake_cast(_:_PP@CAST[%0](%1)))
#define pawn_unref<%0>(%1) (%0:_pp_fake_cast(_:_PP@CAST[Ref<%0>](%1)))

#endif

#if defined PP_SYNTAX_ON_INIT
#define pawn_on_init[%0] forward _pp@on_init@%0();public _pp@on_init@%0()
#endif

#if defined PP_SYNTAX_ON_EXIT
#define pawn_on_exit[%0] forward _pp@on_exit@%0();public _pp@on_exit@%0()
#endif

/*                 */
/*      Tags       */
/*                 */

const tag_uid:tag_uid_unknown = tag_uid:0;
const tag_uid:tag_uid_cell = tag_uid:1;
const tag_uid:tag_uid_bool = tag_uid:2;
const tag_uid:tag_uid_char = tag_uid:3;
const tag_uid:tag_uid_float = tag_uid:4;
const tag_uid:tag_uid_string = tag_uid:5;
const tag_uid:tag_uid_variant = tag_uid:6;
const tag_uid:tag_uid_list = tag_uid:7;
const tag_uid:tag_uid_map = tag_uid:8;
const tag_uid:tag_uid_iter = tag_uid:9;
const tag_uid:tag_uid_ref = tag_uid:10;
const tag_uid:tag_uid_task = tag_uid:11;
const tag_uid:tag_uid_var = tag_uid:12;
const tag_uid:tag_uid_linked_list = tag_uid:13;
const tag_uid:tag_uid_guard = tag_uid:14;
const tag_uid:tag_uid_callback_handler = tag_uid:15;
const tag_uid:tag_uid_native_hook = tag_uid:16;
const tag_uid:tag_uid_handle = tag_uid:17;
const tag_uid:tag_uid_symbol = tag_uid:18;
const tag_uid:tag_uid_signed = tag_uid:19;
const tag_uid:tag_uid_unsigned = tag_uid:20;
const tag_uid:tag_uid_pool = tag_uid:21;
const tag_uid:tag_uid_expression = tag_uid:22;
const tag_uid:tag_uid_address = tag_uid:23;

const TAG_EXPORTED = 0x80000000;
const TAG_STRONG = 0x40000000;

native tag_uid:tag_uid(tag_id);
native tag_id(tag_uid:tag_uid);
native tag_name(tag_uid:tag_uid, name[], size=sizeof(name));
native String:tag_name_s(tag_uid:tag_uid);
native tag_uid:tag_base(tag_uid:tag_uid);
native bool:tag_derived_from(tag_uid:tag_uid, tag_uid:base_uid);
native tag_uid:tag_find(const name[]);
native tag_uid:tag_new(const name[], tag_uid:base=tag_uid_unknown);
native bool:tag_set_op(tag_uid:tag_uid, tag_op:tag_op, const handler[], const additional_format[]="", AnyTag:...);
native tag_call_op(tag_uid:tag_uid, tag_op:tag_op, AnyTag:...);
native bool:tag_lock(tag_uid:tag_uid);
native tag_uid:tag_element(tag_uid:tag_uid);

enum tag_op
{
    tag_op_add = 1,
    tag_op_sub = 2,
    tag_op_mul = 3,
    tag_op_div = 4,
    tag_op_mod = 5,
    tag_op_neg = 6,
    tag_op_inc = 7,
    tag_op_dec = 8,
    
    tag_op_eq = 10,
    tag_op_neq = 11,
    tag_op_lt = 12,
    tag_op_gt = 13,
    tag_op_lte = 14,
    tag_op_gte = 15,
    tag_op_not = 16,
    
    tag_op_string = 20,
    tag_op_delete = 21,
    tag_op_release = 22,
    tag_op_collect = 23,
    tag_op_copy = 24,
    tag_op_clone = 25,
    tag_op_assign = 26,
    tag_op_init = 27,
    tag_op_hash = 28,
    tag_op_acquire = 29,
    tag_op_handle = 30,
}

/*                 */
/*       AMX       */
/*                 */

enum fork_level
{
    fork_exec = 0,
    fork_data = 1,
    fork_machine = 2,
}

const Amx:INVALID_AMX = Amx:0;
const Var:INVALID_AMX_VAR = Var:0;
const AmxGuard:INVALID_AMX_GUARD = AmxGuard:0;

const Amx:AMX_ALL = Amx:-1;
const Amx:AMX_OTHERS = Amx:-2;

native Amx:amx_this();
native Handle:amx_handle();
native Amx:amx_source();
native Handle:amx_source_handle();

native amx_name(name[], size=sizeof(name));
native String:amx_name_s();

native amx_call_native(Amx:amx, const function[], const format[], AnyTag:...);
native amx_call_public(Amx:amx, const function[], const format[], AnyTag:...);
native amx_err:amx_try_call_native(Amx:amx, const function[], &result, const format[], AnyTag:...);
native amx_err:amx_try_call_native_msg(Amx:amx, const function[], &result, msg[], msg_size=sizeof(msg), const format[]="", AnyTag:...);
native amx_err:amx_try_call_native_msg_s(Amx:amx, const function[], &result, &StringTag:msg, const format[], AnyTag:...);
native amx_err:amx_try_call_public(Amx:amx, const function[], &result, const format[], AnyTag:...);

native AmxGuard:amx_guard(AnyTag:value, TagTag:tag_id=tagof(value));
native AmxGuard:amx_guard_arr(AnyTag:value[], size=sizeof(value), TagTag:tag_id=tagof(value));
native bool:amx_guard_valid(AmxGuard:guard);
native amx_guard_free(AmxGuard:guard);

native amx_name_length();
native amx_num_publics();
native amx_public_index(const function[]);
native amx_public_name(index, name[], size=sizeof(name));
native String:amx_public_name_s(index);
native [2]amx_encode_public(index);
native [2]amx_encode_public_name(const function[]);
native [3]amx_encode_value_public(index, AnyTag:value);
native [3]amx_encode_value_public_name(const function[], AnyTag:value);
native amx_public_addr(index);
native amx_public_at(code=cellmin);
native amx_num_natives();
native amx_native_index(const function[]);
native amx_native_name(index, name[], size=sizeof(name));
native String:amx_native_name_s(index);
native [2]amx_encode_native(index);
native [2]amx_encode_native_name(const function[]);
native [3]amx_encode_value_native(index, AnyTag:value);
native [3]amx_encode_value_native_name(const function[], AnyTag:value);
native amx_encoded_length();
native bool:amx_try_decode_value(const encoded[], &AnyTag:value);

native Var:amx_var(&AnyTag:var);
native Var:amx_var_arr(AnyTag:arr[], size=sizeof(arr));
native Var:amx_public_var(const name[]);
native bool:amx_valid(Var:var);
native amx_delete(Var:var);
native bool:amx_linked(Var:var);
native bool:amx_inside(Var:var);
native Var:amx_alloc(size, bool:zero=true);
native bool:amx_free(Var:var);
native amx_set(Var:var, AnyTag:value, index=0);
native amx_get(Var:var, index=0);
native amx_sizeof(Var:var);
native bool:amx_my(Var:var);
native bool:amx_to_ref(Var:var, ref[1][]);

native bool:amx_fork(fork_level:level=fork_machine, &result=0, bool:use_data=true, &amx_err:error=amx_err:0);
native amx_commit(bool:context=true);
native amx_fork_end();
/*
native amx_yield(val=0);
*/
stock amx_yield(val=0)
{
    #emit stor.s.pri val
    #emit halt 0
    return 0;
}
native amx_error(amx_err:code, result=0);

native amx_parallel_begin(count=1);
native amx_parallel_end();

native bool:amx_tailcall();

enum amx_err
{
    amx_err_none = 0,
    amx_err_exit,
    amx_err_assert,
    amx_err_stackerr,
    amx_err_bounds,
    amx_err_memaccess,
    amx_err_invinstr,
    amx_err_stacklow,
    amx_err_heaplow,
    amx_err_callback,
    amx_err_native,
    amx_err_divide,
    amx_err_sleep,
    amx_err_invstate,
    
    amx_err_memory = 16,
    amx_err_format,
    amx_err_version,
    amx_err_notfound,
    amx_err_index,
    amx_err_debug,
    amx_err_init,
    amx_err_userdata,
    amx_err_init_jit,
    amx_err_params,
    amx_err_domain,
    amx_err_general,
}

#if defined PP_SYNTAX_AMX_FORKED

#define amx_forked(%0) for(new PP@af:_@pp_amx_forked=PP@af:amx_fork(%0);_@pp_amx_forked;_@pp_amx_forked=PP@af:amx_fork_end())
stock operator~(PP@af:arr[], count)
{
    for(new i = 0; i < count; i++)
    {
        if(arr[i]) amx_fork_end();
    }
}

#endif

#if defined PP_SYNTAX_AMX_PARALLEL

#define amx_parallel(%0) for(new PP@ap:_@pp_amx_parallel=PP@ap:amx_parallel_begin(%0);_@pp_amx_parallel;_@pp_amx_parallel=PP@ap:amx_parallel_end())
stock operator~(PP@ap:arr[], count)
{
    for(new i = 0; i < count; i++)
    {
        if(arr[i]) amx_parallel_end();
    }
}

#endif


/*                 */
/*     Strings     */
/*                 */

const String:STRING_NULL = String:0;
const String:INVALID_STRING = String:-1;
const INVALID_CHAR = 0xFFFFFF00;

enum str_create_mode
{
    str_preserve = 0,
    str_truncate = 1,
    str_no_nulls = 2
}

native String:str_new(const str[], str_create_mode:mode=str_preserve);
native String:str_new_arr(const arr[], size=sizeof(arr), str_create_mode:mode=str_preserve);
native String:str_new_static(const str[], str_create_mode:mode=str_preserve, size=sizeof(str));
native String:str_new_buf(size);
native AmxString:str_addr(StringTag:str);
native ConstAmxString:str_addr_const(ConstStringTag:str);
native AmxStringBuffer:str_buf_addr(StringTag:str);
native String:str_acquire(StringTag:str);
native String:str_release(StringTag:str);
native str_delete(StringTag:str);
native bool:str_valid(ConstStringTag:str);
native String:str_clone(ConstStringTag:str);

native str_len(ConstStringTag:str);
native str_get(ConstStringTag:str, buffer[], size=sizeof(buffer), start=0, end=cellmax);
native str_getc(ConstStringTag:str, pos);
native str_setc(StringTag:str, pos, value);
native str_cmp(ConstStringTag:str1, ConstStringTag:str2);
native bool:str_empty(ConstStringTag:str);
native bool:str_eq(ConstStringTag:str1, ConstStringTag:str2);
native str_findc(ConstStringTag:str, value, offset=0);
native str_find(ConstStringTag:str, ConstStringTag:value, offset=0);

native String:str_cat(ConstStringTag:str1, ConstStringTag:str2);
native String:str_sub(ConstStringTag:str, start=0, end=cellmax);
native String:str_val(AnyTag:value, TagTag:tag_id=tagof(value));
native String:str_val_arr(const AnyTag:value[], size=sizeof(value), TagTag:tag_id=tagof(value));
native String:str_val_var(ConstVariantTag:value);
native List:str_split(ConstStringTag:str, const delims[]);
native List:str_split_s(ConstStringTag:str, ConstStringTag:delims);
native String:str_join(List:list, const delim[]);
native String:str_join_s(List:list, ConstStringTag:delim);
native String:str_to_lower(ConstStringTag:str);
native String:str_to_upper(ConstStringTag:str);

native String:str_set(StringTag:target, StringTag:other);
native String:str_append(StringTag:target, StringTag:other);
native String:str_ins(StringTag:target, StringTag:other, pos);
native String:str_del(StringTag:target, start=0, end=cellmax);
native String:str_clear(StringTag:str);
native String:str_resize(StringTag:str, size, padding=0);
native String:str_set_to_lower(StringTag:str);
native String:str_set_to_upper(StringTag:str);

native String:str_format(const format[], AnyTag:...);
native String:str_format_s(ConstStringTag:format, AnyTag:...);
native String:str_set_format(StringTag:target, const format[], AnyTag:...);
native String:str_set_format_s(StringTag:target, ConstStringTag:format, AnyTag:...);
native String:str_append_format(StringTag:target, const format[], AnyTag:...);
native String:str_append_format_s(StringTag:target, ConstStringTag:format, AnyTag:...);

enum regex_options (<<= 1)
{
    regex_default = 0,
    regex_basic = 1,
    regex_extended = 2,
    regex_awk = 3,
    regex_grep = 4,
    regex_egrep = 5,
    
    regex_icase = 8,
    regex_nosubs,
    regex_optimize,
    regex_collate,
    
    regex_not_bol = 256,
    regex_not_eol,
    regex_not_bow,
    regex_not_eow,
    regex_any,
    regex_not_null,
    regex_continuous,
    
    regex_no_copy = 65536,
    regex_first_only,
    
    regex_cached = 4194304,
    regex_cached_addr = 4194304 | 8388608
}

native bool:str_match(ConstStringTag:str, const pattern[], &pos=0, regex_options:options=regex_default);
native bool:str_match_s(ConstStringTag:str, ConstStringTag:pattern, &pos=0, regex_options:options=regex_default);
native List:str_extract(ConstStringTag:str, const pattern[], &pos=0, regex_options:options=regex_default);
native List:str_extract_s(ConstStringTag:str, ConstStringTag:pattern, &pos=0, regex_options:options=regex_default);
native String:str_replace(ConstStringTag:str, const pattern[], const replacement[], &pos=0, regex_options:options=regex_default);
native String:str_replace_s(ConstStringTag:str, ConstStringTag:pattern, ConstStringTag:replacement, &pos=0, regex_options:options=regex_default);
native String:str_replace_list(ConstStringTag:str, const pattern[], List:replacement, &pos=0, regex_options:options=regex_default);
native String:str_replace_list_s(ConstStringTag:str, ConstStringTag:pattern, List:replacement, &pos=0, regex_options:options=regex_default);
native String:str_replace_func(ConstStringTag:str, const pattern[], const function[], &pos=0, regex_options:options=regex_default, const additional_format[]="", AnyTag:...);
native String:str_replace_func_s(ConstStringTag:str, ConstStringTag:pattern, const function[], &pos=0, regex_options:options=regex_default, const additional_format[]="", AnyTag:...);
native String:str_replace_expr(ConstStringTag:str, const pattern[], Expression:expr, &pos=0, regex_options:options=regex_default);
native String:str_replace_expr_s(ConstStringTag:str, ConstStringTag:pattern, Expression:expr, &pos=0, regex_options:options=regex_default);
native String:str_set_replace(StringTag:target, ConstStringTag:str, const pattern[], const replacement[], &pos=0, regex_options:options=regex_default);
native String:str_set_replace_s(StringTag:target, ConstStringTag:str, ConstStringTag:pattern, ConstStringTag:replacement, &pos=0, regex_options:options=regex_default);
native String:str_set_replace_list(StringTag:target, ConstStringTag:str, const pattern[], List:replacement, &pos=0, regex_options:options=regex_default);
native String:str_set_replace_list_s(StringTag:target, ConstStringTag:str, ConstStringTag:pattern, List:replacement, &pos=0, regex_options:options=regex_default);
native String:str_set_replace_func(StringTag:target, ConstStringTag:str, const pattern[], const function[], &pos=0, regex_options:options=regex_default, const additional_format[]="", AnyTag:...);
native String:str_set_replace_func_s(StringTag:target, ConstStringTag:str, ConstStringTag:pattern, const function[], &pos=0, regex_options:options=regex_default, const additional_format[]="", AnyTag:...);
native String:str_set_replace_expr(StringTag:target, ConstStringTag:str, const pattern[], Expression:expr, &pos=0, regex_options:options=regex_default);
native String:str_set_replace_expr_s(StringTag:target, ConstStringTag:str, ConstStringTag:pattern, Expression:expr, &pos=0, regex_options:options=regex_default);

#if defined PP_SYNTAX_@
#define @ str_new_static
#endif

#if defined PP_SYNTAX_@@
#define @@ str_val
#endif


/*                 */
/*     Variant     */
/*                 */

const Variant:VAR_NULL = Variant:0;
const Variant:INVALID_VARIANT = Variant:-1;

native Variant:var_new(AnyTag:value, TagTag:tag_id=tagof(value));
native Variant:var_new_arr(const AnyTag:value[], size=sizeof(value), TagTag:tag_id=tagof(value));
native Variant:var_new_arr_2d(const AnyTag:value[][], size=sizeof(value), size2=sizeof(value[]), TagTag:tag_id=tagof(value));
native Variant:var_new_arr_3d(const AnyTag:value[][][], size=sizeof(value), size2=sizeof(value[]), size3=sizeof(value[][]), TagTag:tag_id=tagof(value));
native Variant:var_new_buf(size, TagTag:tag_id=0);
native Variant:var_new_str(const value[]);
native Variant:var_new_str_s(ConstStringTag:value);
native Variant:var_new_var(ConstVariantTag:value);
native AmxVariant:var_addr(VariantTag:var);
native ConstAmxVariant:var_addr_const(ConstVariantTag:var);
native AmxVariantBuffer:var_buf_addr(VariantTag:str);
native Variant:var_acquire(VariantTag:var);
native Variant:var_release(VariantTag:var);
native var_delete(VariantTag:var);
native bool:var_valid(ConstVariantTag:var);
native Variant:var_clone(ConstVariantTag:var);

native var_get(ConstVariantTag:var, offset=0);
native var_get_arr(ConstVariantTag:var, AnyTag:value[], size=sizeof(value));
native var_get_str(ConstVariantTag:var, value[], size=sizeof(value)) = var_get_arr;
native String:var_get_str_s(ConstVariantTag:var);
native bool:var_get_safe(ConstVariantTag:var, &AnyTag:value, offset=0, TagTag:tag_id=tagof(value));
native var_get_arr_safe(ConstVariantTag:var, AnyTag:value[], size=sizeof(value), TagTag:tag_id=tagof(value));
native var_get_str_safe(ConstVariantTag:var, value[], size=sizeof(value));
native String:var_get_str_safe_s(ConstVariantTag:var);

native var_set_cell(VariantTag:var, offset, AnyTag:value);
native bool:var_set_cell_safe(VariantTag:var, offset, AnyTag:value, TagTag:tag_id=tagof(value));
native var_set_cells(VariantTag:var, offset, AnyTag:values[], size=sizeof(values));
native var_set_cells_safe(VariantTag:var, offset, AnyTag:values[], size=sizeof(values), TagTag:tag_id=tagof(values));

native var_get_md(ConstVariantTag:var, const offsets[], offsets_size=sizeof(offsets));
native var_get_md_arr(ConstVariantTag:var, const offsets[], AnyTag:value[], size=sizeof(value), offsets_size=sizeof(offsets));
native var_get_md_str(ConstVariantTag:var, const offsets[], value[], size=sizeof(value), offsets_size=sizeof(offsets)) = var_get_md_arr;
native String:var_get_md_str_s(ConstVariantTag:var, const offsets[], offsets_size=sizeof(offsets));
native bool:var_get_md_safe(ConstVariantTag:var, const offsets[], &AnyTag:value, offsets_size=sizeof(offsets), TagTag:tag_id=tagof(value));
native var_get_md_arr_safe(ConstVariantTag:var, const offsets[], AnyTag:value[], size=sizeof(value), offsets_size=sizeof(offsets), TagTag:tag_id=tagof(value));
native var_get_md_str_safe(ConstVariantTag:var, const offsets[], value[], size=sizeof(value), offsets_size=sizeof(offsets));
native String:var_get_md_str_safe_s(ConstVariantTag:var, const offsets[], offsets_size=sizeof(offsets));

native var_set_cell_md(VariantTag:var, const offsets[], AnyTag:value, offsets_size=sizeof(offsets));
native bool:var_set_cell_md_safe(VariantTag:var, const offsets[], AnyTag:value, offsets_size=sizeof(offsets), TagTag:tag_id=tagof(value));
native var_set_cells_md(VariantTag:var, const offsets[], AnyTag:values[], offsets_size=sizeof(offsets), size=sizeof(values));
native var_set_cells_md_safe(VariantTag:var, const offsets[], AnyTag:values[], offsets_size=sizeof(offsets), size=sizeof(values), TagTag:tag_id=tagof(values));

native var_tagof(ConstVariantTag:var);
native tag_uid:var_tag_uid(ConstVariantTag:var);
native var_sizeof(ConstVariantTag:var);
native var_sizeof_md(ConstVariantTag:var, const offsets[], offsets_size=sizeof(offsets));
native var_rank(ConstVariantTag:var);
native Variant:var_add(ConstVariantTag:var1, ConstVariantTag:var2);
native Variant:var_sub(ConstVariantTag:var1, ConstVariantTag:var2);
native Variant:var_mul(ConstVariantTag:var1, ConstVariantTag:var2);
native Variant:var_div(ConstVariantTag:var1, ConstVariantTag:var2);
native Variant:var_mod(ConstVariantTag:var1, ConstVariantTag:var2);
native Variant:var_neg(ConstVariantTag:var);
native Variant:var_inc(ConstVariantTag:var);
native Variant:var_dec(ConstVariantTag:var);
native bool:var_eq(ConstVariantTag:var1, ConstVariantTag:var2);
native bool:var_neq(ConstVariantTag:var1, ConstVariantTag:var2);
native bool:var_lt(ConstVariantTag:var1, ConstVariantTag:var2);
native bool:var_gt(ConstVariantTag:var1, ConstVariantTag:var2);
native bool:var_lte(ConstVariantTag:var1, ConstVariantTag:var2);
native bool:var_gte(ConstVariantTag:var1, ConstVariantTag:var2);
native bool:var_not(ConstVariantTag:var);
native Variant:var_call_op(VariantTag:var, tag_op:tag_op, AnyTag:...);
native Variant:var_call_op_raw(VariantTag:var, tag_op:tag_op, AnyTag:...);

native Iter:var_iter(VariantTag:value, count=1);


/*                 */
/*      Lists      */
/*                 */

const List:INVALID_LIST = List:0;

native List:list_new();
native List:list_new_arr(AnyTag:values[], size=sizeof(values), TagTag:tag_id=tagof(values));
native List:list_new_args_t(TagTag:tag_id=tagof(arg0), AnyTag:arg0, AnyTag:...) = list_new_args;
native List:list_new_args_packed(ArgTag:...);
/*
native List:list_new_args(AnyTag:...);
*/
#define list_new_args(%0) list_new_args_packed(_PP@TAGGED_PACK(%0))
native List:list_new_args_str(arg0[], ...);
native List:list_new_args_var(ConstVariantTag:arg0, ConstVariantTag:...);
native bool:list_valid(List:list);
native list_delete(List:list);
native list_delete_deep(List:list);
native list_size(List:list);
native list_capacity(List:list);
native list_reserve(List:list, capacity);
native list_clear(List:list);
native list_clear_deep(List:list);
native List:list_clone(List:list);

native list_add(List:list, AnyTag:value, index=-1, TagTag:tag_id=tagof(value));
native list_add_arr(List:list, const AnyTag:value[], index=-1, size=sizeof(value), TagTag:tag_id=tagof(value));
native list_add_str(List:list, const value[], index=-1);
native list_add_var(List:list, ConstVariantTag:value, index=-1);
native list_add_list(List:list, List:range, index=-1);
native list_add_iter(List:list, Iter:iter, index=-1);
native list_add_args_t(TagTag:tag_id=tagof(arg0), List:list, AnyTag:arg0, AnyTag:...) = list_add_args;
native list_add_args_packed(List:list, ArgTag:...);
/*
native List:list_add_args(List:list, AnyTag:...);
*/
#define list_add_args(%0,%1) list_add_args_packed(%0,_PP@TAGGED_PACK(%1))
native list_add_args_str(List:list, arg0[], ...);
native list_add_args_var(List:list, ConstVariantTag:arg0, ConstVariantTag:...);

native list_remove(List:list, index);
native list_remove_deep(List:list, index);
native list_remove_range(List:list, begin, end);
native list_remove_range_deep(List:list, begin, end);
native list_remove_if(List:list, Expression:pred);
native list_remove_if_deep(List:list, Expression:pred);

native list_get(List:list, index, offset=0);
native list_get_arr(List:list, index, AnyTag:value[], size=sizeof(value));
native list_get_str(List:list, index, value[], size=sizeof(value)) = list_get_arr;
native String:list_get_str_s(List:list, index);
native Variant:list_get_var(List:list, index);
native bool:list_get_safe(List:list, index, &AnyTag:value, offset=0, TagTag:tag_id=tagof(value));
native list_get_arr_safe(List:list, index, AnyTag:value[], size=sizeof(value), TagTag:tag_id=tagof(value));
native list_get_str_safe(List:list, index, value[], size=sizeof(value));
native String:list_get_str_safe_s(List:list, index);

native list_set(List:list, index, AnyTag:value, TagTag:tag_id=tagof(value));
native list_set_arr(List:list, index, const AnyTag:value[], size=sizeof(value), TagTag:tag_id=tagof(value));
native list_set_str(List:list, index, const value[]);
native list_set_var(List:list, index, ConstVariantTag:value);
native list_set_cell(List:list, index, offset, AnyTag:value);
native bool:list_set_cell_safe(List:list, index, offset, AnyTag:value, TagTag:tag_id=tagof(value));

native list_resize(List:list, newsize, AnyTag:padding, TagTag:tag_id=tagof(padding));
native list_resize_arr(List:list, newsize, const AnyTag:padding[], size=sizeof(padding), TagTag:tag_id=tagof(padding));
native list_resize_str(List:list, newsize, const padding[]);
native list_resize_var(List:list, newsize, ConstVariantTag:padding);

native list_find(List:list, AnyTag:value, index=0, TagTag:tag_id=tagof(value));
native list_find_arr(List:list, const AnyTag:value[], index=0, size=sizeof(value), TagTag:tag_id=tagof(value));
native list_find_str(List:list, const value[], index=0);
native list_find_var(List:list, ConstVariantTag:value, index=0);
native list_find_last(List:list, AnyTag:value, index=-1, TagTag:tag_id=tagof(value));
native list_find_last_arr(List:list, const AnyTag:value[], index=-1, size=sizeof(value), TagTag:tag_id=tagof(value));
native list_find_last_str(List:list, const value[], index=-1);
native list_find_last_var(List:list, ConstVariantTag:value, index=-1);
native list_find_if(List:list, Expression:pred, index=0);
native list_find_last_if(List:list, Expression:pred, index=-1);

native list_count(List:list, AnyTag:value, TagTag:tag_id=tagof(value));
native list_count_arr(List:list, const AnyTag:value[], size=sizeof(value), TagTag:tag_id=tagof(value));
native list_count_str(List:list, const value[]);
native list_count_var(List:list, ConstVariantTag:value);
native list_count_if(List:list, Expression:pred);

native list_sort(List:list, offset=0, size=-1, bool:reverse=false, bool:stable=true);
native list_sort_expr(List:list, Expression:expr, bool:reverse=false, bool:stable=true);

native list_tagof(List:list, index);
native list_sizeof(List:list, index);

native Iter:list_iter(List:list, index=0);

#if defined PP_SYNTAX_GENERIC

#define list_new<%0>(%1) (List<%0>:list_new(%1))
#define list_new_arr<%0>(%1) (List<%0>:list_new_arr(_PP@CAST_ARR[%0](%1)))
#define list_new_args_of<%0>(%1) (List<%0>:list_new_args_t(_,_PP@CAST[%0](%1)))
#define list_valid<%0>(%1) list_valid(List:_PP@CAST[List<%0>](%1))
#define list_delete<%0>(%1) list_delete(List:_PP@CAST[List<%0>](%1))
#define list_delete_deep<%0>(%1) list_delete_deep(List:_PP@CAST[List<%0>](%1))
#define list_clone<%0>(%1) (List<%0>:list_clone(List:_PP@CAST[List<%0>](%1)))
#define list_size<%0>(%1) list_size(List:_PP@CAST[List<%0>](%1))
#define list_capacity<%0>(%1) list_capacity(List:_PP@CAST[List<%0>](%1))
#define list_reserve<%0>(%1) list_reserve(List:_PP@CAST[List<%0>](%1))
#define list_clear<%0>(%1) list_clear(List:_PP@CAST[List<%0>](%1))

#define list_add<%0>(%1,%2) list_add(List:_PP@CAST[List<%0>](%1),_PP@CAST[%0](%2))
#define list_add_arr<%0>(%1,%2) list_add_arr(List:_PP@CAST[List<%0>](%1),_PP@CAST_ARR[%0](%2))
#define list_add_list<%0>(%1,%2) list_add_list(List:_PP@CAST[List<%0>](%1),List:_PP@CAST[List<%0>](%2))
#define list_add_iter<%0>(%1,%2) list_add_iter(List:_PP@CAST[List<%0>](%1),Iter:_PP@CAST[Iter<%0>](%2))
#define list_add_args_of<%0>(%1,%2) list_add_args_t(_,List:_PP@CAST[List<%0>](%1),_PP@CAST[%0](%2))

#define list_remove<%0>(%1,%2) list_remove(List:_PP@CAST[List<%0>](%1),%2)
#define list_remove_deep<%0>(%1,%2) list_remove_deep(List:_PP@CAST[List<%0>](%1),%2)
#define list_remove_range<%0>(%1,%2) list_remove_range(List:_PP@CAST[List<%0>](%1),%2)
#define list_remove_range_deep<%0>(%1,%2) list_remove_range_deep(List:_PP@CAST[List<%0>](%1),%2)

#define list_get<%0>(%1,%2) (%0:list_get(List:_PP@CAST[List<%0>](%1),%2))
#define list_get_arr<%0>(%1,%2,%3) list_get_arr(List:_PP@CAST[List<%0>](%1),%2,_PP@CAST_ARR[%0](%3))

#define list_set<%0>(%1,%2,%3) list_set(List:_PP@CAST[List<%0>](%1),%2,_PP@CAST[%0](%3))
#define list_set_arr<%0>(%1,%2,%3) list_set_arr(List:_PP@CAST[List<%0>](%1),%2,_PP@CAST_ARR[%0](%3))
#define list_set_cell<%0>(%1,%2,%3,%4) list_set_cell(List:_PP@CAST[List<%0>](%1),%2,%3,_PP@CAST[%0](%4))

#define list_resize<%0>(%1,%2,%3) list_resize(List:_PP@CAST[List<%0>](%1),%2,_PP@CAST[%0](%3))
#define list_resize_arr<%0>(%1,%2,%3) list_resize_arr(List:_PP@CAST[List<%0>](%1),%2,_PP@CAST[%0](%3))

#define list_find<%0>(%1,%2) list_find(List:_PP@CAST[List<%0>](%1),_PP@CAST[%0](%2))
#define list_find_arr<%0>(%1,%2) list_find_arr(List:_PP@CAST[List<%0>](%1),_PP@CAST_ARR[%0](%2))

#define list_count<%0>(%1,%2) list_find(List:_PP@CAST[List<%0>](%1),_PP@CAST[%0](%2))
#define list_count_arr<%0>(%1,%2) list_find_arr(List:_PP@CAST[List<%0>](%1),_PP@CAST_ARR[%0](%2))

#define list_sort<%0>(%1) list_sort(List:_PP@CAST[List<%0>](%1))

#define list_sizeof<%0>(%1) list_sizeof(List:_PP@CAST[List<%0>](%1))

#define list_iter<%0>(%1) (Iter<%0>:list_iter(List:_PP@CAST[List<%0>](%1)))

#endif


/*                 */
/*       Maps      */
/*                 */

const Map:INVALID_MAP = Map:0;

native Map:map_new(bool:ordered=false);
native Map:map_new_args_t(TagTag:key_tag_id=tagof(arg0), TagTag:value_tag_id=tagof(arg1), AnyTag:arg0, AnyTag:arg1, AnyTag:...) = map_new_args;
native Map:map_new_args_packed(ArgTag:...);
/*
native Map:map_new_args(AnyTag:...);
*/
#define map_new_args(%0) map_new_args_packed(_PP@TAGGED_PACK(%0))
native Map:map_new_args_str_t(TagTag:key_tag_id=tagof(arg0), AnyTag:arg0, arg1[], AnyTag:...) = map_new_args_str;
native Map:map_new_args_str_packed({ArgTag,_}:...);
/*
native Map:map_new_str({AnyTag,_}:...);
*/
#define map_new_args_str(%0) map_new_args_str_packed(_PP@TAGGED1_PACK(%0))
native Map:map_new_args_var_t(TagTag:key_tag_id=tagof(arg0), AnyTag:arg0, ConstVariantTag:arg1, AnyTag:...) = map_new_args_var;
native Map:map_new_args_var_packed({ArgTag,ConstVariantTags}:...);
/*
native Map:map_new_args_var({AnyTag,ConstVariantTags}:...);
*/
#define map_new_args_var(%0) map_new_args_var_packed(_PP@TAGGED1_PACK(%0))
native Map:map_new_str_args_t(TagTag:value_tag_id=tagof(arg1), arg0[], AnyTag:arg1, AnyTag:...) = map_new_str_args;
native Map:map_new_str_args_packed({_,ArgTag}:...);
/*
native Map:map_new_str_args({_,AnyTag}:...);
*/
#define map_new_str_args(%0) map_new_str_args_packed(_PP@TAGGED2_PACK(%0))
native Map:map_new_str_args_str(arg0[], arg1[], ...);
native Map:map_new_str_args_var(arg0[], ConstVariantTag:arg1, {_,VariantTags}:...);
native Map:map_new_var_args_t(TagTag:value_tag_id=tagof(arg1), ConstVariantTag:arg0, AnyTag:arg1, AnyTag:...) = map_new_var_args;
native Map:map_new_var_args_packed({ConstVariantTags,ArgTag}:...);
/*
native Map:map_new_var_args({ConstVariantTags,AnyTag}:...);
*/
#define map_new_var_args(%0) map_new_var_args_packed(_PP@TAGGED2_PACK(%0))
native Map:map_new_var_args_str(ConstVariantTag:arg0, arg1[], {_,VariantTags}:...);
native Map:map_new_var_args_var(ConstVariantTag:arg0, ConstVariantTag:arg1, ConstVariantTag:...);
native bool:map_valid(Map:map);
native map_delete(Map:map);
native map_delete_deep(Map:map);
native Map:map_clone(Map:map);
native map_size(Map:map);
native map_capacity(Map:map);
native map_reserve(Map:map, capacity);
native map_clear(Map:map);
native map_clear_deep(Map:map);
native map_set_ordered(Map:map, bool:ordered);
native bool:map_is_ordered(Map:map);

native bool:map_add(Map:map, AnyTag:key, AnyTag:value, TagTag:key_tag_id=tagof(key), TagTag:value_tag_id=tagof(value));
native bool:map_add_arr(Map:map, AnyTag:key, const AnyTag:value[], value_size=sizeof(value), TagTag:key_tag_id=tagof(key), TagTag:value_tag_id=tagof(value));
native bool:map_add_str(Map:map, AnyTag:key, const value[], TagTag:key_tag_id=tagof(key));
native bool:map_add_var(Map:map, AnyTag:key, ConstVariantTag:value, TagTag:key_tag_id=tagof(key));
native bool:map_arr_add(Map:map, const AnyTag:key[], AnyTag:value, key_size=sizeof(key), TagTag:key_tag_id=tagof(key), TagTag:value_tag_id=tagof(value));
native bool:map_arr_add_arr(Map:map, const AnyTag:key[], const AnyTag:value[], key_size=sizeof(key), value_size=sizeof(value), TagTag:key_tag_id=tagof(key), TagTag:value_tag_id=tagof(value));
native bool:map_arr_add_str(Map:map, const AnyTag:key[], const value[], key_size=sizeof(key), TagTag:key_tag_id=tagof(key));
native bool:map_arr_add_var(Map:map, const AnyTag:key[], ConstVariantTag:value, key_size=sizeof(key), TagTag:key_tag_id=tagof(key));
native bool:map_str_add(Map:map, const key[], AnyTag:value, TagTag:value_tag_id=tagof(value));
native bool:map_str_add_arr(Map:map, const key[], const AnyTag:value[], value_size=sizeof(value), TagTag:value_tag_id=tagof(value));
native bool:map_str_add_str(Map:map, const key[], const value[]);
native bool:map_str_add_var(Map:map, const key[], ConstVariantTag:value);
native bool:map_var_add(Map:map, ConstVariantTag:key, AnyTag:value, TagTag:value_tag_id=tagof(value));
native bool:map_var_add_arr(Map:map, ConstVariantTag:key, const AnyTag:value[], value_size=sizeof(value), TagTag:value_tag_id=tagof(value));
native bool:map_var_add_str(Map:map, ConstVariantTag:key, const value[]);
native bool:map_var_add_var(Map:map, ConstVariantTag:key, ConstVariantTag:value);
native map_add_map(Map:map, Map:other, bool:overwrite);
native map_add_args_t(TagTag:key_tag_id=tagof(arg0), TagTag:value_tag_id=tagof(arg1), Map:map, AnyTag:arg0, AnyTag:arg1, AnyTag:...) = map_add_args;
native map_add_args_packed(Map:map, ArgTag:...);
/*
native map_add_args(Map:map, AnyTag:...);
*/
#define map_add_args(%0,%1) map_add_args_packed(%0,_PP@TAGGED_PACK(%1))
native map_add_args_str_t(TagTag:key_tag_id=tagof(arg0), Map:map, AnyTag:arg0, arg1[], AnyTag:...) = map_add_args_str;
native map_add_args_str_packed(Map:map, {ArgTag,_}:...);
/*
native map_add_args_str(Map:map, AnyTag:...);
*/
#define map_add_args_str(%0,%1) map_add_args_str_packed(%0,_PP@TAGGED1_PACK(%1))
native map_add_args_var_t(TagTag:key_tag_id=tagof(arg0), Map:map, AnyTag:arg0, ConstVariantTag:arg1, AnyTag:...) = map_add_args_var;
native map_add_args_var_packed(Map:map, {ArgTag,ConstVariantTags}:...);
/*
native map_add_args_var(Map:map, {AnyTag,ConstVariantTags}:...);
*/
#define map_add_args_var(%0,%1) map_add_args_var_packed(%0,_PP@TAGGED1_PACK(%1))
native map_add_str_args_t(TagTag:value_tag_id=tagof(arg1), Map:map, arg0[], AnyTag:arg1, AnyTag:...) = map_add_str_args;
native map_add_str_args_packed(Map:map, {_,ArgTag}:...);
/*
native map_add_str_args(Map:map, {_,AnyTag}:...);
*/
#define map_add_str_args(%0,%1) map_add_str_args_packed(%0,_PP@TAGGED2_PACK(%1))
native map_add_str_args_str(Map:map, arg0[], arg1[], ...);
native map_add_str_args_var(Map:map, arg0[], ConstVariantTag:arg1, {_,VariantTags}:...);
native map_add_var_args_t(TagTag:value_tag_id=tagof(arg1), Map:map, ConstVariantTag:arg0, AnyTag:arg1, AnyTag:...) = map_add_var_args;
native map_add_var_args_packed(Map:map, {ConstVariantTags,ArgTag}:...);
/*
native map_add_var_args(Map:map, {ConstVariantTags,AnyTag}:...);
*/
#define map_add_var_args(%0,%1) map_add_var_args_packed(%0,_PP@TAGGED2_PACK(%1))
native map_add_var_args_str(Map:map, ConstVariantTag:arg0, arg1[], {_,VariantTags}:...);
native map_add_var_args_var(Map:map, ConstVariantTag:arg0, ConstVariantTag:arg1, ConstVariantTag:...);

native bool:map_remove(Map:map, AnyTag:key, TagTag:key_tag_id=tagof(key));
native bool:map_arr_remove(Map:map, const AnyTag:key[], key_size=sizeof(key), TagTag:key_tag_id=tagof(key));
native bool:map_str_remove(Map:map, const key[]);
native bool:map_var_remove(Map:map, ConstVariantTag:key);
native bool:map_remove_deep(Map:map, AnyTag:key, TagTag:key_tag_id=tagof(key));
native bool:map_arr_remove_deep(Map:map, const AnyTag:key[], key_size=sizeof(key), TagTag:key_tag_id=tagof(key));
native bool:map_str_remove_deep(Map:map, const key[]);
native bool:map_var_remove_deep(Map:map, ConstVariantTag:key);
native map_remove_if(Map:map, Expression:pred);
native map_remove_if_deep(Map:map, Expression:pred);

native bool:map_has_key(Map:map, AnyTag:key, TagTag:key_tag_id=tagof(key));
native bool:map_has_arr_key(Map:map, const AnyTag:key[], key_size=sizeof(key), TagTag:key_tag_id=tagof(key));
native bool:map_has_str_key(Map:map, const key[]);
native bool:map_has_var_key(Map:map, ConstVariantTag:key);

native map_get(Map:map, AnyTag:key, offset=0, TagTag:key_tag_id=tagof(key));
native map_get_arr(Map:map, AnyTag:key, AnyTag:value[], value_size=sizeof(value), TagTag:key_tag_id=tagof(key));
native map_get_str(Map:map, AnyTag:key, value[], value_size=sizeof(value), TagTag:key_tag_id=tagof(key)) = map_get_arr;
native String:map_get_str_s(Map:map, AnyTag:key, TagTag:key_tag_id=tagof(key));
native Variant:map_get_var(Map:map, AnyTag:key, TagTag:key_tag_id=tagof(key));
native bool:map_get_safe(Map:map, AnyTag:key, &AnyTag:value, offset=0, TagTag:key_tag_id=tagof(key), TagTag:value_tag_id=tagof(value));
native map_get_arr_safe(Map:map, AnyTag:key, AnyTag:value[], value_size=sizeof(value), TagTag:key_tag_id=tagof(key), TagTag:value_tag_id=tagof(value));
native map_get_str_safe(Map:map, AnyTag:key, value[], value_size=sizeof(value), TagTag:key_tag_id=tagof(key));
native String:map_get_str_safe_s(Map:map, AnyTag:key, TagTag:key_tag_id=tagof(key));
native map_arr_get(Map:map, const AnyTag:key[], offset=0, key_size=sizeof(key), TagTag:key_tag_id=tagof(key));
native map_arr_get_arr(Map:map, const AnyTag:key[], AnyTag:value[], value_size=sizeof(value), key_size=sizeof(key), TagTag:key_tag_id=tagof(key));
native map_arr_get_str(Map:map, const AnyTag:key[], AnyTag:value[], value_size=sizeof(value), key_size=sizeof(key), TagTag:key_tag_id=tagof(key)) = map_arr_get_arr;
native String:map_arr_get_str_s(Map:map, const AnyTag:key[], key_size=sizeof(key), TagTag:key_tag_id=tagof(key));
native Variant:map_arr_get_var(Map:map, const AnyTag:key[], key_size=sizeof(key), TagTag:key_tag_id=tagof(key));
native bool:map_arr_get_safe(Map:map, const AnyTag:key[], &AnyTag:value, offset=0, key_size=sizeof(key), TagTag:key_tag_id=tagof(key), TagTag:value_tag_id=tagof(value));
native map_arr_get_arr_safe(Map:map, const AnyTag:key[], AnyTag:value[], value_size=sizeof(value), key_size=sizeof(key), TagTag:key_tag_id=tagof(key), TagTag:value_tag_id=tagof(value));
native map_arr_get_str_safe(Map:map, const AnyTag:key[], value[], value_size=sizeof(value), key_size=sizeof(key), TagTag:key_tag_id=tagof(key));
native String:map_arr_get_str_safe_s(Map:map, const AnyTag:key[], key_size=sizeof(key), TagTag:key_tag_id=tagof(key));
native map_str_get(Map:map, const key[], offset=0);
native map_str_get_arr(Map:map, const key[], AnyTag:value[], value_size=sizeof(value));
native map_str_get_str(Map:map, const key[], value[], value_size=sizeof(value)) = map_str_get_arr;
native String:map_str_get_str_s(Map:map, const key[]);
native Variant:map_str_get_var(Map:map, const key[]);
native bool:map_str_get_safe(Map:map, const key[], &AnyTag:value, offset=0, TagTag:value_tag_id=tagof(value));
native map_str_get_arr_safe(Map:map, const key[], AnyTag:value[], value_size=sizeof(value), TagTag:value_tag_id=tagof(value));
native map_str_get_str_safe(Map:map, const key[], value[], value_size=sizeof(value));
native String:map_str_get_str_safe_s(Map:map, const key[]);
native map_var_get(Map:map, ConstVariantTag:key, offset=0);
native map_var_get_arr(Map:map, ConstVariantTag:key, AnyTag:value[], value_size=sizeof(value));
native map_var_get_str(Map:map, ConstVariantTag:key, AnyTag:value[], value_size=sizeof(value)) = map_var_get_arr;
native String:map_var_get_str_s(Map:map, ConstVariantTag:key);
native Variant:map_var_get_var(Map:map, ConstVariantTag:key);
native bool:map_var_get_safe(Map:map, ConstVariantTag:key, &AnyTag:value, offset=0, TagTag:value_tag_id=tagof(value));
native map_var_get_arr_safe(Map:map, ConstVariantTag:key, AnyTag:value[], value_size=sizeof(value), TagTag:value_tag_id=tagof(value));
native map_var_get_str_safe(Map:map, ConstVariantTag:key, value[], value_size=sizeof(value));
native String:map_var_get_str_safe_s(Map:map, ConstVariantTag:key);

native map_set(Map:map, AnyTag:key, AnyTag:value, TagTag:key_tag_id=tagof(key), TagTag:value_tag_id=tagof(value));
native map_set_arr(Map:map, AnyTag:key, const AnyTag:value[], value_size=sizeof(value), TagTag:key_tag_id=tagof(key), TagTag:value_tag_id=tagof(value));
native map_set_str(Map:map, AnyTag:key, const value[], TagTag:key_tag_id=tagof(key));
native map_set_var(Map:map, AnyTag:key, ConstVariantTag:value, TagTag:key_tag_id=tagof(key));
native map_set_cell(Map:map, AnyTag:key, offset, AnyTag:value, TagTag:key_tag_id=tagof(key));
native bool:map_set_cell_safe(Map:map, AnyTag:key, offset, AnyTag:value, TagTag:key_tag_id=tagof(key), TagTag:value_tag_id=tagof(value));
native map_arr_set(Map:map, const AnyTag:key[], AnyTag:value, key_size=sizeof(key), TagTag:key_tag_id=tagof(key), TagTag:value_tag_id=tagof(value));
native map_arr_set_arr(Map:map, const AnyTag:key[], const AnyTag:value[], value_size=sizeof(value), key_size=sizeof(key), TagTag:key_tag_id=tagof(key), TagTag:value_tag_id=tagof(value));
native map_arr_set_str(Map:map, const AnyTag:key[], const value[], key_size=sizeof(key), TagTag:key_tag_id=tagof(key));
native map_arr_set_var(Map:map, const AnyTag:key[], ConstVariantTag:value, key_size=sizeof(key), TagTag:key_tag_id=tagof(key));
native map_arr_set_cell(Map:map, const AnyTag:key[], offset, AnyTag:value, key_size=sizeof(key), TagTag:key_tag_id=tagof(key));
native bool:map_arr_set_cell_safe(Map:map, const AnyTag:key[], offset, AnyTag:value, key_size=sizeof(key), TagTag:key_tag_id=tagof(key), TagTag:value_tag_id=tagof(value));
native map_str_set(Map:map, const key[], AnyTag:value, TagTag:value_tag_id=tagof(value));
native map_str_set_arr(Map:map, const key[], const AnyTag:value[], value_size=sizeof(value), TagTag:value_tag_id=tagof(value));
native map_str_set_str(Map:map, const key[], const value[]);
native map_str_set_var(Map:map, const key[], ConstVariantTag:value);
native map_str_set_cell(Map:map, const key[], offset, AnyTag:value);
native bool:map_str_set_cell_safe(Map:map, const key[], offset, AnyTag:value, TagTag:value_tag_id=tagof(value));
native map_var_set(Map:map, ConstVariantTag:key, AnyTag:value, TagTag:value_tag_id=tagof(value));
native map_var_set_arr(Map:map, ConstVariantTag:key, const AnyTag:value[], value_size=sizeof(value), TagTag:value_tag_id=tagof(value));
native map_var_set_str(Map:map, ConstVariantTag:key, const value[]);
native map_var_set_var(Map:map, ConstVariantTag:key, ConstVariantTag:value);
native map_var_set_cell(Map:map, ConstVariantTag:key, offset, AnyTag:value);
native bool:map_var_set_cell_safe(Map:map, ConstVariantTag:key, offset, AnyTag:value, TagTag:value_tag_id=tagof(value));

native map_key_at(Map:map, index, offset=0);
native map_arr_key_at(Map:map, index, AnyTag:key[], key_size=sizeof(key));
native Variant:map_var_key_at(Map:map, index);
native bool:map_key_at_safe(Map:map, index, &AnyTag:key, offset=0, TagTag:key_tag_id=tagof(key));
native map_arr_key_at_safe(Map:map, index, AnyTag:key[], key_size=sizeof(key), TagTag:key_tag_id=tagof(key));

native map_value_at(Map:map, index, offset=0);
native map_arr_value_at(Map:map, index, AnyTag:value[], value_size=sizeof(value));
native Variant:map_var_value_at(Map:map, index);
native bool:map_value_at_safe(Map:map, index, &AnyTag:value, offset=0, TagTag:value_tag_id=tagof(value));
native map_arr_value_at_safe(Map:map, index, AnyTag:value[], value_size=sizeof(value), TagTag:key_tag_id=tagof(value));

native map_count(Map:map, AnyTag:value, TagTag:tag_id=tagof(value));
native map_count_arr(Map:map, const AnyTag:value[], size=sizeof(value), TagTag:tag_id=tagof(value));
native map_count_str(Map:map, const value[]);
native map_count_var(Map:map, ConstVariantTag:value);
native map_count_if(Map:map, Expression:pred);

native map_tagof(Map:map, AnyTag:key, TagTag:key_tag_id=tagof(key));
native map_sizeof(Map:map, AnyTag:key, TagTag:key_tag_id=tagof(key));
native map_arr_tagof(Map:map, const AnyTag:key[], key_size=sizeof(key), TagTag:key_tag_id=tagof(key));
native map_arr_sizeof(Map:map, const AnyTag:key[], key_size=sizeof(key), TagTag:key_tag_id=tagof(key));
native map_str_tagof(Map:map, const key[]);
native map_str_sizeof(Map:map, const key[]);
native map_var_tagof(Map:map, ConstVariantTag:key);
native map_var_sizeof(Map:map, ConstVariantTag:key);

native Iter:map_iter(Map:map, index=0);
native Iter:map_iter_at(Map:map, AnyTag:key, TagTag:key_tag_id=tagof(key));
native Iter:map_iter_at_arr(Map:map, const AnyTag:key[], key_size=sizeof(key), TagTag:key_tag_id=tagof(key));
native Iter:map_iter_at_str(Map:map, const key[]);
native Iter:map_iter_at_var(Map:map, ConstVariantTag:key);

#if defined PP_SYNTAX_GENERIC

#define map_new<%0,%1>(%2) (Map<%0,%1>:map_new(%2))
#define map_new_args_of<%0,%1>(%2,%3) (Map<%0,%1>:map_new_args_t<%0,%1>(_,_,_PP@CAST[%0](%2),_PP@CAST[%0](%3)))
#define map_valid<%0,%1>(%2) map_valid(Map:_PP@CAST[Map<%0,%1>](%2))
#define map_delete<%0,%1>(%2) map_delete(Map:_PP@CAST[Map<%0,%1>](%2))
#define map_delete_deep<%0,%1>(%2) map_delete_deep(Map:_PP@CAST[Map<%0,%1>](%2))
#define map_clone<%0,%1>(%2) (Map<%0,%1>:map_clone(Map:_PP@CAST[Map<%0,%1>](%2)))
#define map_size<%0,%1>(%2) map_size(Map:_PP@CAST[Map<%0,%1>](%2))
#define map_capacity<%0,%1>(%2) map_capacity(Map:_PP@CAST[Map<%0,%1>](%2))
#define map_reserve<%0,%1>(%2) map_reserve(Map:_PP@CAST[Map<%0,%1>](%2))
#define map_set_ordered<%0,%1>(%2) map_set_ordered(Map:_PP@CAST[Map<%0,%1>](%2))
#define map_is_ordered<%0,%1>(%2) map_is_ordered(Map:_PP@CAST[Map<%0,%1>](%2))

#define map_add<%0,%1>(%2,%3,%4) map_add(Map:_PP@CAST[Map<%0,%1>](%2),_PP@CAST[%0](%3),_PP@CAST[%1](%4))
#define map_add_arr<%0,%1>(%2,%3,%4) map_add_arr(Map:_PP@CAST[Map<%0,%1>](%2),_PP@CAST[%0](%3),_PP@CAST_ARR[%1](%4))
#define map_arr_add<%0,%1>(%2,%3,%4) map_arr_add(Map:_PP@CAST[Map<%0,%1>](%2),_PP@CAST_ARR[%0](%3),_PP@CAST[%1](%4))
#define map_arr_add_arr<%0,%1>(%2,%3,%4) map_arr_add_arr(Map:_PP@CAST[Map<%0,%1>](%2),_PP@CAST_ARR[%0](%3),_PP@CAST_ARR[%1](%4))
#define map_add_map<%0,%1>(%2,%3) map_add_map(Map:_PP@CAST[Map<%0,%1>](%2),_PP@CAST[Map<%0,%1>](%3))
#define map_add_args_of<%0,%1>(%2,%3,%4) map_add_args_t<%0,%1>(_,_,Map:_PP@CAST[Map<%0,%1>](%2),_PP@CAST[%0](%3),_PP@CAST[%0](%4))

#define map_remove<%0,%1>(%2,%3) map_remove(Map:_PP@CAST[Map<%0,%1>](%2),_PP@CAST[%0](%3))
#define map_arr_remove<%0,%1>(%2,%3) map_arr_remove(Map:_PP@CAST[Map<%0,%1>](%2),_PP@CAST_ARR[%0](%3))
#define map_remove_deep<%0,%1>(%2,%3) map_remove_deep(Map:_PP@CAST[Map<%0,%1>](%2),_PP@CAST[%0](%3))
#define map_arr_remove_deep<%0,%1>(%2,%3) map_arr_remove_deep(Map:_PP@CAST[Map<%0,%1>](%2),_PP@CAST_ARR[%0](%3))

#define map_has_key<%0,%1>(%2,%3) map_has_key(Map:_PP@CAST[Map<%0,%1>](%2),_PP@CAST[%0](%3))
#define map_has_arr_key<%0,%1>(%2,%3) map_has_arr_key(Map:_PP@CAST[Map<%0,%1>](%2),_PP@CAST_ARR[%0](%3))

#define map_get<%0,%1>(%2,%3) (%1:map_get(Map:_PP@CAST[Map<%0,%1>](%2),_PP@CAST[%0](%3)))
#define map_get_arr<%0,%1>(%2,%3,%4) map_get_arr(Map:_PP@CAST[Map<%0,%1>](%2),_PP@CAST[%0](%3),_PP@CAST_ARR[%1](%4))
#define map_arr_get<%0,%1>(%2,%3) (%1:map_get(Map:_PP@CAST[Map<%0,%1>](%2),_PP@CAST_ARR[%0](%3)))
#define map_arr_get_arr<%0,%1>(%2,%3,%4) map_arr_get_arr(Map:_PP@CAST[Map<%0,%1>](%2),_PP@CAST_ARR[%0](%3),_PP@CAST_ARR[%1](%4))

#define map_set<%0,%1>(%2,%3,%4) map_set(Map:_PP@CAST[Map<%0,%1>](%2),_PP@CAST[%0](%3),_PP@CAST[%1](%4))
#define map_set_arr<%0,%1>(%2,%3,%4) map_set_arr(Map:_PP@CAST[Map<%0,%1>](%2),_PP@CAST[%0](%3),_PP@CAST_ARR[%1](%4))
#define map_set_cell<%0,%1>(%2,%3,%4,%5) map_set_cell(Map:_PP@CAST[Map<%0,%1>](%2),_PP@CAST[%0](%3),%4,_PP@CAST[%1](%5))
#define map_arr_set<%0,%1>(%2,%3,%4) map_arr_set(Map:_PP@CAST[Map<%0,%1>](%2),_PP@CAST_ARR[%0](%3),_PP@CAST[%1](%4))
#define map_arr_set_arr<%0,%1>(%2,%3,%4) map_arr_set_arr(Map:_PP@CAST[Map<%0,%1>](%2),_PP@CAST_ARR[%0](%3),_PP@CAST_ARR[%1](%4))
#define map_arr_set_cell<%0,%1>(%2,%3,%4,%5) map_arr_set_cell(Map:_PP@CAST[Map<%0,%1>](%2),_PP@CAST_ARR[%0](%3),%4,_PP@CAST[%1](%5))

#define map_key_at<%0,%1>(%2,%3) (%0:map_key_at(Map:_PP@CAST[Map<%0,%1>](%2),%3))
#define map_arr_key_at<%0,%1>(%2,%3,%4) map_arr_key_at(Map:_PP@CAST[Map<%0,%1>](%2),%3,_PP@CAST_ARR[%0](%4))

#define map_value_at<%0,%1>(%2,%3) (%1:map_value_at(Map:_PP@CAST[Map<%0,%1>](%2),%3))
#define map_arr_value_at<%0,%1>(%2,%3,%4) map_arr_value_at(Map:_PP@CAST[Map<%0,%1>](%2),%3,_PP@CAST_ARR[%1](%4))

#define map_count<%0,%1>(%2,%3) map_count(Map:_PP@CAST[Map<%0,%1>](%2),_PP@CAST[%1](%3))
#define map_count_arr<%0,%1>(%2,%3) map_count_arr(Map:_PP@CAST[Map<%0,%1>](%2),_PP@CAST_ARR[%1](%3))

#define map_sizeof<%0,%1>(%2,%3) map_sizeof(Map:_PP@CAST[Map<%0,%1>](%2),_PP@CAST[%0](%3))
#define map_arr_sizeof<%0,%1>(%2,%3) map_arr_sizeof(Map:_PP@CAST[Map<%0,%1>](%2),_PP@CAST_ARR[%0](%3))

#define map_iter<%0,%1>(%2) (PairIter<%0,%1>:map_iter(Map:_PP@CAST[Map<%0,%1>](%2)))
#define map_iter_at<%0,%1>(%2,%3) (PairIter<%0,%1>:map_iter_at(Map:_PP@CAST[Map<%0,%1>](%2),_PP@CAST[%0](%3)))
#define map_iter_at_arr<%0,%1>(%2,%3) (PairIter<%0,%1>:map_iter_at_arr(Map:_PP@CAST[Map<%0,%1>](%2),_PP@CAST_ARR[%0](%3)))

#endif


/*                 */
/*  Linked lists   */
/*                 */

const LinkedList:INVALID_LINKED_LIST = LinkedList:0;

native LinkedList:linked_list_new();
native LinkedList:linked_list_new_arr(AnyTag:values[], size=sizeof(values), TagTag:tag_id=tagof(values));
native LinkedList:linked_list_new_args_t(TagTag:tag_id=tagof(arg0), AnyTag:arg0, AnyTag:...) = linked_list_new_args;
native LinkedList:linked_list_new_args_packed(ArgTag:...);
/*
native LinkedList:linked_list_new_args(AnyTag:...);
*/
#define linked_list_new_args(%0) linked_list_new_args_packed(_PP@TAGGED_PACK(%0))
native LinkedList:linked_list_new_args_str(arg0[], ...);
native LinkedList:linked_list_new_args_var(ConstVariantTag:arg0, ConstVariantTag:...);
native bool:linked_list_valid(LinkedList:linked_list);
native linked_list_delete(LinkedList:linked_list);
native linked_list_delete_deep(LinkedList:linked_list);
native linked_list_size(LinkedList:linked_list);
native linked_list_clear(LinkedList:linked_list);
native linked_list_clear_deep(LinkedList:linked_list);
native LinkedList:linked_list_clone(LinkedList:linked_list);

native linked_list_add(LinkedList:linked_list, AnyTag:value, index=-1, TagTag:tag_id=tagof(value));
native linked_list_add_arr(LinkedList:linked_list, const AnyTag:value[], index=-1, size=sizeof(value), TagTag:tag_id=tagof(value));
native linked_list_add_str(LinkedList:linked_list, const value[], index=-1);
native linked_list_add_var(LinkedList:linked_list, ConstVariantTag:value, index=-1);
native linked_list_add_linked_list(LinkedList:linked_list, LinkedList:range, index=-1);
native linked_list_add_iter(LinkedList:linked_list, Iter:iter, index=-1);
native linked_list_add_args_t(TagTag:tag_id=tagof(arg0), LinkedList:linked_list, AnyTag:arg0, AnyTag:...) = linked_list_add_args;
native linked_list_add_args_packed(LinkedList:linked_list, ArgTag:...);
/*
native linked_list_add_args(LinkedList:linked_list, AnyTag:...);
*/
#define linked_list_add_args(%0,%1) linked_list_add_args_packed(%0,_PP@TAGGED_PACK(%1))
native linked_list_add_args_str(LinkedList:linked_list, arg0[], ...);
native linked_list_add_args_var(LinkedList:linked_list, ConstVariantTag:arg0, ConstVariantTag:...);

native linked_list_remove(LinkedList:linked_list, index);
native linked_list_remove_deep(LinkedList:linked_list, index);
native linked_list_remove_if(LinkedList:linked_list, Expression:pred);
native linked_list_remove_if_deep(LinkedList:linked_list, Expression:pred);

native linked_list_get(LinkedList:linked_list, index, offset=0);
native linked_list_get_arr(LinkedList:linked_list, index, AnyTag:value[], size=sizeof(value));
native linked_list_get_str(LinkedList:linked_list, index, AnyTag:value[], size=sizeof(value)) = linked_list_get_arr;
native String:linked_list_get_str_s(LinkedList:linked_list, index);
native Variant:linked_list_get_var(LinkedList:linked_list, index);
native bool:linked_list_get_safe(LinkedList:linked_list, index, &AnyTag:value, offset=0, TagTag:tag_id=tagof(value));
native linked_list_get_arr_safe(LinkedList:linked_list, index, AnyTag:value[], size=sizeof(value), TagTag:tag_id=tagof(value));
native linked_list_get_str_safe(LinkedList:linked_list, index, value[], size=sizeof(value));
native String:linked_list_get_str_safe_s(LinkedList:linked_list, index);

native linked_list_set(LinkedList:linked_list, index, AnyTag:value, TagTag:tag_id=tagof(value));
native linked_list_set_arr(LinkedList:linked_list, index, const AnyTag:value[], size=sizeof(value), TagTag:tag_id=tagof(value));
native linked_list_set_str(LinkedList:linked_list, index, const value[]);
native linked_list_set_var(LinkedList:linked_list, index, ConstVariantTag:value);
native linked_list_set_cell(LinkedList:linked_list, index, offset, AnyTag:value);
native bool:linked_list_set_cell_safe(LinkedList:linked_list, index, offset, AnyTag:value, TagTag:tag_id=tagof(value));

native linked_list_count(LinkedList:linked_list, AnyTag:value, TagTag:tag_id=tagof(value));
native linked_list_count_arr(LinkedList:linked_list, const AnyTag:value[], size=sizeof(value), TagTag:tag_id=tagof(value));
native linked_list_count_str(LinkedList:linked_list, const value[]);
native linked_list_count_var(LinkedList:linked_list, ConstVariantTag:value);
native linked_list_count_if(LinkedList:linked_list, Expression:pred);

native linked_list_tagof(LinkedList:linked_list, index);
native linked_list_sizeof(LinkedList:linked_list, index);

native Iter:linked_list_iter(LinkedList:linked_list, index=0);

#if defined PP_SYNTAX_GENERIC

#define linked_list_new<%0>(%1) (LinkedList<%0>:linked_list_new(%1))
#define linked_list_new_arr<%0>(%1) (LinkedList<%0>:linked_list_new_arr(_PP@CAST_ARR[%0](%1)))
#define linked_list_new_args_of<%0>(%1) (LinkedList<%0>:linked_list_new_args_t(_,_PP@CAST[%0](%1)))
#define linked_list_valid<%0>(%1) linked_list_valid(LinkedList:_PP@CAST[LinkedList<%0>](%1))
#define linked_list_delete<%0>(%1) linked_list_delete(LinkedList:_PP@CAST[LinkedList<%0>](%1))
#define linked_list_delete_deep<%0>(%1) linked_list_delete_deep(LinkedList:_PP@CAST[LinkedList<%0>](%1))
#define linked_list_clone<%0>(%1) (LinkedList<%0>:linked_list_clone(LinkedList:_PP@CAST[LinkedList<%0>](%1)))
#define linked_list_size<%0>(%1) linked_list_size(LinkedList:_PP@CAST[LinkedList<%0>](%1))
#define linked_list_clear<%0>(%1) linked_list_clear(LinkedList:_PP@CAST[LinkedList<%0>](%1))

#define linked_list_add<%0>(%1,%2) linked_list_add(LinkedList:_PP@CAST[LinkedList<%0>](%1),_PP@CAST[%0](%2))
#define linked_list_add_arr<%0>(%1,%2) linked_list_add_arr(LinkedList:_PP@CAST[LinkedList<%0>](%1),_PP@CAST_ARR[%0](%2))
#define linked_list_add_list<%0>(%1,%2) linked_list_add_list(LinkedList:_PP@CAST[LinkedList<%0>](%1),LinkedList:_PP@CAST[LinkedList<%0>](%2))
#define linked_list_add_args_of<%0>(%1,%2) linked_list_add_args_t(_,LinkedList:_PP@CAST[LinkedList<%0>](%1),_PP@CAST[%0](%2))

#define linked_list_remove<%0>(%1,%2) linked_list_remove(LinkedList:_PP@CAST[LinkedList<%0>](%1),%2)
#define linked_list_remove_deep<%0>(%1,%2) linked_list_remove_deep(LinkedList:_PP@CAST[LinkedList<%0>](%1),%2)

#define linked_list_get<%0>(%1,%2) (%0:linked_list_get(LinkedList:_PP@CAST[LinkedList<%0>](%1),%2))
#define linked_list_get_arr<%0>(%1,%2,%3) linked_list_get_arr(LinkedList:_PP@CAST[LinkedList<%0>](%1),%2,_PP@CAST_ARR[%0](%3))

#define linked_list_set<%0>(%1,%2,%3) linked_list_set(LinkedList:_PP@CAST[LinkedList<%0>](%1),%2,_PP@CAST[%0](%3))
#define linked_list_set_arr<%0>(%1,%2,%3) linked_list_set_arr(LinkedList:_PP@CAST[LinkedList<%0>](%1),%2,_PP@CAST_ARR[%0](%3))
#define linked_list_set_cell<%0>(%1,%2,%3,%4) linked_list_set_cell(LinkedList:_PP@CAST[LinkedList<%0>](%1),%2,%3,_PP@CAST[%0](%4))

#define linked_list_count<%0>(%1,%2) linked_list_count(LinkedList:_PP@CAST[LinkedList<%0>](%1),_PP@CAST[%0](%2))
#define linked_list_count_arr<%0>(%1,%2) linked_list_count_arr(LinkedList:_PP@CAST[LinkedList<%0>](%1),_PP@CAST_ARR[%0](%2))

#define linked_list_sizeof<%0>(%1) linked_list_sizeof(LinkedList:_PP@CAST[LinkedList<%0>](%1))

#define linked_list_iter<%0>(%1) (Iter<%0>:linked_list_iter(LinkedList:_PP@CAST[LinkedList<%0>](%1)))

#endif


/*                 */
/*      Pools      */
/*                 */

const Pool:INVALID_POOL = Pool:0;

native Pool:pool_new(capacity=-1, bool:ordered=false);
native bool:pool_valid(Pool:pool);
native pool_delete(Pool:pool);
native pool_delete_deep(Pool:pool);
native pool_size(Pool:pool);
native pool_resize(Pool:pool, newsize);
native pool_capacity(Pool:pool);
native pool_reserve(Pool:pool, capacity);
native pool_clear(Pool:pool);
native pool_clear_deep(Pool:pool);
native pool_set_ordered(Pool:pool, bool:ordered);
native bool:pool_is_ordered(Pool:pool);
native Pool:pool_clone(Pool:pool);

native pool_add(Pool:pool, AnyTag:value, TagTag:tag_id=tagof(value));
native pool_add_arr(Pool:pool, const AnyTag:value[], size=sizeof(value), TagTag:tag_id=tagof(value));
native pool_add_str(Pool:pool, const value[]);
native pool_add_var(Pool:pool, ConstVariantTag:value);

native bool:pool_remove(Pool:pool, index);
native bool:pool_remove_deep(Pool:pool, index);
native pool_remove_range(Pool:pool, begin, end);
native pool_remove_range_deep(Pool:pool, begin, end);
native pool_remove_if(Pool:pool, Expression:pred);
native pool_remove_if_deep(Pool:pool, Expression:pred);

native bool:pool_has(Pool:pool, index);

native pool_get(Pool:pool, index, offset=0);
native pool_get_arr(Pool:pool, index, AnyTag:value[], size=sizeof(value));
native pool_get_str(Pool:pool, index, value[], size=sizeof(value)) = pool_get_arr;
native String:pool_get_str_s(Pool:pool, index);
native Variant:pool_get_var(Pool:pool, index);
native bool:pool_get_safe(Pool:pool, index, &AnyTag:value, offset=0, TagTag:tag_id=tagof(value));
native pool_get_arr_safe(Pool:pool, index, AnyTag:value[], size=sizeof(value), TagTag:tag_id=tagof(value));
native pool_get_str_safe(Pool:pool, index, value[], size=sizeof(value));
native String:pool_get_str_safe_s(Pool:pool, index);

native pool_set(Pool:pool, index, AnyTag:value, TagTag:tag_id=tagof(value));
native pool_set_arr(Pool:pool, index, const AnyTag:value[], size=sizeof(value), TagTag:tag_id=tagof(value));
native pool_set_str(Pool:pool, index, const value[]);
native pool_set_var(Pool:pool, index, ConstVariantTag:value);
native pool_set_cell(Pool:pool, index, offset, AnyTag:value);
native bool:pool_set_cell_safe(Pool:pool, index, offset, AnyTag:value, TagTag:tag_id=tagof(value));

native pool_find(Pool:pool, AnyTag:value, TagTag:tag_id=tagof(value));
native pool_find_arr(Pool:pool, const AnyTag:value[], size=sizeof(value), TagTag:tag_id=tagof(value));
native pool_find_str(Pool:pool, const value[]);
native pool_find_var(Pool:pool, ConstVariantTag:value);
native pool_find_if(Pool:pool, Expression:pred);

native pool_count(Pool:pool, AnyTag:value, TagTag:tag_id=tagof(value));
native pool_count_arr(Pool:pool, const AnyTag:value[], size=sizeof(value), TagTag:tag_id=tagof(value));
native pool_count_str(Pool:pool, const value[]);
native pool_count_var(Pool:pool, ConstVariantTag:value);
native pool_count_if(Pool:pool, Expression:pred);

native pool_tagof(Pool:pool, index);
native pool_sizeof(Pool:pool, index);

native Iter:pool_iter(Pool:pool, index=0);
native Iter:pool_iter_at(Pool:pool, index);

#if defined PP_SYNTAX_GENERIC

#define pool_new<%0>(%1) (Pool<%0>:pool_new(%1))
#define pool_valid<%0>(%1) pool_valid(Pool:_PP@CAST[Pool<%0>](%1))
#define pool_delete<%0>(%1) pool_delete(Pool:_PP@CAST[Pool<%0>](%1))
#define pool_delete_deep<%0>(%1) pool_delete_deep(Pool:_PP@CAST[Pool<%0>](%1))
#define pool_clone<%0>(%1) (Pool<%0>:pool_clone(Pool:_PP@CAST[Pool<%0>](%1)))
#define pool_size<%0>(%1) pool_size(Pool:_PP@CAST[Pool<%0>](%1))
#define pool_resize<%0>(%1) pool_resize(Pool:_PP@CAST[Pool<%0>](%1))
#define pool_capacity<%0>(%1) pool_capacity(Pool:_PP@CAST[Pool<%0>](%1))
#define pool_reserve<%0>(%1) pool_reserve(Pool:_PP@CAST[Pool<%0>](%1))
#define pool_clear<%0>(%1) pool_clear(Pool:_PP@CAST[Pool<%0>](%1))

#define pool_add<%0>(%1,%2) pool_add(Pool:_PP@CAST[Pool<%0>](%1),_PP@CAST[%0](%2))
#define pool_add_arr<%0>(%1,%2) pool_add_arr(Pool:_PP@CAST[Pool<%0>](%1),_PP@CAST_ARR[%0](%2))
#define pool_add_pool<%0>(%1,%2) pool_add_pool(Pool:_PP@CAST[Pool<%0>](%1),Pool:_PP@CAST[Pool<%0>](%2))
#define pool_add_iter<%0>(%1,%2) pool_add_iter(Pool:_PP@CAST[Pool<%0>](%1),Iter:_PP@CAST[Iter<%0>](%2))
#define pool_add_args_of<%0>(%1,%2) pool_add_args_t(_,Pool:_PP@CAST[Pool<%0>](%1),_PP@CAST[%0](%2))

#define pool_remove<%0>(%1,%2) pool_remove(Pool:_PP@CAST[Pool<%0>](%1),%2)
#define pool_remove_deep<%0>(%1,%2) pool_remove_deep(Pool:_PP@CAST[Pool<%0>](%1),%2)
#define pool_remove_range<%0>(%1,%2) pool_remove(Pool:_PP@CAST[Pool<%0>](%1),%2)
#define pool_remove_range_deep<%0>(%1,%2) pool_remove_deep(Pool:_PP@CAST[Pool<%0>](%1),%2)

#define pool_get<%0>(%1,%2) (%0:pool_get(Pool:_PP@CAST[Pool<%0>](%1),%2))
#define pool_get_arr<%0>(%1,%2,%3) pool_get_arr(Pool:_PP@CAST[Pool<%0>](%1),%2,_PP@CAST_ARR[%0](%3))

#define pool_set<%0>(%1,%2,%3) pool_set(Pool:_PP@CAST[Pool<%0>](%1),%2,_PP@CAST[%0](%3))
#define pool_set_arr<%0>(%1,%2,%3) pool_set_arr(Pool:_PP@CAST[Pool<%0>](%1),%2,_PP@CAST_ARR[%0](%3))
#define pool_set_cell<%0>(%1,%2,%3,%4) pool_set_cell(Pool:_PP@CAST[Pool<%0>](%1),%2,%3,_PP@CAST[%0](%4))

#define pool_find<%0>(%1,%2) pool_find(Pool:_PP@CAST[Pool<%0>](%1),_PP@CAST[%0](%2))
#define pool_find_arr<%0>(%1,%2) pool_find_arr(Pool:_PP@CAST[Pool<%0>](%1),_PP@CAST_ARR[%0](%2))

#define pool_count<%0>(%1,%2) pool_count(Pool:_PP@CAST[Pool<%0>](%1),_PP@CAST[%0](%2))
#define pool_count_arr<%0>(%1,%2) pool_count_arr(Pool:_PP@CAST[Pool<%0>](%1),_PP@CAST_ARR[%0](%2))

#define pool_sizeof<%0>(%1) pool_sizeof(Pool:_PP@CAST[Pool<%0>](%1))

#define pool_iter<%0>(%1) (Iter<%0>:pool_iter(Pool:_PP@CAST[Pool<%0>](%1)))
#define pool_iter_at<%0>(%1) (Iter<%0>:pool_iter_at(Pool:_PP@CAST[Pool<%0>](%1)))

#endif


/*                 */
/*    Iterators    */
/*                 */

const Iter:ITER_NULL = Iter:0;
const Iter:INVALID_ITERATOR = Iter:-1;

native bool:iter_valid(IterTag:iter);
native Iter:iter_acquire(IterTag:iter);
native Iter:iter_release(IterTag:iter);
native iter_delete(IterTag:iter);
native bool:iter_linked(IterTag:iter);
native bool:iter_inside(IterTag:iter);
native bool:iter_empty(IterTag:iter);
native iter_type(IterTag:iter);
native iter_type_str(IterTag:iter, type[], size=sizeof(type));
native String:iter_type_str_s(IterTag:iter);
native Iter:iter_erase(IterTag:iter, bool:stay=false);
native Iter:iter_erase_deep(IterTag:iter, bool:stay=false);
native Iter:iter_reset(IterTag:iter);
native Iter:iter_clone(IterTag:iter);
native Iter:iter_move_next(IterTag:iter, steps=1);
native Iter:iter_move_previous(IterTag:iter, steps=1);
native Iter:iter_to_first(IterTag:iter, index=0);
native Iter:iter_to_last(IterTag:iter, index=0);
native bool:iter_can_reset(IterTag:iter);
native bool:iter_can_insert(IterTag:iter);
native bool:iter_can_erase(IterTag:iter);
native iter_swap(IterTag:iter1, IterTag:iter2);
native bool:iter_eq(IterTag:iter1, IterTag:iter2);

native Iter:iter_range(AnyTag:start, count, skip=1, tag_id=tagof(start));
native Iter:iter_range_arr(const AnyTag:start[], count, skip=1, size=sizeof(start), tag_id=tagof(start));
native Iter:iter_range_str(const start[], count, skip=1);
native Iter:iter_range_str_s(ConstStringTag:start, count, skip=1);
native Iter:iter_range_var(ConstVariantTag:start, count, skip=1);
native Iter:iter_repeat(AnyTag:value, count, tag_id=tagof(value));
native Iter:iter_repeat_arr(const AnyTag:value[], count, size=sizeof(value), tag_id=tagof(value));
native Iter:iter_repeat_str(const value[], count);
native Iter:iter_repeat_str_s(ConstStringTag:value, count);
native Iter:iter_repeat_var(ConstVariantTag:value, count);
native Iter:iter_filter(IterTag:iter, Expression:expr);
native Iter:iter_project(IterTag:iter, Expression:expr);

native iter_get(IterTag:iter, offset=0);
native iter_get_arr(IterTag:iter, AnyTag:value[], size=sizeof(value));
native iter_get_str(IterTag:iter, value[], size=sizeof(value)) = iter_get_arr;
native String:iter_get_str_s(IterTag:iter);
native Variant:iter_get_var(IterTag:iter);
native bool:iter_get_safe(IterTag:iter, &AnyTag:value, offset=0, TagTag:tag_id=tagof(value));
native iter_get_arr_safe(IterTag:iter, AnyTag:value[], size=sizeof(value), TagTag:tag_id=tagof(value));
native iter_get_str_safe(IterTag:iter, value[], size=sizeof(value));
native String:iter_get_str_safe_s(IterTag:iter);

native iter_set(IterTag:iter, AnyTag:value, TagTag:tag_id=tagof(value));
native iter_set_arr(IterTag:iter, const AnyTag:value[], size=sizeof(value), TagTag:tag_id=tagof(value));
native iter_set_arr_2d(IterTag:iter, const AnyTag:value[][], size=sizeof(value), size2=sizeof(value[]), TagTag:tag_id=tagof(value));
native iter_set_arr_3d(IterTag:iter, const AnyTag:value[][][], size=sizeof(value), size2=sizeof(value[]), size3=sizeof(value[][]), TagTag:tag_id=tagof(value));
native iter_set_str(IterTag:iter, const value[]);
native iter_set_var(IterTag:iter, ConstVariantTag:value);
native iter_set_cell(IterTag:iter, offset, AnyTag:value);
native bool:iter_set_cell_safe(IterTag:iter, offset, AnyTag:value, TagTag:tag_id=tagof(value));
native iter_set_cells(IterTag:iter, offset, AnyTag:values[], size=sizeof(values));
native iter_set_cells_safe(IterTag:iter, offset, AnyTag:values[], size=sizeof(values), TagTag:tag_id=tagof(values));

native iter_get_md(IterTag:iter, const offsets[], offsets_size=sizeof(offsets));
native iter_get_md_arr(IterTag:iter, const offsets[], AnyTag:value[], size=sizeof(value), offsets_size=sizeof(offsets));
native iter_get_md_str(IterTag:iter, const offsets[], value[], size=sizeof(value), offsets_size=sizeof(offsets)) = iter_get_md_arr;
native String:iter_get_md_str_s(IterTag:iter, const offsets[], offsets_size=sizeof(offsets));
native bool:iter_get_md_safe(IterTag:iter, const offsets[], &AnyTag:value, offsets_size=sizeof(offsets), TagTag:tag_id=tagof(value));
native iter_get_md_arr_safe(IterTag:iter, const offsets[], AnyTag:value[], size=sizeof(value), offsets_size=sizeof(offsets), TagTag:tag_id=tagof(value));
native iter_get_md_str_safe(IterTag:iter, const offsets[], value[], size=sizeof(value), offsets_size=sizeof(offsets));
native String:iter_get_md_str_safe_s(IterTag:iter, const offsets[], offsets_size=sizeof(offsets));

native iter_set_cell_md(IterTag:iter, const offsets[], AnyTag:value, offsets_size=sizeof(offsets));
native bool:iter_set_cell_md_safe(IterTag:iter, const offsets[], AnyTag:value, offsets_size=sizeof(offsets), TagTag:tag_id=tagof(value));
native iter_set_cells_md(IterTag:iter, const offsets[], AnyTag:values[], offsets_size=sizeof(offsets), size=sizeof(values));
native iter_set_cells_md_safe(IterTag:iter, const offsets[], AnyTag:values[], offsets_size=sizeof(offsets), size=sizeof(values), TagTag:tag_id=tagof(values));

native Iter:iter_insert(IterTag:iter, AnyTag:value, TagTag:tag_id=tagof(value));
native Iter:iter_insert_arr(IterTag:iter, const AnyTag:value[], size=sizeof(value), TagTag:tag_id=tagof(value));
native Iter:iter_insert_arr_2d(IterTag:iter, const AnyTag:value[][], size=sizeof(value), size2=sizeof(value[]), TagTag:tag_id=tagof(value));
native Iter:iter_insert_arr_3d(IterTag:iter, const AnyTag:value[][][], size=sizeof(value), size2=sizeof(value[]), size3=sizeof(value[][]), TagTag:tag_id=tagof(value));
native Iter:iter_insert_str(IterTag:iter, const value[]);
native Iter:iter_insert_var(IterTag:iter, ConstVariantTag:value);

native iter_tagof(IterTag:iter);
native tag_uid:iter_tag_uid(IterTag:iter);
native iter_sizeof(IterTag:iter);
native iter_sizeof_md(IterTag:iter, const offsets[], offsets_size=sizeof(offsets));
native iter_rank(IterTag:iter);

native iter_get_key(IterTag:iter, offset=0);
native iter_get_key_arr(IterTag:iter, AnyTag:value[], size=sizeof(value));
native iter_get_key_str(IterTag:iter, value[], size=sizeof(value)) = iter_get_key_arr;
native String:iter_get_key_str_s(IterTag:iter);
native Variant:iter_get_key_var(IterTag:iter);
native bool:iter_get_key_safe(IterTag:iter, &AnyTag:value, offset=0, TagTag:tag_id=tagof(value));
native iter_get_key_arr_safe(IterTag:iter, AnyTag:value[], size=sizeof(value), TagTag:tag_id=tagof(value));
native iter_get_key_str_safe(IterTag:iter, value[], size=sizeof(value));
native String:iter_get_key_str_safe_s(IterTag:iter);

native iter_get_key_md(IterTag:iter, const offsets[], offsets_size=sizeof(offsets));
native iter_get_key_md_arr(IterTag:iter, const offsets[], AnyTag:value[], size=sizeof(value), offsets_size=sizeof(offsets));
native iter_get_key_md_str(IterTag:iter, const offsets[], value[], size=sizeof(value), offsets_size=sizeof(offsets)) = iter_get_key_md_arr;
native String:iter_get_key_md_str_s(IterTag:iter, const offsets[], offsets_size=sizeof(offsets));
native bool:iter_get_key_md_safe(IterTag:iter, const offsets[], &AnyTag:value, offsets_size=sizeof(offsets), TagTag:tag_id=tagof(value));
native iter_get_key_md_arr_safe(IterTag:iter, const offsets[], AnyTag:value[], size=sizeof(value), offsets_size=sizeof(offsets), TagTag:tag_id=tagof(value));
native iter_get_key_md_str_safe(IterTag:iter, const offsets[], value[], size=sizeof(value), offsets_size=sizeof(offsets));
native String:iter_get_key_md_str_safe_s(IterTag:iter, const offsets[], offsets_size=sizeof(offsets));

native iter_tagof_key(IterTag:iter);
native tag_uid:iter_key_tag_uid(IterTag:iter);
native iter_sizeof_key(IterTag:iter);
native iter_sizeof_key_md(IterTag:iter, const offsets[], offsets_size=sizeof(offsets));
native iter_key_rank(IterTag:iter);

native iter_get_value(IterTag:iter, offset=0) = iter_get;
native iter_get_value_arr(IterTag:iter, AnyTag:value[], size=sizeof(value)) = iter_get_arr;
native iter_get_value_str(IterTag:iter, value[], size=sizeof(value)) = iter_get_arr;
native String:iter_get_value_str_s(IterTag:iter) = iter_get_str_s;
native Variant:iter_get_value_var(IterTag:iter) = iter_get_var;
native bool:iter_get_value_safe(IterTag:iter, &AnyTag:value, offset=0, TagTag:tag_id=tagof(value)) = iter_get_safe;
native iter_get_value_arr_safe(IterTag:iter, AnyTag:value[], size=sizeof(value), TagTag:tag_id=tagof(value)) = iter_get_arr_safe;
native iter_get_value_str_safe(IterTag:iter, value[], size=sizeof(value)) = iter_get_str_safe;
native String:iter_get_value_str_safe_s(IterTag:iter) = iter_get_str_safe_s;

native bool:iter_set_value(IterTag:iter, AnyTag:value, TagTag:value_tag_id=tagof(value)) = iter_set;
native bool:iter_set_value_arr(IterTag:iter, const AnyTag:value[], size=sizeof(value), TagTag:value_tag_id=tagof(value)) = iter_set_arr;
native bool:iter_set_value_str(IterTag:iter, const value[]) = iter_set_str;
native bool:iter_set_value_var(IterTag:iter, ConstVariantTag:value) = iter_set_var;
native bool:iter_set_value_cell(IterTag:iter, offset, AnyTag:value) = iter_set_cell;
native bool:iter_set_value_cell_safe(IterTag:iter, offset, AnyTag:value, TagTag:value_tag_id=tagof(value)) = iter_set_cell_safe;
native iter_set_value_cells(IterTag:iter, offset, AnyTag:values[], size=sizeof(values)) = iter_set_cells;
native iter_set_value_cells_safe(IterTag:iter, offset, AnyTag:values[], size=sizeof(values), TagTag:tag_id=tagof(values)) = iter_set_cells_safe;

native iter_get_value_md(IterTag:iter, const offsets[], offsets_size=sizeof(offsets)) = iter_get_md;
native iter_get_value_md_arr(IterTag:iter, const offsets[], AnyTag:value[], size=sizeof(value), offsets_size=sizeof(offsets)) = iter_get_md_arr;
native iter_get_value_md_str(IterTag:iter, const offsets[], value[], size=sizeof(value), offsets_size=sizeof(offsets)) = iter_get_md_arr;
native String:iter_get_value_md_str_s(IterTag:iter, const offsets[], offsets_size=sizeof(offsets)) = iter_get_md_str_s;
native bool:iter_get_value_md_safe(IterTag:iter, const offsets[], &AnyTag:value, offsets_size=sizeof(offsets), TagTag:tag_id=tagof(value)) = iter_get_md_safe;
native iter_get_value_md_arr_safe(IterTag:iter, const offsets[], AnyTag:value[], size=sizeof(value), offsets_size=sizeof(offsets), TagTag:tag_id=tagof(value)) = iter_get_md_arr_safe;
native iter_get_value_md_str_safe(IterTag:iter, const offsets[], value[], size=sizeof(value), offsets_size=sizeof(offsets)) = iter_get_md_str_safe;
native String:iter_get_value_md_str_safe_s(IterTag:iter, const offsets[], offsets_size=sizeof(offsets)) = iter_get_md_str_safe_s;

native bool:iter_set_value_cell_md(IterTag:iter, const offsets[], AnyTag:value, offsets_size=sizeof(offsets)) = iter_set_cell_md;
native bool:iter_set_value_cell_md_safe(IterTag:iter, const offsets[], AnyTag:value, offsets_size=sizeof(offsets), TagTag:tag_id=tagof(value)) = iter_set_cell_md_safe;
native iter_set_value_cells_md(IterTag:iter, const offsets[], AnyTag:values[], offsets_size=sizeof(offsets), size=sizeof(values)) = iter_set_cells_md;
native iter_set_value_cells_md_safe(IterTag:iter, const offsets[], AnyTag:values[], offsets_size=sizeof(offsets), size=sizeof(values), TagTag:tag_id=tagof(values)) = iter_set_cells_md_safe;

native iter_tagof_value(IterTag:iter) = iter_tagof;
native tag_uid:iter_value_tag_uid(IterTag:iter) = iter_tag_uid;
native iter_sizeof_value(IterTag:iter) = iter_sizeof;
native iter_sizeof_value_md(IterTag:iter, const offsets[], offsets_size=sizeof(offsets)) = iter_sizeof_md;
native iter_value_rank(IterTag:iter) = iter_rank;

#if defined PP_SYNTAX_GENERIC

#define iter_valid<%0>(%1) iter_valid(Iter:_PP@CAST[Iter<%0>](%1))
#define iter_acquire<%0>(%1) (Iter<%0>:iter_acquire(Iter:_PP@CAST[Iter<%0>](%1)))
#define iter_release<%0>(%1) (Iter<%0>:iter_release(Iter:_PP@CAST[Iter<%0>](%1)))
#define iter_delete<%0>(%1) iter_delete(Iter:_PP@CAST[Iter<%0>](%1))
#define iter_linked<%0>(%1) iter_linked(Iter:_PP@CAST[Iter<%0>](%1))
#define iter_inside<%0>(%1) iter_inside(Iter:_PP@CAST[Iter<%0>](%1))
#define iter_erase<%0>(%1) (Iter<%0>:iter_erase(Iter:_PP@CAST[Iter<%0>](%1)))
#define iter_erase_deep<%0>(%1) (Iter<%0>:iter_erase_deep(Iter:_PP@CAST[Iter<%0>](%1)))
#define iter_reset<%0>(%1) (Iter<%0>:iter_reset(Iter:_PP@CAST[Iter<%0>](%1)))
#define iter_clone<%0>(%1) (Iter<%0>:iter_clone(Iter:_PP@CAST[Iter<%0>](%1)))
#define iter_move_next<%0>(%1) (Iter<%0>:iter_move_next(Iter:_PP@CAST[Iter<%0>](%1)))
#define iter_move_previous<%0>(%1) (Iter<%0>:iter_move_previous(Iter:_PP@CAST[Iter<%0>](%1)))
#define iter_to_first<%0>(%1) (Iter<%0>:iter_to_first(Iter:_PP@CAST[Iter<%0>](%1)))
#define iter_to_last<%0>(%1) (Iter<%0>:iter_to_last(Iter:_PP@CAST[Iter<%0>](%1)))
#define iter_can_reset<%0>(%1) iter_can_reset(Iter:_PP@CAST[Iter<%0>](%1))
#define iter_can_insert<%0>(%1) iter_can_insert(Iter:_PP@CAST[Iter<%0>](%1))
#define iter_can_erase<%0>(%1) iter_can_erase(Iter:_PP@CAST[Iter<%0>](%1))
#define iter_swap<%0>(%1,%2) iter_swap(Iter:_PP@CAST[Iter<%0>](%1),Iter:_PP@CAST[Iter<%0>](%2))
#define iter_eq<%0>(%1,%2) iter_eq(Iter:_PP@CAST[Iter<%0>](%1),Iter:_PP@CAST[Iter<%0>](%2))

#define iter_get<%0>(%1) (%0:iter_get(Iter:_PP@CAST[Iter<%0>](%1)))
#define iter_get_arr<%0>(%1,%2) iter_get_arr(Iter:_PP@CAST[Iter<%0>](%1),_PP@CAST_ARR[%0](%2))
#define iter_set<%0>(%1,%2) iter_set(Iter:_PP@CAST[Iter<%0>](%1),_PP@CAST[%0](%2))
#define iter_set_arr<%0>(%1,%2) iter_set_arr(Iter:_PP@CAST[Iter<%0>](%1),_PP@CAST_ARR[%0](%2))
#define iter_set_cell<%0>(%1,%2,%3) iter_set_cell(Iter:_PP@CAST[Iter<%0>](%1),%2,_PP@CAST[%0](%3))
#define iter_set_cells<%0>(%1,%2,%3) iter_set_cell(Iter:_PP@CAST[Iter<%0>](%1),%2,_PP@CAST_ARR[%0](%3))
#define iter_insert<%0>(%1,%2) (Iter<%0>:iter_insert(Iter:_PP@CAST[Iter<%0>](%1),_PP@CAST[%0](%2)))
#define iter_insert_arr<%0>(%1,%2) (Iter<%0>:iter_insert_arr(Iter:_PP@CAST[Iter<%0>](%1),_PP@CAST_ARR[%0](%2)))
#define iter_get_md<%0>(%1) (%0:iter_get_md(Iter:_PP@CAST[Iter<%0>](%1)))
#define iter_get_md_arr<%0>(%1,%2,%3) iter_get_md_arr(Iter:_PP@CAST[Iter<%0>](%1),%2,_PP@CAST_ARR[%0](%3))
#define iter_set_cell_md<%0>(%1,%2,%3) iter_set_cell_md(Iter:_PP@CAST[Iter<%0>](%1),%2,_PP@CAST[%0](%3))
#define iter_set_cells_md<%0>(%1,%2,%3) iter_set_cells_md(Iter:_PP@CAST[Iter<%0>](%1),%2,_PP@CAST_ARR[%0](%3))

#define iter_sizeof<%0>(%1) iter_sizeof(Iter:_PP@CAST[Iter<%0>](%1))
#define iter_sizeof_md<%0>(%1) iter_sizeof_md(Iter:_PP@CAST[Iter<%0>](%1))
#define iter_rank<%0>(%1) iter_rank(Iter:_PP@CAST[Iter<%0>](%1))

#define iter_get_key<%0,%1>(%2) (%0:iter_get_key(Iter:_PP@CAST[PairIter<%0,%1>](%2)))
#define iter_get_key_arr<%0,%1>(%2,%3) iter_get_key_arr(Iter:_PP@CAST[PairIter<%0,%1>](%2),_PP@CAST_ARR[%0](%3))
#define iter_get_value<%0,%1>(%2) (%1:iter_get_value(Iter:_PP@CAST[PairIter<%0,%1>](%2)))
#define iter_get_value_arr<%0,%1>(%2,%3) iter_get_value_arr(Iter:_PP@CAST[PairIter<%0,%1>](%2),_PP@CAST_ARR[%1](%3))
#define iter_get_value_md<%0,%1>(%2) (%0:iter_get_value_md(Iter:_PP@CAST[PairIter<%0,%1>](%2)))
#define iter_get_value_md_arr<%0,%1>(%2,%3,%4) iter_get_value_md_arr(Iter:_PP@CAST[PairIter<%0,%1>](%2),%3,_PP@CAST_ARR[%0](%4))
#define iter_set_value<%0,%1>(%2,%3) iter_set_value(Iter:_PP@CAST[PairIter<%0,%1>](%2),_PP@CAST[%1](%3))
#define iter_set_value_arr<%0,%1>(%2,%3) iter_set_value_arr(Iter:_PP@CAST[PairIter<%0,%1>](%2),_PP@CAST_ARR[%1](%3))
#define iter_set_value_cell<%0,%1>(%2,%3,%4) iter_set_value_cell(Iter:_PP@CAST[PairIter<%0,%1>](%2),%3,_PP@CAST[%1](%4))
#define iter_set_value_cells<%0,%1>(%2,%3,%4) iter_set_value_cells(Iter:_PP@CAST[PairIter<%0,%1>](%2),%3,_PP@CAST_ARR[%1](%4))
#define iter_set_value_cell_md<%0,%1>(%2,%3,%4) iter_set_value_cell_md(Iter:_PP@CAST[PairIter<%0,%1>](%2),%3,_PP@CAST[%1](%4))
#define iter_set_value_cells_md<%0,%1>(%2,%3,%4) iter_set_value_cells_md(Iter:_PP@CAST[PairIter<%0,%1>](%2),%3,_PP@CAST_ARR[%1](%4))

#define iter_sizeof_key<%0,%1>(%2) iter_sizeof_key(Iter:_PP@CAST[PairIter<%0,%1>](%2))
#define iter_sizeof_key_md<%0,%1>(%2) iter_sizeof_key_md(Iter:_PP@CAST[PairIter<%0,%1>](%2))
#define iter_key_rank<%0,%1>(%2) iter_key_rank(Iter:_PP@CAST[PairIter<%0,%1>](%2))
#define iter_sizeof_value<%0,%1>(%2) iter_sizeof_value(Iter:_PP@CAST[PairIter<%0,%1>](%2))
#define iter_sizeof_value_md<%0,%1>(%2) iter_sizeof_value_md(Iter:_PP@CAST[PairIter<%0,%1>](%2))
#define iter_value_rank<%0,%1>(%2) iter_value_rank(Iter:_PP@CAST[PairIter<%0,%1>](%2))

#if defined PP_SYNTAX_FOR_LIST
#define for_list_of<%2>(%0:%1) for(new Iter<%2>:%0=list_iter<%2>(%1);iter_inside(Iter:%0);iter_move_next(Iter:%0))
#endif

#if defined PP_SYNTAX_FOR_MAP
#define for_map_of<%2,%3>(%0:%1) for(new PairIter<%2,%3>:%0=map_iter<%2,%3>(%1);iter_inside(Iter:%0);iter_move_next(Iter:%0))
#endif

#if defined PP_SYNTAX_FOR_LINKED_LIST
#define for_linked_list_of<%2>(%0:%1) for(new Iter<%2>:%0=linked_list_iter<%2>(%1);iter_inside(Iter:%0);iter_move_next(Iter:%0))
#endif

#if defined PP_SYNTAX_FOR_POOL
#define for_pool_of<%2>(%0:%1) for(new Iter<%2>:%0=pool_iter<%2>(%1);iter_inside(Iter:%0);iter_move_next(Iter:%0))
#endif

#endif

#if defined PP_SYNTAX_FOR_LIST
#define for_list(%0:%1) for(new Iter:%0=list_iter(%1);iter_inside(%0);iter_move_next(%0))
#endif

#if defined PP_SYNTAX_FOR_MAP
#define for_map(%0:%1) for(new Iter:%0=map_iter(%1);iter_inside(%0);iter_move_next(%0))
#endif

#if defined PP_SYNTAX_FOR_LINKED_LIST
#define for_linked_list(%0:%1) for(new Iter:%0=linked_list_iter(%1);iter_inside(%0);iter_move_next(%0))
#endif

#if defined PP_SYNTAX_FOR_POOL
#define for_pool(%0:%1) for(new Iter:%0=pool_iter(%1);iter_inside(%0);iter_move_next(%0))
#endif


/*                 */
/*     Handles     */
/*                 */

const Handle:HANDLE_NULL = Handle:0;
const Handle:INVALID_HANDLE = Handle:-1;

native Handle:handle_new(AnyTag:value, bool:weak=false, TagTag:tag_id=tagof(value));
native Handle:handle_new_arr(const AnyTag:value[], bool:weak=false, size=sizeof(value), TagTag:tag_id=tagof(value));
native Handle:handle_new_var(ConstVariantTag:value, bool:weak=false);
native Handle:handle_alias(HandleTag:handle, AnyTag:value, TagTag:tag_id=tagof(value));
native Handle:handle_alias_arr(HandleTag:handle, const AnyTag:value[], size=sizeof(value), TagTag:tag_id=tagof(value));
native Handle:handle_alias_var(HandleTag:handle, ConstVariantTag:value);
native Handle:handle_acquire(HandleTag:handle);
native Handle:handle_release(HandleTag:handle);
native handle_delete(HandleTag:handle);
native bool:handle_valid(HandleTag:handle);
native bool:handle_linked(HandleTag:handle);
native bool:handle_alive(HandleTag:handle);
native bool:handle_weak(HandleTag:handle);

native handle_get(HandleTag:handle, offset=0);
native handle_get_arr(HandleTag:handle, AnyTag:value[], size=sizeof(value));
native Variant:handle_get_var(HandleTag:handle);
native bool:handle_get_safe(HandleTag:handle, &AnyTag:value, offset=0, TagTag:tag_id=tagof(value));
native handle_get_arr_safe(ConstVariantTag:var, AnyTag:value[], size=sizeof(value), TagTag:tag_id=tagof(value));

native bool:handle_eq(HandleTag:handle1, HandleTag:handle2);

native handle_tagof(HandleTag:handle);
native handle_sizeof(HandleTag:handle);

native Iter:handle_iter(HandleTag:handle, count=1);

#if defined PP_SYNTAX_GENERIC

#define handle_new<%0>(%1) (Handle<%0>:handle_new(_PP@CAST[%0](%1)))
#define handle_new_arr<%0>(%1) (Handle<%0>:handle_new_arr(_PP@CAST_ARR[%0](%1)))
#define handle_alias<%0>(%1,%2) (Handle<%0>:handle_new(%1,_PP@CAST[%0](%2)))
#define handle_alias_arr<%0>(%1,%2) (Handle<%0>:handle_new_arr(%1,_PP@CAST_ARR[%0](%2)))
#define handle_acquire<%0>(%1) (Handle<%0>:handle_acquire(Handle:_PP@CAST[Handle<%0>](%1)))
#define handle_release<%0>(%1) (Handle<%0>:handle_release(Handle:_PP@CAST[Handle<%0>](%1)))
#define handle_delete<%0>(%1) handle_delete(Handle:_PP@CAST[Handle<%0>](%1))
#define handle_valid<%0>(%1) handle_valid(Handle:_PP@CAST[Handle<%0>](%1))
#define handle_linked<%0>(%1) handle_linked(Handle:_PP@CAST[Handle<%0>](%1))
#define handle_reset<%0>(%1) handle_reset(Handle:_PP@CAST[Handle<%0>](%1))

#define handle_get<%0>(%1) (%0:handle_get(Handle:_PP@CAST[Handle<%0>](%1)))
#define handle_get_arr<%0>(%1,%2) handle_get_arr(Handle:_PP@CAST[Handle<%0>](%1),_PP@CAST_ARR[%0](%2))

#define handle_sizeof<%0>(%1) handle_sizeof(Handle:_PP@CAST[Handle<%0>](%1))

#endif


/*                 */
/*   Expressions   */
/*                 */

const Expression:INVALID_EXPRESSION = Expression:0;

native Expression:expr_acquire(Expression:expr);
native Expression:expr_release(Expression:expr);
native expr_delete(Expression:expr);
native bool:expr_valid(Expression:expr);
native expr_type(Expression:expr);
native expr_type_str(Expression:expr, type[], size=sizeof(type));
native String:expr_type_str_s(Expression:expr);

native Expression:expr_empty(Expression:...);
native Expression:expr_void(Expression:expr);

native Expression:expr_weak(Expression:expr);
native Expression:expr_weak_set(Expression:weak, Expression:target);

native Expression:expr_const(AnyTag:value, TagTag:tag_id=tagof(value));
native Expression:expr_const_arr(const AnyTag:value[], size=sizeof(value), TagTag:tag_id=tagof(value));
native Expression:expr_const_str(const value[]);
native Expression:expr_const_var(VariantTag:value);
native Expression:expr_true();
native Expression:expr_false();
native Expression:expr_handle(HandleTag:handle);

native Expression:expr_arr(Expression:...);

native Expression:expr_arg(index);
native Expression:expr_arg_pack(begin, end=-1);
native Expression:expr_bind(Expression:expr, Expression:...);
native Expression:expr_nested(Expression:expr);
native Expression:expr_env();
native Expression:expr_set_env(Expression:expr, Map:env=INVALID_MAP, bool:env_readonly=false);
native Expression:expr_global(const name[]);
native Expression:expr_set_amx(Expression:expr, Amx:amx);

native Expression:expr_comma(Expression:left, Expression:right);
native Expression:expr_assign(Expression:left, Expression:right);

native Expression:expr_try(Expression:main, Expression:fallback);

native Expression:expr_symbol(Symbol:symbol);
native Expression:expr_native(const function[]);
native Expression:expr_public(const function[]);
native Expression:expr_pubvar(const name[]);
native Expression:expr_call(Expression:func, Expression:...);
native Expression:expr_index(Expression:arr, Expression:...);

native Expression:expr_quote(Expression:expr);
native Expression:expr_dequote(Expression:expr);

native Expression:expr_extract(Expression:expr);
native Expression:expr_variant(Expression:value);
native Expression:expr_string(Expression:value);

native Expression:expr_add(Expression:left, Expression:right);
native Expression:expr_sub(Expression:left, Expression:right);
native Expression:expr_mul(Expression:left, Expression:right);
native Expression:expr_div(Expression:left, Expression:right);
native Expression:expr_mod(Expression:left, Expression:right);
native Expression:expr_bit_and(Expression:left, Expression:right);
native Expression:expr_bit_or(Expression:left, Expression:right);
native Expression:expr_bit_xor(Expression:left, Expression:right);
native Expression:expr_rs(Expression:left, Expression:right);
native Expression:expr_ls(Expression:left, Expression:right);
native Expression:expr_neg(Expression:expr);
native Expression:expr_inc(Expression:expr);
native Expression:expr_dec(Expression:expr);
native Expression:expr_bit_not(Expression:expr);

native Expression:expr_eq(Expression:left, Expression:right);
native Expression:expr_neq(Expression:left, Expression:right);
native Expression:expr_lt(Expression:left, Expression:right);
native Expression:expr_gt(Expression:left, Expression:right);
native Expression:expr_lte(Expression:left, Expression:right);
native Expression:expr_gte(Expression:left, Expression:right);
native Expression:expr_not(Expression:expr);

native Expression:expr_and(Expression:left, Expression:right);
native Expression:expr_or(Expression:left, Expression:right);

native Expression:expr_cond(Expression:cond, Expression:on_true, Expression:on_false);

native Expression:expr_range(Expression:begin, Expression:end);
native Expression:expr_select(Expression:list, Expression:func);
native Expression:expr_where(Expression:list, Expression:cond);

native Expression:expr_cast(Expression:expr, TagTag:tag_id);
native Expression:expr_tagof(Expression:expr);
native Expression:expr_sizeof(Expression:expr, Expression:...);
native Expression:expr_rankof(Expression:expr);
native Expression:expr_addressof(Expression:expr);
native Expression:expr_nameof(Expression:expr);

native expr_get(Expression:expr, offset=0);
native expr_get_arr(Expression:expr, AnyTag:value[], size=sizeof(value));
native expr_get_str(Expression:expr, value[], size=sizeof(value)) = expr_get_arr;
native String:expr_get_str_s(Expression:expr);
native Variant:expr_get_var(Expression:expr);
native bool:expr_get_safe(Expression:expr, &AnyTag:value, offset=0, TagTag:tag_id=tagof(value));
native expr_get_arr_safe(Expression:expr, AnyTag:value[], size=sizeof(value), TagTag:tag_id=tagof(value));
native expr_get_str_safe(Expression:expr, value[], size=sizeof(value));
native String:expr_get_str_safe_s(Expression:expr);

native expr_set(Expression:expr, AnyTag:value, TagTag:tag_id=tagof(value));
native expr_set_arr(Expression:expr, const AnyTag:value[], size=sizeof(value), TagTag:tag_id=tagof(value));
native expr_set_str(Expression:expr, const value[]);
native expr_set_var(Expression:expr, ConstVariantTag:value);

native expr_exec(Expression:expr);
native expr_exec_multi(Expression:expr, List:output);

enum parser_options (<<= 1)
{
    parser_bare = 0,
	parser_allow_unknown_symbols = 1,
	parser_allow_debug_symbols,
	parser_allow_natives,
	parser_allow_publics,
	parser_allow_pubvars,
	parser_allow_constructions,
	parser_allow_ranges,
	parser_allow_arrays,
	parser_allow_queries,
	parser_allow_arguments,
	parser_allow_environment,
	parser_allow_casts,
    parser_allow_literals,
    parser_allow_intrinsics,
    parser_allow_strings,
    parser_all = -1
}

native Expression:expr_parse(const string[], parser_options:options=parser_all);
native Expression:expr_parse_s(ConstString:string, parser_options:options=parser_all);
native Expression:expr_literal(const name[]);
native Expression:expr_intrinsic(const name[], parser_options:options=parser_all);

#if defined _INC_y_functional && defined _INC_y_va
forward _pp@indirect_call(f, ...);
public _pp@indirect_call(f, ...)
{
    return @.f(___(1));
}

forward _pp@indirect_release(f);
public _pp@indirect_release(f)
{
    Indirect_Release(f);
}

stock expr_func_inner({F@_@, F@_@i, F@_@ii, F@_@iii, F@_@iiii, F@_@iiiii, F@_@iiiiii, F@_@iiiiiii, F@_@iiiiiiii, F@_@iiiiiiiii}:func, tag_id=tagof(func))
{
    static tag_uid:func_tag;
    if(!func_tag)
    {
        func_tag = tag_new("Indirect_Release");
        tag_set_op(func_tag, tag_op_collect, #_pp@indirect_release);
        tag_lock(func_tag);
    }
    new args = str_len(tag_name_s(tag_uid(tag_id))) - 4;
    Indirect_Claim(func);
    return _:expr_quote(expr_set_amx(expr_call(expr_public(#_pp@indirect_call), expr_const(_:func, tag_id), expr_empty(expr_const(_:func, func_tag)), expr_select(expr_arg_pack(0, args - 1), expr_addressof(expr_arg(0))))));
}

stock Expression:expr_func({F@_@, F@_@i, F@_@ii, F@_@iii, F@_@iiii, F@_@iiiii, F@_@iiiiii, F@_@iiiiiii, F@_@iiiiiiii, F@_@iiiiiiiii}:func, tag_id=tagof(func))
{
    return Expression:expr_func_inner(func, tag_id);
}

#define expr_func<%0>({%1}) Expression:LAMBDA_%0<expr_func_inner>{%1}()
#endif


/*                 */
/* Tasks and async */
/*                 */

enum task_restore
{
    task_restore_none = 0,
    task_restore_frame = 1,
    task_restore_context = 2,
    task_restore_full = 3,
}

const Task:INVALID_TASK = Task:0;

native wait_ms(interval);
native wait_ticks(ticks);
native Task:task_new();
native task_delete(Task:task);
native bool:task_valid(Task:task);
native Task:task_keep(Task:task, bool:keep=true);
native task_set_result(Task:task, AnyTag:result, TagTag:tag_id=tagof(result));
native task_set_result_arr(Task:task, const AnyTag:result[], size=sizeof(result), TagTag:tag_id=tagof(result));
native task_set_result_str(Task:task, const result[]);
native task_set_result_var(Task:task, ConstVariantTag:result);
native task_get_result(Task:task, offset=0);
native task_get_result_arr(Task:task, AnyTag:result[], size=sizeof(result));
native task_get_result_str(Task:task, result[], size=sizeof(result)) = task_get_result_arr;
native String:task_get_result_str_s(Task:task);
native Variant:task_get_result_var(Task:task);
native bool:task_get_result_safe(Task:task, &AnyTag:result, offset=0, TagTag:tag_id=tagof(result));
native task_get_result_arr_safe(Task:task, AnyTag:result[], size=sizeof(result), TagTag:tag_id=tagof(result));
native task_set_error(Task:task, amx_err:error);
native amx_err:task_get_error(Task:task);
native task_set_result_ms(Task:task, AnyTag:result, interval, TagTag:tag_id=tagof(result));
native task_set_result_ms_arr(Task:task, const AnyTag:result[], interval, size=sizeof(result), TagTag:tag_id=tagof(result));
native task_set_result_ms_str(Task:task, const result[], interval);
native task_set_result_ms_var(Task:task, interval);
native task_set_result_ticks(Task:task, AnyTag:result, ticks, TagTag:tag_id=tagof(result));
native task_set_result_ticks_arr(Task:tasks, const AnyTag:result[], tick, size=sizeof(result), TagTag:tag_id=tagof(result));
native task_set_result_ticks_str(Task:task, const result[], ticks);
native task_set_result_ticks_var(Task:task, ConstVariantTag:result, ticks);
native task_set_error_ms(Task:task, amx_err:error, interval);
native task_set_error_ticks(Task:task, amx_err:error, ticks);
native bool:task_completed(Task:task);
native bool:task_faulted(Task:task);
native task_state:task_state(Task:task);
native task_reset(Task:task);
native Task:task_ticks(ticks);
native Task:task_ms(interval);
native Task:task_any(Task:...);
native Task:task_all(Task:...);
native task_state:task_wait(Task:task);
/*
native task_await(Task:task);
*/
stock task_await(Task:task) return task_wait(task), task_get_result(task);
/*
native task_await_arr(Task:task, AnyTag:result[], size=sizeof(result));
*/
stock task_await_arr(Task:task, AnyTag:result[], size=sizeof(result)) return task_wait(task), task_get_result_arr(task, result, size);
/*
native Variant:task_await_var(Task:task);
*/
stock Variant:task_await_var(Task:task) return task_wait(task), task_get_result_var(task);
native task_yield(AnyTag:value, TagTag:tag_id=tagof(value));
native task_set_yielded(AnyTag:value);
native task_get_yielded();
native bool:task_bind(Task:task, const function[], const format[], AnyTag:...);
native Task:task_bound();
native task_config(task_restore:heap=task_restore_full, task_restore:stack=task_restore_full);
native task_continue_with(Task:task, const handler[], const additional_format[]="", AnyTag:...);
native Task:task_continue_with_bound(Task:task, Task:bound, const handler[], const additional_format[]="", AnyTag:...);
native task_detach();
native task_detach_bound(Task:task);

const task_state:task_state_default = task_state:0;
const task_state:task_state_completed = task_state:1;
const task_state:task_state_faulted = task_state:2;

#if defined PP_SYNTAX_AWAIT
#if defined PP_MULTILINE_AWAIT

stock _PP@TASK_OUT;
stock _PP@TASK_RESULT:_PP@TASK_IN;
stock _PP@TASK_CAST;

#define _PP@TASK_OUT<%0> %0:_PP@TASK_OUT

stock _PP@TASK_RESULT:operator%(_PP@TASK_RESULT:a, Task:t) return a, _PP@TASK_RESULT:t;
stock operator%(a, _PP@TASK_RESULT:t) return a, task_await(Task:t);

#define await%0\32; _PP@TASK_OUT%0%=_PP@TASK_IN%=Task%0:_PP@TASK_CAST=

#if defined PP_SYNTAX_GENERIC

#define PP_TASK_TAG(%0) \
    stock _PP@TASK_RESULT:operator%(_PP@TASK_RESULT:a, Task<%0>:t) return a, _PP@TASK_RESULT:t; \
    stock %0:operator%(%0:a, _PP@TASK_RESULT:t) return a, %0:task_await(Task:t)

stock _PP@TASK_RESULT:operator%(_PP@TASK_RESULT:a, Task<_>:t) return a, _PP@TASK_RESULT:t;
PP_TASK_TAG(Float);
PP_TASK_TAG(bool);
PP_TASK_TAG(Variant);
PP_TASK_TAG(ConstVariant);
PP_TASK_TAG(String);
PP_TASK_TAG(ConstString);
PP_TASK_TAG(List);
PP_TASK_TAG(Map);
PP_TASK_TAG(Iter);
PP_TASK_TAG(Handle);

#endif

#else

#define await%0\32;%1; task_await%0(%1);

#endif

#define await_arr%0(%1)%2; task_await_arr%0(%2,%1);
#define await_var%0\32;%1; task_await_var%0(%1);

#endif

#if defined PP_SYNTAX_YIELD
#define yield%0\32;%1; task_yield%0(%1);
#endif

#if defined PP_SYNTAX_ASYNC
#define async%0Task%1:%2(%3) %0Task%1:%2(%3) if(!(task_detach_bound(task_new())&&task_set_yielded(task_bound())))return Task%1:INVALID_TASK;else
#define return%0\32;async%1\32;%2; return%0 Task%1:(task_bound()?(task_set_result%1(Task%1:task_bound(),%2)):0);
#define Task_arr%0: Task%0:
#define Task_str: Task:
#define Task_var%0: Task%0:
#endif

#if defined PP_SYNTAX_GENERIC

#define task_new<%0>(%1) (Task<%0>:task_new(%1))
#define task_delete<%0>(%1) task_delete(Task:_PP@CAST[Task<%0>](%1))
#define task_valid<%0>(%1) task_valid(Task:_PP@CAST[Task<%0>](%1))
#define task_keep<%0>(%1) (Task<%0>:task_keep(Task:_PP@CAST[Task<%0>](%1)))
#define task_set_result<%0>(%1,%2) task_set_result(Task:_PP@CAST[Task<%0>](%1),_PP@CAST[%0](%2))
#define task_set_result_arr<%0>(%1,%2) task_set_result_arr(Task:_PP@CAST[Task<%0>](%1),_PP@CAST_ARR[%0](%2))
#define task_get_result<%0>(%1) (%0:task_get_result(Task:_PP@CAST[Task<%0>](%1)))
#define task_get_result_arr<%0>(%1,%2) task_get_result_arr(Task:_PP@CAST[Task<%0>](%1),_PP@CAST_ARR[%0](%2))
#define task_set_error<%0>(%1) task_set_error(Task:_PP@CAST[Task<%0>](%1))
#define task_get_error<%0>(%1) task_get_error(Task:_PP@CAST[Task<%0>](%1))
#define task_set_result_ms<%0>(%1,%2,%3) task_set_result_ms(Task:_PP@CAST[Task<%0>](%1),_PP@CAST[%0](%2),%3)
#define task_set_result_ms_arr<%0>(%1,%2,%3) task_set_result_ms_arr(Task:_PP@CAST[Task<%0>](%1),_PP@CAST_ARR[%0](%2),%3)
#define task_set_result_ticks<%0>(%1,%2,%3) task_set_result_ticks(Task:_PP@CAST[Task<%0>](%1),_PP@CAST[%0](%2),%3)
#define task_set_result_ticks_arr<%0>(%1,%2,%3) task_set_result_ticks_arr(Task:_PP@CAST[Task<%0>](%1),_PP@CAST_ARR[%0](%2),%3)
#define task_set_error_ms<%0>(%1) task_set_error_ms(Task:_PP@CAST[Task<%0>](%1))
#define task_set_error_ticks<%0>(%1) task_set_error_ticks(Task:_PP@CAST[Task<%0>](%1))
#define task_completed<%0>(%1) task_completed(Task:_PP@CAST[Task<%0>](%1))
#define task_faulted<%0>(%1) task_faulted(Task:_PP@CAST[Task<%0>](%1))
#define task_state<%0>(%1) task_state(Task:_PP@CAST[Task<%0>](%1))
#define task_reset<%0>(%1) task_reset(Task:_PP@CAST[Task<%0>](%1))
#define task_wait<%0>(%1) task_wait(Task:_PP@CAST[Task<%0>](%1))
#define task_await<%0>(%1) (%0:task_await(Task:_PP@CAST[Task<%0>](%1)))
#define task_await_arr<%0>(%1,%2) task_await_arr(Task:_PP@CAST[Task<%0>](%1),_PP@CAST_ARR[%0](%2))
#define task_yield<%0>(%1) task_yield(_PP@CAST[%0](%1))
#define task_bind<%0>(%1) task_bind(Task:_PP@CAST[Task<%0>](%1))
#define task_detach_bound<%0>(%1) task_detach_bound(Task:_PP@CAST[Task<%0>](%1))

#endif


/*                 */
/*     Threads     */
/*                 */

enum sync_flags
{
    sync_explicit = 0,
    sync_auto = 1,
    sync_interrupt = 2
}

native thread_detach(sync_flags:flags);
native thread_attach();
native thread_sync();
native thread_sleep(ms);
native thread_id(id[], size=sizeof(id));
native String:thread_id_s();
native bool:thread_fix();

#if defined PP_SYNTAX_THREADED

#define threaded(%0) for(new PP@ts:_@pp_thread_running=PP@ts:(thread_detach(%0)|1);_@pp_thread_running;_@pp_thread_running=PP@ts:(thread_attach()&0))
stock operator~(PP@ts:arr[], count)
{
    for(new i = 0; i < count; i++)
    {
        if(arr[i]) thread_attach();
    }
}

#endif


/*                 */
/*       Math      */
/*                 */

native math_random_seed(seed);
native math_random(min=0, max=cellmax);
native math_random_unsigned(min=0, max=-1);
native Float:math_random_float(Float:min=0.0, Float:max=1.0);
native math_round(Float:val);
native math_floor(Float:val);
native math_ceiling(Float:val);
native math_truncate(Float:val);
native bool:math_try_round(Float:val, &result);
native bool:math_try_floor(Float:val, &result);
native bool:math_try_ceiling(Float:val, &result);
native bool:math_try_truncate(Float:val, &result);

native math_iadd({_,signed}:a, {_,signed}:b);
native math_iadd_ovf({_,signed}:a, {_,signed}:b);
native math_isub({_,signed}:a, {_,signed}:b);
native math_isub_ovf({_,signed}:a, {_,signed}:b);
native math_imul({_,signed}:a, {_,signed}:b);
native math_imul_ovf({_,signed}:a, {_,signed}:b);
native math_idiv({_,signed}:a, {_,signed}:b);
native math_imod({_,signed}:a, {_,signed}:b);
native math_iinc({_,signed}:a);
native math_iinc_ovf({_,signed}:a);
native math_idec({_,signed}:a);
native math_idec_ovf({_,signed}:a);
native math_uadd({_,unsigned}:a, {_,unsigned}:b);
native math_uadd_ovf({_,unsigned}:a, {_,unsigned}:b);
native math_usub({_,unsigned}:a, {_,unsigned}:b);
native math_usub_ovf({_,unsigned}:a, {_,unsigned}:b);
native math_umul({_,unsigned}:a, {_,unsigned}:b);
native math_umul_ovf({_,unsigned}:a, {_,unsigned}:b);
native math_udiv({_,unsigned}:a, {_,unsigned}:b);
native math_umod({_,unsigned}:a, {_,unsigned}:b);
native math_uinc({_,unsigned}:a);
native math_uinc_ovf({_,unsigned}:a);
native math_udec({_,unsigned}:a);
native math_udec_ovf({_,unsigned}:a);

native bool:math_ilt({_,signed}:a, {_,signed}:b);
native bool:math_ilte({_,signed}:a, {_,signed}:b);
native bool:math_igt({_,signed}:a, {_,signed}:b);
native bool:math_igte({_,signed}:a, {_,signed}:b);
native bool:math_ult({_,unsigned}:a, {_,unsigned}:b);
native bool:math_ulte({_,unsigned}:a, {_,unsigned}:b);
native bool:math_ugt({_,unsigned}:a, {_,unsigned}:b);
native bool:math_ugte({_,unsigned}:a, {_,unsigned}:b);

native signed:operator+(signed:a, signed:b) = math_iadd_ovf;
native signed:operator-(signed:a, signed:b) = math_isub_ovf;
native signed:operator*(signed:a, signed:b) = math_imul_ovf;
native signed:operator++(signed:a) = math_iinc_ovf;
native signed:operator--(signed:a) = math_idec_ovf;

native unsigned:operator+(unsigned:a, unsigned:b) = math_uadd_ovf;
native unsigned:operator-(unsigned:a, unsigned:b) = math_usub_ovf;
native unsigned:operator*(unsigned:a, unsigned:b) = math_umul_ovf;
native unsigned:operator/(unsigned:a, unsigned:b) = math_udiv;
native unsigned:operator%(unsigned:a, unsigned:b) = math_umod;
native unsigned:operator++(unsigned:a) = math_uinc_ovf;
native unsigned:operator--(unsigned:a) = math_udec_ovf;
native bool:operator<(unsigned:a, unsigned:b) = math_ult;
native bool:operator<=(unsigned:a, unsigned:b) = math_ulte;
native bool:operator>(unsigned:a, unsigned:b) = math_ugt;
native bool:operator>=(unsigned:a, unsigned:b) = math_ugte;

stock signed:operator=(a) return signed:a;
stock unsigned:operator=(a) return unsigned:a;


/*                 */
/*      Debug      */
/*                 */

const Symbol:INVALID_SYMBOL_ID = Symbol:cellmin;

enum symbol_kind (<<= 1)
{
    symbol_kind_variable = 1,
    symbol_kind_reference,
    symbol_kind_array,
    symbol_kind_array_reference,
    symbol_kind_function = 1 << 8
}

enum symbol_class (<<= 1)
{
    symbol_class_global = 1,
    symbol_class_local,
    symbol_class_static
}

native bool:debug_loaded();
native AmxDebug:debug_get_ptr();
native debug_code(level=0);
native debug_line(code=cellmin);
native debug_file(file[], code=cellmin, size=sizeof(file));
native String:debug_file_s(code=cellmin);

native debug_num_symbols();
native Symbol:debug_symbol(const name[], code=cellmin, symbol_kind:kind=symbol_kind:-1, symbol_class:class=symbol_class:-1);
native Symbol:debug_func(code=cellmin);
native Symbol:debug_var(&AnyTag:var);
native Symbol:debug_var_arr(AnyTag:var[]) = debug_var;
native symbol_kind:debug_symbol_kind(Symbol:symbol);
native symbol_class:debug_symbol_class(Symbol:symbol);
native debug_symbol_tag(Symbol:symbol);
native tag_uid:debug_symbol_tag_uid(Symbol:symbol);
native Symbol:debug_symbol_func(Symbol:symbol);
native debug_symbol_name(Symbol:symbol, name[], size=sizeof(name));
native String:debug_symbol_name_s(Symbol:symbol);
native debug_symbol_addr(Symbol:symbol);
native debug_symbol_range(Symbol:symbol, &codestart, &codeend);
native debug_symbol_line(Symbol:symbol);
native debug_symbol_file(Symbol:symbol, file[], size=sizeof(file));
native String:debug_symbol_file_s(Symbol:symbol);
native bool:debug_symbol_in_scope(Symbol:symbol, level=0);
native debug_symbol_rank(Symbol:symbol);
native debug_symbol_size(Symbol:symbol, dimension=0);
native debug_symbol_size_tag(Symbol:symbol, dimension=0);
native tag_uid:debug_symbol_size_tag_uid(Symbol:symbol, dimension=0);
native Var:debug_symbol_to_amx_var(Symbol:symbol, level=0);

native debug_symbol_get(Symbol:symbol, level=0, offset=0);
native debug_symbol_get_arr(Symbol:symbol, AnyTag:value[], level=0, offset=0, size=sizeof(value));
native Variant:debug_symbol_get_var(Symbol:symbol, level=0);
native bool:debug_symbol_get_safe(Symbol:symbol, &AnyTag:value, level=0, offset=0, tag_id=tagof(value));
native debug_symbol_get_arr_safe(Symbol:symbol, AnyTag:value[], level=0, offset=0, size=sizeof(value), tag_id=tagof(value));

native debug_symbol_set(Symbol:symbol, AnyTag:value, level=0, offset=0);
native bool:debug_symbol_set_safe(Symbol:symbol, AnyTag:value, level=0, offset=0, tag_id=tagof(value));
native debug_symbol_set_arr(Symbol:symbol, const AnyTag:values[], level=0, offset=0, size=sizeof(values));
native debug_symbol_set_arr_safe(Symbol:symbol, const AnyTag:values[], level=0, offset=0, size=sizeof(values), tag_id=tagof(values));

native debug_symbol_call(Symbol:symbol, AnyTag:...);
native debug_symbol_call_list(Symbol:symbol, List:args);
native debug_symbol_indirect_call(Symbol:symbol, AnyTag:...);
native debug_symbol_indirect_call_list(Symbol:symbol, List:args);
native amx_err:debug_symbol_try_call(Symbol:symbol, &result, AnyTag:...);
native amx_err:debug_symbol_try_call_list(Symbol:symbol, &result, List:args);

native Iter:debug_symbol_variables(Symbol:symbol);


/*                 */
/*  Operators etc. */
/*                 */

#define PP_PROHIBIT_UNARY_OPERATORS(%0) \
    forward operator-(%0:arg); \
    forward operator++(%0:arg); \
    forward operator--(%0:arg); \
    forward %0:operator=(_:arg); \
    forward %0:operator=(Float:arg)

PP_PROHIBIT_UNARY_OPERATORS(String);
PP_PROHIBIT_UNARY_OPERATORS(ConstString);
PP_PROHIBIT_UNARY_OPERATORS(AmxString);
PP_PROHIBIT_UNARY_OPERATORS(ConstAmxString);
PP_PROHIBIT_UNARY_OPERATORS(AmxStringBuffer);
PP_PROHIBIT_UNARY_OPERATORS(AmxVariant);
PP_PROHIBIT_UNARY_OPERATORS(ConstAmxVariant);
PP_PROHIBIT_UNARY_OPERATORS(AmxVariantBuffer);

forward String:operator=(AmxString:arg);
forward ConstString:operator=(AmxString:arg);
forward String:operator=(ConstAmxString:arg);
forward ConstString:operator=(ConstAmxString:arg);
forward String:operator=(AmxStringBuffer:arg);
forward ConstString:operator=(AmxStringBuffer:arg);
forward Variant:operator=(AmxVariant:arg);
forward ConstVariant:operator=(AmxVariant:arg);
forward Variant:operator=(ConstAmxVariant:arg);
forward ConstVariant:operator=(ConstAmxVariant:arg);
forward Variant:operator=(AmxVariantBuffer:arg);
forward ConstVariant:operator=(AmxVariantBuffer:arg);

#undef PP_PROHIBIT_UNARY_OPERATORS
    
#define PP_PROHIBIT_BINARY_OPEARTORS(%0,%1) \
    forward operator+(%0:arg1,%1:arg2); \
    forward operator-(%0:arg1,%1:arg2); \
    forward operator*(%0:arg1,%1:arg2); \
    forward operator/(%0:arg1,%1:arg2); \
    forward operator%(%0:arg1,%1:arg2)
    
#define PP_PROHIBIT_BINARY_OPEARTORS2(%0) \
    PP_PROHIBIT_BINARY_OPEARTORS(%0,_); \
    PP_PROHIBIT_BINARY_OPEARTORS(%0,Float); \
    PP_PROHIBIT_BINARY_OPEARTORS(%0,String); \
    PP_PROHIBIT_BINARY_OPEARTORS(%0,ConstString); \
    PP_PROHIBIT_BINARY_OPEARTORS(%0,AmxString); \
    PP_PROHIBIT_BINARY_OPEARTORS(%0,ConstAmxString); \
    PP_PROHIBIT_BINARY_OPEARTORS(%0,AmxStringBuffer); \
    PP_PROHIBIT_BINARY_OPEARTORS(%0,AmxVariant); \
    PP_PROHIBIT_BINARY_OPEARTORS(%0,ConstAmxVariant); \
    PP_PROHIBIT_BINARY_OPEARTORS(%0,AmxVariantBuffer)

PP_PROHIBIT_BINARY_OPEARTORS2(String);
PP_PROHIBIT_BINARY_OPEARTORS2(ConstString);
PP_PROHIBIT_BINARY_OPEARTORS2(AmxString);
PP_PROHIBIT_BINARY_OPEARTORS2(ConstAmxString);
PP_PROHIBIT_BINARY_OPEARTORS2(AmxStringBuffer);
PP_PROHIBIT_BINARY_OPEARTORS2(AmxVariant);
PP_PROHIBIT_BINARY_OPEARTORS2(ConstAmxVariant);
PP_PROHIBIT_BINARY_OPEARTORS2(AmxVariantBuffer);

#undef PP_PROHIBIT_BINARY_OPEARTORS

stock ConstString:operator=(String:arg) return ConstString:arg;
forward String:operator=(ConstString:arg);

native AmxString:operator=(String:arg) = str_addr;
native ConstAmxString:operator=(String:arg) = str_addr_const;
native ConstAmxString:operator=(ConstString:arg) = str_addr_const;
stock ConstAmxString:operator=(AmxString:arg) return ConstAmxString:arg;
forward AmxString:operator=(ConstString:arg);
forward AmxString:operator=(ConstAmxString:arg);
native AmxStringBuffer:operator=(String:arg) = str_buf_addr;
stock AmxStringBuffer:operator+(AmxStringBuffer:arg1, arg2) return AmxStringBuffer:(_:arg1+arg2);

native AmxVariant:operator=(Variant:arg) = var_addr;
native ConstAmxVariant:operator=(Variant:arg) = var_addr_const;
native ConstAmxVariant:operator=(ConstVariant:arg) = var_addr_const;
stock ConstAmxVariant:operator=(AmxVariant:arg) return ConstAmxVariant:arg;
forward AmxVariant:operator=(ConstVariant:arg);
forward AmxVariant:operator=(ConstAmxVariant:arg);
native AmxVariantBuffer:operator=(Variant:arg) = var_buf_addr;
stock AmxVariantBuffer:operator+(AmxVariantBuffer:arg1, arg2) return AmxVariantBuffer:(_:arg1+arg2);

native String:operator+(String:arg1, String:arg2) = str_cat;
native String:operator+(String:arg1, ConstString:arg2) = str_cat;
native String:operator+(ConstString:arg1, String:arg2) = str_cat;
native String:operator+(ConstString:arg1, ConstString:arg2) = str_cat;

native bool:operator==(String:arg1, String:arg2) = str_eq;
native bool:operator==(String:arg1, ConstString:arg2) = str_eq;
native bool:operator==(ConstString:arg1, String:arg2) = str_eq;
native bool:operator==(ConstString:arg1, ConstString:arg2) = str_eq;
stock bool:operator!=(String:arg1, String:arg2) return !(arg1 == arg2);
stock bool:operator!=(String:arg1, ConstString:arg2) return !(arg1 == arg2);
stock bool:operator!=(ConstString:arg1, String:arg2) return !(arg1 == arg2);
stock bool:operator!=(ConstString:arg1, ConstString:arg2) return !(arg1 == arg2);

native String:operator%(String:arg1, String:arg2) = str_cat;
native String:operator%(String:arg1, ConstString:arg2) = str_cat;
native String:operator%(ConstString:arg1, String:arg2) = str_cat;
native String:operator%(ConstString:arg1, ConstString:arg2) = str_cat;

#if defined PP_SYNTAX_STRING_OP

stock String:operator=(arg) return str_val(arg);
stock String:operator=(Float:arg) return str_val(arg);
stock String:operator%(String:arg1, arg2) return str_cat(arg1, str_val(arg2));
stock String:operator%(ConstString:arg1, arg2) return str_cat(arg1, str_val(arg2));
stock String:operator%(arg1, String:arg2) return str_cat(str_val(arg1), arg2);
stock String:operator%(arg1, ConstString:arg2) return str_cat(str_val(arg1), arg2);
stock String:operator%(String:arg1, Float:arg2) return str_cat(arg1, str_val(arg2));
stock String:operator%(ConstString:arg1, Float:arg2) return str_cat(arg1, str_val(arg2));
stock String:operator%(Float:arg1, String:arg2) return str_cat(str_val(arg1), arg2);
stock String:operator%(Float:arg1, ConstString:arg2) return str_cat(str_val(arg1), arg2);

#endif

stock ConstVariant:operator=(Variant:arg) return ConstVariant:arg;
forward Variant:operator=(ConstVariant:arg);

#define PP_VARIANT_BIN_OP(%0,%1) \
    native Variant:operator%0(Variant:arg1, Variant:arg2) = %1; \
    native Variant:operator%0(Variant:arg1, ConstVariant:arg2) = %1; \
    native Variant:operator%0(ConstVariant:arg1, Variant:arg2) = %1; \
    native Variant:operator%0(ConstVariant:arg1, ConstVariant:arg2) = %1

#define PP_VARIANT_UN_OP(%0,%1) \
    native Variant:operator%0(Variant:arg) = %1; \
    native Variant:operator%0(ConstVariant:arg) = %1

#define PP_VARIANT_UN_MOD_OP(%0,%1) \
    native Variant:operator%0(Variant:arg) = %1; \
    native Variant:operator%0(ConstVariant:arg) = %1

#define PP_VARIANT_LOG_BIN_OP(%0,%1) \
    native bool:operator%0(Variant:arg1, Variant:arg2) = %1; \
    native bool:operator%0(Variant:arg1, ConstVariant:arg2) = %1; \
    native bool:operator%0(ConstVariant:arg1, Variant:arg2) = %1; \
    native bool:operator%0(ConstVariant:arg1, ConstVariant:arg2) = %1

#define PP_VARIANT_LOG_UN_OP(%0,%1) \
    native bool:operator%0(Variant:arg) = %1; \
    native bool:operator%0(ConstVariant:arg) = %1

PP_VARIANT_BIN_OP(+, var_add);
PP_VARIANT_BIN_OP(-, var_sub);
PP_VARIANT_BIN_OP(*, var_mul);
PP_VARIANT_BIN_OP(/, var_div);
PP_VARIANT_BIN_OP(%, var_mod);
PP_VARIANT_UN_OP(-, var_neg);
PP_VARIANT_UN_MOD_OP(++, var_inc);
PP_VARIANT_UN_MOD_OP(--, var_dec);
PP_VARIANT_LOG_BIN_OP(==, var_eq);
PP_VARIANT_LOG_BIN_OP(!=, var_neq);
PP_VARIANT_LOG_BIN_OP(<, var_lt);
PP_VARIANT_LOG_BIN_OP(>, var_gt);
PP_VARIANT_LOG_BIN_OP(<=, var_lte);
PP_VARIANT_LOG_BIN_OP(>=, var_gte);
PP_VARIANT_LOG_UN_OP(!, var_not);

#undef PP_VARIANT_BIN_OP
#undef PP_VARIANT_UN_OP
#undef PP_VARIANT_UN_MOD_OP
#undef PP_VARIANT_LOG_BIN_OP
#undef PP_VARIANT_LOG_UN_OP

native bool:operator==(Iter:arg1, Iter:arg2) = iter_eq;
stock bool:operator!=(Iter:arg1, Iter:arg2) return !(arg1 == arg2);
native bool:operator!(Iter:arg) = iter_empty;

native bool:operator==(Handle:arg1, Handle:arg2) = handle_eq;
stock bool:operator!=(Handle:arg1, Handle:arg2) return !(arg1 == arg2);
stock bool:operator!(Handle:handle) return !handle_alive(handle);

stock Expression:operator=(arg) return expr_const(arg);
stock Expression:operator=(Float:arg) return expr_const(arg);
stock Expression:operator=(bool:arg) return expr_const(arg);
native Expression:operator+(Expression:arg1, Expression:arg2) = expr_add;
native Expression:operator-(Expression:arg1, Expression:arg2) = expr_sub;
native Expression:operator*(Expression:arg1, Expression:arg2) = expr_mul;
native Expression:operator/(Expression:arg1, Expression:arg2) = expr_div;
native Expression:operator%(Expression:arg1, Expression:arg2) = expr_mod;
native Expression:operator-(Expression:arg) = expr_neg;
native Expression:operator++(Expression:arg) = expr_inc;
native Expression:operator--(Expression:arg) = expr_dec;

#if defined PP_SYNTAX_GENERIC

stock String:operator=(Ref<String>:arg) return String:arg;
stock ConstString:operator=(Ref<ConstString>:arg) return ConstString:arg;
stock Variant:operator=(Ref<Variant>:arg) return Variant:arg;
stock ConstVariant:operator=(Ref<ConstVariant>:arg) return ConstVariant:arg;
stock List:operator=(Ref<List>:arg) return List:arg;
stock Map:operator=(Ref<Map>:arg) return Map:arg;
stock LinkedList:operator=(Ref<LinkedList>:arg) return LinkedList:arg;
stock Iter:operator=(Ref<Iter>:arg) return Iter:arg;

stock Ref<String>:operator=(String:arg) return Ref<String>:arg;
stock Ref<ConstString>:operator=(ConstString:arg) return Ref<ConstString>:arg;
stock Ref<Variant>:operator=(Variant:arg) return Ref<Variant>:arg;
stock Ref<ConstVariant>:operator=(ConstVariant:arg) return Ref<ConstVariant>:arg;
stock Ref<List>:operator=(List:arg) return Ref<List>:arg;
stock Ref<Map>:operator=(Map:arg) return Ref<Map>:arg;
stock Ref<LinkedList>:operator=(LinkedList:arg) return Ref<LinkedList>:arg;
stock Ref<Iter>:operator=(Iter:arg) return Ref<Iter>:arg;

stock Ref:operator=(Ref<String>:arg) return Ref:arg;
stock Ref:operator=(Ref<ConstString>:arg) return Ref:arg;
stock Ref:operator=(Ref<Variant>:arg) return Ref:arg;
stock Ref:operator=(Ref<ConstVariant>:arg) return Ref:arg;
stock Ref:operator=(Ref<List>:arg) return Ref:arg;
stock Ref:operator=(Ref<Map>:arg) return Ref:arg;
stock Ref:operator=(Ref<LinkedList>:arg) return Ref:arg;
stock Ref:operator=(Ref<Iter>:arg) return Ref:arg;

#endif

/** PawnPlus v0.8 by IllidanS4 **/
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

#define GlobalString String@Global
#define GlobalVariant Variant@Global
#define List<%0> List@%0
#define Map<%0,%1> Map@%0@%1
#define GlobalIter Iter@Global
#define Iter<%0> Iter@%0
#define PairIter<%0,%1> Iter@%1@%0

#define PP_TAG_PREFIX@String<%0> %0@String
#define PP_TAG_PREFIX@Variant<%0> %0@Variant
#define PP_TAG_PREFIX@String@Global<%0> %0@String@Global
#define PP_TAG_PREFIX@Variant@Global<%0> %0@Variant@Global
#define PP_TAG_PREFIX@GlobalString<%0> %0@String@Global
#define PP_TAG_PREFIX@GlobalVariant<%0> %0@Variant@Global
#define PP_TAG_PREFIX@List<%0> %0@List
#define PP_TAG_PREFIX@Map<%0> %0@Map
#define PP_TAG_PREFIX@Iter<%0> %0@Iter
#define PP_TAG_PREFIX@Iter@Global<%0> %0@Iter@Global
#define PP_TAG_PREFIX@GlobalIter<%0> %0@Iter@Global
#define Ref<%0> PP_TAG_PREFIX@%0<Ref>

#define StringTags String,GlobalString
#define StringTag {StringTags}
#define VariantTags Variant,GlobalVariant
#define VariantTag {VariantTags}
#define IterTags Iter,GlobalIter
#define IterTag {IterTags}

#define TagTags _,tag_uid
#define TagTag {TagTags}

#define PP_ALL_TAGS _,bool,Float,VariantTags,StringTags,List,Map
#if defined PP_ADDITIONAL_TAGS
#define AnyTags PP_ALL_TAGS,PP_ADDITIONAL_TAGS
#else
#define AnyTags PP_ALL_TAGS
#endif
#define AnyTag {AnyTags}

#if defined PP_SYNTAX
#define PP_SYNTAX_AMX_FORKED
#define PP_SYNTAX_@
#define PP_SYNTAX_@@
#define PP_SYNTAX_AWAIT
#define PP_SYNTAX_YIELD
#define PP_SYNTAX_THREADED
#define PP_SYNTAX_FOR_LIST
#define PP_SYNTAX_FOR_MAP
#define PP_SYNTAX_STRING_OP
#endif


/*                 */
/*  Configuration  */
/*                 */

native pp_hook_strlen(bool:hook);
native pp_hook_check_ref_args(bool:hook);

/*                 */
/*      Debug      */
/*                 */

native pp_num_tasks();
native pp_num_local_strings();
native pp_num_global_strings();
native pp_num_local_variants();
native pp_num_global_variants();
native pp_num_lists();
native pp_num_maps();
native pp_num_guards();
native String:pp_entry_s();
native pp_num_local_iters();
native pp_num_global_iters();
native pp_collect();

/*                 */
/*      Pawn       */
/*                 */

native pawn_call_native(const function[], const format[], AnyTag:...);
native pawn_call_public(const function[], const format[], AnyTag:...);
native CallbackHandler:pawn_register_callback(const callback[], const handler[], const additional_format[]="", AnyTag:...);
native pawn_unregister_callback(CallbackHandler:id);
native NativeHook:pawn_native_filter(const function[], const format[], bool:output, const handler[], const additional_format[]="", AnyTag:...);
native pawn_remove_hook(NativeHook:id);
native Guard:pawn_guard(AnyTag:value, TagTag:tag_id=tagof(value));
native Guard:pawn_guard_arr(AnyTag:value[], size=sizeof(value), TagTag:tag_id=tagof(value));
native bool:pawn_guard_valid(Guard:guard);
native bool:pawn_guard_free(Guard:guard);

/*                 */
/*      Tags       */
/*                 */

#define tag_uid_unknown (tag_uid:0)
#define tag_uid_cell (tag_uid:1)
#define tag_uid_bool (tag_uid:2)
#define tag_uid_char (tag_uid:3)
#define tag_uid_float (tag_uid:4)
#define tag_uid_string (tag_uid:5)
#define tag_uid_variant (tag_uid:6)
#define tag_uid_list (tag_uid:7)
#define tag_uid_map (tag_uid:8)
#define tag_uid_iter (tag_uid:9)
#define tag_uid_ref (tag_uid:10)
#define tag_uid_task (tag_uid:11)

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
    tag_op_delete_deep = 22,
    tag_op_collect = 23,
    tag_op_copy = 24,
    tag_op_clone = 25,
    tag_op_assign = 26,
    tag_op_init = 27,
    tag_op_hash = 28,
}

/*                 */
/*       AMX       */
/*                 */

enum fork_level
{
    fork_exec = 0,
    fork_data = 1,
    fork_machine = 2,
};

native AMX:amx_this();
native Var:amx_var(&AnyTag:var);
native Var:amx_var_arr(AnyTag:arr[], size=sizeof(arr));
native Var:amx_public_var(AMX:amx, const name[]);
native bool:amx_valid(Var:var);
native bool:amx_delete(Var:var);
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
native amx_end_fork();
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

enum amx_err
{
    amx_err_exit = 1,
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
};

#if defined PP_SYNTAX_AMX_FORKED

#define amx_forked(%0) for(new PP@af:_@pp_amx_forked=PP@af:amx_fork(%0);_@pp_amx_forked;_@pp_amx_forked=PP@af:amx_end_fork())
stock operator~(PP@af:arr[], count)
{
    for(new i = 0; i < count; i++)
    {
        if(arr[i]) amx_end_fork();
    }
}

#endif


/*                 */
/*     Strings     */
/*                 */

#define STRING_NULL (GlobalString:0)
#define INVALID_CHAR (0xFFFFFF00)

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
native AmxStringBuffer:str_buf_addr(StringTag:str);
native GlobalString:str_to_global(StringTag:str);
native String:str_to_local(StringTag:str);
native bool:str_delete(StringTag:str);
native bool:str_valid(StringTag:str);
native String:str_clone(StringTag:str);

native str_len(StringTag:str);
native str_get(StringTag:str, buffer[], size=sizeof(buffer), start=0, end=cellmax);
native str_getc(StringTag:str, pos);
native str_setc(StringTag:str, pos, value);
native str_cmp(StringTag:str1, StringTag:str2);
native bool:str_empty(StringTag:str);
native bool:str_eq(StringTag:str1, StringTag:str2);
native str_findc(StringTag:str, value, offset=0);
native str_find(StringTag:str, StringTag:value, offset=0);

native String:str_cat(StringTag:str1, StringTag:str2);
native String:str_sub(StringTag:str, start=0, end=cellmax);
native String:str_val(AnyTag:value, TagTag:tag_id=tagof(value));
native String:str_val_arr(AnyTag:value[], size=sizeof(value), TagTag:tag_id=tagof(value));
native List:str_split(StringTag:str, const delims[]);
native List:str_split_s(StringTag:str, StringTag:delims);
native String:str_join(List:list, const delim[]);
native String:str_join_s(List:list, StringTag:delim);
native String:str_to_lower(StringTag:str);
native String:str_to_upper(StringTag:str);

native String:str_set(StringTag:target, StringTag:other);
native String:str_append(StringTag:target, StringTag:other);
native String:str_del(StringTag:target, start=0, end=cellmax);
native String:str_clear(StringTag:str);
native String:str_resize(StringTag:str, size, padding=0);
native String:str_set_to_lower(StringTag:str);
native String:str_set_to_upper(StringTag:str);

native String:str_format(const format[], {StringTags,Float,_}:...);
native String:str_format_s(StringTag:format, {StringTags,Float,_}:...);
native String:str_set_format(StringTag:target, const format[], {StringTags,Float,_}:...);
native String:str_set_format_s(StringTag:target, StringTag:format, {StringTags,Float,_}:...);

#if defined PP_SYNTAX_@
#define @ str_new_static
#endif

#if defined PP_SYNTAX_@@
#define @@ str_val
#endif


/*                 */
/*     Variant     */
/*                 */

#define VAR_NULL (GlobalVariant:0)

native Variant:var_new(AnyTag:value, TagTag:tag_id=tagof(value));
native Variant:var_new_arr(const AnyTag:value[], size=sizeof(value), TagTag:tag_id=tagof(value));
native Variant:var_new_arr_2d(const AnyTag:value[][], size=sizeof(value), size2=sizeof(value[]), TagTag:tag_id=tagof(value));
native Variant:var_new_arr_3d(const AnyTag:value[][][], size=sizeof(value), size2=sizeof(value[]), size3=sizeof(value[][]), TagTag:tag_id=tagof(value));
native Variant:var_new_buf(size, TagTag:tag_id=0);
native Variant:var_new_str(const value[]);
native Variant:var_new_str_s(StringTag:value);
native Variant:var_new_var(VariantTag:value);
native GlobalVariant:var_to_global(VariantTag:var);
native Variant:var_to_local(VariantTag:var);
native bool:var_delete(VariantTag:var);
native bool:var_delete_deep(VariantTag:var);
native bool:var_valid(VariantTag:var);
native Variant:var_clone(VariantTag:var);

native var_get(VariantTag:var, const offsets[]={cellmin}, offsets_size=sizeof(offsets));
native var_get_arr(VariantTag:var, AnyTag:value[], const offsets[]={cellmin}, size=sizeof(value), offsets_size=sizeof(offsets));
native var_get_str(VariantTag:var, value[], const offsets[]={cellmin}, size=sizeof(value), offsets_size=sizeof(offsets)) = var_get_arr;
native String:var_get_str_s(VariantTag:var, const offsets[]={cellmin}, offsets_size=sizeof(offsets));
native bool:var_get_safe(VariantTag:var, &AnyTag:value, const offsets[]={cellmin}, offsets_size=sizeof(offsets), TagTag:tag_id=tagof(value));
native var_get_arr_safe(VariantTag:var, AnyTag:value[], const offsets[]={cellmin}, size=sizeof(value), offsets_size=sizeof(offsets), TagTag:tag_id=tagof(value));
native var_get_str_safe(VariantTag:var, value[], const offsets[]={cellmin}, size=sizeof(value), offsets_size=sizeof(offsets));
native String:var_get_str_safe_s(VariantTag:var, const offsets[]={cellmin}, offsets_size=sizeof(offsets));

native bool:var_set_cell(VariantTag:var, AnyTag:value, const offsets[]={cellmin}, offsets_size=sizeof(offsets));
native bool:var_set_cell_safe(VariantTag:var, AnyTag:value, const offsets[]={cellmin}, offsets_size=sizeof(offsets), TagTag:tag_id=tagof(value));

native var_tagof(VariantTag:var);
native tag_uid:var_tag_uid(VariantTag:var);
native var_sizeof(VariantTag:var, const offsets[]={cellmin}, offsets_size=sizeof(offsets));
native Variant:var_add(VariantTag:var1, VariantTag:var2);
native Variant:var_sub(VariantTag:var1, VariantTag:var2);
native Variant:var_mul(VariantTag:var1, VariantTag:var2);
native Variant:var_div(VariantTag:var1, VariantTag:var2);
native Variant:var_mod(VariantTag:var1, VariantTag:var2);
native Variant:var_neg(VariantTag:var);
native Variant:var_inc(VariantTag:var);
native Variant:var_dec(VariantTag:var);
native bool:var_eq(VariantTag:var1, VariantTag:var2);
native bool:var_neq(VariantTag:var1, VariantTag:var2);
native bool:var_lt(VariantTag:var1, VariantTag:var2);
native bool:var_gt(VariantTag:var1, VariantTag:var2);
native bool:var_lte(VariantTag:var1, VariantTag:var2);
native bool:var_gte(VariantTag:var1, VariantTag:var2);
native bool:var_not(VariantTag:var);
native Variant:var_call_op(VariantTag:var, tag_op:tag_op, AnyTag:...);
native Variant:var_call_op_raw(VariantTag:var, tag_op:tag_op, AnyTag:...);


/*                 */
/*      Lists      */
/*                 */

native List:list_new();
native List:list_new_arr(AnyTag:values[], size=sizeof(values), TagTag:tag_id=tagof(values));
native List:list_new_args_t(TagTag:tag_id=tagof(arg0), AnyTag:arg0, AnyTag:...) = list_new_args;
#define list_new_args(%0) list_new_args_t(_,%0)
native List:list_new_args_str(arg0[], ...);
native List:list_new_args_var(VariantTag:arg0, VariantTag:...);
native bool:list_valid(List:list);
native bool:list_delete(List:list);
native bool:list_delete_deep(List:list);
native list_size(List:list);
native bool:list_clear(List:list);
native List:list_clone(List:list);

native list_add(List:list, AnyTag:value, index=-1, TagTag:tag_id=tagof(value));
native list_add_arr(List:list, const AnyTag:value[], index=-1, size=sizeof(value), TagTag:tag_id=tagof(value));
native list_add_str(List:list, const value[], index=-1);
native list_add_var(List:list, VariantTag:value, index=-1);
native list_add_list(List:list, List:range, index=-1);
native list_add_args_t(TagTag:tag_id=tagof(arg0), List:list, AnyTag:arg0, AnyTag:...) = list_add_args;
#define list_add_args(%0) list_add_args_t(_,%0)
native list_add_args_str(List:list, arg0[], ...);
native list_add_args_var(List:list, VariantTag:arg0, VariantTag:...);

native bool:list_remove(List:list, index);

native list_get(List:list, index, offset=0);
native list_get_arr(List:list, index, AnyTag:value[], size=sizeof(value));
native Variant:list_get_var(List:list, index);
native bool:list_get_safe(List:list, index, &AnyTag:value, offset=0, TagTag:tag_id=tagof(value));
native list_get_arr_safe(List:list, index, AnyTag:value[], size=sizeof(value), TagTag:tag_id=tagof(value));

native bool:list_set(List:list, index, AnyTag:value, TagTag:tag_id=tagof(value));
native bool:list_set_arr(List:list, index, const AnyTag:value[], size=sizeof(value), TagTag:tag_id=tagof(value));
native bool:list_set_str(List:list, index, const value[]);
native bool:list_set_var(List:list, index, VariantTag:value);
native bool:list_set_cell(List:list, index, offset, AnyTag:value);
native bool:list_set_cell_safe(List:list, index, offset, AnyTag:value, TagTag:tag_id=tagof(value));

native list_tagof(List:list, index);
native list_sizeof(List:list, index);

native Iter:list_iter(List:list);

#define list_new<%0> list_new@%0
#define list_new_arr<%0> list_new_arr@%0
#define list_new_args_t<%0> list_new_args_t@%0
#define list_new_args_gen<%0>(%1) list_new_args_t<%0>(_,%1)
#define list_valid<%0> list_valid@%0
#define list_delete<%0> list_delete@%0
#define list_delete_deep<%0> list_delete_deep@%0
#define list_clone<%0> list_clone@%0
#define list_size<%0> list_size@%0
#define list_clear<%0> list_clear@%0

#define list_add<%0> list_add@%0
#define list_add_arr<%0> list_add_arr@%0
#define list_add_list<%0> list_add_list@%0
#define list_add_args_t<%0> list_add_args_t@%0
#define list_add_args_gen<%0>(%1) list_add_args_t<%0>(_,%1)

#define list_remove<%0> list_remove@%0

#define list_get<%0> list_get@%0
#define list_get_arr<%0> list_get_arr@%0

#define list_set<%0> list_set@%0
#define list_set_arr<%0> list_set_arr@%0
#define list_set_cell<%0> list_set_cell@%0

#define list_sizeof<%0> list_sizeof@%0

#define list_iter<%0> list_iter@%0

#define @define_List<%0> \
enum{Void:List<%0>};\
native List<%0>:list_new<%0>()=list_new;\
native List<%0>:list_new_arr<%0>(%0:v[],s=sizeof(v),t=(tagof(%0:)))=list_new_arr;\
native List<%0>:list_new_args_t<%0>(t=(tagof(%0:)),%0:a,%0:...)=list_new_args;\
native bool:list_valid<%0>(List<%0>:l)=list_valid;\
native bool:list_delete<%0>(List<%0>:l)=list_delete;\
native bool:list_delete_deep<%0>(List<%0>:l)=list_delete_deep;\
native List<%0>:list_clone<%0>(List<%0>:l)=list_clone;\
native list_size<%0>(List<%0>:l)=list_size;\
native bool:list_clear<%0>(List<%0>:l)=list_clear;\
native list_add<%0>(List<%0>:l,%0:v,i=-1,t=(tagof(%0:)))=list_add;\
native list_add_arr<%0>(List<%0>:l,const %0:v[],i=-1,s=sizeof(v),t=(tagof(%0:)))=list_add_arr;\
native list_add_list<%0>(List<%0>:l,List<%0>:l2,i=-1)=list_add_list;\
native list_add_args_t<%0>(t=(tagof(%0:)),%0:a,%0:...)=list_add_args;\
native bool:list_remove<%0>(List<%0>:l,i)=list_remove;\
native %0:list_get<%0>(List<%0>:l,i,o=0)=list_get;\
native list_get_arr<%0>(List<%0>:l,i,%0:v[],s=sizeof(v))=list_get_arr;\
native bool:list_set<%0>(List<%0>:l,i,%0:v,t=(tagof(%0:)))=list_set;\
native bool:list_set_arr<%0>(List<%0>:l,i,const %0:v[],s=sizeof(v),t=(tagof(%0:)))=list_set_arr;\
native bool:list_set_cell<%0>(List<%0>:l,i,o,%0:v)=list_set_cell;\
native list_sizeof<%0>(List<%0>:l,i)=list_sizeof;\
forward List:operator=(List<%0>:l);\
forward List<%0>:operator=(List:l);\
native Iter<%0>:list_iter<%0>(List<%0>:l)=list_iter;\
forward Iter<%0>:operator=(Iter:i);\
native iter_get<%0>(Iter<%0>:i,o=0)=iter_get;\
native iter_get_arr<%0>(Iter<%0>:i,%0:v[],s=sizeof(v))=iter_get_arr;\
native bool:iter_set<%0>(Iter<%0>:i,%0:v,t=(tagof(%0:)))=iter_set;\
native bool:iter_set_arr<%0>(Iter<%0>:i,const %0:v[],s=sizeof(v),t=(tagof(%0:)))=iter_set_arr;\
native bool:iter_set_cell<%0>(Iter<%0>:i,o,%0:v)=iter_set_cell;\
native bool:iter_insert<%0>(Iter<%0>:i,%0:v,t=(tagof(%0:)))=iter_insert;\
native bool:iter_insert_arr<%0>(Iter<%0>:i,const %0:v[],s=sizeof(v),t=(tagof(%0:)))=iter_insert_arr


/*                 */
/*       Maps      */
/*                 */

native Map:map_new();
native Map:map_new_args_t(TagTag:key_tag_id=tagof(arg0), TagTag:value_tag_id=tagof(arg1), AnyTag:arg0, AnyTag:arg1, AnyTag:...) = map_new_args;
#define map_new_args(%0) map_new_args_t(_,_,%0)
native Map:map_new_args_str_t(TagTag:key_tag_id=tagof(arg0), AnyTag:arg0, arg1[], AnyTag:...) = map_new_args_str;
#define map_new_args_str(%0) map_new_args_str_t(_,%0)
native Map:map_new_args_var_t(TagTag:key_tag_id=tagof(arg0), AnyTag:arg0, VariantTag:arg1, AnyTag:...) = map_new_args_var;
#define map_new_args_var(%0) map_new_args_var_t(_,%0)
native Map:map_new_str_args_t(TagTag:value_tag_id=tagof(arg1), arg0[], AnyTag:arg1, AnyTag:...) = map_new_str_args;
#define map_new_str_args(%0) map_new_str_args_t(_,%0)
native Map:map_new_str_args_str(arg0[], arg1[], ...);
native Map:map_new_str_args_var(arg0[], VariantTag:arg1, {_,VariantTags}:...);
native Map:map_new_var_args_t(TagTag:value_tag_id=tagof(arg1), VariantTag:arg0, AnyTag:arg1, AnyTag:...) = map_new_var_args;
#define map_new_var_args(%0) map_new_var_args_t(_,%0)
native Map:map_new_var_args_str(VariantTag:arg0, arg1[], {_,VariantTags}:...);
native Map:map_new_var_args_var(VariantTag:arg0, VariantTag:arg1, VariantTag:...);
native bool:map_valid(Map:map);
native bool:map_delete(Map:map);
native bool:map_delete_deep(Map:map);
native Map:map_clone(Map:map);
native map_size(Map:map);

native bool:map_add(Map:map, AnyTag:key, AnyTag:value, TagTag:key_tag_id=tagof(key), TagTag:value_tag_id=tagof(value));
native bool:map_add_arr(Map:map, AnyTag:key, const AnyTag:value[], value_size=sizeof(value), TagTag:key_tag_id=tagof(key), TagTag:value_tag_id=tagof(value));
native bool:map_add_str(Map:map, AnyTag:key, const value[], TagTag:key_tag_id=tagof(key));
native bool:map_add_var(Map:map, AnyTag:key, VariantTag:value, TagTag:key_tag_id=tagof(key));
native bool:map_arr_add(Map:map, const AnyTag:key[], AnyTag:value, key_size=sizeof(key), TagTag:key_tag_id=tagof(key), TagTag:value_tag_id=tagof(value));
native bool:map_arr_add_arr(Map:map, const AnyTag:key[], const AnyTag:value[], key_size=sizeof(key), value_size=sizeof(value), TagTag:key_tag_id=tagof(key), TagTag:value_tag_id=tagof(value));
native bool:map_arr_add_str(Map:map, const AnyTag:key[], const value[], key_size=sizeof(key), TagTag:key_tag_id=tagof(key));
native bool:map_arr_add_var(Map:map, const AnyTag:key[], VariantTag:value, key_size=sizeof(key), TagTag:key_tag_id=tagof(key));
native bool:map_str_add(Map:map, const key[], AnyTag:value, TagTag:value_tag_id=tagof(value));
native bool:map_str_add_arr(Map:map, const key[], const AnyTag:value[], value_size=sizeof(value), TagTag:value_tag_id=tagof(value));
native bool:map_str_add_str(Map:map, const key[], const value[]);
native bool:map_str_add_var(Map:map, const key[], VariantTag:value);
native bool:map_var_add(Map:map, VariantTag:key, AnyTag:value, TagTag:value_tag_id=tagof(value));
native bool:map_var_add_arr(Map:map, VariantTag:key, const AnyTag:value[], value_size=sizeof(value), TagTag:value_tag_id=tagof(value));
native bool:map_var_add_str(Map:map, VariantTag:key, const value[]);
native bool:map_var_add_var(Map:map, VariantTag:key, VariantTag:value);
native map_add_map(Map:map, Map:other, bool:overwrite);
native map_add_args_t(TagTag:key_tag_id=tagof(arg0), TagTag:value_tag_id=tagof(arg1), Map:map, AnyTag:arg0, AnyTag:arg1, AnyTag:...) = map_add_args;
#define map_add_args(%0) map_add_args_t(_,_,%0)
native map_add_args_str_t(TagTag:key_tag_id=tagof(arg0), Map:map, AnyTag:arg0, arg1[], AnyTag:...) = map_add_args_str;
#define map_add_args_str(%0) map_add_args_str_t(_,%0)
native map_add_args_var_t(TagTag:key_tag_id=tagof(arg0), Map:map, AnyTag:arg0, VariantTag:arg1, AnyTag:...) = map_add_args_var;
#define map_add_args_var(%0) map_add_args_var_t(_,%0)
native map_add_str_args_t(TagTag:value_tag_id=tagof(arg1), Map:map, arg0[], AnyTag:arg1, AnyTag:...) = map_add_str_args;
#define map_add_str_args(%0) map_add_str_args_t(_,%0)
native map_add_str_args_str(Map:map, arg0[], arg1[], ...);
native map_add_str_args_var(Map:map, arg0[], VariantTag:arg1, {_,VariantTags}:...);
native map_add_var_args_t(TagTag:value_tag_id=tagof(arg1), Map:map, VariantTag:arg0, AnyTag:arg1, AnyTag:...) = map_add_var_args;
#define map_add_var_args(%0) map_add_var_args_t(_,%0)
native map_add_var_args_str(Map:map, VariantTag:arg0, arg1[], {_,VariantTags}:...);
native map_add_var_args_var(Map:map, VariantTag:arg0, VariantTag:arg1, VariantTag:...);

native bool:map_remove(Map:map, AnyTag:key, TagTag:key_tag_id=tagof(key));
native bool:map_arr_remove(Map:map, const AnyTag:key[], key_size=sizeof(key), TagTag:key_tag_id=tagof(key));
native bool:map_str_remove(Map:map, const key[]);
native bool:map_var_remove(Map:map, VariantTag:key);

native bool:map_has_key(Map:map, AnyTag:key, TagTag:key_tag_id=tagof(key));
native bool:map_has_arr_key(Map:map, const AnyTag:key[], key_size=sizeof(key), TagTag:key_tag_id=tagof(key));
native bool:map_has_str_key(Map:map, const key[]);
native bool:map_has_var_key(Map:map, VariantTag:key);

native map_get(Map:map, AnyTag:key, offset=0, TagTag:key_tag_id=tagof(key));
native map_get_arr(Map:map, AnyTag:key, AnyTag:value[], value_size=sizeof(value), TagTag:key_tag_id=tagof(key));
native Variant:map_get_var(Map:map, AnyTag:key, TagTag:key_tag_id=tagof(key));
native bool:map_get_safe(Map:map, AnyTag:key, &AnyTag:value, offset=0, TagTag:key_tag_id=tagof(key), TagTag:value_tag_id=tagof(value));
native map_get_arr_safe(Map:map, AnyTag:key, AnyTag:value[], value_size=sizeof(value), TagTag:key_tag_id=tagof(key), TagTag:value_tag_id=tagof(value));
native map_arr_get(Map:map, const AnyTag:key[], offset=0, key_size=sizeof(key), TagTag:key_tag_id=tagof(key));
native map_arr_get_arr(Map:map, const AnyTag:key[], AnyTag:value[], value_size=sizeof(value), key_size=sizeof(key), TagTag:key_tag_id=tagof(key));
native Variant:map_arr_get_var(Map:map, const AnyTag:key[], key_size=sizeof(key), TagTag:key_tag_id=tagof(key));
native bool:map_arr_get_safe(Map:map, const AnyTag:key[], &AnyTag:value, offset=0, key_size=sizeof(key), TagTag:key_tag_id=tagof(key), TagTag:value_tag_id=tagof(value));
native map_arr_get_arr_safe(Map:map, const AnyTag:key[], AnyTag:value[], value_size=sizeof(value), key_size=sizeof(key), TagTag:key_tag_id=tagof(key), TagTag:value_tag_id=tagof(value));
native map_str_get(Map:map, const key[], offset=0);
native map_str_get_arr(Map:map, const key[], AnyTag:value[], value_size=sizeof(value));
native Variant:map_str_get_var(Map:map, const key[]);
native bool:map_str_get_safe(Map:map, const key[], &AnyTag:value, offset=0, TagTag:value_tag_id=tagof(value));
native map_str_get_arr_safe(Map:map, const key[], AnyTag:value[], value_size=sizeof(value), TagTag:value_tag_id=tagof(value));
native map_var_get(Map:map, VariantTag:key, offset=0);
native map_var_get_arr(Map:map, VariantTag:key, AnyTag:value[], value_size=sizeof(value));
native Variant:map_var_get_var(Map:map, VariantTag:key);
native bool:map_var_get_safe(Map:map, VariantTag:key, &AnyTag:value, offset=0, TagTag:value_tag_id=tagof(value));
native map_var_get_arr_safe(Map:map, VariantTag:key, AnyTag:value[], value_size=sizeof(value), TagTag:value_tag_id=tagof(value));

native bool:map_set(Map:map, AnyTag:key, AnyTag:value, TagTag:key_tag_id=tagof(key), TagTag:value_tag_id=tagof(value));
native bool:map_set_arr(Map:map, AnyTag:key, const AnyTag:value[], value_size=sizeof(value), TagTag:key_tag_id=tagof(key), TagTag:value_tag_id=tagof(value));
native bool:map_set_str(Map:map, AnyTag:key, const value[], TagTag:key_tag_id=tagof(key));
native bool:map_set_var(Map:map, AnyTag:key, VariantTag:value, TagTag:key_tag_id=tagof(key));
native bool:map_set_cell(Map:map, AnyTag:key, offset, AnyTag:value, TagTag:key_tag_id=tagof(key));
native bool:map_set_cell_safe(Map:map, AnyTag:key, offset, AnyTag:value, TagTag:key_tag_id=tagof(key), TagTag:value_tag_id=tagof(value));
native bool:map_arr_set(Map:map, const AnyTag:key[], AnyTag:value, key_size=sizeof(key), TagTag:key_tag_id=tagof(key), TagTag:value_tag_id=tagof(value));
native bool:map_arr_set_arr(Map:map, const AnyTag:key[], const AnyTag:value[], value_size=sizeof(value), key_size=sizeof(key), TagTag:key_tag_id=tagof(key), TagTag:value_tag_id=tagof(value));
native bool:map_arr_set_str(Map:map, const AnyTag:key[], const value[], key_size=sizeof(key), TagTag:key_tag_id=tagof(key));
native bool:map_arr_set_var(Map:map, const AnyTag:key[], VariantTag:value, key_size=sizeof(key), TagTag:key_tag_id=tagof(key));
native bool:map_arr_set_cell(Map:map, const AnyTag:key[], offset, AnyTag:value, key_size=sizeof(key), TagTag:key_tag_id=tagof(key));
native bool:map_arr_set_cell_safe(Map:map, const AnyTag:key[], offset, AnyTag:value, key_size=sizeof(key), TagTag:key_tag_id=tagof(key), TagTag:value_tag_id=tagof(value));
native bool:map_str_set(Map:map, const key[], AnyTag:value, TagTag:value_tag_id=tagof(value));
native bool:map_str_set_arr(Map:map, const key[], const AnyTag:value[], value_size=sizeof(value), TagTag:value_tag_id=tagof(value));
native bool:map_str_set_str(Map:map, const key[], const value[]);
native bool:map_str_set_var(Map:map, const key[], VariantTag:value);
native bool:map_str_set_cell(Map:map, const key[], offset, AnyTag:value);
native bool:map_str_set_cell_safe(Map:map, const key[], offset, AnyTag:value, TagTag:value_tag_id=tagof(value));
native bool:map_var_set(Map:map, VariantTag:key, AnyTag:value, TagTag:value_tag_id=tagof(value));
native bool:map_var_set_arr(Map:map, VariantTag:key, const AnyTag:value[], value_size=sizeof(value), TagTag:value_tag_id=tagof(value));
native bool:map_var_set_str(Map:map, VariantTag:key, const value[]);
native bool:map_var_set_var(Map:map, VariantTag:key, VariantTag:value);
native bool:map_var_set_cell(Map:map, VariantTag:key, offset, AnyTag:value);
native bool:map_var_set_cell_safe(Map:map, VariantTag:key, offset, AnyTag:value, TagTag:value_tag_id=tagof(value));

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

native map_tagof(Map:map, AnyTag:key, TagTag:key_tag_id=tagof(key));
native map_sizeof(Map:map, AnyTag:key, TagTag:key_tag_id=tagof(key));
native map_arr_tagof(Map:map, const AnyTag:key[], key_size=sizeof(key), TagTag:key_tag_id=tagof(key));
native map_arr_sizeof(Map:map, const AnyTag:key[], key_size=sizeof(key), TagTag:key_tag_id=tagof(key));
native map_str_tagof(Map:map, const key[]);
native map_str_sizeof(Map:map, const key[]);
native map_var_tagof(Map:map, VariantTag:key);
native map_var_sizeof(Map:map, VariantTag:key);

native Iter:map_iter(Map:map);

#define map_new<%0,%1> map_new@%0@%1
#define map_new_args_t<%0,%1> map_new_args_t@%0@%1
#define map_new_args_gen<%0,%1>(%2) map_new_args_t<%0,%1>(_,_,%2)
#define map_valid<%0,%1> map_valid@%0@%1
#define map_delete<%0,%1> map_delete@%0@%1
#define map_delete_deep<%0,%1> map_delete_deep@%0@%1
#define map_clone<%0,%1> map_clone@%0@%1
#define map_size<%0,%1> map_size@%0@%1

#define map_add<%0,%1> map_add@%0@%1
#define map_add_arr<%0,%1> map_add_arr@%0@%1
#define map_arr_add<%0,%1> map_arr_add@%0@%1
#define map_arr_add_arr<%0,%1> map_arr_add_arr@%0@%1
#define map_add_map<%0,%1> map_add_map@%0@%1
#define map_add_args_t<%0,%1> map_add_args_t@%0@%1
#define map_add_args_gen<%0,%1>(%2) map_add_args_t<%0,%1>(_,_,%2)

#define map_remove<%0,%1> map_remove@%0@%1
#define map_arr_remove<%0,%1> map_arr_remove@%0@%1

#define map_get<%0,%1> map_get@%0@%1
#define map_get_arr<%0,%1> map_get_arr@%0@%1
#define map_arr_get<%0,%1> map_arr_get@%0@%1
#define map_arr_get_arr<%0,%1> map_arr_get_arr@%0@%1

#define map_set<%0,%1> map_set@%0@%1
#define map_set_arr<%0,%1> map_set_arr@%0@%1
#define map_set_cell<%0,%1> map_set_cell@%0@%1
#define map_arr_set<%0,%1> map_arr_set@%0@%1
#define map_arr_set_arr<%0,%1> map_arr_set_arr@%0@%1
#define map_arr_set_cell<%0,%1> map_arr_set_cell@%0@%1

#define map_key_at<%0,%1> map_key_at@%0@%1
#define map_arr_key_at<%0,%1> map_arr_key_at@%0@%1

#define map_value_at<%0,%1> map_value_at@%0@%1
#define map_arr_value_at<%0,%1> map_arr_value_at@%0@%1

#define map_sizeof<%0,%1> map_sizeof@%0@%1
#define map_arr_sizeof<%0,%1> map_arr_sizeof@%0@%1

#define map_iter<%0,%1> map_iter@%0@%1

#define @define_Map<%0,%1> \
enum{Void:Map<%0,%1>};\
native Map<%0,%1>:map_new<%0,%1>()=map_new;\
native Map<%0,%1>:map_new_args_t<%0,%1>(kt=(tagof(%0:)),vt=(tagof(%1:)),%0:a0,%1:a1,{%0,%1}:...)=map_new_args;\
native bool:map_valid<%0,%1>(Map<%0,%1>:m)=map_valid;\
native bool:map_delete<%0,%1>(Map<%0,%1>:m)=map_delete;\
native bool:map_delete_deep<%0,%1>(Map<%0,%1>:m)=map_delete_deep;\
native Map<%0,%1>:map_clone<%0,%1>(Map<%0,%1>:m)=map_clone;\
native map_size<%0,%1>(Map<%0,%1>:m)=map_size;\
native bool:map_add<%0,%1>(Map<%0,%1>:m,%0:k,%1:v,kt=(tagof(%0:)),vt=(tagof(%1:)))=map_add;\
native bool:map_add_arr<%0,%1>(Map<%0,%1>:m,%0:k,const %1:v[],vs=sizeof(v),kt=(tagof(%0:)),vt=(tagof(%1:)))=map_add_arr;\
native bool:map_arr_add<%0,%1>(Map<%0,%1>:m,const %0:k[],%1:v,ks=sizeof(k),kt=(tagof(%0:)),vt=(tagof(%1:)))=map_arr_add;\
native bool:map_arr_add_arr<%0,%1>(Map<%0,%1>:m,const %0:k[],const %1:v[],ks=sizeof(k),vs=sizeof(v),kt=(tagof(%0:)),vt=(tagof(%1:)))=map_arr_add_arr;\
native map_add_map<%0,%1>(Map<%0,%1>:m,Map<%0,%1>:m2,bool:ow);\
native map_add_args_t<%0,%1>(kt=(tagof(%0:)),vt=(tagof(%1:)),Map<%0,%1>:m,%0:a0,%1:a1,{%0,%1}:...)=map_add_args;\
native bool:map_remove<%0,%1>(Map<%0,%1>:m,%0:k,kt=(tagof(%0:)))=map_remove;\
native bool:map_arr_remove<%0,%1>(Map<%0,%1>:m,const %0:k[],ks=sizeof(k),kt=(tagof(%0:)))=map_arr_remove;\
native %0:map_get<%0,%1>(Map<%0,%1>:m,%0:k,o=0,kt=(tagof(%0:)))=map_get;\
native map_get_arr<%0,%1>(Map<%0,%1>:m,%0:k,%1:v[],vs=sizeof(v),kt=(tagof(%0:)))=map_get_arr;\
native map_arr_get<%0,%1>(Map<%0,%1>:m,const %0:k[],o=0,ks=sizeof(k),kt=(tagof(%0:)))=map_arr_get;\
native map_arr_get_arr<%0,%1>(Map<%0,%1>:m,const %0:k[],%1:v[],vs=sizeof(v),ks=sizeof(k),kt=(tagof(%0:)))=map_arr_get_arr;\
native bool:map_set<%0,%1>(Map<%0,%1>:m,%0:k,%1:v,kt=(tagof(%0:)),vt=(tagof(%1:)))=map_set;\
native bool:map_set_arr<%0,%1>(Map<%0,%1>:m,%0:k,const %1:v[],vs=sizeof(v),kt=(tagof(%0:)),vt=(tagof(%1:)))=map_set_arr;\
native bool:map_set_cell<%0,%1>(Map<%0,%1>:m,%0:k,o,%1:v,kt=(tagof(%0:)))=map_set_cell;\
native bool:map_arr_set<%0,%1>(Map<%0,%1>:m,const %0:k[],%1:v,ks=sizeof(k),kt=(tagof(%0:)),vt=(tagof(%1:)))=map_arr_set;\
native bool:map_arr_set_arr<%0,%1>(Map<%0,%1>:m,const %0:k[],const %1:v[],vs=sizeof(v),ks=sizeof(k),kt=(tagof(%0:)),vt=(tagof(%1:)))=map_arr_set_arr;\
native bool:map_arr_set_cell<%0,%1>(Map<%0,%1>:m,const %0:k[],o,%1:v,ks=sizeof(k),kt=(tagof(%0:)))=map_arr_set_cell;\
native %0:map_key_at<%0,%1>(Map<%0,%1>:m,i,o=0)=map_key_at;\
native map_arr_key_at<%0,%1>(Map<%0,%1>:m,i,%0:k[],ks=sizeof(k))=map_arr_key_at;\
native %1:map_value_at<%0,%1>(Map<%0,%1>:m,i,o=0)=map_value_at;\
native map_arr_value_at<%0,%1>(Map<%0,%1>:m,i,%1:v[],vs=sizeof(v))=map_arr_value_at;\
native map_sizeof<%0,%1>(Map<%0,%1>:m,%0:k,kt=(tagof(%0:)))=map_sizeof;\
native map_arr_sizeof<%0,%1>(Map<%0,%1>:m,const %0:k[],ks=sizeof(k),kt=(tagof(%0:)))=map_arr_sizeof;\
forward Map:operator=(Map<%0,%1>:m);\
forward Map<%0,%1>:operator=(Map:m);\
native PairIter<%0,%1>:map_iter<%0,%1>(Map<%0,%1>:m)=map_iter;\
forward PairIter<%0,%1>:operator=(Iter:i);\
forward PairIter<%0,%1>:operator=(Iter<%0>:i);\
native %0:iter_get_key<%0,%1>(PairIter<%0,%1>:i,o=0)=iter_get_key;\
native iter_get_key_arr<%0,%1>(PairIter<%0,%1>:i,%0:v[],s=sizeof(v))=iter_get_key_arr;\
native %1:iter_get_value<%0,%1>(PairIter<%0,%1>:i,o=0)=iter_get;\
native iter_get_value_arr<%0,%1>(PairIter<%0,%1>:i,%1:v[],s=sizeof(v))=iter_get_arr;\
native bool:iter_set_value<%0,%1>(PairIter<%0,%1>:i,%1:v,t=(tagof(%0:)))=iter_set;\
native bool:iter_set_value_arr<%0,%1>(PairIter<%0,%1>:i,const %1:v[],s=sizeof(v),t=tagof(v))=iter_set_arr;\
native bool:iter_set_value_cell<%0,%1>(PairIter<%0,%1>:i,o,%1:v)=iter_set_cell

/*                 */
/*    Iterators    */
/*                 */

native bool:iter_valid(IterTag:iter);
native GlobalIter:iter_to_global(IterTag:iter);
native Iter:iter_to_local(IterTag:iter);
native bool:iter_delete(IterTag:iter);
native bool:iter_linked(IterTag:iter);
native bool:iter_inside(IterTag:iter);
native Iter:iter_erase(IterTag:iter);
native Iter:iter_reset(IterTag:iter);
native Iter:iter_clone(IterTag:iter);
native Iter:iter_move_next(IterTag:iter);
native Iter:iter_move_previous(IterTag:iter);
native Iter:iter_to_first(IterTag:iter);
native Iter:iter_to_last(IterTag:iter);
native bool:iter_eq(IterTag:iter1, IterTag:iter2);

native iter_get(IterTag:iter, offset=0);
native iter_get_arr(IterTag:iter, AnyTag:value[], size=sizeof(value));
native Variant:iter_get_var(IterTag:iter);
native bool:iter_get_safe(IterTag:iter, &AnyTag:value, offset=0, TagTag:tag_id=tagof(value));
native iter_get_arr_safe(IterTag:iter, AnyTag:value[], size=sizeof(value), TagTag:tag_id=tagof(value));

native bool:iter_set(IterTag:iter, AnyTag:value, TagTag:tag_id=tagof(value));
native bool:iter_set_arr(IterTag:iter, const AnyTag:value[], size=sizeof(value), TagTag:tag_id=tagof(value));
native bool:iter_set_str(IterTag:iter, const value[]);
native bool:iter_set_var(IterTag:iter, VariantTag:value);
native bool:iter_set_cell(IterTag:iter, offset, AnyTag:value);
native bool:iter_set_cell_safe(IterTag:iter, offset, AnyTag:value, TagTag:tag_id=tagof(value));

native bool:iter_insert(IterTag:iter, AnyTag:value, TagTag:tag_id=tagof(value));
native bool:iter_insert_arr(IterTag:iter, const AnyTag:value[], size=sizeof(value), TagTag:tag_id=tagof(value));
native bool:iter_insert_str(IterTag:iter, const value[]);
native bool:iter_insert_var(IterTag:iter, VariantTag:value);

native iter_get_key(IterTag:iter, offset=0);
native iter_get_key_arr(IterTag:iter, AnyTag:value[], size=sizeof(value));
native Variant:iter_get_key_var(IterTag:iter);
native bool:iter_get_key_safe(IterTag:iter, &AnyTag:value, offset=0, TagTag:tag_id=tagof(value));
native iter_get_key_arr_safe(IterTag:iter, AnyTag:value[], size=sizeof(value), TagTag:tag_id=tagof(value));

native iter_get_value(IterTag:iter, offset=0) = iter_get;
native iter_get_value_arr(IterTag:iter, AnyTag:value[], size=sizeof(value)) = iter_get_arr;
native Variant:iter_get_value_var(IterTag:iter) = iter_get_var;
native bool:iter_get_value_safe(IterTag:iter, &AnyTag:value, offset=0, TagTag:tag_id=tagof(value)) = iter_get_safe;
native iter_get_value_arr_safe(IterTag:iter, AnyTag:value[], size=sizeof(value), TagTag:tag_id=tagof(value)) = iter_get_arr_safe;

native bool:iter_set_value(IterTag:iter, AnyTag:value, TagTag:value_tag_id=tagof(value)) = iter_set;
native bool:iter_set_value_arr(IterTag:iter, const AnyTag:value[], size=sizeof(value), TagTag:value_tag_id=tagof(value)) = iter_set_arr;
native bool:iter_set_value_str(IterTag:iter, const value[]) = iter_set_str;
native bool:iter_set_value_var(IterTag:iter, VariantTag:value) = iter_set_var;
native bool:iter_set_value_cell(IterTag:iter, offset, AnyTag:value) = iter_set_cell;
native bool:iter_set_value_cell_safe(IterTag:iter, offset, AnyTag:value, TagTag:value_tag_id=tagof(value)) = iter_set_cell_safe;

#define iter_get<%0> iter_get@%0
#define iter_get_arr<%0> iter_get_arr@%0
#define iter_set<%0> iter_set@%0
#define iter_set_arr<%0> iter_set_arr@%0
#define iter_set_cell<%0> iter_set_cell@%0
#define iter_insert<%0> iter_insert@%0
#define iter_insert_arr<%0> iter_set_insert@%0
#define iter_get_key<%0,%1> iter_get_key@%0@%1
#define iter_get_key_arr<%0,%1> iter_get_key_arr@%0@%1
#define iter_get_value<%0,%1> iter_get_value@%0@%1
#define iter_get_value_arr<%0,%1> iter_get_value_arr@%0@%1
#define iter_set_value<%0,%1> iter_set_value@%0@%1
#define iter_set_value_arr<%0,%1> iter_set_value_arr@%0@%1
#define iter_set_value_cell<%0,%1> iter_set_value_cell@%0@%1

#if defined PP_SYNTAX_FOR_LIST
#define for_list(%0:%1) for(new Iter:%0=list_iter(%1);iter_inside(%0);iter_move_next(%0))
#define for_list_of<%2>(%0:%1) for(new Iter<%2>:%0=list_iter<%2>(%1);iter_inside(Iter:%0);iter_move_next(Iter:%0))
#endif

#if defined PP_SYNTAX_FOR_MAP
#define for_map(%0:%1) for(new Iter:%0=map_iter(%1);iter_inside(%0);iter_move_next(%0))
#define for_map_of<%2,%3>(%0:%1) for(new PairIter<%2,%3>:%0=map_iter<%2,%3>(%1);iter_inside(Iter:%0);iter_move_next(Iter:%0))
#endif

/*                 */
/* Tasks and async */
/*                 */

native wait_ticks(ticks);
native wait_ms(interval);
native Task:task_new();
native bool:task_delete(Task:task);
native bool:task_valid(Task:task);
native Task:task_keep(Task:task, bool:keep=true);
native task_set_result(Task:task, AnyTag:result);
native task_get_result(Task:task);
native task_set_error(Task:task, amx_err:error);
native amx_err:task_get_error(Task:task);
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
stock task_await(Task:task)
{
    task_wait(task);
    return task_get_result(task);
}
native task_yield(AnyTag:value);

#define task_state_default (task_state:0)
#define task_state_completed (task_state:1)
#define task_state_faulted (task_state:2)

#if defined PP_SYNTAX_AWAIT
#define await%9\32;%0; task_await(%0);
#endif

#if defined PP_SYNTAX_YIELD
#define yield%9\32;%0; task_yield(%0);
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
native thread_id(id[], size=sizeof id);
native String:thread_id_s();

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
/*  Operators etc. */
/*                 */

#define PP_PROHIBIT_UNARY_OPERATORS(%0) \
    forward operator-(%0:arg); \
    forward %0:operator=(_:arg); \
    forward %0:operator=(Float:arg)

PP_PROHIBIT_UNARY_OPERATORS(String);
PP_PROHIBIT_UNARY_OPERATORS(GlobalString);
PP_PROHIBIT_UNARY_OPERATORS(AmxString);
PP_PROHIBIT_UNARY_OPERATORS(AmxStringBuffer);

forward String:operator=(AmxString:arg);
forward GlobalString:operator=(AmxString:arg);
forward String:operator=(AmxStringBuffer:arg);
forward GlobalString:operator=(AmxStringBuffer:arg);

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
    PP_PROHIBIT_BINARY_OPEARTORS(%0,GlobalString); \
    PP_PROHIBIT_BINARY_OPEARTORS(%0,AmxString); \
    PP_PROHIBIT_BINARY_OPEARTORS(%0,AmxStringBuffer)

PP_PROHIBIT_BINARY_OPEARTORS2(String);
PP_PROHIBIT_BINARY_OPEARTORS2(GlobalString);
PP_PROHIBIT_BINARY_OPEARTORS2(AmxString);
PP_PROHIBIT_BINARY_OPEARTORS2(AmxStringBuffer);

#undef PP_PROHIBIT_BINARY_OPEARTORS

native GlobalString:operator=(String:arg) = str_to_global;
stock String:operator=(GlobalString:arg) return String:arg;
native AmxString:operator=(String:arg) = str_addr;
native AmxString:operator=(GlobalString:arg) = str_addr;
native AmxStringBuffer:operator=(String:arg) = str_buf_addr;
native AmxStringBuffer:operator=(GlobalString:arg) = str_buf_addr;

stock AmxStringBuffer:operator+(AmxStringBuffer:arg1, arg2) return AmxStringBuffer:(_:arg1+arg2);

native String:operator+(String:arg1, String:arg2) = str_cat;
native String:operator+(String:arg1, GlobalString:arg2) = str_cat;
native String:operator+(GlobalString:arg1, String:arg2) = str_cat;
native String:operator+(GlobalString:arg1, GlobalString:arg2) = str_cat;
native bool:operator==(String:arg1, String:arg2) = str_eq;
native bool:operator==(String:arg1, GlobalString:arg2) = str_eq;
native bool:operator==(GlobalString:arg1, String:arg2) = str_eq;
native bool:operator==(GlobalString:arg1, GlobalString:arg2) = str_eq;
stock bool:operator!=(String:arg1, String:arg2) return !(arg1 == arg2);
stock bool:operator!=(String:arg1, GlobalString:arg2) return !(arg1 == arg2);
stock bool:operator!=(GlobalString:arg1, String:arg2) return !(arg1 == arg2);
stock bool:operator!=(GlobalString:arg1, GlobalString:arg2) return !(arg1 == arg2);

native String:operator%(String:arg1, String:arg2) = str_cat;
native String:operator%(GlobalString:arg1, String:arg2) = str_cat;
native String:operator%(GlobalString:arg1, GlobalString:arg2) = str_cat;

#if defined PP_SYNTAX_STRING_OP

stock String:operator=(arg) return str_val(arg);
stock String:operator=(Float:arg) return str_val(arg);
stock GlobalString:operator=(arg) return str_to_global(str_val(arg));
stock GlobalString:operator=(Float:arg) return str_to_global(str_val(arg));
stock String:operator%(String:arg1, arg2) return str_cat(arg1, str_val(arg2));
stock String:operator%(GlobalString:arg1, arg2) return str_cat(arg1, str_val(arg2));
stock String:operator%(arg1, String:arg2) return str_cat(str_val(arg1), arg2);
stock String:operator%(arg1, GlobalString:arg2) return str_cat(str_val(arg1), arg2);
stock String:operator%(String:arg1, Float:arg2) return str_cat(arg1, str_val(arg2));
stock String:operator%(GlobalString:arg1, Float:arg2) return str_cat(arg1, str_val(arg2));
stock String:operator%(Float:arg1, String:arg2) return str_cat(str_val(arg1), arg2);
stock String:operator%(Float:arg1, GlobalString:arg2) return str_cat(str_val(arg1), arg2);

#endif

native GlobalVariant:operator=(Variant:arg) = var_to_global;
stock Variant:operator=(GlobalVariant:arg) return Variant:arg;

#define PP_VARIANT_BIN_OP(%0,%1) \
    native Variant:operator%0(Variant:arg1, Variant:arg2) = %1; \
    native Variant:operator%0(Variant:arg1, GlobalVariant:arg2) = %1; \
    native Variant:operator%0(GlobalVariant:arg1, Variant:arg2) = %1; \
    native Variant:operator%0(GlobalVariant:arg1, GlobalVariant:arg2) = %1

#define PP_VARIANT_UN_OP(%0,%1) \
    native Variant:operator%0(Variant:arg) = %1; \
    native Variant:operator%0(GlobalVariant:arg) = %1

#define PP_VARIANT_LOG_BIN_OP(%0,%1) \
    native bool:operator%0(Variant:arg1, Variant:arg2) = %1; \
    native bool:operator%0(Variant:arg1, GlobalVariant:arg2) = %1; \
    native bool:operator%0(GlobalVariant:arg1, Variant:arg2) = %1; \
    native bool:operator%0(GlobalVariant:arg1, GlobalVariant:arg2) = %1

#define PP_VARIANT_LOG_UN_OP(%0,%1) \
    native bool:operator%0(Variant:arg) = %1; \
    native bool:operator%0(GlobalVariant:arg) = %1

PP_VARIANT_BIN_OP(+, var_add);
PP_VARIANT_BIN_OP(-, var_sub);
PP_VARIANT_BIN_OP(*, var_mul);
PP_VARIANT_BIN_OP(/, var_div);
PP_VARIANT_BIN_OP(%, var_mod);
PP_VARIANT_UN_OP(-, var_neg);
PP_VARIANT_LOG_BIN_OP(==, var_eq);
PP_VARIANT_LOG_BIN_OP(!=, var_neq);
PP_VARIANT_LOG_BIN_OP(<, var_lt);
PP_VARIANT_LOG_BIN_OP(>, var_gt);
PP_VARIANT_LOG_BIN_OP(<=, var_lte);
PP_VARIANT_LOG_BIN_OP(>=, var_gte);
PP_VARIANT_LOG_UN_OP(!, var_not);

#undef PP_VARIANT_BIN_OP
#undef PP_VARIANT_UN_OP
#undef PP_VARIANT_LOG_BIN_OP
#undef PP_VARIANT_LOG_UN_OP

native bool:operator==(Iter:arg1, Iter:arg2) = iter_eq;
native bool:operator==(Iter:arg1, GlobalIter:arg2) = iter_eq;
native bool:operator==(GlobalIter:arg1, Iter:arg2) = iter_eq;
native bool:operator==(GlobalIter:arg1, GlobalIter:arg2) = iter_eq;
stock bool:operator!=(Iter:arg1, Iter:arg2) return !(arg1 == arg2);
stock bool:operator!=(Iter:arg1, GlobalIter:arg2) return !(arg1 == arg2);
stock bool:operator!=(GlobalIter:arg1, Iter:arg2) return !(arg1 == arg2);
stock bool:operator!=(GlobalIter:arg1, GlobalIter:arg2) return !(arg1 == arg2);

stock String:operator=(Ref<String>:arg) return String:arg;
stock GlobalString:operator=(Ref<GlobalString>:arg) return GlobalString:arg;
stock Variant:operator=(Ref<Variant>:arg) return Variant:arg;
stock GlobalVariant:operator=(Ref<GlobalVariant>:arg) return GlobalVariant:arg;
stock List:operator=(Ref<List>:arg) return List:arg;
stock Map:operator=(Ref<Map>:arg) return Map:arg;
stock Iter:operator=(Ref<Iter>:arg) return Iter:arg;
stock GlobalIter:operator=(Ref<GlobalIter>:arg) return GlobalIter:arg;

stock Ref<String>:operator=(String:arg) return Ref<String>:arg;
stock Ref<GlobalString>:operator=(GlobalString:arg) return Ref<GlobalString>:arg;
stock Ref<Variant>:operator=(Variant:arg) return Ref<Variant>:arg;
stock Ref<GlobalVariant>:operator=(GlobalVariant:arg) return Ref<GlobalVariant>:arg;
stock Ref<List>:operator=(List:arg) return Ref<List>:arg;
stock Ref<Map>:operator=(Map:arg) return Ref<Map>:arg;
stock Ref<Iter>:operator=(Iter:arg) return Ref<Iter>:arg;
stock Ref<GlobalIter>:operator=(GlobalIter:arg) return Ref<GlobalIter>:arg;

stock Ref:operator=(Ref<String>:arg) return Ref:arg;
stock Ref:operator=(Ref<GlobalString>:arg) return Ref:arg;
stock Ref:operator=(Ref<Variant>:arg) return Ref:arg;
stock Ref:operator=(Ref<GlobalVariant>:arg) return Ref:arg;
stock Ref:operator=(Ref<List>:arg) return Ref:arg;
stock Ref:operator=(Ref<Map>:arg) return Ref:arg;
stock Ref:operator=(Ref<Iter>:arg) return Ref:arg;
stock Ref:operator=(Ref<GlobalIter>:arg) return Ref:arg;

stock operator~(Guard:arr[], size)
{
    for(new i = 0; i < size; i++)
    {
        pawn_guard_free(arr[i]);
        arr[i] = Guard:0;
    }
}

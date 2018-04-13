/** PawnPlus v0.4 by IllidanS4 **/
//github.com/IllidanS4/PawnPlus

/*                 */
/*      Setup      */
/*                 */

#define GlobalString String@Global
#define GlobalVariant Variant@Global
#define List<%0> List@%0
#define Map<%0,%1> Map@%0@%1
#define ListIterator<%0> ListIterator@%0
#define MapIterator<%0,%1> MapIterator@%0@%1

#define PP_TAG_PREFIX@String<%0> %0@String
#define PP_TAG_PREFIX@Variant<%0> %0@Variant
#define PP_TAG_PREFIX@String@Global<%0> %0@String@Global
#define PP_TAG_PREFIX@Variant@Global<%0> %0@Variant@Global
#define PP_TAG_PREFIX@GlobalString<%0> %0@String@Global
#define PP_TAG_PREFIX@GlobalVariant<%0> %0@Variant@Global
#define Ref<%0> PP_TAG_PREFIX@%0<Ref>

#define StringTags String,GlobalString
#define StringTag {StringTags}
#define VariantTags Variant,GlobalVariant
#define VariantTag {VariantTags}

#define PP_ALL_TAGS _,bool,Float,VariantTags,StringTags,List,Map,Ref<String>,Ref<GlobalString>,Ref<Variant>,Ref<GlobalVariant>
#if defined PP_ADDITIONAL_TAGS
#define AnyTags PP_ALL_TAGS,PP_ADDITIONAL_TAGS
#else
#define AnyTags PP_ALL_TAGS
#endif
#define AnyTag {AnyTags}


/*                 */
/*  Configuration  */
/*                 */

native pp_hook_strlen(bool:hook);
native pp_hook_check_ref_args(bool:hook);

/*                 */
/*      Pawn       */
/*                 */

native pawn_call_native(const function[], const format[], AnyTag:...);
native pawn_call_public(const function[], const format[], AnyTag:...);
native callback:pawn_register_callback(const callback[], const handler[], const additional_format[], {_,Float}:...);
native pawn_unregister_callback(callback:id);


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
native bool:str_is_valid(StringTag:str);

native str_len(StringTag:str);
native str_get(StringTag:str, buffer[], size=sizeof(buffer), start=0, end=cellmax);
native str_getc(StringTag:str, pos);
native str_setc(StringTag:str, pos, value);
native str_cmp(StringTag:str1, StringTag:str2);
native bool:str_empty(StringTag:str);
native bool:str_equal(StringTag:str1, StringTag:str2);
native str_findc(StringTag:str, value, offset=0);
native str_find(StringTag:str, StringTag:value, offset=0);

native String:str_cat(StringTag:str1, StringTag:str2);
native String:str_sub(StringTag:str, start=0, end=cellmax);
native String:str_clone(StringTag:str);
native String:str_val(AnyTag:value, tag_id=tagof(value));
native String:str_val_arr(AnyTag:value[], size=sizeof(value), tag_id=tagof(value));

native String:str_set(StringTag:target, StringTag:other);
native String:str_append(StringTag:target, StringTag:other);
native String:str_del(StringTag:target, start=0, end=cellmax);
native String:str_clear(StringTag:str);
native String:str_resize(StringTag:str, size, padding=0);

native String:str_format(const format[], {StringTags,Float,_}:...);
native String:str_format_s(StringTag:format, {StringTags,Float,_}:...);
native String:str_set_format(StringTag:target, const format[], {StringTags,Float,_}:...);
native String:str_set_format_s(StringTag:target, StringTag:format, {StringTags,Float,_}:...);

#if !defined PP_NO_ALIASES

#define @ str_new_static
#if defined PP_MORE_ALIASES
#define @@ str_val
#endif

#endif


/*                 */
/*     Variant     */
/*                 */

#define VAR_NULL (GlobalVariant:0)

native Variant:var_new(AnyTag:value, tag_id=tagof(value));
native Variant:var_new_arr(const AnyTag:value[], size=sizeof(value), tag_id=tagof(value));
native Variant:var_new_str(const value[]);
native Variant:var_new_var(VariantTag:value);
native GlobalVariant:var_to_global(VariantTag:var);
native Variant:var_to_local(VariantTag:var);
native bool:var_delete(VariantTag:var);
native bool:var_is_valid(VariantTag:var);

native var_get(VariantTag:var, offset=0);
native var_get_arr(VariantTag:var, AnyTag:value[], size=sizeof(value));
native bool:var_get_safe(VariantTag:var, &AnyTag:value, offset=0, tag_id=tagof(value));
native var_get_arr_safe(VariantTag:var, AnyTag:value[], size=sizeof(value), tag_id=tagof(value));

native bool:var_set_cell(VariantTag:var, offset, AnyTag:value);
native bool:var_set_cell_safe(VariantTag:var, offset, AnyTag:value, tag_id=tagof(value));

native var_tagof(VariantTag:var);
native var_sizeof(VariantTag:var);
native bool:var_equal(VariantTag:var1, VariantTag:var2);
native Variant:var_add(VariantTag:var1, VariantTag:var2);
native Variant:var_sub(VariantTag:var1, VariantTag:var2);
native Variant:var_mul(VariantTag:var1, VariantTag:var2);
native Variant:var_div(VariantTag:var1, VariantTag:var2);
native Variant:var_mod(VariantTag:var1, VariantTag:var2);


/*                 */
/*      Lists      */
/*                 */

native List:list_new();
native bool:list_delete(List:list);
native list_size(List:list);
native bool:list_clear(List:list);

native list_add(List:list, AnyTag:value, index=-1, tag_id=tagof(value));
native list_add_arr(List:list, const AnyTag:value[], index=-1, size=sizeof(value), tag_id=tagof(value));
native list_add_str(List:list, const value[], index=-1);
native list_add_var(List:list, VariantTag:value, index=-1);
native list_add_list(List:list, List:range, index=-1);

native bool:list_remove(List:list, index);

native list_get(List:list, index, offset=0);
native list_get_arr(List:list, index, AnyTag:value[], size=sizeof(value));
native Variant:list_get_var(List:list, index);
native bool:list_get_safe(List:list, index, &AnyTag:value, offset=0, tag_id=tagof(value));
native list_get_arr_safe(List:list, index, AnyTag:value[], size=sizeof(value), tag_id=tagof(value));

native bool:list_set(List:list, index, AnyTag:value, tag_id=tagof(value));
native bool:list_set_arr(List:list, index, const AnyTag:value[], size=sizeof(value), tag_id=tagof(value));
native bool:list_set_str(List:list, index, const value[]);
native bool:list_set_var(List:list, index, VariantTag:value);
native bool:list_set_cell(List:list, index, offset, AnyTag:value);
native bool:list_set_cell_safe(List:list, index, offset, AnyTag:value, tag_id=tagof(value));

native list_tagof(List:list, index);
native list_sizeof(List:list, index);

native ListIterator:list_iter(List:list);
native ListIterator:list_next(ListIterator:iter);
native bool:list_valid(List:list, ListIterator:iter);

native iter_get(ListIterator:iter, offset=0);
native iter_get_arr(ListIterator:iter, AnyTag:value[], size=sizeof(value));
native Variant:iter_get_var(ListIterator:iter);
native bool:iter_get_safe(ListIterator:iter, &AnyTag:value, offset=0, tag_id=tagof(value));
native iter_get_arr_safe(ListIterator:iter, AnyTag:value[], size=sizeof(value), tag_id=tagof(value));

native bool:iter_set(ListIterator:iter, AnyTag:value, tag_id=tagof(value));
native bool:iter_set_arr(ListIterator:iter, const AnyTag:value[], size=sizeof(value), tag_id=tagof(value));
native bool:iter_set_str(ListIterator:iter, const value[]);
native bool:iter_set_var(ListIterator:iter, VariantTag:value);
native bool:iter_set_cell(ListIterator:iter, offset, AnyTag:value);
native bool:iter_set_cell_safe(ListIterator:iter, offset, AnyTag:value, tag_id=tagof(value));

native ListIterator:operator++(ListIterator:iter) = list_next;

#define list_new<%0> list_new@%0
#define list_delete<%0> list_delete@%0
#define list_size<%0> list_size@%0
#define list_clear<%0> list_clear@%0

#define list_add<%0> list_add@%0
#define list_add_arr<%0> list_add_arr@%0
#define list_add_list<%0> list_add_list@%0

#define list_remove<%0> list_remove@%0

#define list_get<%0> list_get@%0
#define list_get_arr<%0> list_get_arr@%0

#define list_set<%0> list_set@%0
#define list_set_arr<%0> list_set_arr@%0
#define list_set_cell<%0> list_set_cell@%0

#define list_sizeof<%0> list_sizeof@%0

#define list_iter<%0> list_iter@%0
#define list_next<%0> list_next@%0
#define list_valid<%0> list_valid@%0

#define iter_get<%0> iter_get@%0
#define iter_get_arr<%0> iter_get_arr@%0

#define iter_set<%0> iter_set@%0
#define iter_set_arr<%0> iter_set_arr@%0
#define iter_set_cell<%0> iter_set_cell@%0

#define @define_List<%0> \
stock const Void:List<%0>;\
native List<%0>:list_new<%0>()=list_new;\
native bool:list_delete<%0>(List<%0>:l)=list_delete;\
native list_size<%0>(List<%0>:l)=list_size;\
native bool:list_clear<%0>(List<%0>:l)=list_clear;\
native list_add<%0>(List<%0>:l,%0:v,i=-1,t=(tagof(%0:)))=list_add;\
native list_add_arr<%0>(List<%0>:l,const %0:v[],i=-1,s=sizeof(v),t=(tagof(%0:)))=list_add_arr;\
native list_add_list<%0>(List<%0>:l,List<%0>:l2,i=-1)=list_add_list;\
native bool:list_remove<%0>(List<%0>:l,i)=list_remove;\
native %0:list_get<%0>(List<%0>:l,i,o=0)=list_get;\
native list_get_arr<%0>(List<%0>:l,i,%0:v[],s=sizeof(v))=list_get_arr;\
native bool:list_set<%0>(List<%0>:l,i,%0:v,t=(tagof(%0:)))=list_set;\
native bool:list_set_arr<%0>(List<%0>:l,i,const %0:v[],s=sizeof(v),t=(tagof(%0:)))=list_set_arr;\
native bool:list_set_cell<%0>(List<%0>:l,i,o,%0:v)=list_set_cell;\
native list_sizeof<%0>(List<%0>:l,i)=list_sizeof;\
forward List:operator=(List<%0>:l);\
forward List<%0>:operator=(List:l);\
native ListIterator<%0>:list_iter<%0>(List<%0>:l)=list_iter;\
native ListIterator<%0>:list_next<%0>(ListIterator<%0>:i)=list_next;\
native bool:list_valid<%0>(List<%0>:l,ListIterator<%0>:i)=list_valid;\
native iter_get<%0>(ListIterator<%0>:i,o=0)=iter_get;\
native iter_get_arr<%0>(ListIterator<%0>:i,%0:v[],s=sizeof(v))=iter_get_arr;\
native bool:iter_set<%0>(ListIterator<%0>:i,%0:v,t=(tagof(%0:)))=iter_set;\
native bool:iter_set_arr<%0>(ListIterator<%0>:i,const %0:v[],s=sizeof(v),t=(tagof(%0:)))=iter_set_arr;\
native bool:iter_set_cell<%0>(ListIterator<%0>:i,o,%0:v)=iter_set_cell;\
native ListIterator<%0>:operator++(ListIterator<%0>:i)=list_next


/*                 */
/*       Maps      */
/*                 */

native Map:map_new();
native map_delete(Map:map);
native map_size(Map:map);

native bool:map_add(Map:map, AnyTag:key, AnyTag:value, key_tag_id=tagof(key), value_tag_id=tagof(value));
native bool:map_add_arr(Map:map, AnyTag:key, const AnyTag:value[], value_size=sizeof(value), key_tag_id=tagof(key), value_tag_id=tagof(value));
native bool:map_add_str(Map:map, AnyTag:key, const value[], key_tag_id=tagof(key));
native bool:map_add_var(Map:map, AnyTag:key, VariantTag:value, key_tag_id=tagof(key));
native bool:map_arr_add(Map:map, const AnyTag:key[], AnyTag:value, key_size=sizeof(key), key_tag_id=tagof(key), value_tag_id=tagof(value));
native bool:map_arr_add_arr(Map:map, const AnyTag:key[], const AnyTag:value[], key_size=sizeof(key), value_size=sizeof(value), key_tag_id=tagof(key), value_tag_id=tagof(value));
native bool:map_arr_add_str(Map:map, const AnyTag:key[], const value[], key_size=sizeof(key), key_tag_id=tagof(key));
native bool:map_arr_add_var(Map:map, const AnyTag:key[], VariantTag:value, key_size=sizeof(key), key_tag_id=tagof(key));
native bool:map_str_add(Map:map, const key[], AnyTag:value, value_tag_id=tagof(value));
native bool:map_str_add_arr(Map:map, const key[], const AnyTag:value[], value_size=sizeof(value), value_tag_id=tagof(value));
native bool:map_str_add_str(Map:map, const key[], const value[]);
native bool:map_str_add_var(Map:map, const key[], VariantTag:value);
native bool:map_var_add(Map:map, VariantTag:key, AnyTag:value, value_tag_id=tagof(value));
native bool:map_var_add_arr(Map:map, VariantTag:key, const AnyTag:value[], value_size=sizeof(value), value_tag_id=tagof(value));
native bool:map_var_add_str(Map:map, VariantTag:key, const value[]);
native bool:map_var_add_var(Map:map, VariantTag:key, VariantTag:value);
native map_add_map(Map:map, Map:other, bool:overwrite);

native bool:map_remove(Map:map, AnyTag:key, key_tag_id=tagof(key));
native bool:map_arr_remove(Map:map, const AnyTag:key[], key_size=sizeof(key), key_tag_id=tagof(key));
native bool:map_str_remove(Map:map, const key[]);
native bool:map_var_remove(Map:map, VariantTag:key);

native map_get(Map:map, AnyTag:key, offset=0, key_tag_id=tagof(key));
native map_get_arr(Map:map, AnyTag:key, AnyTag:value[], value_size=sizeof(value), key_tag_id=tagof(key));
native Variant:map_get_var(Map:map, AnyTag:key, key_tag_id=tagof(key));
native bool:map_get_safe(Map:map, AnyTag:key, &AnyTag:value, offset=0, key_tag_id=tagof(key), value_tag_id=tagof(value));
native map_get_arr_safe(Map:map, AnyTag:key, AnyTag:value[], value_size=sizeof(value), key_tag_id=tagof(key), value_tag_id=tagof(value));
native map_arr_get(Map:map, const AnyTag:key[], offset=0, key_size=sizeof(key), key_tag_id=tagof(key));
native map_arr_get_arr(Map:map, const AnyTag:key[], AnyTag:value[], value_size=sizeof(value), key_size=sizeof(key), key_tag_id=tagof(key));
native Variant:map_arr_get_var(Map:map, const AnyTag:key[], key_size=sizeof(key), key_tag_id=tagof(key));
native bool:map_arr_get_safe(Map:map, const AnyTag:key[], &AnyTag:value, offset=0, key_size=sizeof(key), key_tag_id=tagof(key), value_tag_id=tagof(value));
native map_arr_get_arr_safe(Map:map, const AnyTag:key[], AnyTag:value[], value_size=sizeof(value), key_size=sizeof(key), key_tag_id=tagof(key), value_tag_id=tagof(value));
native map_str_get(Map:map, const key[], offset=0);
native map_str_get_arr(Map:map, const key[], AnyTag:value[], value_size=sizeof(value));
native Variant:map_str_get_var(Map:map, const key[]);
native bool:map_str_get_safe(Map:map, const key[], &AnyTag:value, offset=0, value_tag_id=tagof(value));
native map_str_get_arr_safe(Map:map, const key[], AnyTag:value[], value_size=sizeof(value), value_tag_id=tagof(value));
native map_var_get(Map:map, VariantTag:key, offset=0);
native map_var_get_arr(Map:map, VariantTag:key, AnyTag:value[], value_size=sizeof(value));
native Variant:map_var_get_var(Map:map, VariantTag:key);
native bool:map_var_get_safe(Map:map, VariantTag:key, &AnyTag:value, offset=0, value_tag_id=tagof(value));
native map_var_get_arr_safe(Map:map, VariantTag:key, AnyTag:value[], value_size=sizeof(value), value_tag_id=tagof(value));

native bool:map_set(Map:map, AnyTag:key, AnyTag:value, key_tag_id=tagof(key), value_tag_id=tagof(value));
native bool:map_set_arr(Map:map, AnyTag:key, const AnyTag:value[], value_size=sizeof(value), key_tag_id=tagof(key), value_tag_id=tagof(value));
native bool:map_set_str(Map:map, AnyTag:key, const value[], key_tag_id=tagof(key));
native bool:map_set_var(Map:map, AnyTag:key, VariantTag:value, key_tag_id=tagof(key));
native bool:map_set_cell(Map:map, AnyTag:key, offset, AnyTag:value, key_tag_id=tagof(key));
native bool:map_set_cell_safe(Map:map, AnyTag:key, offset, AnyTag:value, key_tag_id=tagof(key), value_tag_id=tagof(value));
native bool:map_arr_set(Map:map, const AnyTag:key[], AnyTag:value, key_size=sizeof(key), key_tag_id=tagof(key), value_tag_id=tagof(value));
native bool:map_arr_set_arr(Map:map, const AnyTag:key[], const AnyTag:value[], value_size=sizeof(value), key_size=sizeof(key), key_tag_id=tagof(key), value_tag_id=tagof(value));
native bool:map_arr_set_str(Map:map, const AnyTag:key[], const value[], key_size=sizeof(key), key_tag_id=tagof(key));
native bool:map_arr_set_var(Map:map, const AnyTag:key[], VariantTag:value, key_size=sizeof(key), key_tag_id=tagof(key));
native bool:map_arr_set_cell(Map:map, const AnyTag:key[], offset, AnyTag:value, key_size=sizeof(key), key_tag_id=tagof(key));
native bool:map_arr_set_cell_safe(Map:map, const AnyTag:key[], offset, AnyTag:value, key_size=sizeof(key), key_tag_id=tagof(key), value_tag_id=tagof(value));
native bool:map_str_set(Map:map, const key[], AnyTag:value, value_tag_id=tagof(value));
native bool:map_str_set_arr(Map:map, const key[], const AnyTag:value[], value_size=sizeof(value), value_tag_id=tagof(value));
native bool:map_str_set_str(Map:map, const key[], const value[]);
native bool:map_str_set_var(Map:map, const key[], VariantTag:value);
native bool:map_str_set_cell(Map:map, const key[], offset, AnyTag:value);
native bool:map_str_set_cell_safe(Map:map, const key[], offset, AnyTag:value, value_tag_id=tagof(value));
native bool:map_var_set(Map:map, VariantTag:key, AnyTag:value, value_tag_id=tagof(value));
native bool:map_var_set_arr(Map:map, VariantTag:key, const AnyTag:value[], value_size=sizeof(value), value_tag_id=tagof(value));
native bool:map_var_set_str(Map:map, VariantTag:key, const value[]);
native bool:map_var_set_var(Map:map, VariantTag:key, VariantTag:value);
native bool:map_var_set_cell(Map:map, VariantTag:key, offset, AnyTag:value);
native bool:map_var_set_cell_safe(Map:map, VariantTag:key, offset, AnyTag:value, value_tag_id=tagof(value));

native map_key_at(Map:map, index, offset=0);
native map_arr_key_at(Map:map, index, AnyTag:key[], key_size=sizeof(key));
native Variant:map_var_key_at(Map:map, index);
native bool:map_key_at_safe(Map:map, index, &AnyTag:key, offset=0, key_tag_id=tagof(key));
native map_arr_key_at_safe(Map:map, index, AnyTag:key[], key_size=sizeof(key), key_tag_id=tagof(key));

native map_value_at(Map:map, index, offset=0);
native map_arr_value_at(Map:map, index, AnyTag:value[], value_size=sizeof(value));
native Variant:map_var_value_at(Map:map, index);
native bool:map_value_at_safe(Map:map, index, &AnyTag:value, offset=0, value_tag_id=tagof(value));
native map_arr_value_at_safe(Map:map, index, AnyTag:value[], value_size=sizeof(value), key_tag_id=tagof(value));

native map_tagof(Map:map, AnyTag:key, key_tag_id=tagof(key));
native map_sizeof(Map:map, AnyTag:key, key_tag_id=tagof(key));
native map_arr_tagof(Map:map, const AnyTag:key[], key_size=sizeof(key), key_tag_id=tagof(key));
native map_arr_sizeof(Map:map, const AnyTag:key[], key_size=sizeof(key), key_tag_id=tagof(key));
native map_str_tagof(Map:map, const key[]);
native map_str_sizeof(Map:map, const key[]);
native map_var_tagof(Map:map, VariantTag:key);
native map_var_sizeof(Map:map, VariantTag:key);

native MapIterator:map_iter(Map:map);
native MapIterator:map_next(MapIterator:iter);
native bool:map_valid(Map:map, MapIterator:iter);

native iter_get_key(MapIterator:iter, offset=0);
native iter_get_key_arr(MapIterator:iter, AnyTag:value[], size=sizeof(value));
native Variant:iter_get_key_var(MapIterator:iter);
native bool:iter_get_key_safe(MapIterator:iter, &AnyTag:value, offset=0, tag_id=tagof(value));
native iter_get_key_arr_safe(MapIterator:iter, AnyTag:value[], size=sizeof(value), tag_id=tagof(value));

native iter_get_value(MapIterator:iter, offset=0);
native iter_get_value_arr(MapIterator:iter, AnyTag:value[], size=sizeof(value));
native Variant:iter_get_value_var(MapIterator:iter);
native bool:iter_get_value_safe(MapIterator:iter, &AnyTag:value, offset=0, tag_id=tagof(value));
native iter_get_value_arr_safe(MapIterator:iter, AnyTag:value[], size=sizeof(value), tag_id=tagof(value));

native bool:iter_set_value(MapIterator:iter, AnyTag:value, value_tag_id=tagof(value));
native bool:iter_set_value_arr(MapIterator:iter, const AnyTag:value[], size=sizeof(value), value_tag_id=tagof(value));
native bool:iter_set_value_str(MapIterator:iter, const value[]);
native bool:iter_set_value_var(MapIterator:iter, VariantTag:value);
native bool:iter_set_value_cell(MapIterator:iter, offset, AnyTag:value);
native bool:iter_set_value_cell_safe(MapIterator:iter, offset, AnyTag:value, value_tag_id=tagof(value));

native MapIterator:operator++(MapIterator:iter) = map_next;

#define map_new<%0,%1> map_new@%0@%1
#define map_delete<%0,%1> map_delete@%0@%1
#define map_size<%0,%1> map_size@%0@%1

#define map_add<%0,%1> map_add@%0@%1
#define map_add_arr<%0,%1> map_add_arr@%0@%1
#define map_arr_add<%0,%1> map_arr_add@%0@%1
#define map_arr_add_arr<%0,%1> map_arr_add_arr@%0@%1
#define map_add_map<%0,%1> map_add_map@%0@%1

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
#define map_next<%0,%1> map_next@%0@%1
#define map_valid<%0,%1> map_valid@%0@%1

#define iter_get_key<%0,%1> iter_get_key@%0@%1
#define iter_get_key_arr<%0,%1> iter_get_key_arr@%0@%1

#define iter_get_value<%0,%1> iter_get_value@%0@%1
#define iter_get_value_arr<%0,%1> iter_get_value_arr@%0@%1

#define iter_set_value<%0,%1> iter_set_value@%0@%1
#define iter_set_value_arr<%0,%1> iter_set_value_arr@%0@%1
#define iter_set_value_cell<%0,%1> iter_set_value_cell@%0@%1

#define @define_Map<%0,%1> \
stock const Void:Map<%0,%1>;\
native Map<%0,%1>:map_new<%0,%1>()=map_new;\
native map_delete<%0,%1>(Map<%0,%1>:m)=map_delete;\
native map_size<%0,%1>(Map<%0,%1>:m)=map_size;\
native bool:map_add<%0,%1>(Map<%0,%1>:m,%0:k,%1:v,kt=(tagof(%0:)),vt=(tagof(%1:)))=map_add;\
native bool:map_add_arr<%0,%1>(Map<%0,%1>:m,%0:k,const %1:v[],vs=sizeof(v),kt=(tagof(%0:)),vt=(tagof(%1:)))=map_add_arr;\
native bool:map_arr_add<%0,%1>(Map<%0,%1>:m,const %0:k[],%1:v,ks=sizeof(k),kt=(tagof(%0:)),vt=(tagof(%1:)))=map_arr_add;\
native bool:map_arr_add_arr<%0,%1>(Map<%0,%1>:m,const %0:k[],const %1:v[],ks=sizeof(k),vs=sizeof(v),kt=(tagof(%0:)),vt=(tagof(%1:)))=map_arr_add_arr;\
native map_add_map<%0,%1>(Map<%0,%1>:m,Map<%0,%1>:m2,bool:ow);\
native bool:map_remove<%0,%1>(Map<%0,%1>:m,%0:k,kt=(tagof(%0:)))=map_remove;\
native bool:map_arr_remove<%0,%1>(Map<%0,%1>:m,const %0:k[],ks=sizeof(k),kt=(tagof(%0:)))=map_arr_remove;\
native map_get<%0,%1>(Map<%0,%1>:m,%0:k,o=0,kt=(tagof(%0:)))=map_get;\
native map_get_arr<%0,%1>(Map<%0,%1>:m,%0:k,%1:v[],vs=sizeof(v),kt=(tagof(%0:)))=map_get_arr;\
native map_arr_get<%0,%1>(Map<%0,%1>:m,const %0:k[],o=0,ks=sizeof(k),kt=(tagof(%0:)))=map_arr_get;\
native map_arr_get_arr<%0,%1>(Map<%0,%1>:m,const %0:k[],%1:v[],vs=sizeof(v),ks=sizeof(k),kt=(tagof(%0:)))=map_arr_get_arr;\
native bool:map_set<%0,%1>(Map<%0,%1>:m,%0:k,%1:v,kt=(tagof(%0:)),vt=(tagof(%1:)))=map_set;\
native bool:map_set_arr<%0,%1>(Map<%0,%1>:m,%0:k,const %1:v[],vs=sizeof(v),kt=(tagof(%0:)),vt=(tagof(%1:)))=map_set_arr;\
native bool:map_set_cell<%0,%1>(Map<%0,%1>:m,%0:k,o,%1:v,kt=(tagof(%0:)))=map_set_cell;\
native bool:map_arr_set<%0,%1>(Map<%0,%1>:m,const %0:k[],%1:v,ks=sizeof(k),kt=(tagof(%0:)),vt=(tagof(%1:)))=map_arr_set;\
native bool:map_arr_set_arr<%0,%1>(Map<%0,%1>:m,const %0:k[],const %1:v[],vs=sizeof(v),ks=sizeof(k),kt=(tagof(%0:)),vt=(tagof(%1:)))=map_arr_set_arr;\
native bool:map_arr_set_cell<%0,%1>(Map<%0,%1>:m,const %0:k[],o,%1:v,ks=sizeof(k),kt=(tagof(%0:)))=map_arr_set_cell;\
native map_key_at<%0,%1>(Map<%0,%1>:m,i,o=0)=map_key_at;\
native map_arr_key_at<%0,%1>(Map<%0,%1>:m,i,%0:k[],ks=sizeof(k))=map_arr_key_at;\
native map_value_at<%0,%1>(Map<%0,%1>:m,i,o=0)=map_value_at;\
native map_arr_value_at<%0,%1>(Map<%0,%1>:m,i,%1:v[],vs=sizeof(v))=map_arr_value_at;\
native map_sizeof<%0,%1>(Map<%0,%1>:m,%0:k,kt=(tagof(%0:)))=map_sizeof;\
native map_arr_sizeof<%0,%1>(Map<%0,%1>:m,const %0:k[],ks=sizeof(k),kt=(tagof(%0:)))=map_arr_sizeof;\
forward Map:operator=(Map<%0,%1>:m);\
forward Map<%0,%1>:operator=(Map:m);\
native MapIterator<%0,%1>:map_iter<%0,%1>(Map<%0,%1>:m)=map_iter;\
native MapIterator<%0,%1>:map_next<%0,%1>(MapIterator<%0,%1>:i)=map_next;\
native bool:map_valid<%0,%1>(Map<%0,%1>:m,MapIterator<%0,%1>:i)=map_valid;\
native %0:iter_get_key<%0,%1>(MapIterator<%0,%1>:i,o=0)=iter_get_key;\
native iter_get_key_arr<%0,%1>(MapIterator<%0,%1>:i,%0:v[],size=sizeof(v))=iter_get_key_arr;\
native %1:iter_get_value<%0,%1>(MapIterator<%0,%1>:i,o=0)=iter_get_value;\
native iter_get_value_arr<%0,%1>(MapIterator<%0,%1>:i,%1:v[],size=sizeof(v))=iter_get_value_arr;\
native bool:iter_set_value<%0,%1>(MapIterator<%0,%1>:i,%1:v,vt=(tagof(%1:)))=iter_set_value;\
native bool:iter_set_value_arr<%0,%1>(MapIterator<%0,%1>:i,const %1:v[],size=sizeof(v),vt=(tagof(%1:)))=iter_set_value_arr;\
native bool:iter_set_value_cell<%0,%1>(MapIterator<%0,%1>:i,o,%1:v)=iter_set_value_cell;\
native MapIterator<%0,%1>:operator++(MapIterator<%0,%1>:i)=map_next

/*                 */
/* Tasks and async */
/*                 */

native wait_ticks(ticks);
native wait_ms(interval);
native task:task_new();
native task_set_result(task:task, AnyTag:result);
native task_get_result(task:task);
native task:task_ticks(ticks);
native task:task_ms(interval);
native task_await(task:task);
native task_yield(AnyTag:value);

#if !defined PP_NO_ALIASES

#define await%9\32;%0; task_await(%0);
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

#if !defined PP_NO_ALIASES

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
native bool:operator==(String:arg1, String:arg2) = str_equal;
native bool:operator==(String:arg1, GlobalString:arg2) = str_equal;
native bool:operator==(GlobalString:arg1, String:arg2) = str_equal;
native bool:operator==(GlobalString:arg1, GlobalString:arg2) = str_equal;
stock bool:operator!=(String:arg1, String:arg2) return !(arg1 == arg2);
stock bool:operator!=(String:arg1, GlobalString:arg2) return !(arg1 == arg2);
stock bool:operator!=(GlobalString:arg1, String:arg2) return !(arg1 == arg2);
stock bool:operator!=(GlobalString:arg1, GlobalString:arg2) return !(arg1 == arg2);

native String:operator%(String:arg1, String:arg2) = str_cat;
native String:operator%(GlobalString:arg1, String:arg2) = str_cat;
native String:operator%(GlobalString:arg1, GlobalString:arg2) = str_cat;

#if !defined PP_NO_AUTO_STRINGS

native String:operator=(arg) = str_int;
native String:operator=(Float:arg) = str_float;
stock GlobalString:operator=(arg) return str_to_global(str_int(arg));
stock GlobalString:operator=(Float:arg) return str_to_global(str_float(arg));
stock String:operator%(String:arg1, arg2) return str_cat(arg1, str_int(arg2));
stock String:operator%(GlobalString:arg1, arg2) return str_cat(arg1, str_int(arg2));
stock String:operator%(arg1, String:arg2) return str_cat(str_int(arg1), arg2);
stock String:operator%(arg1, GlobalString:arg2) return str_cat(str_int(arg1), arg2);
stock String:operator%(String:arg1, Float:arg2) return str_cat(arg1, str_float(arg2));
stock String:operator%(GlobalString:arg1, Float:arg2) return str_cat(arg1, str_float(arg2));
stock String:operator%(Float:arg1, String:arg2) return str_cat(str_float(arg1), arg2);
stock String:operator%(Float:arg1, GlobalString:arg2) return str_cat(str_float(arg1), arg2);

#endif

#define PP_VARIANT_OP(%0,%1) \
    native Variant:operator%0(Variant:arg1, Variant:arg2) = %1; \
    native Variant:operator%0(Variant:arg1, GlobalVariant:arg2) = %1; \
    native Variant:operator%0(GlobalVariant:arg1, Variant:arg2) = %1; \
    native Variant:operator%0(GlobalVariant:arg1, GlobalVariant:arg2) = %1

PP_VARIANT_OP(+, var_add);
PP_VARIANT_OP(-, var_sub);
PP_VARIANT_OP(*, var_mul);
PP_VARIANT_OP(/, var_div);
PP_VARIANT_OP(%, var_mod);

#undef PP_VARIANT_OP
    
native GlobalVariant:operator=(Variant:arg) = var_to_global;
stock Variant:operator=(GlobalVariant:arg) return Variant:arg;
native bool:operator==(Variant:arg1, Variant:arg2) = var_equal;
native bool:operator==(Variant:arg1, GlobalVariant:arg2) = var_equal;
native bool:operator==(GlobalVariant:arg1, Variant:arg2) = var_equal;
native bool:operator==(GlobalVariant:arg1, GlobalVariant:arg2) = var_equal;
stock bool:operator!=(Variant:arg1, Variant:arg2) return !(arg1 == arg2);
stock bool:operator!=(Variant:arg1, GlobalVariant:arg2) return !(arg1 == arg2);
stock bool:operator!=(GlobalVariant:arg1, Variant:arg2) return !(arg1 == arg2);
stock bool:operator!=(GlobalVariant:arg1, GlobalVariant:arg2) return !(arg1 == arg2);

stock String:operator=(Ref<String>:arg) return String:arg;
stock GlobalString:operator=(Ref<GlobalString>:arg) return GlobalString:arg;
stock Variant:operator=(Ref<Variant>:arg) return Variant:arg;
stock GlobalVariant:operator=(Ref<GlobalVariant>:arg) return GlobalVariant:arg;
stock Ref<String>:operator=(String:arg) return Ref<String>:arg;
stock Ref<GlobalString>:operator=(GlobalString:arg) return Ref<GlobalString>:arg;
stock Ref<Variant>:operator=(Variant:arg) return Ref<Variant>:arg;
stock Ref<GlobalVariant>:operator=(GlobalVariant:arg) return Ref<GlobalVariant>:arg;

/** PawnPlus by IllidanS4 **/
//github.com/IllidanS4/PawnPlus

/* Configuration */

native pp_hook_strlen(bool:hook);

/* Pawn */

native pawn_call_native(const function[], const format[], {_,AmxString,Float}:...);
native callback:pawn_register_callback(const callback[], const handler[], const additional_format[], {_,Float}:...);
native pawn_unregister_callback(callback:id);

/* Tasks and async */

native task:wait_ticks(ticks);
native task:wait_ms(interval);
native task:task_new();
native task_set_result(task:task, {_,Float}:result);
native task_get_result(task:task);
native task:task_ticks(ticks);
native task:task_ms(interval);
native task:task_await(task:task);
native task_yield({_,Float}:value);

#define await task_await
#define yield task_yield

/* Strings */

#define StringTags String,GlobalString
#define StringTag {StringTags}
#define STRING_NULL (GlobalString:0)
#define INVALID_CHAR (0xFFFFFF00)

native String:str_new(const str[], bool:wide=true);
native String:str_new_arr(const arr[], size=sizeof(arr), bool:wide=true);
native AmxString:str_addr(StringTag:str);
native GlobalString:str_to_global(StringTag:str);
native String:str_to_local(StringTag:str);
native str_free(&StringTag:str);
native bool:str_is_valid(StringTag:str);

native str_len(StringTag:str);
native str_get(StringTag:str, buffer[], size=sizeof(buffer), start=0, end=cellmax);
native str_getc(StringTag:str, pos);
native str_setc(StringTag:str, pos, value);
native bool:str_cmp(StringTag:str1, StringTag:str2);
native bool:str_empty(StringTag:str);

native String:str_int(val);
native String:str_float(Float:val);
native String:str_cat(StringTag:str1, StringTag:str2);
native String:str_sub(StringTag:str, start=0, end=cellmax);
native String:str_clone(StringTag:str);

native String:str_set(StringTag:target, StringTag:other);
native String:str_append(StringTag:target, StringTag:other);
native String:str_del(StringTag:target, start=0, end=cellmax);
native String:str_clear(StringTag:str);
native String:str_resize(StringTag:str, size, padding=0);

stock String:str_val({_,StringTags,Float}:val, tag=tagof(val))
{
    static tag_string = tagof(String:), tag_global_string = tagof(GlobalString:), tag_float = tagof(Float:), tag_int = tagof(_:);
    if(tag == tag_float)
    {
        return str_float(Float:val);
    }else if(tag == 0 || tag == tag_int)
    {
        return str_int(_:val);
    }else if(tag == tag_string || tag == tag_global_string)
    {
        return String:val;
    }else{
        return STRING_NULL;
    }
}

stock GlobalString:operator=(String:arg) return str_to_global(arg);
stock String:operator=(GlobalString:arg) return String:arg;
stock AmxString:operator=(String:arg) return str_addr(arg);
stock AmxString:operator=(GlobalString:arg) return str_addr(arg);

stock String:operator+(String:arg1, String:arg2) return str_cat(arg1, arg2);
stock String:operator+(String:arg1, GlobalString:arg2) return str_cat(arg1, arg2);
stock String:operator+(GlobalString:arg1, String:arg2) return str_cat(arg1, arg2);
stock String:operator+(GlobalString:arg1, GlobalString:arg2) return str_cat(arg1, arg2);
stock bool:operator==(String:arg1, String:arg2) return !str_cmp(arg1, arg2);
stock bool:operator==(String:arg1, GlobalString:arg2) return !str_cmp(arg1, arg2);
stock bool:operator==(GlobalString:arg1, String:arg2) return !str_cmp(arg1, arg2);
stock bool:operator==(GlobalString:arg1, GlobalString:arg2) return !str_cmp(arg1, arg2);

#define PP_PROHIBIT_OPERATORS(%0) \
    forward %0:operator=(_:arg); \
    forward %0:operator=(Float:arg)

PP_PROHIBIT_OPERATORS(String);
PP_PROHIBIT_OPERATORS(GlobalString);
PP_PROHIBIT_OPERATORS(AmxString);

#undef PP_PROHIBIT_OPERATORS

### Exception handling and value lifetime management
`attacha` runtime manages value destructors and exception handlers separately to provide flexibility with value lifetime, value lifetime manages just with two instructions `value_hold(id)` and `value_unhold(id)`, exceptions managed with four instructions `handle_begin(id)`, `handle_catch(id, mode, data...)`, `handle_finally(id)`, `handle_end(id)`


both instructions type can use same id and reuse id after calling value_unhold or handle_end

`value_unhold` not destroy value when exception not occurred you need also use `remove` instruction to remove(i recommend to use `value_unhold` before `remove` to not call destructor when exception happened in destruction)

exceptions handled a little differently 
* `handle_begin` declares new scope
* `handle_end` declares scope where exception ends
* `handle_catch` declares exception handler that will be called on exception
* `handle_finally` declares function that will be called on unwind

`handle_catch` and `handle_finally` can not be used out of scope and `handle_begin` can not define id when it still used


### handle_catch
`handle_catch` has 6 modes:
* `0`: jump to current instruction location when exception name same as one of provided names list 
   * encoding: [0][string count: packed][name: string]... 
* `1`: jump to current instruction location when exception name same as value contained in environment
    * encoding: [1][env_value_id: ui16]
* `2` jump to current instruction location when exception name same as one of provided values contained in environment
    * encoding: [2][envs count: packed][ids: ui16]
* `3` jump to current instruction location when exception name same as one of provided values contained in environment and provided strings
    * encoding: [3][values count: packed][{[type: boolean = false][name:string] or [type: boolean = true][id:ui16]}]
* `4` jump to current instruction location when any exception occurred
    * encoding: [4]
* `5` jump to current instruction location when filter returns true
    * encoding: [5][handle_function_location: value_pos_id][enviro_slice_begin: ui16][enviro_slice_end :ui16]

in 5 st mode called function from constants with slice of current function local environment, it can be used to simulate catch block from c++ or other languages, but when filter returns true, exception fully unwound and destructed, then jumped to location where instruction defined, more information about exception can not be got after unwind

### handle_finally
`handle_finally` will call function from constants with slice of current function local environment like filter in `5` st mode `handle_catch` instruction but only when proceed unwind, it used for finalization
* NOTE this not calls function when no exception occurred

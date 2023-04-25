# chanel

## chanel
    constructor: '# chanel chanel'
        arguments: ignored
        returns: proxy<chanel>

    fun: notify
        arguments: any
        arguments: any,...
        returns: noting
    
    fun: auto_notify
        arguments: async_res
            desc: 'put every results to chanel(auto yield if posible)'
        arguments: function
            desc: 'call function in task context without arguments, call auto_notify(async_res)'
        returns: proxy<auto_notify_chanel>

    fun: auto_notify_continue
        arguments: async_res
            desc: 'put every results to chanel(auto yield if posible), and put every stored results in async_res'
        arguments: function
            desc: 'same as auto_notify(function)'
        returns: proxy<auto_notify_chanel>
    
    fun: auto_notify_skip
        arguments: async_res, integer
            desc: 'put every results to chanel(auto yield if posible), but ignore n results from results buffer'
        arguments: function, integer
            desc: 'same as auto_notify(function), but ignore n results'
        returns: proxy<auto_notify_chanel>

    fun: create_handle
        arguments: ignored
        returns: proxy<chanel_handle>
    

    fun: remove_handle
        arguments: proxy<chanel_handle>
        returns: noting
    
    fun: remove_auto_notify
        arguments: proxy<auto_notify_chanel>
        returns: noting

    fun: add_handle
        arguments: proxy<chanel_handle>
        returns: noting

## chanel_handler
    constructor: '# chanel chanel_handler'
        arguments: ignored
        returns: proxy<chanel_handler>

    fun: get
        blocking
        arguments: ignored
        throws: 'TaskDeath' when chanel destructed and buffer empty
        returns: any

    fun: try_get
        arguments: ignored
        returns: farr[boolean, any]
    
    fun: can_get
        arguments: ignored
        returns: boolean

    fun: end_of_chanel
        arguments: ignored
        returns: boolean
            desc: 'true if chanel destructed'

    fun: wait_item
        blocking
        arguments: ignored
        returns: boolean
            desc: 'true if recived value'

## auto_notify_chanel
    constructor: none
    desc: 'act as dummy handle, and hold internal data'
    

# console
    fun: printLine
        arguments: any...
        desc: 'print all arguments, convert all non string values to string, at end put \n'
        returns: noting
        
    fun: print
        arguments: any...
        desc: 'print all arguments, convert all non string values to string'
        returns: noting
	
    fun: printf
        unsafe
        c_function
        dynamic_call
        variadic
        arguments: string, any...
        returns: noting

    fun: resetModifiers
        arguments: ignored
        returns: noting

        fun: boldText
        arguments: ignored
        returns: noting

	fun: italicText
        arguments: ignored
        returns: noting
        
	fun: underlineText
        arguments: ignored
        returns: noting
        
	fun: slowBlink
        arguments: ignored
        returns: noting
        
	fun: rapidBlink
        arguments: ignored
        returns: noting
        
	fun: invertColors
        arguments: ignored
        returns: noting
        
	fun: notBoldText
        arguments: ignored
        returns: noting
        
	fun: notUnderlinedText
        arguments: ignored
        returns: noting
        
	fun: hideBlinkText
        arguments: ignored
        returns: noting
        
	fun: resetTextColor
        arguments: ignored
        returns: noting
        
	fun: resetBgColor
        arguments: ignored
        returns: noting
        
	fun: setTextColor
        arguments: ui8'r', ui8'g', ui8'b'
        returns noting

	fun: setBgColor
        arguments: ui8'r', ui8'g', ui8'b'
        returns noting

	fun: setPos
        arguments: ui16'row', ui16'col'
        returns noting

	fun: saveCurPos
        arguments: ignored
        returns: noting
        
	fun: loadCurPos
        arguments: ignored
        returns: noting

	fun: setLine
        arguments: ui32
        returns noting

	fun: showCursor
        arguments: ignored
        returns: noting

	fun: hideCursor
        arguments: ignored
        returns: noting

	fun: readWord
        arguments: ignored
        returns: string

	fun: readLine
        arguments: ignored
        returns: string

	fun: readInput
        arguments: ignored
        returns: string

	fun: readValue
        arguments: ignored
        desc: 'try return recognized value from input'
        returns: any

	fun: readInt
        arguments: ignored
        returns: integer

# internal
## memory
    fun: dump
        concurent_unsafe
        arguments: any
        returns: farr[ farr[undefined_ptr'from', undefined_ptr'to', integer'len',string'desc',boolean'is_fault']]
            desc: 'returns array of pages, desc is os depend value, is_fault mean this page is guard page'

## stack
    fun: shrink
        concurent_unsafe
        arguments: {opt}integer'treeshold'
        desc: 'reduce current stack size'
        returns: boolean
            desc:: 'true if operation success'
    
    fun: prepare
        concurent_unsafe
        arguments: {opt}integer'grow_count'
        desc: 'increase stack size'
        returns: boolean
            desc: 'false if there no enough reserved memory'

    fun: reserve
        concurent_unsafe
        arguments: integer'grow_count'
        desc: 'prepare stack for heavly operation, do noting if stack enough'
        returns: boolean
            desc: 'false if there no enough reserved memory'


    fun: dump
        concurent_unsafe
        arguments: ignored
        desc: 'capture stack like @(internal memory dump)'
        returns: farr[ farr[undefined_ptr'from', undefined_ptr'to', integer'len',string'desc',boolean'is_fault']]
            desc: 'array of pages, desc is os depend value, is_fault mean this page is guard page'

    fun: bs_supported
        arguments: ignored
        returns: boolean
            desc: 'true if all functions in this namespace supported'
        
    fun: used_size
        argumetns: ignored
        returns: integer
            desc: 'used stack size'

    fun: unused_size
        argumetns: ignored
        returns: integer
            desc: 'free stack size'

    fun: allocated_size
        argumetns: ignored
        returns: integer
            desc: 'full stack size(unused + used)'

    fun: free_size
        argumetns: ignored
        returns: integer
            desc: 'reserved stack size'
    
    fun: trace
        arguments: {opt}integer'framesToSkip', {opt}boolean'include_native', {opt}integer'max_frames'
        returns: farr[farr[string'file_path',string'fun_name', integer'line']]


    fun: trace_frames        
        arguments: {opt}integer'framesToSkip', {opt}boolean'include_native', {opt}integer'max_frames'
        returns: farr[undefined_ptr'rip',...]
    
    fun: resolve_frame
        arguments: undefined_ptr'rip frame', {opt}boolean'include_native'
        returns: farr[string'file_path',string'fun_name', integer'line']

## run_time
    fun: gc_pause
        debug_function
        arguments: ignored
        desc: right now do noting
        returns: noting
    
    fun: gc_resume
        debug_function
        arguments: ignored
        desc: right now do noting
        returns: noting
    
    fun: gc_hinit_collect
        debug_function
        arguments: ignored
        desc: right now do noting
        returns: noting

### native
        
##### NativeValue
    constructor: '# internal run_time native native_value'
    arguments: ignored
    throws: class NotImplementedException
    
##### NativeTemplate
    constructor: '# internal run_time native native_template'
    arguments: ignored
    returns: proxy<native_template>

    setter: return_type
        arguments: proxy<native_value>
    
    setter: is_variadic
        arguments: @boolean

    fun: add_argument
        arguments: proxy<native_value>
        returns: noting
    

##### NativeLib
    constructor: '# internal run_time native native_lib'
    arguments: string'os resolvable path'
    desc: 'do not use functions from this instance when destructor called'
    returns: proxy<native_lib>
    

    fun: get_function
        arguments: string'symbolic name', proxy<native_template>
        desc: 'native_template native template used here for casting'
    
## asm
    everyiting here throws 'class NotImplementedException'

# paralel
    fun: createThread
        arguments: function, any...
        returns: noting

    fun: createThreadAndWait
        arguments: function, any...
        returns: noting

    fun: createThreadAwaiter
        arguments: function, any...
        returns: async_res
    
    fun: createTask
        arguments: s/farr[function, {opt}function'fault_function',{opt}time_point'timeout', boolean'used_task_local'], any'arguments'
        returns: async_res
## mutex
    constructor: '# paralel mutex'
        arguments: ignored
        returns: proxy<mutex>

    fun: lock
        arguments: ignored
        returns: noting

    fun: unlock
        arguments: ignored
        returns: noting

    fun: try_lock
        arguments: ignored
        returns: boolean

    fun: try_lock_until
        arguments: time_point
        returns: boolean

    fun: is_locked
        arguments: ignored
        returns: boolean

## condition_variable
    constructor: '# paralel condition_variable'
        arguments: ignored
        returns: proxy<condition_variable>

    fun: wait
        arguments: noting
        desc: 'unsafe wait(without locks)'
        returns: noting

    fun: wait
        arguments: proxy<mutex>
        desc: 'unlock mutex when on wait, and lock back when notified'
        returns: noting
    
    fun: wait
        arguments: integer'timeout_ms'
        desc: 'unsafe wait(without locks)'
        returns: boolean
            desc: 'false if timeouted'
    
    fun: wait
        arguments: proxy<mutex>, integer'timeout_ms'
        desc: 'unlock mutex when on wait, and lock back when notified'
        returns: boolean
            desc: 'false if timeouted'

    fun: wait_until
        arguments: proxy<mutex>, time_point
        desc: 'unlock mutex when on wait, and lock back when notified'
        returns: boolean
            desc: 'false if time_point reached'

    fun: wait_until
        arguments: time_point
        desc: 'unsafe wait(without locks)'
        returns: boolean
            desc: 'false if time_point reached'
    
    fun: notify_one
        arguments: ignored
        desc: 'notify one task/thread'
        returns: noting

    fun: notify_all
        arguments: ignored
        desc: 'notify all tasks/threads'
        returns: noting

## semaphore
    constructor: '# paralel semaphore'
        arguments: ignored
        returns: proxy<semaphore>

    fun: lock
        arguments: ignored
        returns: noting
    
    fun: release
        arguments: ignored
        returns: noting

    fun: release_all
        arguments: ignored
        returns: noting

    fun: try_lock
        arguments: noting
        returns: boolean

    fun: try_lock
        arguments: integer'timeout_ms'
        returns: boolean

    fun: try_lock_until
        arguments: time_point
        returns: boolean

    fun: is_locked
        arguments: ignored
        returns: boolean
    
## event_system
    constructor: '# paralel event_system'
        arguments: ignored
        returns: proxy<event_system>

    fun: @symbols::structures::add_operator
        arguments: function
        desc: 'same as fun: join(fun, false, 2)'
        returns: noting

    fun: join
        arguments: function, {opt}boolean'is_async', {opt, def:2ui8}ui8'priorithy'
        desc: 'enum priorithy{ heigh = 0, upper_avg= 1, avg= 2, lower_avg = 3, low = 4}' 
        returns: noting

    fun: leave
        arguments: function, {opt}boolean'is_async', {opt, def:2ui8}ui8'priorithy'
        desc: 'enum priorithy{ heigh = 0, upper_avg= 1, avg= 2, lower_avg = 3, low = 4}' 
        returns: noting

    fun: notify
        arguments: any...
            desc: 'arguments passed to handlers'
        desc: 'sync events handled sync, async - async'
        returns: boolean
            desc: 'true if some event canceled'

	fun: sync_notify
        arguments: any...
            desc: 'arguments passed to handlers'
        desc: 'all events handled sync'
        returns: boolean
            desc: 'true if some event canceled'

	fun: await_notify
        arguments: any...
            desc: 'arguments passed to handlers'
        desc: 'same as sync_notify, but async handlers handled by priorithy and start next when prev not returned true'
        returns: boolean
            desc: 'true if some event canceled'

	fun: async_notify
        arguments: any...
            desc: 'arguments passed to handlers'
        desc: 'execute all as like async in await_notify, returns task for awaiting'
        returns: async_res
            desc: 'true if some event canceled'

## task_limiter
    constructor: '# paralel task_limiter'
        arguments: ignored
        desc: 'limit max cout task executing one/multiply fragment/s code'
        returns: proxy<task_limiter>

    fun: set_max_treeshold
        arguments: ui64
        returns noting

    fun: lock
        arguments: ignored
        returns: noting

    fun: unlock
        arguments: ignored
        returns: noting

    fun: try_lock
        arguments: noting
        returns: boolean

    fun: try_lock
        arguments: integer'timeout_ms'
        returns: boolean

    fun: try_lock_until
        arguments: time_point
        returns: boolean

## task_query
    constructor: '# paralel task_query'
        arguments: {opt}integer'max_at_execution'
        desc: 'start tasks by query'
        returns: proxy<task_query>

    fun: add_task
        arguments: s/farr[function, {opt}function'fault_function',{opt}time_point'timeout', boolean'used_task_local'], any'arguments'
        returns: async_res

    fun: enable
        arguments: ignored
        returns: noting

    fun: disable
        arguments: ignored
        returns: noting

    fun: in_query
        arguments: async_res
        returns: boolean

    fun: set_max_at_execution
        arguments: integer
        returns: noting

    fun: get_max_at_execution
        arguments: noting
        returns: integer
    
    

# net

## ip
    constructor: '# net ip'
        arguments: string'ip6/ip4', {opt, def: 0}integer'port'
        returns: proxy<universal_address>

    constructor: '# net ip#v6'
        arguments: string'ip6', {opt, def: 0}integer'port'
        returns: proxy<universal_address>

    constructor: '# net ip#v4'
        arguments: string'ip4', {opt, def: 0}integer'port'
        returns: proxy<universal_address>

    constructor: '# net ip#address'
        arguments: string'ip:port'
            desc: 'ipv6 address will be encoded like as [::]:1234, ipv4 will be encoded as is: 127.0.0.1:1234'
        returns: proxy<universal_address>

    fun: to_string
        arguments: ignored
        returns: string
            desc: 'just ip address, without port, and unmap ipv4 mapped ipv6'
    
    fun: port
        arguments: ignored
        returns: integer
    
    fun: type
        arguments: ignored
        desc: 'checks type of address, and returns ipv4 even if address is ipv4 mapped ipv6'
        returns: ui8
            desc: '0 - ipv4, 1 - ipv6'
    
    fun: actual_type
        arguments: ignored
        desc: 'checks type of address, and returns actual type'
        returns: ui8
            desc: '0 - ipv4, 1 - ipv6, 2 - ipv4 mapped ipv6'

    fun: full_address
        arguments: ignored
        returns: string
            desc: 'ipv6 address will be encoded like as [::]:1234, ipv4 will be encoded as 127.0.0.1:1234, do unmap ipv4 mapped ipv6'
    
## tcp_server
    desc: 'high efficient tcp server with dual stack ip support'
    oem'windows': 'use winsock2 with iocp'
    oem: 'not implemented'
    constructor: '# net tcp_server'
        arguments: function'handler', proxy<universal_address>, {opt, def: 1ui8'write_deleayed'}ui8'manage_type', {opt, def: 10}integer'acceptors'
            desc: 'enum manage_type{ blocking = 0, write_deleayed = 1 }'
        returns: proxy<universal_address>

    fun: start
        arguments: {opt}integer'pool_size'
            desc: 'if pool_size == 0, then pool size will be set to number of cores'
        returns: noting

    fun: pause
        arguments: ignored
        desc: 'stop getting new connections, and continue handling connected'
        returns: noting
        
    fun: resume
        arguments: ignored
        desc: 'resume getting new connections'
        returns: noting
        
    fun: stop
        arguments: ignored
        desc: 'stop and close all connections'
        returns: noting
    
    fun: mainline
        arguments: ignored
        desc: 'start handling, and use the current thread to handle connections, also can be used afer start to wait server to stop'
        returns: noting
    
    fun: is_running
        arguments: ignored
        desc: 'check if the server is running'
        returns: boolean
    
    fun: is_corrupted
        arguments: ignored
        desc: 'return status of the server, will be checked after constructor'
        returns: boolean
    
    fun: is_paused
        arguments: ignored
        desc: 'check if the server is paused'
        returns: boolean
    
    fun: set_pool_size
        arguments: {opt, def: 0}integer'pool_size'
            desc: 'if pool_size == 0, then pool size will be set to number of cores'
        desc: 'change pool size, if server is running'
        returns: noting

    fun: server_port
        arguments: ignored
        desc: 'get port of the server'
        returns: integer

    fun: server_ip
        arguments: ignored
        desc: 'get ip of the server'
        returns: string
    
    fun: server_address
        arguments: ignored
        desc: 'get address of the server'
        returns: proxy<universal_address>


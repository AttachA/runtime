# bytes

    fun: current_endian
        arguments: ignored
            desc: 'get current endian'
        returns: ui8 'endian'
            desc: '0 'little' - little endian
                  1 'big' - big endian'

    fun: convert_endian
        arguments: any, integer 'endian'
            desc: 'convert value to specified endian'
        arguments: any, string 'endian'
            desc: 'convert value to specified endian, endian: "little" | "big" | "native"'
        returns: noting
    
    fun: swap_bytes
        arguments: any
            desc: 'swap bytes in value'
        returns: noting
    
    fun: from_bytes
        arguments: raw_array_ui8 'bytes', type_identifier
            desc: 'convert bytes to value'
        returns: any 'value'

    fun: to_bytes
        arguments: any
            desc: 'convert value to bytes'
        returns: raw_array_ui8 'bytes'
    

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
            desc: 'put every results to chanel(auto yield if possible)'
        arguments: function
            desc: 'call function in task context without arguments, call auto_notify(async_res)'
        returns: proxy<auto_notify_chanel>

    fun: auto_notify_continue
        arguments: async_res
            desc: 'put every results to chanel(auto yield if possible), and put every stored results in async_res'
        arguments: function
            desc: 'same as auto_notify(function)'
        returns: proxy<auto_notify_chanel>
    
    fun: auto_notify_skip
        arguments: async_res, integer
            desc: 'put every results to chanel(auto yield if possible), but ignore n results from results buffer'
        arguments: function, integer
            desc: 'same as auto_notify(function), but ignore n results'

    fun: auto_event
        arguments: proxy<event_system>
            desc: 'put every notify to event_system'
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

    fun: remove_auto_event
        arguments: proxy<auto_event_chanel>
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
        returns: faarr[boolean, any]
    
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
            desc: 'true if received value'

## auto_notify_chanel
    constructor: none
    desc: 'act as dummy handle, and hold internal data'

## auto_event_chanel
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

    fun: register_format_operator
        arguments: string'operator', function'string(any)'
        desc: 'defined or override operator for this string'
        returns: noting

    fun: format
        arguments: string, any...
        desc: '
            format string like printf
            calls internal _format function that uses this rule:
            if found [] then get next item from arguments
            if found [n] then get n+1 argument from arguments
            if found [:] then cast all arguments from 1-len to string and print [1:len]
            if found [n:] then cast all arguments from n-len to string and print [n:len]
            if found [:n] then cast all arguments from 1-n to string and print [1:n]
            if found [n:m] then cast all arguments from n-m to string and print [n:m]
            if found \[ then print [
            if found \] then print ]
            if found \\ then print \
            if found \n then print next line
            if found \t then print tab
            if found \r then print 
            if found \v then print 
            if found \a then print 
            if found \b then print 
            if found \f then print 
            if found \ and other symbol then print \ and symbol
			if found [any symbols] execute operator and put result to string
            if found % then execute short operator and put result to string
        '
        returns: string

    fun: printf
        arguments: string, any...
        desc: act same as format but returns result to console
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
        arguments: ui8 'r', ui8 'g', ui8 'b'
        returns noting

	fun: setBgColor
        arguments: ui8 'r', ui8 'g', ui8 'b'
        returns noting

	fun: setPos
        arguments: ui16 'row ', ui16 'col'
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

# exception
    fun: current_get_name
        arguments: ignored
        returns: string
    
    fun: current_get_description
        arguments: ignored
        returns: string

    fun: current_get_full_description
        arguments: ignored
        returns: string

    fun: current_has_inner
        arguments: ignored
        returns: bool

    fun: current_unpack
        arguments: ignored
        throws: 'inner exception'
        returns: noting

    fun: now_in_exception
        arguments: ignored
        returns: bool

    fun: set_as_catched
        arguments: ignored
        returns: noting

## filter

    fun: stacktrace
        alias of: internal stack trace

# file

## file_handle
    desc: 'file handle abstraction on top of native file api'
    oem 'windows': 'use win32 api'
    oem: 'not implemented'
    constructor: '# file file_handle'
        arguments: 
            string 'file_path',
            {opt, def:true}boolean 'is_async',
            {opt, def:2 'read_write'}ui8 'open_mode',
            {opt, def:0 'open'}ui8 'on_open_action',
            {opt, def:0}ui8 'async_flags/sync_flags',
            {opt, def:3"{1 'read' | 2 'write'}"}ui8 'share_mode',
            {opt, def:1 'combined'}ui8 'pointer_mode'
            desc:
                "open_mode:
                    0 'read' - open file for read
                    1 'write' - open file for write
                    2 'read_write' - open file for read and write
                    3 'append' - open file for append
                on_open_action:
                    0 'open' - open file if exists, create if not
                    1 'always_new' - create file if not exists, recreate if exists
                    2 'create_new' - create only if not exists, fail if exists
                    3 'open_exists' - just open file if exists, fail if not exists
                    4 'truncate_exists' - truncate file if exists, fail if not exists
                async_flags:
                    1 'delete_on_close' - delete file on close
                    2 'posix_semantics' - posix semantics
                    4 'random_access' - hint to cache manager to optimize for random access
                    8 'sequential_scan' - hint to cache manager to optimize for sequential scan
                sync_flags:
                    1 'delete_on_close' - delete file on close
                    2 'posix_semantics' - posix semantics
                    4 'random_access' - hint to cache manager to optimize for random access
                    8 'sequential_scan' - hint to cache manager to optimize for sequential scan
                    16 'no_buffering' - hint to cache manager, affect seek and read write operations, like disc page size aligned operations
                    32 'write_through' - hint to cache manager to optimize for write through
                share_mode:
                    0 'none' - no share
                    1 'read' - share read
                    2 'write' - share write
                    4 'delete' - share delete
                pointer_mode:
                    0 'separated' - pointer separated for read and write
                    1 'combined' - pointer is same for read and write"
        returns: proxy<file_handle>

    fun: read
        arguments: ui32 'bytes'
            returns: 'faarr[raw_array_ui8] if success, else faarr[raw_array_ui8, ui8 'io_error'] first element is readed bytes, second element is error code'
                desc: 'if raw_array_ui8 less than queried bytes, then that is end of file'
        arguments: raw_array_ui8 'buffer', ui32 'bytes'
            returns: ui32
                desc: 'return readed bytes count, if failed return 0'
    
    fun: read_fixed
        arguments: ui32 'bytes'
            returns: raw_array_ui8
            throws: FileException
                desc: 'return raw_array_ui8 if success, else throw FileException'
        arguments: raw_array_ui8 'buffer', ui32 'bytes'
            returns: ui32 'readed bytes count'
            throws: FileException
                desc: 'throw FileException if failed'
                
    fun: write
        arguments: raw_array_ui8 'buffer'
            returns: ui32 'written bytes count'
            throws: FileException
                desc: 'throw FileException if failed'
            
    fun: append
        arguments: raw_array_ui8 'buffer'
        desc: 'append to end of file, not affected by pointer and not affect pointer'
        returns: ui32 'written bytes count'
        throws: FileException
            desc: 'throw FileException if failed'
    
    fun: seek_pos
        arguments: ui64 'offset', {opt, def:0 'begin'}pointer_offset 'pointer_offset', {opt, def:0}pointer 'pointer'
            desc: 'seek to offset from specified pointer offset in pointer type,pointer_offset{begin, current, end}, pointer{read, write}'
        arguments: ui64 'offset', {opt, def:0 'begin'}pointer_offset 'pointer_offset'
            desc: 'seek to offset from specified pointer offset in both pointers, basic pointer is write, pointer_offset{begin, current, end}'
        returns: noting

    fun: tell_pos
        arguments: {opt, def:0}pointer 'pointer'
            desc: 'get current pointer position in pointer type, pointer{read, write}'
        returns: ui64 'offset'

    fun: flush
        arguments: ignored
        desc: 'flush file buffers'
        returns: boolean
    
    fun: size
        arguments: ignored
        desc: 'get file size'
        returns: ui64 'size' | noting
            desc: 'return noting if failed to get size, otherwise return size'
        
    fun: get_native_handle
        access: intern
        arguments: ignored
        returns: 'implementation defined'
            oem 'windows': 'return undefined_ptr'

## blocking_file_handle
    thread_block
    desc: 'a lower file handle abstraction on top of native file api, it is blocking, it is not recommended to use in task, useful for communicating with old libraries'
    oem 'windows': 'use win32 api'
    oem: 'not implemented'
    constructor: '# file blocking_file_handle'
        arguments: 
            string 'file_path',
            {opt, def:2 'read_write'}ui8 'open_mode',
            {opt, def:0 'open'}ui8 'on_open_action',
            {opt, def:0}ui8 'sync_flags',
            {opt, def:3}ui8 'share_mode',
            desc:
                "open_mode:
                    0 'read' - open file for read
                    1 'write' - open file for write
                    2 'read_write' - open file for read and write
                    3 'append' - open file for append
                on_open_action:
                    0 'open' - open file if exists, create if not
                    1 'always_new' - create file if not exists, recreate if exists
                    2 'create_new' - create only if not exists, fail if exists
                    3 'open_exists' - just open file if exists, fail if not exists
                    4 'truncate_exists' - truncate file if exists, fail if not exists
                sync_flags:
                    1 'delete_on_close' - delete file on close
                    2 'posix_semantics' - posix semantics
                    4 'random_access' - hint to cache manager to optimize for random access
                    8 'sequential_scan' - hint to cache manager to optimize for sequential scan
                    16 'no_buffering' - hint to cache manager, affect seek and read write operations, like disc page size aligned operations
                    32 'write_through' - hint to cache manager to optimize for write through
                share_mode:
                    0 'none' - no share
                    1 'read' - share read
                    2 'write' - share write
                    4 'delete' - share delete"
        returns: proxy<blocking_file_handle>

    fun: read
        arguments: raw_array_ui8 'buffer'
        returns: ui32
            desc: 'return readed bytes count, if failed return 0'
    
    fun: write
        arguments: raw_array_ui8 'buffer'
        returns: ui32 'written bytes count'
            desc: 'return written bytes count, if failed return 0'

    fun: seek_pos
        arguments: ui64 'offset', {opt, def:0 'begin'}pointer_offset 'pointer_offset'
            desc: 'seek to offset from specified pointer offset, pointer_offset{begin, current, end}'
        returns: boolean
    
    fun: tell_pos
        arguments: ignored
        returns: ui64 'offset'
    
    fun: flush
        arguments: ignored
        desc: 'flush file buffers'
        returns: boolean
    
    fun: size
        arguments: ignored
        desc: 'get file size'
        returns: ui64 'size' | noting
            desc: 'return noting if failed to get size, otherwise return size'
    
    fun: eof_state
        arguments: ignored
        desc: 'get end of file state'
        returns: boolean 'is_eof'
    
    fun: valid
        arguments: ignored
        desc: 'check if file handle is valid'
        returns: boolean 'is_valid'
    
    fun: get_native_handle
        access: intern
        arguments: ignored
        returns: 'implementation defined'
            oem 'windows': 'return undefined_ptr'
    
## text_file
    desc: 'high level abstraction on top of file_handle with text encoding support'
    constructor: '# file text_file'
        arguments: proxy<file_handle> 'file_handle', {opt,def: utf8}ui8 'encoding, {opt,def: native} ui8 'endian'
            desc: 'encoding:
                    0 'utf8' - use utf8 encoding
                    1 'utf16' - use utf16 encoding
                    2 'utf32' - use utf32 encoding
                    3 'ascii' - use ascii encoding
                endian:
                    0 'little' - use little endian
                    1 'big' - use big endian'
        returns: proxy<text_file>

    fun: read_line
        arguments:  ignored
            desc: 'read line from file'
        returns: string
    
    fun: read_word
        arguments: ignored
            desc: 'read word from file'
        returns: string
    
    fun: read_symbol
        arguments: {opt, def:false}boolean 'skip_spaces'
            desc: 'read symbol from file'
        returns: string
    
    fun: write
        arguments: string | any
            desc: 'write string to file, if argument is not string, then convert it to string'
        returns: noting

    fun: read_bom
        arguments: ignored
            desc: 'determine file endian, work only if read pointer is at file begin'
        returns: noting

    fun: init_bom
        arguments: ignored
            desc: 'write file endian to file begin, work only if write pointer is at file begin'
        returns: noting

## folder_browser
    constructor: '# file folder_browser'
    arguments: noting
    arguments: string'folder_path'
    returns: proxy<folder_browser>


## folder_changes_monitor
    constructor: '# file folder_changes_monitor'
    arguments: string'folder_path', {opt, default:false}boolean'handle_depth'
    returns: proxy<folder_changes_monitor>

    fun: start
        arguments: ignored
        desc: 'scan folder and then start async scan'
        returns: noting
    
    fun: lazy_start
        arguments: ignored
        desc: 'start async scan, without syncing, i.e. all will be new'
        returns: noting

    fun: once_scan
        arguments: ignored
        desc: 'start sync scan once'
        returns: noting
    
    fun: stop
        arguments: ignored
        desc: 'stop async scan'
        returns: noting

    fun: get_event_folder_name_change
        arguments: ignored
        returns: proxy<event_system>

    fun: get_event_file_name_change
        arguments: ignored
        returns: proxy<event_system>

    fun: get_event_folder_size_change
        arguments: ignored
        returns: proxy<event_system>

    fun: get_event_file_size_change
        arguments: ignored
        returns: proxy<event_system>

    fun: get_event_folder_creation
        arguments: ignored
        returns: proxy<event_system>

    fun: get_event_file_creation
        arguments: ignored
        returns: proxy<event_system>

    fun: get_event_folder_removed
        arguments: ignored
        returns: proxy<event_system>

    fun: get_event_file_removed
        arguments: ignored
        returns: proxy<event_system>

    fun: get_event_watcher_shutdown
        arguments: ignored
        returns: proxy<event_system>

    fun: get_event_folder_last_access
        arguments: ignored
        returns: proxy<event_system>

    fun: get_event_folder_last_write
        arguments: ignored
        returns: proxy<event_system>

    fun: get_event_file_last_write
        arguments: ignored
        returns: proxy<event_system>

    fun: get_event_folder_security_change
        arguments: ignored
        returns: proxy<event_system>

    fun: get_event_file_security_change
        arguments: ignored
        returns: proxy<event_system>

# internal

## function_builder
    constructor: '# internal function_builder'
    arguments: {opt, def: true}boolean 'strict_mode', {opt, def: true}boolean 'use_dynamic_values'
    returns: proxy<function_builder>

    //TODO put here function_builder methods

## index_pos
    constructor: '# internal index_pos'
    arguments: {opt, def: 0}ui16 'index', {opt, def: 0}ui8 'pos'
    desc: 'pos is enum, 0 - in_enviro, 1 - in_arguments, 2 - in_static, 3 - in_constants'
    returns: proxy<index_pos>

    fun: set_index
        arguments: ui16
        returns: noting

    fun: set_pos
        arguments: ui8
        returns: noting

    fun: get_index
        arguments: ignored
        returns: ui16

    fun: get_pos
        arguments: ignored
        returns: ui8

## line_info
    constructor: '# internal line_info'
    returns: proxy<line_info>

    fun: set_line
        arguments: ui64
        returns: noting

    fun: set_column
        arguments: ui64
        returns: noting

    fun: get_line
        arguments: ignored
        returns: ui64

    fun: get_column
        arguments: ignored
        returns: ui64

## memory
    fun: dump
        concurrent_unsafe
        arguments: any
        returns: faarr[ faarr[undefined_ptr 'from', undefined_ptr 'to', integer 'len',string 'desc',boolean 'is_fault']]
            desc: 'returns array of pages, desc is os depend value, is_fault mean this page is guard page'

## stack
    fun: shrink
        concurrent_unsafe
        arguments: {opt}integer 'threshold'
        desc: 'reduce current stack size'
        returns: boolean
            desc:: 'true if operation success'
    
    fun: prepare
        concurrent_unsafe
        arguments: {opt}integer 'grow_count'
        desc: 'increase stack size'
        returns: boolean
            desc: 'false if there no enough reserved memory'

    fun: reserve
        concurrent_unsafe
        arguments: integer 'grow_count'
        desc: 'prepare stack for heavy operation, do noting if stack enough'
        returns: boolean
            desc: 'false if there no enough reserved memory'


    fun: dump
        concurrent_unsafe
        arguments: ignored
        desc: 'capture stack like @(internal memory dump)'
        returns: faarr[ faarr[undefined_ptr 'from', undefined_ptr 'to', integer 'len',string 'desc',boolean 'is_fault']]
            desc: 'array of pages, desc is os depend value, is_fault mean this page is guard page'

    fun: bs_supported
        arguments: ignored
        returns: boolean
            desc: 'true if all functions in this namespace supported'
        
    fun: used_size
        arguments: ignored
        returns: integer
            desc: 'used stack size'

    fun: unused_size
        arguments: ignored
        returns: integer
            desc: 'free stack size'

    fun: allocated_size
        arguments: ignored
        returns: integer
            desc: 'full stack size(unused + used)'

    fun: free_size
        arguments: ignored
        returns: integer
            desc: 'reserved stack size'
    
    fun: trace
        arguments: {opt}integer 'framesToSkip', {opt}boolean 'include_native', {opt}integer 'max_frames'
        returns: faarr[faarr[string 'file_path',string 'fun_name', integer 'line']]

    fun: clean_trace
        arguments: {opt}integer 'framesToSkip', {opt}boolean 'include_native', {opt}integer 'max_frames'
        desc: 'removes from trace self and entry points and trampolines'
        returns: faarr[faarr[string 'file_path',string 'fun_name', integer 'line']]

    fun: trace_frames        
        arguments: {opt}integer 'framesToSkip', {opt}boolean 'include_native', {opt}integer 'max_frames'
        returns: faarr[undefined_ptr 'rip',...]
    
    fun: resolve_frame
        arguments: undefined_ptr 'rip frame', {opt}boolean 'include_native'
        returns: faarr[string 'file_path',string 'fun_name', integer 'line']

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
    
    fun: gc_hint_collect
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
    arguments: string 'os resolvable path'
    desc: 'do not use functions from this instance when destructor called'
    returns: proxy<native_lib>
    

    fun: get_function
        arguments: string 'symbolic name', proxy<native_template>
        desc: 'native_template native template used here for casting'
        returns: function
    
# net

## ip
    constructor: '# net ip'
        arguments: string 'ip6/ip4', {opt, def: 0}integer 'port'
        returns: proxy<universal_address>

    constructor: '# net ip#v6'
        arguments: string 'ip6', {opt, def: 0}integer 'port'
        returns: proxy<universal_address>

    constructor: '# net ip#v4'
        arguments: string 'ip4', {opt, def: 0}integer 'port'
        returns: proxy<universal_address>

    constructor: '# net ip#address'
        arguments: string 'ip:port'
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
    //TODO fix tcp_server constructor arguments and functions
    OUTDATED
    desc: 'high efficient tcp server with dual stack ip support'
    oem 'windows': 'use winsock2 with iocp'
    oem 'linux': 'use liburing'
    oem: 'not implemented'

    constructor: '# net tcp_server'
        arguments: function 'handler', proxy<universal_address>, {opt, def: 1ui8 'write_delayed'}ui8 'manage_type', {opt, def: 10}integer 'acceptors', {opt, def: 8192, non_negative}i32 'default_buffer'
            desc: 'enum manage_type{ blocking = 0, write_delayed = 1 }'
            desc 'handler': 'function(proxy<tcp_network_stream || tcp_network_blocking>, proxy<universal_address> 'local_ip', proxy<universal_address> 'client ip') -> void'
        returns: proxy<tcp_server>

    fun: start
        arguments: ignored
        desc: 'start server'
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
    
    fun: await
        arguments: ignored
        desc: 'wait until server will be stopped'
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

    fun: set_default_buffer_size
        arguments: {non_negative}i32
        desc: 'set default buffer size for new connections'
        returns: noting

    fun: set_accept_filter
        arguments: function 'filter'
            desc 'filter': 'function(proxy<universal_address> 'client_ip',proxy<universal_address> 'local_ip') -> boolean'
        desc: 'set filter for new connections, if filter return false, connection will be closed, called before construction of client_context'
            desc: 'client_context is proxy<tcp_network_stream || tcp_network_blocking>'
        returns: noting
        
### tcp_network_stream
    desc: 'delay write to the network, when has available read data'
    constructor: none
        desc: 'can get in tcp_server when used write_delayed flag'

    fun: read_available_ref
        arguments: ignored
        desc: 'get reference internal buffer for reading'
        returns: ref<raw_array_ui8>
    
	fun: read_available
        arguments: ignored
        returns: raw_array_ui8
    
	fun: data_available
        arguments: ignored
        returns: boolean

	fun: write
        arguments: raw_array_ui8
        returns: noting
        
	fun: write_file
        arguments: string 'file_path', {opt, def: 0}ui64 'offset', {opt, def: 0}ui64 'bytes', {opt, def: 0}ui32 'block_size'
        arguments: proxy<file_handle>, {opt, def: 0}ui64 'offset', {opt, def: 0}ui64 'bytes', {opt, def: 0}ui32 'block_size'
        arguments: proxy<blocking_file_handle>, {opt, def: 0}ui64 'offset', {opt, def: 0}ui64 'bytes', {opt, def: 0}ui32 'block_size'
        desc: 'send up to end of file if bytes == 0'
        warn: 'this function erase available read data, will be lost'
        returns: noting

	fun: force_write
        arguments: ignored
        desc: 'flush write buffer to socket, all available read data will be cached'
        returns: noting

	fun: force_write_and_close
        arguments: raw_array_ui8
        desc: 'ignore all buffers and send data, then close socket, useful to close connection without waiting for all data to be sent'
        returns: noting

	fun: close
        arguments: ignored
        desc: 'close socket'
        returns: noting

	fun: reset
        arguments: ignored
        desc: 'close socket without waiting for all data to be sent, useful when need close bad connection'
        returns: noting

	fun: rebuffer
        arguments: {non_negative}i32
        desc: 'set new buffer size for shared buffer'
        returns: noting

	fun: is_closed
        arguments: ignored
        desc: 'check if socket is closed'
        returns: boolean

	fun: error
        arguments: ignored
        desc: 'get error code of socket'
        returns: ui8
            desc: 'enum tcp_error{none = 0, remote_close = 1, local_close = 2, local_reset = 3, read_queue_overflow = 4, invalid_state = 5,undefined_error = 0xFF}'

    fun: local_address
        arguments: ignored
        returns: proxy<ip>

    fun: remote_address
        arguments: ignored
        returns: proxy<ip>

### tcp_network_blocking
    constructor: none
        desc: 'can get in tcp_server when used blocking flag'
        desc: 'every function is forcibly and not use internal buffers'

    fun: read
        arguments: ui32 'bytes'
        returns: raw_array_ui8
            desc: 'return count of bytes that was readed'

	fun: available_bytes
        arguments: ignored
        returns: ui32
            desc: 'return count of bytes that can be readed'

	fun: write
        arguments: raw_array_ui8 'buffer'
        returns: ui32
            desc: 'return count of bytes that was sent'

	fun: write_file
        arguments: string 'file_path', {opt, def: 0}ui64 'offset', {opt, def: 0}ui64 'bytes', {opt, def: 0}ui32 'block_size'
        arguments: proxy<file_handle>, {opt, def: 0}ui64 'offset', {opt, def: 0}ui64 'bytes', {opt, def: 0}ui32 'block_size'
        arguments: proxy<blocking_file_handle>, {opt, def: 0}ui64 'offset', {opt, def: 0}ui64 'bytes', {opt, def: 0}ui32 'block_size'
        desc: 'send up to end of file if bytes == 0'
        returns: noting

	fun: close
        arguments: ignored
        desc: 'close socket'
        returns: noting

	fun: reset
        arguments: ignored
        desc: 'close socket without waiting for all data to be sent, useful when need close bad connection'
        returns: noting

	fun: rebuffer
        arguments: {non_negative}i32
        desc: 'set new buffer size for shared buffer'
        returns: noting

	fun: is_closed
        arguments: ignored
        desc: 'check if socket is closed'
        returns: boolean

	fun: error
        arguments: ignored
        desc: 'get error code of socket'
        returns: ui8
            desc: 'enum tcp_error{none = 0, remote_close = 1, local_close = 2, undefined_error = 0xFF}'

    fun: local_address
        arguments: ignored
        returns: proxy<ip>

    fun: remote_address
        arguments: ignored
        returns: proxy<ip>

## tcp_client
    OUTDATED
    desc: 'tcp client abstraction to mimic posix socket api but not 100% compatible'
    compare 'table': '
        |---------------------------|-------------------------------|
        | posix sync				| attacha tcp_client async	    |
        |---------------------------|-------------------------------|
        | socket()					| (inlined in connect)			|
        | connect(socket,ip, port)	| connect(ip_port)				|
        | recv(socket,data, size)	| .recv(data, size)				|
        | send(socket,data, size)	| .send(data, size)				|
        | 							| .send_file(file_path,...)		|
        | 							| .send_file(file handle,...)	|
        | close(socket)				| .close()						|
        |---------------------------|-------------------------------|'

    compare 'desc': '
        send_file functions used in http server, and in file transfer protocols
        attacha cannot use sync functions, because it block workers threads and
        tcp_client abstraction created to provide sync like api but internally
        is async send function cache data in internal buffer if socket has incoming data,
        and send when read buffer is empty'
    
    oem 'windows': 'use winsock2 with iocp'
    oem 'linux': 'use liburing'
    oem: 'not implemented'
    creator: 'net tcp_client_connect'
        arguments: proxy<universal_address>, {opt, def: 0, constraint: v>0}i32 'timeout_ms'
        arguments: proxy<universal_address>, raw_array_ui8 'on_connect_data', {opt, def: 0, constraint: v>0}i32 'timeout_ms'
        arguments: proxy<universal_address>, raw_array_i8 'on_connect_data', {opt, def: 0, constraint: v>0}i32 'timeout_ms'
        returns: proxy<tcp_client>

    fun: recv
        arguments: raw_array_ui8 'buffer', ui32 'bytes'
        returns: i32
            desc: 'return -1 if connection closed, 0 if no data, else count of bytes'
    
    fun: send
        arguments: raw_array_ui8 'buffer', ui32 'bytes'
        returns: boolean
            desc: 'return false if connection closed, else true'

    fun: send_file
        arguments: string 'file_path', {opt, def: 0}ui64 'offset', {opt, def: 0}ui64 'bytes', {opt, def: 0}ui32 'block_size'
        arguments: proxy<file_handle>, {opt, def: 0}ui64 'offset', {opt, def: 0}ui64 'bytes', {opt, def: 0}ui32 'block_size'
        arguments: proxy<blocking_file_handle>, {opt, def: 0}ui64 'offset', {opt, def: 0}ui64 'bytes', {opt, def: 0}ui32 'block_size'
        desc: 'send up to end of file if bytes == 0'
        returns: boolean
            desc: 'return false if connection closed, else true'

    fun: close
        arguments: ignored
        desc: 'close connection'
        returns: noting

	fun: reset
        arguments: ignored
        desc: 'close connection without waiting for all data to be sent, useful when need close bad connection'
        returns: noting
    
	fun: rebuffer
        arguments: {non_negative}i32
        desc: 'set new buffer size for shared buffer'
        returns: noting


## udp_socket
    oem 'windows': 'use winsock2 with iocp'
    oem 'linux': 'use liburing'
    oem: 'not implemented'
    constructor: '# net udp_socket'
        arguments: proxy<universal_address>, {opt, def: 0}ui32 'timeout_ms'
            desc: 'address will be used for recv operations'
        returns: proxy<udp_socket>

    fun: recv
        arguments: raw_array_ui8 'buffer', ui32 'bytes', {result}proxy<universal_address>
        returns: boolean
            desc: 'received bytes'

    fun: send
        arguments: raw_array_ui8 'buffer', ui32 'bytes', proxy<universal_address>
        returns: boolean
            desc: 'sended bytes'

# parallel
    fun: create_thread
        arguments: function, any...
        returns: noting

    fun: create_thread_and_wait
        arguments: function, any...
        returns: any

    fun: create_async_thread
        arguments: function, any...
        returns: async_res
    
    fun: create_task
        arguments: s/faarr[function, {opt}function 'fault_function',{opt}time_point 'timeout', boolean 'used_task_local'], any 'arguments'
        returns: async_res

## task_runtime
    fun: await_end_tasks
    fun: await_no_tasks
    fun: become_task_executor
    fun: clean_up
    fun: create_executor
    fun: explicitStartTimer
    fun: reduce_executor
    fun: total_executors
    fun: create_bind_only_executor
    fun: close_bind_only_executor


## this_task
    fun: check_cancellation
    fun: is_task
    fun: self_cancel
    fun: sleep
    fun: sleep_until
    fun: task_id
    fun: yield
    fun: yield_result

## native

### mutex
    constructor: '# parallel native mutex'

### rw_mutex
    constructor: '# parallel native rw_mutex'

### recursive_mutex
    constructor: '# parallel native recursive_mutex'

### timed_mutex
    constructor: '# parallel native timed_mutex'

### condition_variable
    constructor: '# parallel native condition_variable'

### thread
    constructor: '# parallel native thread'



### this_thread
    fun: get_id
    fun: sleep_for
    fun: sleep_until
    fun: yield

## atomic

### any
    constructor: '# parallel atomic any'

### boolean
    constructor: '# parallel atomic boolean'

### i8
    constructor: '# parallel atomic i8'

### i16
    constructor: '# parallel atomic i16'

### i32
    constructor: '# parallel atomic i32'

### i64
    constructor: '# parallel atomic i64'

### ui8
    constructor: '# parallel atomic ui8'

### ui16
    constructor: '# parallel atomic ui16'

### ui32
    constructor: '# parallel atomic ui32'

### ui64
    constructor: '# parallel atomic ui64'

### float
    constructor: '# parallel atomic float'

### double
    constructor: '# parallel atomic double'

### undefined_ptr
    constructor: '# parallel atomic undefined_ptr'

## mutex
    constructor: '# parallel mutex'
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
    constructor: '# parallel condition_variable'
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
        arguments: integer 'timeout_ms'
        desc: 'unsafe wait(without locks)'
        returns: boolean
            desc: 'false if timed out'
    
    fun: wait
        arguments: proxy<mutex>, integer 'timeout_ms'
        desc: 'unlock mutex when on wait, and lock back when notified'
        returns: boolean
            desc: 'false if timed out'

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
    constructor: '# parallel semaphore'
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
        arguments: integer 'timeout_ms'
        returns: boolean

    fun: try_lock_until
        arguments: time_point
        returns: boolean

    fun: is_locked
        arguments: ignored
        returns: boolean
    
## event_system
    constructor: '# parallel event_system'
        arguments: ignored
        returns: proxy<event_system>

    fun: @symbols::structures::add_operator
        arguments: function
        desc: 'same as fun: join(fun, false, 2)'
        returns: noting

    fun: join
        arguments: function, {opt}boolean 'is_async', {opt, def:2ui8}ui8 'priority'
        desc: 'enum priority{ heigh = 0, upper_avg= 1, avg= 2, lower_avg = 3, low = 4}' 
        returns: noting

    fun: leave
        arguments: function, {opt}boolean 'is_async', {opt, def:2ui8}ui8 'priority'
        desc: 'enum priority{ heigh = 0, upper_avg= 1, avg= 2, lower_avg = 3, low = 4}' 
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
        desc: 'same as sync_notify, but async handlers handled by priority and start next when prev not returned true'
        returns: boolean
            desc: 'true if some event canceled'

	fun: async_notify
        arguments: any...
            desc: 'arguments passed to handlers'
        desc: 'execute all as like async in await_notify, returns task for awaiting'
        returns: async_res
            desc: 'true if some event canceled'

## task_limiter
    constructor: '# parallel task_limiter'
        arguments: ignored
        desc: 'limit max count task executing one/multiply fragment/s code'
        returns: proxy<task_limiter>

    fun: set_max_threshold
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
        arguments: integer 'timeout_ms'
        returns: boolean

    fun: try_lock_until
        arguments: time_point
        returns: boolean

## task_query
    constructor: '# parallel task_query'
        arguments: {opt}integer 'max_at_execution'
        desc: 'start tasks by query'
        returns: proxy<task_query>

    fun: add_task
        arguments: s/faarr[function, {opt}function 'fault_function',{opt}time_point 'timeout', boolean 'used_task_local'], any 'arguments'
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
    
## task
    constructor: '# parallel task'
    arguments: async_res
    arguments: proxy<task>
    arguments: function'entry', {opt}function'non catched exception', time_point 'timeout', boolean 'used_task_local', any...'arguments'
    desc: 'Provides full control over tasks'
    returns: proxy<task>

    fun: schedule
        arguments: ui64'milliseconds wait'
        returns: noting

    fun: schedule_until
        arguments: time_point'start on'
        returns: noting
    
    fun: start
        arguments: ignored
        returns: noting
    
    fun: yield_iterate
        arguments: ignored
        returns: boolean'result can be available'

    fun: get_result
        arguments: {opt}ui64'result number'
        desc: 'wait if result with this id currently not available, returns noting if result not available or task returned noting, returns copy of value that returned by task'
        returns: any

    fun: @symbols::structures::index_operator
        alias_method: get_result

    fun: @symbols::structures::size
        arguments:noting
        returns: ui64'currently available results from task'

    fun: has_result
        arguments: {opt}ui64'result number'
        returns: boolean

    fun: await_task
        arguments: {opt, default:true}boolean'make start'
        returns: noting

    fun: await_results
        arguments: noting
        desc: 'start task and await all results, then return'
        returns: uarr<any>

    fun: notify_cancel
        arguments: noting
        desc: 'mark task for cancellation or send cancel request'
        returns: noting

    fun: set_auto_bind_worker
        arguments: boolean'make auto bind'
        desc: 'detach task from workers and mark it to bind to free workers'
        returns: noting

    fun: set_worker_id
        arguments: ui16
        desc: manually bind task to another worker
        returns: noting

    fun: @symbols::structures::convert::to_uarr
        alias_method: get_results

    fun: @symbols::structures::convert::to_string
        arguments: noting
        returns: string'full string representation of current state of results'

    fun: @symbols::structures::convert::to_ui8
        alias_of: (ui8)get_result(0)

    fun: @symbols::structures::convert::to_ui16
        alias_of: (ui16)get_result(0)

    fun: @symbols::structures::convert::to_ui32
        alias_of: (ui32)get_result(0)

    fun: @symbols::structures::convert::to_ui64
        alias_of: (ui64)get_result(0)
        
    fun: @symbols::structures::convert::to_i8
        alias_of: (i8)get_result(0)

    fun: @symbols::structures::convert::to_i16
        alias_of: (i16)get_result(0)

    fun: @symbols::structures::convert::to_i32
        alias_of: (i32)get_result(0)

    fun: @symbols::structures::convert::to_i64
        alias_of: (i64)get_result(0)

    fun: @symbols::structures::convert::to_float
        alias_of: (float)get_result(0)

    fun: @symbols::structures::convert::to_double
        alias_of: (double)get_result(0)

    fun: @symbols::structures::convert::to_boolean
        alias_of: (boolean)get_result(0)

    fun: @symbols::structures::convert::to_timepoint
        alias_of: (timepoint)get_result(0)

    fun: @symbols::structures::convert::to_type_identifier
        alias_of: (type_identifier)get_result(0)

    fun: @symbols::structures::convert::to_function
        alias_of: (function)get_result(0)

    fun: @symbols::structures::convert::to_map
        alias_of: (map)get_result(0)

    fun: @symbols::structures::convert::to_set
        alias_of: (set)get_result(0)
    
    fun: @symbols::structures::convert::to_ui8_arr
        alias_of: (ui8[])get_result(0)

    fun: @symbols::structures::convert::to_ui16_arr
        alias_of: (ui16[])get_result(0)

    fun: @symbols::structures::convert::to_ui32_arr
        alias_of: (ui32[])get_result(0)

    fun: @symbols::structures::convert::to_ui64_arr
        alias_of: (ui64[])get_result(0)

    fun: @symbols::structures::convert::to_i8_arr
        alias_of: (i8[])get_result(0)

    fun: @symbols::structures::convert::to_i16_arr
        alias_of: (i16[])get_result(0)

    fun: @symbols::structures::convert::to_i32_arr
        alias_of: (i32[])get_result(0)

    fun: @symbols::structures::convert::to_i64_arr
        alias_of: (i64[])get_result(0)

    fun: @symbols::structures::convert::to_float_arr
        alias_of: (float[])get_result(0)

    fun: @symbols::structures::convert::to_double_arr
        alias_of: (double[])get_result(0)

    fun: @symbols::structures::convert::to_faarr
        alias_of: (any[])get_result(0)

    fun: @symbols::structures::iterable::begin
        arguments: ignored
        returns: proxy<task_result_iterator>

    fun: @symbols::structures::iterable::end
        arguments: ignored
        desc: 'also await task to complete'
        returns: proxy<task_result_iterator>

## task_result_iterator
    constructor: none

    fun: @symbols::structures::iterable::next
    fun: @symbols::structures::iterable::prev
    fun: @symbols::structures::iterable::get
    fun: @symbols::structures::iterable::begin
    fun: @symbols::structures::iterable::end

## task_group
    constructor: '# parallel task_group'
    arguments: {look: 'file: parallel.cpp, first_join: ___createProxy_TaskGroup__push_item'}vary...
    returns: proxy<task_group>

    fun: start
    fun: await_multiple
    fun: await_results
    fun: notify_cancel
    fun: @symbols::structures::convert::to_ui8_arr
    fun: @symbols::structures::convert::to_ui16_arr
    fun: @symbols::structures::convert::to_ui32_arr
    fun: @symbols::structures::convert::to_ui64_arr
    fun: @symbols::structures::convert::to_i8_arr
    fun: @symbols::structures::convert::to_i16_arr
    fun: @symbols::structures::convert::to_i32_arr
    fun: @symbols::structures::convert::to_i64_arr
    fun: @symbols::structures::convert::to_float_arr 
    fun: @symbols::structures::convert::to_double_arr
    fun: @symbols::structures::convert::to_faarr
    fun: @symbols::structures::convert::to_set
    fun: @symbols::structures::convert::to_uarr
    fun: @symbols::structures::add_operator

# math
    fun: abs
        arguments: number
            desc: 'get absolute value'
        arguments: raw_array 'numbers'
            desc: 'get absolute values'
        arguments: uarr 'numbers' | faarr 'numbers' | saarr 'numbers'
            desc: 'get absolute values, not modify non number values'
        
        returns: {argument}
    
    fun: min
        arguments: uarr 'numbers' | faarr 'numbers' | saarr 'numbers' | raw_array 'numbers'
            desc: 'get minimum value from array'
        arguments: number...
            desc: 'get minimum value from arguments'
        returns: number
    
    fun: max
        arguments: uarr 'numbers' | faarr 'numbers' | saarr 'numbers' | raw_array 'numbers'
            desc: 'get maximum value from array'
        arguments: number...
            desc: 'get maximum value from arguments'
        returns: number

    fun: median
        arguments: uarr 'numbers' | faarr 'numbers' | saarr 'numbers' | raw_array 'numbers'
            desc: 'get median value from array'
        arguments: number...
            desc: 'get median value from arguments'
        returns: number

    fun: range
        arguments: uarr 'numbers' | faarr 'numbers' | saarr 'numbers' | raw_array 'numbers'
            desc: 'get min max value from array and return difference'
        arguments: number...
            desc: 'get min max value from arguments and return difference'
        returns: number

    fun: mode
        arguments: uarr 'numbers' | faarr 'numbers' | saarr 'numbers' | raw_array 'numbers'
            desc: 'get mode value from array'
        arguments: number...
            desc: 'get mode value from arguments'
        returns: number
    
    fun: round
        arguments: floats
            desc: 'round value'
        arguments: raw_array 'floats'
            desc: 'round values'
        arguments: uarr 'floats' | faarr 'floats' | saarr 'floats'
            desc: 'round values, not modify non number values'
        returns: {argument}

    fun: floor
        arguments: floats
            desc: 'floor value'
        arguments: raw_array 'floats'
            desc: 'floor values'
        arguments: uarr 'floats' | faarr 'floats' | saarr 'floats'
            desc: 'floor values, not modify non number values'
        returns: {argument}
    
    fun: ceil
        arguments: floats
            desc: 'ceil value'
        arguments: raw_array 'floats'
            desc: 'ceil values'
        arguments: uarr 'floats' | faarr 'floats' | saarr 'floats'
            desc: 'ceil values, not modify non number values'
        returns: {argument}

    fun: trunc
        arguments: floats
            desc: 'trunc value'
        arguments: raw_array 'floats'
            desc: 'trunc values'
        arguments: uarr 'floats' | faarr 'floats' | saarr 'floats'
            desc: 'trunc values, not modify non number values'
        returns: {argument}

    fun: factorial
        arguments: number
            desc: 'get factorial value'
        arguments: raw_array 'numbers'
            desc: 'get factorial values'
        arguments: uarr 'numbers' | faarr 'numbers' | saarr 'numbers'
            desc: 'get factorial values, not modify non number values'
        returns: {argument}

    fun: sin
        arguments: floating | integer
            desc: 'get sin value'
        returns: floating

    fun: asin
        arguments: floating | integer
            desc: 'get asin value'
        returns: floating

    fun: cos
        arguments: floating | integer
            desc: 'get cos value'
        returns: floating
    
    fun: acos
        arguments: floating | integer
            desc: 'get acos value'
        returns: floating
    
    fun: tan
        arguments: floating | integer
            desc: 'get tan value'
        returns: floating
    
    fun: atan
        arguments: floating | integer
            desc: 'get atan value'
        returns: floating
    
    fun: sec
        arguments: floating | integer
            desc: 'get sec value'
        returns: floating
    
    fun: asec
        arguments: floating | integer
            desc: 'get asec value'
        returns: floating

    fun: csc
        arguments: floating | integer
            desc: 'get csc value'
        returns: floating

    fun: acsc
        arguments: floating | integer
            desc: 'get acsc value'
        returns: floating
    
    fun: cot
        arguments: floating | integer
            desc: 'get cot value'
        returns: floating
    
    fun: acot
        arguments: floating | integer
            desc: 'get acot value'
        returns: floating
    
    fun: sinh
        arguments: floating | integer
            desc: 'get sinh value'
        returns: floating
    
    fun: asinh
        arguments: floating | integer
            desc: 'get asinh value'
        returns: floating

    fun: cosh
        arguments: floating | integer
            desc: 'get cosh value'
        returns: floating
    
    fun: acosh
        arguments: floating | integer
            desc: 'get acosh value'
        returns: floating
    
    fun: tanh
        arguments: floating | integer
            desc: 'get tanh value'
        returns: floating
    
    fun: atanh
        arguments: floating | integer
            desc: 'get atanh value'
        returns: floating
    
    fun: sech
        arguments: floating | integer
            desc: 'get sech value'
        returns: floating
    
    fun: asech
        arguments: floating | integer
            desc: 'get asech value'
        returns: floating

    fun: csch
        arguments: floating | integer
            desc: 'get csch value'
        returns: floating
    
    fun: acsch
        arguments: floating | integer
            desc: 'get acsch value'
        returns: floating
    
    fun: coth
        arguments: floating | integer
            desc: 'get coth value'
        returns: floating
    
    fun: acoth
        arguments: floating | integer
            desc: 'get acoth value'
        returns: floating

    fun: sqrt
        arguments: floating | integer
            desc: 'get sqrt value'
        returns: floating
    
    fun: cbrt
        arguments: floating | integer
            desc: 'get cbrt value'
        returns: floating
    
    fun: pow
        arguments: uarr 'numbers' | faarr 'numbers' | saarr 'numbers' | raw_array 'numbers' | number...
            desc: 'get pow value'
        returns: {argument}

    fun: log
        arguments: floating | integer
            desc: 'get log value'
        returns: floating
    
    fun: log2
        arguments: floating | integer
            desc: 'get log2 value'
        returns: floating
    
    fun: log10
        arguments: floating | integer
            desc: 'get log10 value'
        returns: floating
    
# debug

    fun: start
        desc: 'initialize debug, can be unavailable when used initStandardLib_safe'
        returns: noting

    fun: thread_id
        desc: 'get current thread id'
        returns: integer

    fun: set_thread_name
        arguments: string
            desc: 'set current thread name'
        returns: noting
    
    fun: get_thread_name
        arguments: ignored
        desc: 'get current thread name'
        returns: string
    
    fun: invite
        arguments: string 'reason'
            desc: 'invite user to debug'
        returns: noting

# configuration

    fun: modify
        arguments: string 'key', string 'value'
            desc: 'modify configuration'
        returns: noting
    
    fun: get
        arguments: string 'key'
            desc: 'get configuration'
        returns: string

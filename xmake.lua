---------------------------------top xmake.lua set--------------------------------------
set_project("my_test") --设置工程名称
set_version("0.0.2", {build = "%Y%m%d%H%M"})--设置版本历史
set_xmakever("2.7.8")--设置xmake最低版本
add_rules("mode.release", "mode.debug")--添加规则？
-----------------------------------------------------------------------------------------


---------------------------------set exe or binary name----------------------------------
local USER_PROJECT_NAME = "ring_buffer"   --default project_name---------------------------------
-----------如果定义在windows中设置了环境变量PROJECT_NAME,那么从新得到USER_PROJECT_NAME------
if os.getenv("PROJECT_NAME") then 
    USER_PROJECT_NAME = os.getenv("PROJECT_NAME")
end
--
--print(USER_PROJECT_NAME)
TOP_DIR  = os.curdir()----------------------------------------------得到顶层的xmake.lua路径
-------------------------------------------------------------------print(USER_PROJECT_NAME)
--set_config("win","windows")--设置单核cpu or smp

--------------------------------------操作编译选项，选择哪个编译器和目标机器-------------------
option("win") --构建选项
    set_default(true)--默认true
    set_showmenu(true)--显示菜单·
    set_category("win")--设置分类选项
    set_description("Enable or disable win Plat", "  =y|n")--描述信息
option_end()--构建选项结束
--------------------------------------------------------------------------------------------
   set_config("plat", "linux")
   --set_config("sdk","C:/Users/AppData/mingw64")
-------------------------------------编译工具链判断------------------------------------------
if has_config("win") then--判断是否定义或者传入win，如果传入，那么用mingw
   
end 

includes(TOP_DIR .. "/lib/")--包含project目录
---------------------------------------------------------------------------------------------
local TARGET_NAME = "ring_buffer_test"

target(TARGET_NAME)
-- [don't edit] ---
    	set_kind("binary")
	
	
	--add_global_deps(TARGET_NAME)
-- [   end    ] ---
		--包含库文件
		--add_deps("base")--设置依赖,依赖就是设置让链接器知道去哪里找到函数
		add_deps("ring_buffer")--设置依赖，这个依赖是另外的target

        --私有头文件路径
        --add_includedirs("inc", {public = true})
    	--add_includedirs("lib/inc", {public = true})

    	--私有宏定义
    	--add_defines("DEBUG")
    	--源文件包含
        add_files("main.c")
        add_ldflags(" /usr/lib/x86_64-linux-gnu/libpthread-2.31.so ",{force = true })
    	--add_srcdir("")
    	-- add_files("src/portable/risc-v/n310/src/*.S")
    	--对外头文件
    	--add_headerfiles("inc/*.h", {prefixdir = "os"})
	
target_end()

-------------------控制子文件编译设置，具体编译哪个文件夹下面的xmake.lua--------------------------




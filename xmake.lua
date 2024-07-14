-- add_requires("llvm 17.x", {alias = "llvm-17"})
--- set_toolchains("llvm@llvm-17")

toolchain("my-clang")
    -- mark as standalone toolchain
    set_kind("standalone")

    -- set toolset
    set_toolset("cc", "clang-18")
    set_toolset("cxx", "clang-18", "clang++-18")
    set_toolset("sh", "clang++-18", "clang-18")
    set_toolset("ar", "ar")
    set_toolset("ex", "ar")
    set_toolset("strip", "strip")
    set_toolset("mm", "clang-18")
    set_toolset("mxx", "clang-18", "clang++-18")
    set_toolset("as", "clang-18")

    add_defines("MYCLANG")

    -- check toolchain
    on_check(function (toolchain)
        return import("lib.detect.find_tool")("clang")
    end)

toolchain_end()

set_languages("c++23")
add_requires("microsoft-gsl", { alias = "gsl" })
add_requires("xxhash")
add_cxxflags("-stdlib=libstdc++")
set_options("debugger", "lldb")
set_toolchains("my-clang")

-- 如果当前编译模式是debug
if is_mode("debug") then
    -- 添加DEBUG编译宏
    add_defines("DEBUG")
    -- 启用调试符号
    set_symbols("debug")
    -- 禁用优化
    set_optimize("none")
end

-- 如果是release或者profile模式
if is_mode("release", "profile") then
    -- 如果是release模式
    if is_mode("release") then
        -- 隐藏符号
        set_symbols("hidden")
        -- strip所有符号
        set_strip("all")
        -- 忽略帧指针
        add_cxflags("-fomit-frame-pointer")
        add_mxflags("-fomit-frame-pointer")
    -- 如果是profile模式
    else
        -- 启用调试符号
        set_symbols("debug")
    end

    -- 添加扩展指令集
    add_vectorexts("sse2", "sse3", "ssse3", "avx2", "avx512")
end

set_policy("build.merge_archive", true)
add_packages("gsl", "xxhash")
set_pmxxheader("include/common.h")



target("MagClust", function()
    set_kind("binary")
    add_includedirs("include")
    add_files("src/*.cpp")
end)

target("test", function()
    set_default(false)
    add_includedirs("include")
    add_includedirs("test")
    add_files("src/*.cpp")
    remove_files("src/main.cpp")
    add_files("test/*.cpp")
    add_tests("default")
end)
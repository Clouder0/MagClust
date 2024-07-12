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
set_options("debugger", "lldb")
set_toolchains("my-clang")
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
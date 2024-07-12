-- add_requires("llvm 17.x", {alias = "llvm-17"})
--- set_toolchains("llvm@llvm-17")

toolchain("my-clang")
    -- mark as standalone toolchain
    set_kind("standalone")

    -- set toolset
    set_toolset("cc", "clang-18")
    set_toolset("cxx", "clang-18", "clang++-18")
    set_toolset("sh", "clang++-18", "clang-18")
    set_toolset("mm", "clang-18")
    set_toolset("mxx", "clang-18", "clang++-18")
    set_toolset("as", "clang-18")

    add_defines("MYCLANG")

    -- check toolchain
    on_check(function (toolchain)
        return import("lib.detect.find_tool")("clang")
    end)

toolchain_end()

set_toolchains("my-clang")
set_languages("c++23")
set_options("debugger", "lldb")

add_requires("microsoft-gsl", {alias = "gsl"})

target("MagClust", function()
    set_kind("binary")
    add_includedirs("include")
    add_files("src/*.cpp")
    add_packages("gsl")
end)
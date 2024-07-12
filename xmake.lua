add_requires("llvm 17.x", {alias = "llvm-17"})
set_toolchains("llvm@llvm-17")
-- set_toolchains("clang")
set_languages("c++23")
set_options("debugger", "lldb")

add_requires("microsoft-gsl", {alias = "gsl"})

target("MagClust", function()
    set_kind("binary")
    add_includedirs("include")
    add_files("src/*.cpp")
    add_packages("gsl")
end)
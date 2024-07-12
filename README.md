# MagClust

Block Clustering for data of huge magnitude.

## Contribute

Use `just` as task runner.

Use xmake as build system.

### Quick Start

Ensure you have `just` prepared. You can install via system package manager or whatever method you like.

First install dependencies. We use LLVM-18 toolchain.

```bash
xmake require
```

After installing dependencies, you can build the project and run.

```bash
just build
just run
```

Make use of `compile_commands.json`. Clangd is recommended. Generally you should not have to set compile-commands-dir explicitly, as it is generated in project root dir every time build runs. You can manually re-generate it by:

```bash
just generate
```

Format code using:

```bash
just format
```

Run static analyze by:

```bash
just check
```

You can auto-fix errors if you trust clang-tidy enough:

```bash
just fix
```

Always do tests after modifying code:

```bash
just test
```

Don't forget to ensure clang-format and clang-tidy are ready!

### Code Guidelines

We use C++23 Standard. Modern C++ features are heavily used.

We follow CPP Core Guidelines.

Our target platform is x86-64.

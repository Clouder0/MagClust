setup:
    meson setup builddir

build:
    meson compile -C builddir

run:
    ./builddir/src/magclust

clean-run: build run

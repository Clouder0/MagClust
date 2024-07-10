setup:
    meson setup builddir

build:
    meson compile -C builddir
    meson install -C builddir 

run:
    ./builddir/bin/magclust

clean-run: build run

build:
    xmake

run: build
    xmake run
    
run-old: # don't remake
    xmake run
    
clean:
    xmake clean

clean-run: clean run

format:
    xmake format

check:
    xmake check api
    xmake check clang.tidy
    
fix:
    xmake check clang.tidy --fix

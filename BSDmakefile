.if exists(/usr/local/bin/clang40)
    CC=/usr/local/bin/clang40
    CXX=/usr/local/bin/clang++40
.elif exists(/usr/local/bin/clang38)
    CC=/usr/local/bin/clang38
    CXX=/usr/local/bin/clang++38
.else
    CC=/usr/bin/clang
    CXX=/usr/bin/clang++
.endif

.export CC
.export CXX

.include "COMMONmakefile"

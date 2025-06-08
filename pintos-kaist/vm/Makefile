include ../Makefile.kernel
.PHONY: vm
vm:
	make clean && \
	make -j && \
	cd ./build && \
	make check 2>&1 | tee ../log.txt


.PHONY: debuga
debuga:
	make clean && \
	make -j && \
	cd ./build && \
	pintos -v -k -T 60 -m 20   --fs-disk=10 -p tests/vm/mmap-kernel:mmap-kernel -p ../../tests/vm/sample.txt:sample.txt --swap-disk=4 -- -q   -f run mmap-kernel

.PHONY: debugb
debugb:
	make clean && \
	make -j && \
	cd ./build && \
	pintos -v -k -T 60 -m 20   --fs-disk=10 -p tests/vm/mmap-kernel:mmap-kernel -p ../../tests/vm/sample.txt:sample.txt --swap-disk=4 -- -q   -f run mmap-kernel
.PHONY: debugc
debugc:
	make clean && \
	make -j && \
	cd ./build && \
	pintos -v -k -T 60 -m 20   --fs-disk=10 -p tests/vm/mmap-kernel:mmap-kernel -p ../../tests/vm/sample.txt:sample.txt --swap-disk=4 -- -q   -f run mmap-kernel
.PHONY: debugd
debugd:
	make clean && \
	make -j && \
	cd ./build && \
	pintos -v -k -T 60 -m 20   --fs-disk=10 -p tests/vm/mmap-kernel:mmap-kernel -p ../../tests/vm/sample.txt:sample.txt --swap-disk=4 -- -q   -f run mmap-kernel

.PHONY: debuge
debuge:
	make clean && \
	make -j && \
	cd ./build && \
	pintos -v -k -T 60 -m 20   --fs-disk=10 -p tests/vm/mmap-kernel:mmap-kernel -p ../../tests/vm/sample.txt:sample.txt --swap-disk=4 -- -q   -f run mmap-kernel

.PHONY: debugd
debugd:
	make clean && \
	make -j && \
	cd ./build && \
	pintos -v -k -T 60 -m 20   --fs-disk=10 -p tests/vm/mmap-kernel:mmap-kernel -p ../../tests/vm/sample.txt:sample.txt --swap-disk=4 -- -q   -f run mmap-kernel

.PHONY: zeros
zeros:
	make clean && \
	make -j && \
	cd ./build && \
	dd if=/dev/zero of=tests/vm/zeros bs=1024 count=6 && \
	pintos -v -k -T 60 -m 20   --fs-disk=10 -p tests/vm/mmap-overlap:mmap-overlap -p tests/vm/zeros:zeros --swap-disk=4 -- -q   -f run mmap-overlap

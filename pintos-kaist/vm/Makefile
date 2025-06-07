include ../Makefile.kernel
.PHONY: vm
vm:
	make clean && \
	make -j && \
	cd ./build && \
	make check 2>&1 | tee ../log.txt


.PHONY: debug
debug:
	make clean && \
	make -j && \
	cd ./build && \
	pintos -v -k -T 60 -m 20   --fs-disk=10 -p tests/userprog/fork-read:fork-read -p ../../tests/userprog/sample.txt:sample.txt --swap-disk=4 -- -q   -f run fork-read

.PHONY: zeros
zeros:
	make clean && \
	make -j && \
	cd ./build && \
	dd if=/dev/zero of=tests/vm/zeros bs=1024 count=6 && \
	pintos -v -k -T 60 -m 20   --fs-disk=10 -p tests/vm/mmap-overlap:mmap-overlap -p tests/vm/zeros:zeros --swap-disk=4 -- -q   -f run mmap-overlap
	
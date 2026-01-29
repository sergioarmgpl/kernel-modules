obj-m += simple_module.o
simple_module-objs := simple_module_c.o rust_helper.o

RUSTC := rustc
RUST_FLAGS := --edition 2021 --crate-type staticlib --emit obj -C opt-level=2 \
              -C debuginfo=0 -C relocation-model=static -C panic=abort

all: rust_lib
	cp simple_module.c simple_module_c.c
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) modules
	rm -f simple_module_c.c

rust_lib: rust_helper.rs
	$(RUSTC) $(RUST_FLAGS) -o rust_helper.o rust_helper.rs

clean:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) clean
	rm -f rust_helper.o simple_module_c.c

CC := gcc
src := $(wildcard *.c)
objs := $(patsubst %.c, %.o, $(src))
target := Video_Server

$(target): $(objs)
	$(CC) $^ -lpthread -o $@ 

dep_files := $(foreach f, $(objs), .$(f).d)
dep_files := $(wildcard $(dep_files))

ifneq ($(dep_files), )
	include $(dep_files)
endif

%.o: %.c
	$(CC) -Wp,-MD,.$@.d -c $< -o $@

.PHONY: clean

clean:
	rm -f *.o $(target)

.PHONY: distclean

distclean:
	rm -f .*.o.d *.o $(target)

CC = gcc
CFLAGS = -Wall -Wextra -std=c11 -Iinclude

APP = fiberlib
LIB = src/fiber.o

EXAMPLES = \
	examples/create_demo \
	examples/round_robin_demo \
	examples/priority_demo \
	examples/join_demo \
	examples/fiber_monitor

TESTS = \
	tests/test_init \
	tests/test_priority \
	tests/test_invalid_create \
	tests/test_self_join \
	tests/test_invalid_join \
	tests/test_uninitialized \
	tests/test_double_init \
	tests/test_double_shutdown \
	tests/test_join_completed \
	tests/test_deadlock \
	tests/test_max_fibers \
	tests/test_starvation

all: $(APP) $(LIB) examples tests

$(APP): main.c $(LIB)
	$(CC) $(CFLAGS) main.c $(LIB) -o $(APP)

$(LIB): src/fiber.c include/fiber.h
	$(CC) $(CFLAGS) -c src/fiber.c -o src/fiber.o

examples: $(EXAMPLES)

examples/create_demo: examples/create_demo.c $(LIB)
	$(CC) $(CFLAGS) $< $(LIB) -o $@

examples/round_robin_demo: examples/round_robin_demo.c $(LIB)
	$(CC) $(CFLAGS) $< $(LIB) -o $@

examples/priority_demo: examples/priority_demo.c $(LIB)
	$(CC) $(CFLAGS) $< $(LIB) -o $@

examples/join_demo: examples/join_demo.c $(LIB)
	$(CC) $(CFLAGS) $< $(LIB) -o $@

examples/fiber_monitor: examples/fiber_monitor.c $(LIB)
	$(CC) $(CFLAGS) $< $(LIB) -o $@

tests: $(TESTS)

tests/test_init: tests/test_init.c $(LIB)
	$(CC) $(CFLAGS) $< $(LIB) -o $@

tests/test_priority: tests/test_priority.c $(LIB)
	$(CC) $(CFLAGS) $< $(LIB) -o $@

tests/test_invalid_create: tests/test_invalid_create.c $(LIB)
	$(CC) $(CFLAGS) $< $(LIB) -o $@

tests/test_self_join: tests/test_self_join.c $(LIB)
	$(CC) $(CFLAGS) $< $(LIB) -o $@

tests/test_invalid_join: tests/test_invalid_join.c $(LIB)
	$(CC) $(CFLAGS) $< $(LIB) -o $@

tests/test_uninitialized: tests/test_uninitialized.c $(LIB)
	$(CC) $(CFLAGS) $< $(LIB) -o $@

tests/test_double_init: tests/test_double_init.c $(LIB)
	$(CC) $(CFLAGS) $< $(LIB) -o $@

tests/test_double_shutdown: tests/test_double_shutdown.c $(LIB)
	$(CC) $(CFLAGS) $< $(LIB) -o $@

tests/test_join_completed: tests/test_join_completed.c $(LIB)
	$(CC) $(CFLAGS) $< $(LIB) -o $@

tests/test_deadlock: tests/test_deadlock.c $(LIB)
	$(CC) $(CFLAGS) $< $(LIB) -o $@

tests/test_max_fibers: tests/test_max_fibers.c $(LIB)
	$(CC) $(CFLAGS) $< $(LIB) -o $@

tests/test_starvation: tests/test_starvation.c $(LIB)
	$(CC) $(CFLAGS) $< $(LIB) -o $@

test: $(TESTS)
	@for test in $(TESTS); do \
		echo "===== $$test ====="; \
		./$$test; \
		echo; \
	done

clean:
	rm -f $(APP)
	rm -f src/fiber.o
	rm -f $(EXAMPLES)
	rm -f $(TESTS)

.PHONY: all examples tests test clean

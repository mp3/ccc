CC = gcc
CFLAGS = -Wall -Wextra -g -std=c99 -pedantic
LDFLAGS = 

TARGET = ccc
SRCDIR = src
OBJDIR = build
TESTDIR = tests

SOURCES = $(wildcard $(SRCDIR)/*.c)
OBJECTS = $(patsubst $(SRCDIR)/%.c,$(OBJDIR)/%.o,$(SOURCES))

.PHONY: all clean test

all: $(TARGET)

$(TARGET): $(OBJECTS)
	$(CC) $(LDFLAGS) -o $@ $^

$(OBJDIR)/%.o: $(SRCDIR)/%.c | $(OBJDIR)
	$(CC) $(CFLAGS) -c -o $@ $<

$(OBJDIR):
	mkdir -p $(OBJDIR)

test: $(TARGET)
	python3 $(TESTDIR)/run_tests.py

test-quick: $(TARGET)
	python3 $(TESTDIR)/quick_test.py

test-verbose: $(TARGET)
	python3 $(TESTDIR)/run_tests.py --verbose

test-category: $(TARGET)
	python3 $(TESTDIR)/run_tests.py --category $(CATEGORY)

test-pytest: $(TARGET)
	python3 -m pytest $(TESTDIR) -v

clean:
	rm -rf $(OBJDIR) $(TARGET) ccc.log *.ll *.o *.out __pycache__ .pytest_cache
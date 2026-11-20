# CC0. Native host tests do not open serial ports or program hardware.
XC16 ?= xc16-
PYTHON ?= python
.PHONY: all v3 v4 test clean
all: v3 v4
v3 v4:
	$(MAKE) -C $@ "XC16=$(XC16)"
test:
	$(PYTHON) -m unittest discover -s tests -v
clean:
	$(MAKE) -C v3 clean
	$(MAKE) -C v4 clean

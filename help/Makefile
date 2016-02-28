
TARGETS=wincvt.chm

all: $(TARGETS)

clean: 
	@REM @if exist *.chm erase *.chm
	@if exist *~ erase *~

# hhc may not be present and if it is, may return a failure code.
# This can be ignored by the majority of users, since we ship the
# chm anyway.

.IGNORE:

wincvt.chm: *.htm wincvt.hhk wincvt.hhp wincvt.hhc
	@echo wincvt.hhp
	@-hhc wincvt.hhp 



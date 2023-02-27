# Uses ../.clang-format
FMTFILES=main.c lib.h lib.c proto.h proto.c types.h assert.c assert.h led.c led.h
.PHONY: fmt
fmt:
	clang-format --dry-run --ferror-limit=0 $(FMTFILES)
	clang-format --verbose -i $(FMTFILES)
.PHONY: checkfmt
checkfmt:
	clang-format --dry-run --ferror-limit=0 --Werror $(FMTFILES)

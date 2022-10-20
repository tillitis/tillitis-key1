FMTFILES=main.c lib.h lib.c proto.h proto.c types.h
.PHONY: fmt
fmt:
	# Uses ../.clang-format
	clang-format --dry-run --ferror-limit=0 $(FMTFILES)
	clang-format --verbose -i $(FMTFILES)

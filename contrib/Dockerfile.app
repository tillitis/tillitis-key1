FROM alpine:3.20

RUN apk add --no-cache \
    clang \
    clang-extra-tools \
    go \
    lld \
    llvm \
    make

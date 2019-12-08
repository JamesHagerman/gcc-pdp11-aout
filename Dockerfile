##
# Build with: `docker build --tag gcc-pdp11-aout .`
# Run with: `docker run -it gcc-pdp11-aout bash`
#
# Most of these steps were taken from:
# https://xw.is/wiki/Bare_metal_PDP-11_GCC_9.2.0_cross_compiler_instructions
#
# Worker image (used to compile stuff but not dumped into the final image)
FROM gcc:9.2.0 as builder

# Install build dependencies
RUN ["bash", "-c", "\
  apt-get update && apt-get install -y --no-install-recommends \
    wget \
"]

# Working environment
WORKDIR /usr/local/lib

# Download and extract GCC source tree
RUN ["bash", "-c", "\
  wget https://ftp.gnu.org/gnu/gcc/gcc-9.2.0/gcc-9.2.0.tar.gz \
  && wget https://ftp.gnu.org/gnu/binutils/binutils-2.33.1.tar.gz \
  && wget ftp://sourceware.org/pub/newlib/newlib-3.1.0.tar.gz \
  && tar -zxf gcc-9.2.0.tar.gz \
  && tar -zxf binutils-2.33.1.tar.gz \
  && tar -zxf newlib-3.1.0.tar.gz \
  && pushd gcc-9.2.0 \
  && ./contrib/download_prerequisites \
  && popd \
"]

# Connfigure, compile, and install binutils
RUN ["bash", "-c", "\
  mkdir binutils-build \
  && pushd binutils-build \
  && ../binutils-2.33.1/configure --prefix /usr/local/lib/xgcc --bindir /usr/local/lib/bin --target pdp11-aout \
  && make -j8 \
  && make install \
  && popd \
"]

# Configure, compile, and install gcc
RUN ["bash", "-c", "\
  mkdir gcc-build \
  && pushd gcc-build \
  && ../gcc-9.2.0/configure --prefix /usr/local/lib/xgcc --bindir /usr/local/lib/bin --target pdp11-aout --enable-languages=c --with-gnu-as --with-gnu-ld --without-headers --disable-libssp \
  && make -j8 \
  && make install \
  && popd \
"]

#&& ../gcc-9.2.0/configure --prefix /usr/local/lib/xgcc --bindir /usr/local/lib/bin --target pdp11-aout --enable-languages=c,c++,fortran --with-gnu-as --with-gnu-ld --without-headers --disable-libstdc++-v3 --disable-libbacktrack --disable-libssp \

#ENV PATH="/usr/local/lib/bin:${PATH}"
#CMD ["bash"]

# Final image (the final image that will be sent up to dockerhub or whatever image repo)
FROM debian:stretch-slim as gcc-pdp11-aout

COPY --from=builder /usr/local/lib/xgcc /usr/local/lib/xgcc
COPY --from=builder /usr/local/lib/bin /usr/local/lib/bin

ENV PATH="/usr/local/lib/bin:${PATH}"

WORKDIR /usr/local/lib

RUN ["bash", "-c", "\
  echo 'int start() { return 0; }' > foo.c; pdp11-aout-gcc -nostdlib foo.c \
  && pdp11-aout-objdump -D a.out \
"]

CMD ["bash"]

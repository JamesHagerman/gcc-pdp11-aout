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
    unzip \
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

# Configure, compile, and install binutils
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

# Download, extract, and compile bin2load
RUN ["bash", "-c", "\
  wget https://github.com/jguillaumes/retroutils/archive/cd2ecbd096c2c59829000fdabd51bc5284f007f8.zip \
  && unzip cd2ecbd096c2c59829000fdabd51bc5284f007f8.zip \
  && pushd retroutils-cd2ecbd096c2c59829000fdabd51bc5284f007f8/bin2load/ \
  && make \
  && mv bin2load ../../bin/ \
  && popd \
"]

# Add and compile useful LDA file generator tool from local source tree:
# `atolda.c` was provided by Stephen Casner on the pidp-11 forums:
# https://groups.google.com/d/msg/pidp-11/ZT-84hWwBlo/3XiYaFQ7AwAJ
ADD ./tools ./tools
RUN ["bash", "-c", "\
  pushd tools/ \
  && gcc atolda.c -o atolda \
  && popd \
"]

#ENV PATH="/usr/local/lib/bin:${PATH}"
#CMD ["bash"]

# Final image (the final image that will be sent up to dockerhub or whatever image repo)
FROM debian:stretch-slim as gcc-pdp11-aout

# Copy the compiled tools to the smaller image:
COPY --from=builder /usr/local/lib/xgcc /usr/local/lib/xgcc
COPY --from=builder /usr/local/lib/bin /usr/local/lib/bin
COPY --from=builder /usr/local/lib/tools /usr/local/lib/tools
COPY --from=builder /usr/local/lib/example /usr/local/lib/example

# Copy the small bin2load source tree to the smaller image as well (It is a good learning tool!):
COPY --from=builder /usr/local/lib/retroutils-cd2ecbd096c2c59829000fdabd51bc5284f007f8/bin2load /usr/local/lib/tools/bin2load


ENV PATH="/usr/local/lib/bin:${PATH}"

WORKDIR /usr/local/lib

RUN ["bash", "-c", "\
  echo 'int start() { return 0; }' > foo.c; pdp11-aout-gcc -nostdlib foo.c \
  && pdp11-aout-objdump -D a.out \
"]

CMD ["bash"]

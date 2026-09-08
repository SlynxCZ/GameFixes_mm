FROM registry.gitlab.steamos.cloud/steamrt/sniper/sdk:latest

WORKDIR /app

# The sniper image is Debian bullseye, whose security repo Release files have
# expired since bullseye went EOL; without this every apt update fails.
RUN echo 'Acquire::Check-Valid-Until "false";' > /etc/apt/apt.conf.d/99-no-check-valid-until

RUN echo "deb http://apt.llvm.org/bullseye/ llvm-toolchain-bullseye-18 main" >> /etc/apt/sources.list.d/llvm.list
RUN echo "deb-src http://apt.llvm.org/bullseye/ llvm-toolchain-bullseye-18 main" >> /etc/apt/sources.list.d/llvm.list
RUN wget -O - https://apt.llvm.org/llvm-snapshot.gpg.key | apt-key add -
RUN apt update && apt install -y clang-18 lld-18 libc++-18-dev libc++abi-18-dev
RUN ln -sf /usr/bin/clang-18 /usr/bin/clang
RUN ln -sf /usr/bin/clang++-18 /usr/bin/clang++
RUN git clone https://github.com/SlynxCZ/ambuild
RUN cd ambuild && python setup.py install && cd ..

RUN apt update && apt install -y \
    build-essential \
    cmake \
    ninja-build \
    git \
    zlib1g \
    zlib1g-dev \
    libssl-dev \
    libprotobuf-dev \
    protobuf-compiler \
    pkg-config \
    curl

RUN git config --system --add safe.directory '*'

COPY docker/docker-entrypoint.sh ./docker-entrypoint.sh
RUN sed -i 's/\r$//' ./docker-entrypoint.sh && chmod +x ./docker-entrypoint.sh

WORKDIR /app/source

CMD ["/bin/bash", "../docker-entrypoint.sh"]

# Use Ubuntu for Dev Containers as the base image
FROM mcr.microsoft.com/devcontainers/base:ubuntu-22.04

# Set LLVM version
ENV LLVM_VERSION=17

# Prepare LLMV repository
RUN apt-get update && \
    apt-get install -y \
    software-properties-common \
    lsb-release \
    wget
RUN wget https://apt.llvm.org/llvm.sh && \
    chmod +x llvm.sh && \
    ./llvm.sh ${LLVM_VERSION}

# Install necessary dependencies
RUN apt-get update && apt-get install -y \
    clang-${LLVM_VERSION} lldb-${LLVM_VERSION} lld-${LLVM_VERSION} llvm-${LLVM_VERSION} \
    llvm-${LLVM_VERSION}-dev llvm-${LLVM_VERSION}-runtime \
    clangd-${LLVM_VERSION} clang-tidy-${LLVM_VERSION} clang-format-${LLVM_VERSION} \
    libclang-${LLVM_VERSION}-dev libclang-common-${LLVM_VERSION}-dev \
    cmake ninja-build gdb \
    build-essential python3 python3-pip \
    libtinfo5 libncurses5 libstdc++-12-dev \
    && rm -rf /var/lib/apt/lists/*

# Set specific version of clang as default
RUN update-alternatives --install /usr/bin/clang clang /usr/bin/clang-${LLVM_VERSION} 100 && \
    update-alternatives --install /usr/bin/clang++ clang++ /usr/bin/clang++-${LLVM_VERSION} 100 && \
    update-alternatives --install /usr/bin/clangd clangd /usr/bin/clangd-${LLVM_VERSION} 100 && \
    update-alternatives --install /usr/bin/clang-tidy clang-tidy /usr/bin/clang-tidy-${LLVM_VERSION} 100 && \
    update-alternatives --install /usr/bin/clang-format clang-format /usr/bin/clang-format-${LLVM_VERSION} 100

CMD ["/bin/bash"]

FROM ubuntu:22.04 AS builder

RUN apt-get update && apt-get install -y \
    build-essential cmake git \
    libsqlite3-dev \
    python3 python3-dev python3-pip \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /app
COPY . .

RUN mkdir -p build && cd build && \
    cmake .. -DCMAKE_BUILD_TYPE=Release \
             -DBUILD_PYTHON_BINDINGS=ON && \
    make -j$(nproc)

FROM ubuntu:22.04

RUN apt-get update && apt-get install -y \
    libsqlite3 python3 python3-pip \
    && rm -rf /var/lib/apt/lists/*

COPY --from=builder /app/build/riskengine /usr/local/bin/
COPY --from=builder /app/build/tools/* /usr/local/bin/
COPY --from=builder /app/build/python/risk /usr/lib/python3/dist-packages/risk
COPY --from=builder /app/build/librisk_python*.so /usr/lib/python3/dist-packages/

RUN pip3 install plotly dash pandas numpy pyarrow click websockets pyyaml

EXPOSE 8050 9000

CMD ["riskengine", "--mode", "replay"]

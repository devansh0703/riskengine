FROM ubuntu:24.04 AS builder

RUN apt-get update && apt-get install -y \
    build-essential cmake git libsqlite3-dev python3 python3-dev python3-pip \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /app
COPY . .

RUN rm -rf build && mkdir -p build && cd build && \
    cmake .. -DCMAKE_BUILD_TYPE=Release \
             -DBUILD_TESTS=ON \
             -DBUILD_TOOLS=ON \
             -DBUILD_PYTHON_BINDINGS=OFF && \
    make -j$(nproc)

FROM ubuntu:24.04

RUN apt-get update && apt-get install -y \
    libsqlite3-0 python3 python3-pip python3-venv \
    && rm -rf /var/lib/apt/lists/*

RUN python3 -m venv /app/.venv && \
    /app/.venv/bin/pip install --no-cache-dir plotly dash pandas numpy pyarrow click websockets pyyaml

COPY --from=builder /app/build/riskengine /usr/local/bin/
COPY --from=builder /app/build/risk_cli /usr/local/bin/
COPY --from=builder /app/build/replay_tool /usr/local/bin/
COPY --from=builder /app/build/export_tool /usr/local/bin/
COPY --from=builder /app/build/test/test_greeks /usr/local/bin/

COPY python/ /app/python/
COPY config/ /app/config/
COPY data/ /app/data/

ENV PATH="/app/.venv/bin:$PATH"

EXPOSE 8050 9000

CMD ["riskengine", "--mode", "replay", "--csv", "data/sample_trades.csv"]
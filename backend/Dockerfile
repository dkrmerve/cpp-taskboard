# ---- build stage ----
FROM ubuntu:24.04 AS build

ENV DEBIAN_FRONTEND=noninteractive
RUN apt-get update && apt-get install -y --no-install-recommends \
        build-essential cmake git ca-certificates \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /src
COPY CMakeLists.txt ./
COPY include ./include
COPY src ./src
COPY tests ./tests

ARG BUILD_TESTS=ON
RUN cmake -S . -B build -DCMAKE_BUILD_TYPE=Release -DTASKBOARD_BUILD_TESTS=${BUILD_TESTS} \
    && cmake --build build --parallel

# Run the test suite as part of the image build so a broken build never ships.
RUN if [ "$BUILD_TESTS" = "ON" ]; then ctest --test-dir build --output-on-failure; fi

# ---- runtime stage ----
FROM ubuntu:24.04 AS runtime

RUN apt-get update && apt-get install -y --no-install-recommends curl \
    && rm -rf /var/lib/apt/lists/* \
    && useradd --system --no-create-home taskboard

COPY --from=build /src/build/taskboard_server /usr/local/bin/taskboard_server

USER taskboard
ENV PORT=8080 HOST=0.0.0.0
EXPOSE 8080

HEALTHCHECK --interval=10s --timeout=3s --retries=3 \
    CMD curl -fsS http://127.0.0.1:8080/api/health || exit 1

ENTRYPOINT ["/usr/local/bin/taskboard_server"]

FROM ubuntu:20.04

RUN apt-get update && DEBIAN_FRONTEND=noninteractive apt-get install -y \
    fuse \
    gcc \
    libfuse-dev \
    pkg-config \
    && rm -rf /var/lib/apt/lists/*

COPY antink.c .

RUN gcc -Wall `pkg-config fuse --cflags` antink.c -o antink `pkg-config fuse --libs`

CMD ["./antink", "-f", "./antink_mount"]

FROM ubuntu:20.04

ENV DEBIAN_FRONTEND=noninteractive

RUN apt-get update && \
    apt-get install -y fuse libfuse-dev gcc make pkg-config

# Buat folder kerja dan salin source
WORKDIR /app
COPY antink.c .

# Compile source
RUN gcc antink.c -o antink `pkg-config fuse --cflags --libs`

# Jalankan FUSE mount
CMD ["./antink", "/mnt/mount"]

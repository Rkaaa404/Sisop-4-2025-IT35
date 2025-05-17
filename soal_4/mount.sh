#!/bin/bash

# Unmount if already mounted
fusermount -u ./fuse_dir 2>/dev/null

# Mount the FUSE filesystem with nonempty option
./maimai_fs ./fuse_dir -o nonempty -f -d &

# Wait for the filesystem to be mounted
sleep 2

echo "FUSE filesystem mounted at ./fuse_dir"
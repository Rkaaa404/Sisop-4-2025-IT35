#!/usr/bin/env python3

import os
import sys
import subprocess
import time

def check_file_content(file_path):
    try:
        with open(file_path, 'r') as f:
            return f.read()
    except Exception as e:
        return f"Error reading file: {e}"

def transform_metro_content(content):
    """Apply the Metropolis transformation to content"""
    result = ""
    for i, char in enumerate(content):
        # Shift character by position (mod 256)
        result += chr((ord(char) + (i % 256)) % 256)
    return result

def main():
    print("Test sample")
    
    # Test A: Starter module
    print("A. starter")
    source_file = "/workspace/chiho/starter/a.txt"
    source_content = check_file_content(source_file)
    
    # For starter module, just add .mai suffix
    print(f"{source_file} with content \"{source_content.strip()}\" would be keep as original and mirrored")
    print(f"to /fuse_dir/starter/a.txt.mai with content \"{source_content.strip()}\"")
    print("explaination : spec A : add suffix \".mai\"")
    print("")
    
    # Test B: Metropolis module
    print("B. Metropolis")
    source_file = "/workspace/chiho/metro/a.txt"
    source_content = check_file_content(source_file)
    
    # For metro module, shift characters by position (mod 256)
    transformed_content = transform_metro_content(source_content.strip())
    
    print(f"{source_file} with content \"{source_content.strip()}\" would be keep as original and mirrored")
    print(f"to /fuse_dir/metro/a.txt.ccc with content \"{transformed_content}\"")
    print("explaination : spec B : add suffix \".ccc\" then shift characters by position (mod 256)")

if __name__ == "__main__":
    main()
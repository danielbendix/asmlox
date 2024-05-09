#!/bin/zsh

if [[ $# -ne 1 ]]; then
    echo "Usage: $0 <filename>"
        exit 1
        fi

        name="$1"

        # Add .c and .o to the filename
        c_file="ops/${name}.c"
        o_file="native/arm64/${name}.s"

        clang "$c_file" -O3 -S -o "$o_file"

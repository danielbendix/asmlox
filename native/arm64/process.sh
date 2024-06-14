for file in *.sx; do
    python3 ../../util/asm.py $file -o ${file%.sx}.s
done

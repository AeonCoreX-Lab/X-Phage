#!/bin/bash
# X-Phage Automated Build Script (Termux Optimized)
mkdir -p bin

echo "⚙️  Compiling X-Phage Engine..."
clang++ src/xphage_core.cpp -o bin/xphage

if [ $? -eq 0 ]; then
    echo "✔ Compiler Built Successfully."
    echo "🔓 Ensuring execution permissions..."
    chmod +x bin/xphage
    
    echo "🚀 Running Test: main.xp0"
    ./bin/xphage tests/main.xp0
else
    echo "✖ Compilation Failed!"
fi

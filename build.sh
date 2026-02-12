#!/bin/bash
# X-Phage Ultimate Build Script
# Uses O3 Optimization (Maximum Speed)

echo "🧬 AeonCoreX: Building Genesis Engine..."

# Clean old binaries
rm -rf bin/xphage

mkdir -p bin

# Compile with High Optimization (-O3) and C++17 Standard
clang++ src/xphage_core.cpp -o bin/xphage -std=c++17 -O3 -Wall

if [ $? -eq 0 ]; then
    echo "✔ Build Complete. Engine is Optimized."
    chmod +x bin/xphage
    
    echo "⚡ Running Genesis Test..."
    ./bin/xphage tests/genesis_test.xp0
else
    echo "✖ Fatal Error: Compilation Failed."
fi

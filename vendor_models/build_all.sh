#!/bin/bash

# Build script for DHT simulation vendor models
# This script builds both vendor_client and vendor_server projects

echo "Building DHT Simulation Vendor Models..."
echo "========================================"

# Check if IDF_PATH is set
if [ -z "$IDF_PATH" ]; then
    echo "Error: IDF_PATH is not set. Please source the ESP-IDF environment first."
    echo "Example: . \$HOME/esp/esp-idf/export.sh"
    exit 1
fi

# Build vendor_server (DHT sensor node)
echo ""
echo "Building vendor_server (DHT Sensor Node)..."
echo "--------------------------------------------"
cd vendor_server
if idf.py build; then
    echo "✓ vendor_server build successful"
else
    echo "✗ vendor_server build failed"
    exit 1
fi
cd ..

# Build vendor_client (data receiver)
echo ""
echo "Building vendor_client (Data Receiver)..."
echo "------------------------------------------"
cd vendor_client
if idf.py build; then
    echo "✓ vendor_client build successful"
else
    echo "✗ vendor_client build failed"
    exit 1
fi
cd ..

echo ""
echo "========================================"
echo "✓ All builds completed successfully!"
echo ""
echo "To flash and monitor:"
echo "  Node 1 (DHT Sensor): cd vendor_server && idf.py flash monitor"
echo "  Node 2 (Receiver):   cd vendor_client && idf.py flash monitor"
echo ""
echo "Remember to start Node 2 (receiver) first!" 
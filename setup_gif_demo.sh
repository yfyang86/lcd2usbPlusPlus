#!/bin/bash
# Install giflib and build gif_to_lcd_demo

echo "=== GIF to LCD Demo Setup ==="
echo ""

# Check if Homebrew is installed
if ! command -v brew &> /dev/null; then
    echo "Error: Homebrew is not installed."
    echo "Please install Homebrew first: https://brew.sh"
    exit 1
fi

echo "✓ Homebrew found"

# Check if giflib is already installed
if pkg-config --exists giflib 2>/dev/null; then
    VERSION=$(pkg-config --modversion giflib)
    echo "✓ giflib already installed (version $VERSION)"
else
    echo "Installing giflib..."
    brew install giflib
    if [ $? -eq 0 ]; then
        echo "✓ giflib installed successfully"
    else
        echo "✗ Failed to install giflib"
        exit 1
    fi
fi

echo ""
echo "Building gif_to_lcd_demo..."
echo ""

# Get script directory
SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
PROJECT_ROOT="$SCRIPT_DIR"

# Create build directory if it doesn't exist
mkdir -p "$PROJECT_ROOT/build"

# Build
cd "$PROJECT_ROOT/build"
cmake .. && make gif_to_lcd_demo

if [ $? -eq 0 ]; then
    echo ""
    echo "=== Setup Complete ==="
    echo ""
    echo "✓ gif_to_lcd_demo built successfully!"
    echo ""
    echo "Run the example:"
    echo "  cd $PROJECT_ROOT/build/examples"
    echo "  ./gif_to_lcd_demo --help"
    echo ""
    echo "Test with Bad Apple:"
    echo "  ./gif_to_lcd_demo -i $PROJECT_ROOT/assets/Bad-Apple.gif"
    echo ""
else
    echo ""
    echo "✗ Build failed. Check the errors above."
    exit 1
fi

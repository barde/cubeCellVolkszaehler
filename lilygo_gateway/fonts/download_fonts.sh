#!/bin/bash

# Download fonts for ESPHome OLED display

echo "Downloading Roboto Mono fonts..."

# Create fonts directory if it doesn't exist
cd "$(dirname "$0")"

# Download Roboto Mono Bold
if [ ! -f "roboto_mono_bold.ttf" ]; then
    echo "Downloading Roboto Mono Bold..."
    curl -L https://github.com/google/fonts/raw/main/apache/robotomono/RobotoMono-Bold.ttf -o roboto_mono_bold.ttf
    echo "✓ Roboto Mono Bold downloaded"
else
    echo "✓ Roboto Mono Bold already exists"
fi

# Download Roboto Mono Regular
if [ ! -f "roboto_mono_regular.ttf" ]; then
    echo "Downloading Roboto Mono Regular..."
    curl -L https://github.com/google/fonts/raw/main/apache/robotomono/RobotoMono-Regular.ttf -o roboto_mono_regular.ttf
    echo "✓ Roboto Mono Regular downloaded"
else
    echo "✓ Roboto Mono Regular already exists"
fi

echo ""
echo "Fonts downloaded successfully!"
echo "You can now compile the ESPHome configuration."
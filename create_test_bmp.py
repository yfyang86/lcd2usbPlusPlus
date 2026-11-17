#!/usr/bin/env python3
"""
Create a simple test BMP file for the bmp_to_lcd_demo
This creates a 24-bit uncompressed BMP with a simple pattern
"""

import struct
import sys

def create_test_bmp(filename, width=32, height=32):
    """Create a simple test BMP file with a smiley face pattern"""
    
    # BMP file header (14 bytes)
    # Calculate row size with padding (rows must be multiple of 4 bytes)
    bytes_per_pixel = 3  # 24-bit
    row_size = ((width * bytes_per_pixel + 3) // 4) * 4
    pixel_data_size = row_size * height
    file_size = 14 + 40 + pixel_data_size  # header + info header + pixel data
    
    bmp_file_header = struct.pack('<2sIHHI',
        b'BM',           # Signature
        file_size,       # File size
        0,               # Reserved1
        0,               # Reserved2
        54               # Data offset (14 + 40)
    )
    
    # BMP info header (40 bytes)
    bmp_info_header = struct.pack('<IiiHHIIiiII',
        40,              # Header size
        width,           # Width
        height,          # Height (positive = bottom-up)
        1,               # Planes
        24,              # Bits per pixel
        0,               # Compression (0 = uncompressed)
        pixel_data_size, # Image size
        2835,            # X pixels per meter
        2835,            # Y pixels per meter
        0,               # Colors used
        0                # Important colors
    )
    
    # Create pixel data - simple smiley face pattern
    pixels = []
    center_x, center_y = width // 2, height // 2
    radius = min(width, height) // 3
    
    for y in range(height):
        row = []
        for x in range(width):
            # Calculate distance from center
            dx = x - center_x
            dy = y - center_y
            dist = (dx*dx + dy*dy) ** 0.5
            
            # Create a simple pattern
            if dist < radius * 0.3:
                # Center (nose area) - gray
                r, g, b = 128, 128, 128
            elif dist < radius * 0.9 and dist > radius * 0.7:
                # Ring (face outline) - black
                r, g, b = 0, 0, 0
            elif abs(dx) > radius * 0.3 and abs(dy) < radius * 0.3 and dist < radius * 0.6:
                # Eyes - black dots
                if abs(dx - radius * 0.5) < 3 and abs(dy) < 3:
                    r, g, b = 0, 0, 0
                else:
                    r, g, b = 255, 255, 255
            elif dy > radius * 0.2 and dist < radius * 0.7:
                # Mouth - dark
                if abs(dx) < radius * 0.5 and dy > radius * 0.3 and dy < radius * 0.5:
                    r, g, b = 0, 0, 0
                else:
                    r, g, b = 255, 255, 255
            else:
                # Background - white
                r, g, b = 255, 255, 255
            
            # BMP stores pixels in BGR order
            row.extend([b, g, r])
        
        # Add padding to make row size multiple of 4
        padding = row_size - (width * bytes_per_pixel)
        row.extend([0] * padding)
        pixels.extend(row)
    
    # Write the BMP file
    with open(filename, 'wb') as f:
        f.write(bmp_file_header)
        f.write(bmp_info_header)
        f.write(bytes(pixels))
    
    print(f"Created test BMP file: {filename} ({width}x{height} pixels)")

if __name__ == '__main__':
    output_file = 'test_pattern.bmp' if len(sys.argv) < 2 else sys.argv[1]
    create_test_bmp(output_file)

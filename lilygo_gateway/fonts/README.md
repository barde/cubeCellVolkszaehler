# Fonts for OLED Display

The display configuration uses Roboto Mono fonts for a clean, monospace look on the OLED.

## Required Fonts

Download the following fonts and place them in this directory:

1. **roboto_mono_bold.ttf** - For large power display
2. **roboto_mono_regular.ttf** - For normal text

## Download Instructions

### Option 1: Google Fonts (Recommended)
```bash
# Download Roboto Mono family
wget https://github.com/google/fonts/raw/main/apache/robotomono/RobotoMono-Bold.ttf -O roboto_mono_bold.ttf
wget https://github.com/google/fonts/raw/main/apache/robotomono/RobotoMono-Regular.ttf -O roboto_mono_regular.ttf
```

### Option 2: Manual Download
1. Visit [Google Fonts - Roboto Mono](https://fonts.google.com/specimen/Roboto+Mono)
2. Click "Download family"
3. Extract and copy the required TTF files to this directory

### Option 3: Use System Fonts
If you prefer different fonts, you can use any TTF font. Update the font paths in `lilygo_lora_gateway_display.yaml`:

```yaml
font:
  - file: "fonts/your_font_bold.ttf"
    id: font_large
    size: 20
```

## Alternative Fonts

For a different look, consider:
- **Ubuntu Mono** - Clean and modern
- **Fira Code** - Developer-friendly with ligatures
- **Source Code Pro** - Adobe's monospace font
- **DejaVu Sans Mono** - Good Unicode support

## Font Sizes

The configuration uses three font sizes:
- **Large (20pt)**: Current power display
- **Medium (14pt)**: Values and statistics
- **Small (10pt)**: Labels and status text

Adjust these in the YAML if your display needs different sizes.

## Troubleshooting

### Font not found
- Ensure the TTF files are in the `fonts/` directory
- Check file names match exactly (case-sensitive)
- Use absolute paths if relative paths don't work

### Characters missing
- Add missing characters to the `glyphs` parameter
- Or remove the `glyphs` parameter to include all characters (uses more memory)

### Display looks wrong
- Try different font sizes
- Adjust the `rotation` parameter if text is upside down
- Check I2C address with `i2cdetect` command
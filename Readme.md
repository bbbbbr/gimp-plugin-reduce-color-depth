# GIMP Plugin for reducing the color bit depth of images
Allows for reducing the color bit-depth of the R/G/B channels source images (RGB-888/RGBA-8888) to between 1-8 bits. The channel bit-depths can be locked together or modified separately.

For example it can reduce an RGB-888 image to bit-depths used by old game consoles such as the Game Boy Color(BGR-555), Game Gear(BGR-444), or Sega Master System(BGR-222).

The functionality is similar to the built-in `Colors -> Dither` plugin except that it ensures alignment of color reductions to be within scaled bit-depth boundaries instead of arbitrary ranges.

## Menu
The plugin menu location is: `Colors -> Reduce Color Bit-Depth...`

## Compatibility
At least compatible with some versions of GIMP 2.10 on Linux and Windows.

## Download
Pre-built binaries can be downloaded from the [Releases section](https://github.com/bbbbbr/gimp-plugin-reduce-color-depth/releases).

## Install
Copy the plugin file to the relevant location below.

Plug-in folder locations:
- Linux: `~/.config/GIMP/2.10/plug-ins`
- Windows: `C:\Program Files\GIMP 2\lib\gimp\2.0\plug-ins`


Screenshot:

![GIMP Image Editor using Reduce Color Depth Plugin](/info/gimp-plugin-reduce-color-depth.png)


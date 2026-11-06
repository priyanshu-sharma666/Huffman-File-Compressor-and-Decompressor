# Huffman File Compressor and Decompressor

A lightweight lossless file compression desktop application built with **C++17**, **Dear ImGui**, **GLFW**, and **OpenGL 3**.

---

## Technologies

- **C++17**
- **Dear ImGui**
- **GLFW 3**
- **OpenGL 3.0 Core**
- **Windows (64-bit)**

---

##  Project Structure

| File / Directory | Description |
| :--- | :--- |
| `main.cpp` | Application entry point and ImGui interface |
| `Huffman.h` | Compression & decompression interfaces and result structures |
| `Huffman.cpp` | Huffman tree, compression, and decompression logic |
| `build.bat` | 1 click build and run script for Windows |
| `external/imgui/`| [Dear ImGui](https://github.com/ocornut/imgui) source files |
| `external/glfw-3.5.1.bin.WIN64/` | [GLFW](https://www.glfw.org/) headers and precompiled 64-bit Windows binaries |

---

##  Build & Run

### Windows
Run `build.bat` from the project folder.

The script uses MinGW `g++` to compile the project and launch the application.

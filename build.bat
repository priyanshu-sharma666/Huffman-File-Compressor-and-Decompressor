@echo off
echo Building Huffman Compressor...

g++ -std=c++17 -O2 ^
    main.cpp Huffman.cpp ^
    external/imgui/imgui.cpp ^
    external/imgui/imgui_draw.cpp ^
    external/imgui/imgui_tables.cpp ^
    external/imgui/imgui_widgets.cpp ^
    external/imgui/backends/imgui_impl_glfw.cpp ^
    external/imgui/backends/imgui_impl_opengl3.cpp ^
    -I. ^
    -Iexternal/imgui ^
    -Iexternal/imgui/backends ^
    -Iexternal/glfw-3.5.1.bin.WIN64/include ^
    -Lexternal/glfw-3.5.1.bin.WIN64/lib-mingw-w64 ^
    -lglfw3 -lopengl32 -lgdi32 -luser32 -lkernel32 -lcomdlg32 -lshell32 ^
    -static-libgcc -static-libstdc++ ^
    -o HuffmanCompressor.exe

if %ERRORLEVEL% equ 0 (
    echo [SUCCESS] Running HuffmanCompressor.exe...
    start HuffmanCompressor.exe
) else (
    echo [ERROR] Build failed.
)
pause
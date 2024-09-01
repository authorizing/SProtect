# SProtect

SProtect is a simple tool designed to protect Windows executables and DLLs through basic obfuscation and virtualization techniques.

## Description

SProtect employs a combination of code obfuscation and virtualization to make reverse engineering of Windows PE (Portable Executable) files significantly more challenging.
At the moment it only supports 32-bit executables and DLLs, including .NET assemblies.

Key features:
- Virtualization of the entry point
- Code obfuscation
- .NET metadata obfuscation (for .NET assemblies)
- Addition of a custom section to the PE file

## Supported Architectures

- x86 (32-bit)

## Prerequisites

- Windows operating system
- Visual Studio 2019 or later (for building)
- CMake 3.10 or later

## Building

1. Clone the repository:
   ```
   git clone https://github.com/authorizing/SProtect.git
   cd SProtect
   ```

2. Create a build directory:
   ```
   mkdir build
   cd build
   ```

3. Generate the project files:
   ```
   cmake ..
   ```

4. Build the project:
   ```
   cmake --build .
   ```

## Usage

To protect an executable or DLL:

```
SProtect.exe <path_to_input_file>
```

For example:
```
SProtect.exe C:\path\to\your\application.exe
```
The protected file will be created in the same directory as the input file, with "_protected" appended to the filename.

## Limitations

- The current version may not work correctly with executables that have complex entry point code or rely on specific entry point behavior.
- Protecting already protected or packed executables may lead to unexpected results.
- Some antivirus software may flag protected executables as potentially malicious due to the nature of the protection techniques used.

## Contributing

Contributions are welcome! Please feel free to submit a Pull Request.

## Disclaimer

This tool is intended for educational and research purposes only. Always ensure you have the right to modify and protect the executables you use with this tool.

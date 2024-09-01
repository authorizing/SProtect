#include <iostream>
#include <vector>
#include <fstream>
#include <string>
#include <cstdlib>
#include <windows.h>
#include <cstring>
#include <filesystem>
#include <random>
#include <sstream>
#include <iomanip>
#include <cor.h>
#include <metahost.h>

#pragma comment(lib, "mscoree.lib")

// Helper function to get the last section of the PE file
IMAGE_SECTION_HEADER* getLastSection(IMAGE_NT_HEADERS* ntHeaders) {
    IMAGE_SECTION_HEADER* sectionHeader = IMAGE_FIRST_SECTION(ntHeaders);
    return sectionHeader + ntHeaders->FileHeader.NumberOfSections - 1;
}

// Simple Virtual Machine class for code obfuscation
class VirtualMachine {
private:
    std::vector<uint8_t> bytecode;
    size_t pc = 0; // Program counter
    int accumulator = 0;

public:
    void loadBytecode(const std::vector<uint8_t>& code) {
        bytecode = code;
    }

    // Execute the bytecode
    static void execute(VirtualMachine* vm) {
        while (vm->pc < vm->bytecode.size()) {
            uint8_t opcode = vm->bytecode[vm->pc++];
            switch (opcode) {
                case 0x01: // LOAD_CONST
                    vm->accumulator = vm->bytecode[vm->pc++];
                    break;
                case 0x02: // ADD
                    vm->accumulator += vm->bytecode[vm->pc++];
                    break;
                case 0x03: // PRINT
                    std::cout << vm->accumulator << std::endl;
                    break;
                case 0x04: // HALT
                    return;
                default:
                    std::cerr << "Unknown opcode: " << static_cast<int>(opcode) << std::endl;
                    return;
            }
        }
    }
};

// Function to read the input executable
std::vector<uint8_t> readExecutable(const std::string& path) {
    std::ifstream file(path, std::ios::binary);
    return std::vector<uint8_t>((std::istreambuf_iterator<char>(file)),
                                 std::istreambuf_iterator<char>());
}

// Function to write the output executable
void writeExecutable(const std::string& path, const std::vector<uint8_t>& data) {
    std::ofstream file(path, std::ios::binary);
    if (!file) {
        throw std::runtime_error("Failed to open output file for writing: " + path);
    }
    file.write(reinterpret_cast<const char*>(data.data()), data.size());
    if (!file) {
        throw std::runtime_error("Failed to write data to output file: " + path);
    }
    file.close();
    if (!file) {
        throw std::runtime_error("Failed to close output file: " + path);
    }
    std::cout << "Successfully wrote " << data.size() << " bytes to " << path << std::endl;
}

// Function to generate a random Chinese character for .NET obfuscation
std::wstring getRandomChineseChar() {
    static std::random_device rd;
    static std::mt19937 gen(rd());
    static std::uniform_int_distribution<> dis(0x4E00, 0x9FFF);
    
    wchar_t ch = static_cast<wchar_t>(dis(gen));
    return std::wstring(1, ch);
}

// Function to generate a random obfuscated name
std::wstring getObfuscatedName(int length) {
    std::wstring result;
    for (int i = 0; i < length; ++i) {
        result += getRandomChineseChar();
    }
    return result;
}

// Main function to protect the executable
std::vector<uint8_t> protectExecutable(const std::vector<uint8_t>& inputFile) {
    std::vector<uint8_t> protectedFile = inputFile;

    try {
        std::cout << "Input file size: " << inputFile.size() << " bytes" << std::endl;
        std::cout << "Parsing PE format..." << std::endl;

        // Check if the file is large enough to be a valid PE file
        if (inputFile.size() < sizeof(IMAGE_DOS_HEADER)) {
            throw std::runtime_error("File too small to be a valid PE file");
        }

        // Parse the PE format
        IMAGE_DOS_HEADER* dosHeader = (IMAGE_DOS_HEADER*)protectedFile.data();
        if (dosHeader->e_magic != IMAGE_DOS_SIGNATURE) {
            throw std::runtime_error("Invalid DOS signature");
        }

        if (inputFile.size() < dosHeader->e_lfanew + sizeof(IMAGE_NT_HEADERS)) {
            throw std::runtime_error("File too small to contain a valid NT header");
        }

        IMAGE_NT_HEADERS* ntHeaders = (IMAGE_NT_HEADERS*)(protectedFile.data() + dosHeader->e_lfanew);
        if (ntHeaders->Signature != IMAGE_NT_SIGNATURE) {
            throw std::runtime_error("Invalid NT signature");
        }

        std::cout << "DOS Header e_lfanew: " << dosHeader->e_lfanew << std::endl;
        std::cout << "NT Headers SizeOfImage: " << ntHeaders->OptionalHeader.SizeOfImage << std::endl;
        std::cout << "Number of Sections: " << ntHeaders->FileHeader.NumberOfSections << std::endl;

        // Check file architecture
        std::cout << "Checking file architecture..." << std::endl;
        bool is64bit = ntHeaders->OptionalHeader.Magic == IMAGE_NT_OPTIONAL_HDR64_MAGIC;
        std::cout << "File is " << (is64bit ? "64-bit" : "32-bit") << std::endl;

        if (is64bit) {
            std::cerr << "64-bit executables are not supported in this version." << std::endl;
            throw std::runtime_error("Unsupported file architecture");
        }

        // Check if it's a DLL
        std::cout << "Checking if DLL..." << std::endl;
        bool isDll = (ntHeaders->FileHeader.Characteristics & IMAGE_FILE_DLL) != 0;

        // Identify the function to protect
        std::cout << "Identifying function to protect..." << std::endl;
        DWORD entryPointRVA = ntHeaders->OptionalHeader.AddressOfEntryPoint;

        // Prepare VM code
        std::cout << "Preparing VM code..." << std::endl;
        IMAGE_SECTION_HEADER* lastSection = getLastSection(ntHeaders);
        DWORD vmCodeRVA = lastSection->VirtualAddress + lastSection->Misc.VirtualSize;
        DWORD vmCodeFileOffset = lastSection->PointerToRawData + lastSection->SizeOfRawData;

        // Align vmCodeRVA to the next section alignment
        DWORD sectionAlignment = ntHeaders->OptionalHeader.SectionAlignment;
        vmCodeRVA = (vmCodeRVA + sectionAlignment - 1) & ~(sectionAlignment - 1);

        std::cout << "vmCodeRVA: " << vmCodeRVA << std::endl;
        std::cout << "vmCodeFileOffset: " << vmCodeFileOffset << std::endl;

        // Create VM code (x86 assembly)
        std::vector<uint8_t> vmCode = {
            0x60,                          // pushad
            0x9C,                          // pushfd
            0x68, 0x00, 0x00, 0x00, 0x00,  // push <bytecode_address>
            0xE8, 0x00, 0x00, 0x00, 0x00,  // call <vm_execute>
            0x9D,                          // popfd
            0x61,                          // popad
            0xE9, 0x00, 0x00, 0x00, 0x00   // jmp <original_entry_point>
        };

        // Add VM code and bytecode to the executable
        std::cout << "Adding VM code and bytecode..." << std::endl;
        std::vector<uint8_t> bytecode = { 0x01, 0x05, 0x02, 0x03, 0x03, 0x01, 0x0A, 0x03, 0x04 };

        // Ensure the protectedFile is large enough
        if (vmCodeFileOffset + vmCode.size() + bytecode.size() > protectedFile.size()) {
            protectedFile.resize(vmCodeFileOffset + vmCode.size() + bytecode.size());
        }

        // Copy VM code and bytecode
        std::copy(vmCode.begin(), vmCode.end(), protectedFile.begin() + vmCodeFileOffset);
        DWORD bytecodeFileOffset = vmCodeFileOffset + vmCode.size();
        std::copy(bytecode.begin(), bytecode.end(), protectedFile.begin() + bytecodeFileOffset);

        DWORD bytecodeRVA = vmCodeRVA + vmCode.size();

        // Update VM code with correct addresses
        std::cout << "Updating VM code addresses..." << std::endl;
        try {
            std::cout << "vmCodeRVA: " << vmCodeRVA << std::endl;
            std::cout << "bytecodeRVA: " << bytecodeRVA << std::endl;
            std::cout << "entryPointRVA: " << entryPointRVA << std::endl;

            if (vmCodeFileOffset + 3 >= protectedFile.size()) {
                throw std::runtime_error("VM code RVA out of bounds");
            }
            DWORD* bytecodeAddress = (DWORD*)(protectedFile.data() + vmCodeFileOffset + 3);
            *bytecodeAddress = bytecodeRVA;
            std::cout << "Bytecode address updated." << std::endl;

            if (vmCodeFileOffset + 8 >= protectedFile.size()) {
                throw std::runtime_error("VM execute address out of bounds");
            }
            DWORD* vmExecuteAddress = (DWORD*)(protectedFile.data() + vmCodeFileOffset + 8);
            *vmExecuteAddress = (DWORD)(UINT_PTR)&VirtualMachine::execute - (vmCodeRVA + 12);
            std::cout << "VM execute address updated." << std::endl;

            if (vmCodeFileOffset + 15 >= protectedFile.size()) {
                throw std::runtime_error("Original entry address out of bounds");
            }
            DWORD* originalEntryAddress = (DWORD*)(protectedFile.data() + vmCodeFileOffset + 15);
            *originalEntryAddress = entryPointRVA - (vmCodeRVA + 19);
            std::cout << "Original entry address updated." << std::endl;
        }
        catch (const std::exception& e) {
            std::cerr << "Error updating VM code addresses: " << e.what() << std::endl;
            std::cerr << "Protected file size: " << protectedFile.size() << " bytes" << std::endl;
            throw;
        }

        // Update the entry point to our VM code
        std::cout << "Updating entry point..." << std::endl;
        ntHeaders->OptionalHeader.AddressOfEntryPoint = vmCodeRVA;
        std::cout << "Entry point updated to: 0x" << std::hex << vmCodeRVA << std::dec << std::endl;

        // Verify the update
        if (ntHeaders->OptionalHeader.AddressOfEntryPoint != vmCodeRVA) {
            std::cerr << "Error: Failed to update entry point." << std::endl;
            throw std::runtime_error("Entry point update failed");
        }

        // Add a new section to contain our VM code and bytecode
        std::cout << "Adding new section..." << std::endl;
        IMAGE_SECTION_HEADER newSection = {};
        strncpy_s((char*)newSection.Name, sizeof(newSection.Name), ".vmprot", _TRUNCATE);
        newSection.VirtualAddress = vmCodeRVA;
        newSection.PointerToRawData = vmCodeFileOffset;
        newSection.SizeOfRawData = vmCode.size() + bytecode.size();
        newSection.Misc.VirtualSize = newSection.SizeOfRawData;
        newSection.Characteristics = IMAGE_SCN_MEM_EXECUTE | IMAGE_SCN_MEM_READ | IMAGE_SCN_CNT_CODE;

        // Add the new section header
        *(lastSection + 1) = newSection;
        ntHeaders->FileHeader.NumberOfSections++;

        // Update SizeOfImage
        ntHeaders->OptionalHeader.SizeOfImage = vmCodeRVA + newSection.Misc.VirtualSize;

        // Handle .NET executables/DLLs
        std::cout << "Handling .NET metadata..." << std::endl;
        IMAGE_DATA_DIRECTORY* clrHeader = &ntHeaders->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_COM_DESCRIPTOR];
        if (clrHeader->VirtualAddress != 0) {
            std::cout << "Detected .NET assembly." << std::endl;
            std::cout << "CLR Header VirtualAddress: 0x" << std::hex << clrHeader->VirtualAddress << std::dec << std::endl;
            std::cout << "CLR Header Size: " << clrHeader->Size << std::endl;
            
            if (clrHeader->VirtualAddress >= protectedFile.size()) {
                std::cerr << "Warning: CLR header is outside file bounds. Skipping .NET-specific modifications." << std::endl;
            } else {
                IMAGE_COR20_HEADER* corHeader = (IMAGE_COR20_HEADER*)(protectedFile.data() + clrHeader->VirtualAddress);

                if (clrHeader->VirtualAddress + sizeof(IMAGE_COR20_HEADER) > protectedFile.size()) {
                    std::cerr << "Warning: COR20 header extends beyond file bounds. Skipping .NET-specific modifications." << std::endl;
                } else {
                    std::cout << "Original EntryPointToken: 0x" << std::hex << corHeader->EntryPointToken << std::dec << std::endl;
                    std::cout << "Original Flags: 0x" << std::hex << corHeader->Flags << std::dec << std::endl;

                    // Obfuscate .NET metadata
                    std::cout << "Obfuscating .NET metadata..." << std::endl;
                    
                    // Load the CLR metadata
                    ICLRMetaHost* pMetaHost = nullptr;
                    ICLRRuntimeInfo* pRuntimeInfo = nullptr;
                    IMetaDataDispenserEx* pDispenser = nullptr;
                    IMetaDataImport2* pImport = nullptr;
                    IMetaDataEmit2* pEmit = nullptr;

                    HRESULT hr = CLRCreateInstance(CLSID_CLRMetaHost, IID_ICLRMetaHost, (LPVOID*)&pMetaHost);
                    if (SUCCEEDED(hr)) {
                        hr = pMetaHost->GetRuntime(L"v4.0.30319", IID_ICLRRuntimeInfo, (LPVOID*)&pRuntimeInfo);
                        if (SUCCEEDED(hr)) {
                            hr = pRuntimeInfo->GetInterface(CLSID_CorMetaDataDispenser, IID_IMetaDataDispenserEx, (LPVOID*)&pDispenser);
                            if (SUCCEEDED(hr)) {
                                hr = pDispenser->OpenScope(reinterpret_cast<LPCWSTR>(protectedFile.data()), ofRead, IID_IMetaDataImport2, (IUnknown**)&pImport);
                                if (SUCCEEDED(hr)) {
                                    hr = pDispenser->OpenScope(reinterpret_cast<LPCWSTR>(protectedFile.data()), ofWrite, IID_IMetaDataEmit2, (IUnknown**)&pEmit);
                                    if (SUCCEEDED(hr)) {
                                        // Obfuscate type names
                                        HCORENUM hEnum = nullptr;
                                        mdTypeDef typeDef;
                                        ULONG count;
                                        while (SUCCEEDED(pImport->EnumTypeDefs(&hEnum, &typeDef, 1, &count)) && count > 0) {
                                            WCHAR szName[1024];
                                            ULONG cchName;
                                            DWORD dwTypeDefFlags;
                                            mdToken tkExtends;
                                            hr = pImport->GetTypeDefProps(typeDef, szName, 1024, &cchName, &dwTypeDefFlags, &tkExtends);
                                            if (SUCCEEDED(hr)) {
                                                std::wstring newName = getObfuscatedName(10);
                                                hr = pEmit->SetTypeDefProps(typeDef, dwTypeDefFlags, tkExtends, nullptr);
                                                if (SUCCEEDED(hr)) {
                                                    hr = pEmit->SetTypeDefProps(typeDef, dwTypeDefFlags, tkExtends, nullptr);
                                                }
                                            }
                                        }
                                        pImport->CloseEnum(hEnum);

                                        // Obfuscate method names
                                        hEnum = nullptr;
                                        mdMethodDef methodDef;
                                        while (SUCCEEDED(pImport->EnumMethods(&hEnum, typeDef, &methodDef, 1, &count)) && count > 0) {
                                            WCHAR szName[1024];
                                            ULONG cchName;
                                            DWORD dwMethodAttributes;
                                            PCCOR_SIGNATURE pvSigBlob;
                                            ULONG cbSigBlob;
                                            ULONG ulCodeRVA;
                                            DWORD dwImplFlags;
                                            hr = pImport->GetMethodProps(methodDef, nullptr, szName, 1024, &cchName, &dwMethodAttributes, &pvSigBlob, &cbSigBlob, &ulCodeRVA, &dwImplFlags);
                                            if (SUCCEEDED(hr)) {
                                                std::wstring newName = getObfuscatedName(10);
                                                // Update the method name
                                                hr = pEmit->SetMethodProps(methodDef, dwMethodAttributes, ulCodeRVA, dwImplFlags);
                                            }
                                        }
                                        pImport->CloseEnum(hEnum);

                                        std::cout << "Metadata obfuscation completed." << std::endl;
                                    }
                                }
                            }
                        }
                    }

                    // Clean up
                    if (pEmit) pEmit->Release();
                    if (pImport) pImport->Release();
                    if (pDispenser) pDispenser->Release();
                    if (pRuntimeInfo) pRuntimeInfo->Release();
                    if (pMetaHost) pMetaHost->Release();

                    // Modify the .NET metadata to include our VM
                    corHeader->EntryPointToken = 0;
                    corHeader->Flags |= COMIMAGE_FLAGS_NATIVE_ENTRYPOINT;
                    ntHeaders->OptionalHeader.AddressOfEntryPoint = vmCodeRVA;

                    std::cout << "Modified EntryPointToken: 0x" << std::hex << corHeader->EntryPointToken << std::dec << std::endl;
                    std::cout << "Modified Flags: 0x" << std::hex << corHeader->Flags << std::dec << std::endl;
                    std::cout << "New AddressOfEntryPoint: 0x" << std::hex << ntHeaders->OptionalHeader.AddressOfEntryPoint << std::dec << std::endl;
                }
            }
        } else {
            std::cout << "Not a .NET assembly, skipping .NET-specific modifications." << std::endl;
        }

        std::cout << "Final protected file size: " << protectedFile.size() << " bytes" << std::endl;
        std::cout << "New number of sections: " << ntHeaders->FileHeader.NumberOfSections << std::endl;
        std::cout << "New SizeOfImage: " << ntHeaders->OptionalHeader.SizeOfImage << std::endl;
        std::cout << "Protection process completed successfully." << std::endl;
    }
    catch (const std::exception& e) {
        std::cerr << "Error in protectExecutable: " << e.what() << std::endl;
        throw;
    }
    catch (...) {
        std::cerr << "Unknown error in protectExecutable" << std::endl;
        throw;
    }

    return protectedFile;
}


int main(int argc, char* argv[])
{
    try {
        if (argc != 2) {
            std::cerr << "Usage: " << argv[0] << " <input_file>" << std::endl;
            return 1;
        }

        std::string inputPath = argv[1];
        std::filesystem::path inputFilePath(inputPath);
        
        if (!std::filesystem::exists(inputFilePath)) {
            std::cerr << "Input file does not exist: " << inputPath << std::endl;
            return 1;
        }

        if (inputFilePath.extension() != ".exe" && inputFilePath.extension() != ".dll") {
            std::cerr << "Input file must be either .exe or .dll" << std::endl;
            return 1;
        }

        std::filesystem::path outputFilePath = inputFilePath.parent_path() / (inputFilePath.stem().string() + "_protected" + inputFilePath.extension().string());

        std::cout << "Reading input file: " << inputPath << std::endl << std::flush;
        std::vector<uint8_t> inputFile = readExecutable(inputPath);
        std::cout << "Input file size: " << inputFile.size() << " bytes" << std::endl << std::flush;

        std::cout << "Protecting " << (inputFilePath.extension() == ".dll" ? "DLL" : "executable") << "..." << std::endl << std::flush;
        std::vector<uint8_t> protectedFile;
        try {
            protectedFile = protectExecutable(inputFile);
        } catch (const std::exception& e) {
            std::cerr << "Error during protection: " << e.what() << std::endl << std::flush;
            return 1;
        }
        std::cout << "Protected file size: " << protectedFile.size() << " bytes" << std::endl << std::flush;

        std::cout << "Writing protected file to: " << outputFilePath << std::endl << std::flush;
        writeExecutable(outputFilePath.string(), protectedFile);

        std::cout << "Protection complete. Protected file written to: " << outputFilePath << std::endl << std::flush;
    }
    catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl << std::flush;
        return 1;
    }
    catch (...) {
        std::cerr << "Unknown error occurred" << std::endl << std::flush;
        return 1;
    }

    return 0;
}
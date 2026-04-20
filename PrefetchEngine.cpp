#include <iostream>
#include <cstdint> // Fixed width integer types like uint32_t
#include <fstream> // Read files
#include <vector> // Dynamic Array
#include <windows.h> // Gives windows types like DWORD, BYTE, etc.

/**
 * @brief Acts as a template.
 * 
 * Maps the first eight bytes of the file,
 * to determine if the data is compressed,
 * if so, exactly how much memory is needed to
 * hold the file once fully decompressed
 * 
 * @param Signature Creates the format
 * @param UncompressedSize Determines the adjusted file size
 * 
 */
struct CompressionHeader {
    uint32_t Signature; // Typically "MAM"
    uint32_t UncompressedSize; // How big the file size will be after fixed
};

/**
 * @brief
 * 
 * @param version Prefetch file version
 * @param signature Prefetch file format
 * @param unknown
 * @param fileSize
 * @param executableName
 * @param hash
 */
struct PrefetchHeader {
    uint32_t version;
    uint32_t signature;
    uint32_t unknown;
    uint32_t fileSize;
    wchar_t executableName[30]; // Where the name will be stored
    uint32_t hash;
};

/**
 * @brief
 * 
 * @param CompressionFormatAndEngine
 * @param CompressBufferWorSpaceSize
 * @param CompressFragmentWorkSpaceSize
 */
typedef DWORD(WINAPI *RtlGetCompressionWorkSpaceSize_t)( // Two new hidden API Functions
    USHORT CompressionFormatAndEngine,
    PULONG CompressBufferWorkSpaceSize,
    PULONG CompressFragmentWorkSpaceSize
);

// Function pointer type for the hidden Windows API

/**
 * @brief
 * 
 * @param CompressionFormat
 * @param UncompressedBuffer
 * @param UncompressedBufferSize
 * @param CompressedBuffer
 * @param CompressedBufferSize
 * @param FinalUncompressedSize
 * @param Workspace
 */
typedef DWORD(WINAPI *RtlDecompressBufferEx_t)(
    USHORT CompressionFormat,
    PUCHAR UncompressedBuffer,
    ULONG UncompressedBufferSize,
    PUCHAR CompressedBuffer,
    ULONG CompressedBufferSize,
    PULONG FinalUncompressedSize,
    PVOID Workspace
);


int main() {
    std::string filename = "C:\\Windows\\Prefetch\\CALCULATORAPP.EXE-BF9881B2.pf";

    
    std::ifstream file(filename, std::ios::binary | std::ios::ate); // Open file. 'ate' = "at the end" so we can tell the size.

    if (!file.is_open()) {
        std::cerr << "Failed to open file!" << std::endl;
        return 1;
    }

    
    std::streamsize size = file.tellg(); // Retrieve file size
    file.seekg(0, std::ios::beg); // Move to the start

    // create a buffer (a container) to hold the raw file data
    std::vector<char> fileData(size);

    // Read the whole file into our buffer
    if (!file.read(fileData.data(), size)) {
        std::cerr << "Unable to read file" << std::endl;
        return 1;
    }
    //--- 2.3 Check Compression ---
    // Point a header pointer to the start of the data
    CompressionHeader* header = reinterpret_cast<CompressionHeader*>(fileData.data());

    // std::cout << "[DEBUG] First 4 bytes (hex): " << std::hex << header -> Signature << std::endl;
    // Verify the signature matching "MAM" (It's suppose to be CALCULATORAPP.EXE-89  09A534.pf)
    if (header->Signature == 0x044D414D) {
        std::cout << "File is indeed compressed." << std::endl;
        std::cout << "Uncompressed size will be: " << header->UncompressedSize << std::endl;
    } else {
        std::cout << "File is NOT compressed." << std::endl;
        // Handling uncompressed logic here (rare on Windows 10)
    }


    //--- 2.4 decopmression ---
    // Load the library manually
    HMODULE hNtdll = LoadLibraryA("ntdll.dll");
    if (!hNtdll) return 1;

    RtlGetCompressionWorkSpaceSize_t RtlGetWorkspaceSize = (RtlGetCompressionWorkSpaceSize_t)GetProcAddress(hNtdll, "RtlGetCompressionWorkSpaceSize");

    RtlDecompressBufferEx_t RtlDecompressBufferEx = (RtlDecompressBufferEx_t)GetProcAddress(hNtdll, "RtlDecompressBufferEx");

    if (!RtlGetWorkspaceSize || !RtlDecompressBufferEx) {
        std::cerr << "[-] Unable to find extended compression APIs!" << std::endl;
        return 1;
    }

    // How big does the "scratchpad" need to be?
    ULONG bufferWorkspaceSize = 0;
    ULONG fragmentWorkspaceSize = 0;

    DWORD wsStatus = RtlGetWorkspaceSize(
        0x0004, // Xpress Huffman
        &bufferWorkspaceSize,
        &fragmentWorkspaceSize
    );

    if (wsStatus != 0) {
        std::cerr << "[-] Unable to get workspace size. Error Code: 0x" << std::hex << wsStatus << std::endl;
        return 1;
    }

    // Create the Scratchpad (workspace) and the Uncompressed Data Buffer
    std::vector<char> workspace(fragmentWorkspaceSize);
    std::vector<char> uncompressedData(header->UncompressedSize);
    ULONG finalSize = 0;

    // Run the decompression
    DWORD status = RtlDecompressBufferEx(
        0x0004, // LZX compression format
        (PUCHAR)uncompressedData.data(),
        header->UncompressedSize,
        (PUCHAR)fileData.data() + 8, // Skip the first 8 bytes (the header)
        size - 8,
        &finalSize,
        workspace.data() // The Missing scratchpad
    );

    if (status != 0) {
        std::cerr << "Decompression has FAILED! NSTATUS Error Code: 0x" << std::hex << status <<std::endl;
        return 1;
    }

    // -------- 2.5 Read Results --------
    // Point our Prefetch struct to the new clean data
    PrefetchHeader* pfHeader = reinterpret_cast<PrefetchHeader*>(uncompressedData.data());

    // Print the results
    std::cout << "-----------------------------" << std::endl;
    std::wcout << L"Executable: " << pfHeader->executableName << std::endl;
    std::cout << "File Hash: " << std::hex << pfHeader->hash << std::endl;
    std::cout << "-----------------------------" << std::endl;

    //00000----000000
    // Writing to CSV -- 2.6
    std::wofstream logFile("evidence_log.csv", std::ios::app);

    if (logFile.is_open()) {
        logFile << pfHeader -> executableName << L"," << std::hex << pfHeader -> hash << L"\n";
        logFile.close();
        std::wcout << L"[+] Successfully appended to evidence_log.csv" << std::endl;
    } else {
        std::cerr << "[-] Unable to open CSV file for writing" << std::endl;
    }

}
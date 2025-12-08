#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>
#include <cstdint>

#include <windows.h>
#include <wincodec.h>
#include <io.h>
#include <fcntl.h>
#include <wrl/client.h>

#pragma comment(lib, "windowscodecs.lib")
#pragma comment(lib, "ole32.lib")

#include "sponeocr.hpp"

using Microsoft::WRL::ComPtr;

static HRESULT RunOCROnStream(OCRPipelineHandle pipelineHandle, IStream* pStream, OCRProcessOptionsHandle processOptionsHandle, OCRResultHandle* outResultHandle) {
    HRESULT hr = S_OK;
    ImageInfo imageInfo{};

    ComPtr<IWICImagingFactory> pIWICFactory;
    hr = CoCreateInstance(CLSID_WICImagingFactory, nullptr, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&pIWICFactory));
    if (FAILED(hr)) {
        std::wcerr << L"Failed to create WIC Imaging Factory. Error code: " << hr << std::endl;
        return hr;
    }

    // Create decoder from stream
    ComPtr<IWICBitmapDecoder> pDecoder;
    hr = pIWICFactory->CreateDecoderFromStream(
        pStream, nullptr, WICDecodeMetadataCacheOnDemand, &pDecoder);
    if (FAILED(hr)) {
        // std::wcerr << L"Failed to create decoder from stream. Error code: " << hr << std::endl;
        return hr;
    }

    // Get the first frame
    ComPtr<IWICBitmapFrameDecode> pFrame;
    hr = pDecoder->GetFrame(0, &pFrame);
    if (FAILED(hr)) return hr;

    // Create format converter
    ComPtr<IWICFormatConverter> pFormatConverter;
    hr = pIWICFactory->CreateFormatConverter(&pFormatConverter);
    if (FAILED(hr)) return hr;

    hr = pFormatConverter->Initialize(
        pFrame.Get(), GUID_WICPixelFormat32bppPBGRA, WICBitmapDitherTypeNone, nullptr, 0.0f, WICBitmapPaletteTypeCustom);
    if (FAILED(hr)) return hr;

    // Create bitmap from the converted frame
    ComPtr<IWICBitmap> pIWICBitmap;
    hr = pIWICFactory->CreateBitmapFromSource(pFormatConverter.Get(), WICBitmapCacheOnLoad, &pIWICBitmap);
    if (FAILED(hr)) return hr;

    // Get image dimensions
    UINT width, height;
    hr = pIWICBitmap->GetSize(&width, &height);
    if (FAILED(hr)) return hr;

    if (width >= 10000 || height >= 10000) {
        return E_FAIL;
    }

    // Pad image if smaller than 50 x 50
    if (width < 50 || height < 50) {
        UINT newWidth = (width > 50) ? width : 50;
        UINT newHeight = (height > 50) ? height : 50;

        ComPtr<IWICBitmap> pPaddedBitmap;
        hr = pIWICFactory->CreateBitmap(newWidth, newHeight, GUID_WICPixelFormat32bppPBGRA, WICBitmapCacheOnLoad, &pPaddedBitmap);
        if (FAILED(hr)) return hr;

        {
            ComPtr<IWICBitmapLock> pDestLock;
            WICRect destRect = { 0, 0, (INT)newWidth, (INT)newHeight };
            hr = pPaddedBitmap->Lock(&destRect, WICBitmapLockWrite, &pDestLock);
            if (FAILED(hr)) return hr;

            UINT destSize = 0;
            BYTE* pDestData = nullptr;
            UINT destStride = 0;
            hr = pDestLock->GetDataPointer(&destSize, &pDestData);
            if (FAILED(hr) || pDestData == nullptr) return hr;
            hr = pDestLock->GetStride(&destStride);
            if (FAILED(hr)) return hr;

            // Fill with white (0xFF)
            // PBGRA: B=255, G=255, R=255, A=255
            memset(pDestData, 0xFF, destSize);

            // Use CopyPixels to copy the source image into the destination buffer
            // We pass destStride so that it knows how to skip bytes to reach the next row in the destination
            hr = pIWICBitmap->CopyPixels(nullptr, destStride, destSize, pDestData);
            if (FAILED(hr)) return hr;
        }

        // Replace the original bitmap with the padded one
        pIWICBitmap = pPaddedBitmap;
        width = newWidth;
        height = newHeight;
    }

    WICRect rect = { 0, 0, static_cast<INT>(width), static_cast<INT>(height) };
    ComPtr<IWICBitmapLock> pLock;
    hr = pIWICBitmap->Lock(&rect, WICBitmapLockRead, &pLock);
    if (FAILED(hr)) return hr;

    UINT bufferSize = 0, stride = 0;
    BYTE* pData = nullptr;
    hr = pLock->GetDataPointer(&bufferSize, &pData);
    if (FAILED(hr)) return hr;

    hr = pLock->GetStride(&stride);
    if (FAILED(hr)) return hr;

    imageInfo.type = 3;
    imageInfo.width = static_cast<uint32_t>(width);
    imageInfo.height = static_cast<uint32_t>(height);
    imageInfo.stride = stride;
    imageInfo.dataPointer = reinterpret_cast<uint64_t>(pData);

    int result = RunOcrPipeline(pipelineHandle, &imageInfo, processOptionsHandle, outResultHandle);

    if (result == 0) {
        return S_OK;
    } else {
        return E_FAIL;
    }
}

OCRInitOptionsHandle initOptionsHandle = NULL;
OCRPipelineHandle pipelineHandle = NULL;
OCRProcessOptionsHandle processOptionsHandle = NULL;

bool inited = false;
extern "C" __declspec(dllexport) int init(const char* model_path) {
    if (inited) {
        return 0;
    }
    HMODULE hDll = LoadLibraryW(L"oneocr.dll");
    if (!hDll) {
        printf("Failed to load oneocr.dll\n");
        return false;
    }
    CreateOcrInitOptions = (CreateOcrInitOptions_t)GetProcAddress(hDll, "CreateOcrInitOptions");
    CreateOcrPipeline = (CreateOcrPipeline_t)GetProcAddress(hDll, "CreateOcrPipeline");
    CreateOcrProcessOptions = (CreateOcrProcessOptions_t)GetProcAddress(hDll, "CreateOcrProcessOptions");
    GetImageAngle = (GetImageAngle_t)GetProcAddress(hDll, "GetImageAngle");
    GetOcrLine = (GetOcrLine_t)GetProcAddress(hDll, "GetOcrLine");
    GetOcrLineBoundingBox = (GetOcrLineBoundingBox_t)GetProcAddress(hDll, "GetOcrLineBoundingBox");
    GetOcrLineContent = (GetOcrLineContent_t)GetProcAddress(hDll, "GetOcrLineContent");
    GetOcrLineCount = (GetOcrLineCount_t)GetProcAddress(hDll, "GetOcrLineCount");
    GetOcrLineStyle = (GetOcrLineStyle_t)GetProcAddress(hDll, "GetOcrLineStyle");
    GetOcrLineWordCount = (GetOcrLineWordCount_t)GetProcAddress(hDll, "GetOcrLineWordCount");
    GetOcrWord = (GetOcrWord_t)GetProcAddress(hDll, "GetOcrWord");
    GetOcrWordBoundingBox = (GetOcrWordBoundingBox_t)GetProcAddress(hDll, "GetOcrWordBoundingBox");
    GetOcrWordConfidence = (GetOcrWordConfidence_t)GetProcAddress(hDll, "GetOcrWordConfidence");
    GetOcrWordContent = (GetOcrWordContent_t)GetProcAddress(hDll, "GetOcrWordContent");
    OcrInitOptionsSetUseModelDelayLoad = (OcrInitOptionsSetUseModelDelayLoad_t)GetProcAddress(hDll, "OcrInitOptionsSetUseModelDelayLoad");
    OcrProcessOptionsGetMaxRecognitionLineCount = (OcrProcessOptionsGetMaxRecognitionLineCount_t)GetProcAddress(hDll, "OcrProcessOptionsGetMaxRecognitionLineCount");
    OcrProcessOptionsGetResizeResolution = (OcrProcessOptionsGetResizeResolution_t)GetProcAddress(hDll, "OcrProcessOptionsGetResizeResolution");
    OcrProcessOptionsSetMaxRecognitionLineCount = (OcrProcessOptionsSetMaxRecognitionLineCount_t)GetProcAddress(hDll, "OcrProcessOptionsSetMaxRecognitionLineCount");
    OcrProcessOptionsSetResizeResolution = (OcrProcessOptionsSetResizeResolution_t)GetProcAddress(hDll, "OcrProcessOptionsSetResizeResolution");
    ReleaseOcrInitOptions = (ReleaseOcrInitOptions_t)GetProcAddress(hDll, "ReleaseOcrInitOptions");
    ReleaseOcrPipeline = (ReleaseOcrPipeline_t)GetProcAddress(hDll, "ReleaseOcrPipeline");
    ReleaseOcrProcessOptions = (ReleaseOcrProcessOptions_t)GetProcAddress(hDll, "ReleaseOcrProcessOptions");
    ReleaseOcrResult = (ReleaseOcrResult_t)GetProcAddress(hDll, "ReleaseOcrResult");
    RunOcrPipeline = (RunOcrPipeline_t)GetProcAddress(hDll, "RunOcrPipeline");

    uint32_t result;

    result = CreateOcrInitOptions(&initOptionsHandle);
    // std::cout << "CreateOcrInitOptions: " << result << "\n";
    // std::cout << "OCR Handle: " << initOptionsHandle << "\n";
    if (result != 0) return result;

    result = OcrInitOptionsSetUseModelDelayLoad(initOptionsHandle, 0);
    // std::cout << "OcrInitOptionsSetUseModelDelayLoad: " << result << "\n";
    if (result != 0) return result;

    result = CreateOcrPipeline(model_path, "kj)TGtrK>f]b[Piow.gU+nC@s\"\"\"\"\"\"4", initOptionsHandle, &pipelineHandle);
    // std::cout << "CreateOcrPipeline: " << result << "\n";
    // std::cout << "OCR Pipeline Handle: " << pipelineHandle << "\n";
    if (result != 0) return result;

    result = CreateOcrProcessOptions(&processOptionsHandle);
    // std::cout << "CreateOcrProcessOptions: " << result << "\n";
    // std::cout << "OCR Options Handle: " << processOptionsHandle << "\n";
    if (result != 0) return result;

    result = OcrProcessOptionsSetMaxRecognitionLineCount(processOptionsHandle, 1000);
    // std::cout << "OcrProcessOptionsSetMaxRecognitionLineCount: " << result << "\n";
    if (result != 0) return result;

    uint32_t outCount = 0;
    result = OcrProcessOptionsGetMaxRecognitionLineCount(processOptionsHandle, &outCount);
    // std::cout << "OcrProcessOptionsGetMaxRecognitionLineCount: " << result << "\n";
    // std::cout << "outCount: " << outCount << "\n";
    if (result != 0) return result;

    result = OcrProcessOptionsSetResizeResolution(processOptionsHandle, 1152, 768);
    // std::cout << "OcrProcessOptionsSetResizeResolution: " << result << "\n";
    if (result != 0) return result;

    uint64_t outWidth = 0;
    uint64_t outHeight = 0;
    result = OcrProcessOptionsGetResizeResolution(processOptionsHandle, &outWidth, &outHeight);
    // std::cout << "OcrProcessOptionsGetResizeResolution: " << result << "\n";
    // std::cout << "outWidth: " << outWidth << " outHeight: " << outHeight << "\n";
    if (result != 0) return result;

    inited = true;
    return 0;
}

static std::string getOcrResultAsText(OCRResultHandle resultHandle) {
    uint64_t linecount = 0;
    if (GetOcrLineCount(resultHandle, &linecount) != 0) {
        return "";
    }

    std::string outputText = "";
    // Process each OCR line
    for (uint32_t i = 0; i < linecount; i++) {
        OCRLineHandle lineHandle;
        if (GetOcrLine(resultHandle, i, &lineHandle) != 0) {
            continue;
        }

        LPCCH lineContent;
        if (GetOcrLineContent(lineHandle, &lineContent) != 0) {
            continue;
        }

        if (!outputText.empty()) {
            outputText += "\r\n";
        }
        outputText += lineContent;
    }
    return outputText;
}

extern "C" __declspec(dllexport) int uninit() {
    ReleaseOcrProcessOptions(processOptionsHandle);
    ReleaseOcrPipeline(pipelineHandle);
    ReleaseOcrInitOptions(initOptionsHandle);
    return 0;
}

static void LogMessage(const std::string& message) {
    std::cerr << message << std::endl;
}

int main(int argc, char* argv[]) {
    LogMessage("--- OCR Process Started ---");

    if (argc < 3) {
        std::cerr << "Usage: sponeocr.exe <input|stdin> <output|stdout>" << std::endl;
        LogMessage("Error: Startup failed. Insufficient arguments. Expected 2.");
        return 1;
    }

    // Determine Input Source
    bool readFromStdin = true;
    std::string inputPath = "";
    if (argc > 1) {
        std::string arg1 = argv[1];
        if (arg1 != "stdin" && arg1 != "-") {
            readFromStdin = false;
            inputPath = arg1;
        }
    }
    LogMessage("Arguments parsed. ReadFromStdin: " + std::to_string(readFromStdin));

    // Determine Output Destination
    bool writeToStdout = true;
    std::string outputPath = "";
    if (argc > 2) {
        std::string arg2 = argv[2];
        if (arg2 != "stdout" && arg2 != "-") {
            writeToStdout = false;
            outputPath = arg2;
        }
    }

    // 1. Read content into buffer
    std::vector<uint8_t> buffer;

    if (readFromStdin) {
        _setmode(_fileno(stdin), _O_BINARY);
        char tempBuf[4096];
        int bytesRead;
        while ((bytesRead = _read(_fileno(stdin), tempBuf, sizeof(tempBuf))) > 0) {
            buffer.insert(buffer.end(), tempBuf, tempBuf + bytesRead);
        }
        LogMessage("Finished reading stdin. Buffer size: " + std::to_string(buffer.size()));
    } else {
        std::ifstream file(inputPath, std::ios::binary | std::ios::ate);
        if (!file.is_open()) {
            std::string msg = "Failed to open input file: " + inputPath;
            std::cerr << msg << std::endl;
            LogMessage(msg);
            return 1;
        }
        std::streamsize size = file.tellg();
        file.seekg(0, std::ios::beg);
        
        if (size > 0) {
            buffer.resize(size);
            if (!file.read(reinterpret_cast<char*>(buffer.data()), size)) {
                std::string msg = "Failed to read input file.";
                std::cerr << msg << std::endl;
                LogMessage(msg);
                return 1;
            }
        }
        LogMessage("Finished reading file. Buffer size: " + std::to_string(buffer.size()));
    }

    if (buffer.empty()) {
        std::string msg = "No input data received.";
        std::cerr << msg << std::endl;
        LogMessage(msg);
        return 1;
    }

    HRESULT hr = CoInitialize(NULL);
    if (FAILED(hr)) {
        std::string msg = "CoInitialize failed. HR=" + std::to_string(hr);
        std::cerr << msg << std::endl;
        LogMessage(msg);
        return 1;
    }

    char exePathBuf[MAX_PATH];
    GetModuleFileNameA(NULL, exePathBuf, MAX_PATH);
    std::filesystem::path exeDir = std::filesystem::path(exePathBuf).parent_path();
    std::string modelPath = (exeDir / "oneocr.onemodel").string();
    
    LogMessage("Model Path: " + modelPath);

    // 2. Initialize OCR
    LogMessage("Initializing OCR engine...");
    if (init(modelPath.c_str()) != 0) {
        std::string msg = "Failed to init OCR engine. Ensure oneocr.onemodel is present at: " + modelPath;
        std::cerr << msg << std::endl;
        LogMessage(msg);
        CoUninitialize();
        return 1;
    }

    // 3. Create Stream from buffer
    HGLOBAL hMem = GlobalAlloc(GMEM_MOVEABLE, buffer.size());
    if (!hMem) {
        LogMessage("GlobalAlloc failed.");
        uninit();
        CoUninitialize();
        return 1;
    }

    void* pMem = GlobalLock(hMem);
    memcpy(pMem, buffer.data(), buffer.size());
    GlobalUnlock(hMem);

    IStream* pStream = nullptr;
    if (FAILED(CreateStreamOnHGlobal(hMem, TRUE, &pStream))) {
        LogMessage("CreateStreamOnHGlobal failed.");
        GlobalFree(hMem);
        uninit();
        CoUninitialize();
        return 1;
    }

    // 4. Run OCR
    LogMessage("Running OCR pipeline...");
    OCRResultHandle resultHandle = NULL;
    hr = RunOCROnStream(pipelineHandle, pStream, processOptionsHandle, &resultHandle);
    pStream->Release();

    if (FAILED(hr)) {
        std::string msg = "OCR Processing Failed. HR=" + std::to_string(hr);
        std::cerr << msg << std::endl;
        LogMessage(msg);
    } else {
        LogMessage("OCR Success. Generating Text...");
        // 5. Output Result
        std::string outputText = getOcrResultAsText(resultHandle);

        LogMessage(outputText);

        if (writeToStdout) {
            _setmode(_fileno(stdout), _O_BINARY);
            std::cout << outputText << "\r\n";
            LogMessage("Written Text to stdout.");
        } else {
            std::ofstream outFile(outputPath, std::ios::binary);
            if (!outFile.is_open()) {
                std::string msg = "Failed to open output file: " + outputPath;
                std::cerr << msg << std::endl;
                LogMessage(msg);
            } else {
                outFile << outputText;
                LogMessage("Written Text to output file: " + outputPath);
            }
        }
        
        ReleaseOcrResult(resultHandle);
    }

    // Cleanup
    uninit();
    CoUninitialize();
    LogMessage("--- Exiting Main ---");
    return 0;
}
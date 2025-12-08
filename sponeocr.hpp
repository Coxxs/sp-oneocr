#include <windows.h>
#include <cstdint>

struct ImageInfo {
    uint32_t type;
    uint32_t width;
    uint32_t height;
    uint64_t stride;
    uint64_t dataPointer;
};

struct Point {
    float x;
    float y;
};

struct BoundingBox {
    Point TopLeft;
    Point TopRight;
    Point BottomRight;
    Point BottomLeft;
};

enum LineStyle {
    Handwritten = 0,
    Other = 1
};

using OCRInitOptionsHandle = uint64_t;
using OCRPipelineHandle = uint64_t;
using OCRProcessOptionsHandle = uint64_t;
using OCRResultHandle = uint64_t;
using OCRLineHandle = uint64_t;
using OCRWordHandle = uint64_t;

using CreateOcrInitOptions_t = uint32_t(*)(OCRInitOptionsHandle* outInitOptionsHandle);
using CreateOcrPipeline_t = uint32_t(*)(const char* modelPath, const char* licenseKey, OCRInitOptionsHandle initOptionsHandle, OCRPipelineHandle* outPipelineHandle);
using CreateOcrProcessOptions_t = uint32_t(*)(OCRProcessOptionsHandle* outProcessOptionsHandle);
using GetImageAngle_t = uint32_t(*)(OCRResultHandle resultHandle, float* angle);
using GetOcrLine_t = uint32_t(*)(OCRResultHandle resultHandle, uint64_t index, OCRLineHandle* outLineHandle);
using GetOcrLineBoundingBox_t = uint32_t(*)(OCRLineHandle lineHandle, BoundingBox** outBoundingBox);
using GetOcrLineContent_t = uint32_t(*)(OCRLineHandle lineHandle, LPCCH* content);
using GetOcrLineCount_t = uint32_t(*)(OCRResultHandle resultHandle, uint64_t* outLineCount);
using GetOcrLineStyle_t = uint32_t(*)(OCRLineHandle lineHandle, LineStyle* outLineStyle, float* outConfidence);
using GetOcrLineWordCount_t = uint32_t(*)(OCRLineHandle lineHandle, uint64_t* outWordCount);
using GetOcrWord_t = uint32_t(*)(OCRLineHandle lineHandle, uint64_t index, OCRWordHandle* outWordHandle);
using GetOcrWordBoundingBox_t = uint32_t(*)(OCRWordHandle wordHandle, BoundingBox** outBoundingBox);
using GetOcrWordConfidence_t = uint32_t(*)(OCRWordHandle wordHandle, float* outConfidence);
using GetOcrWordContent_t = uint32_t(*)(OCRWordHandle wordHandle, LPCCH* content);
using OcrInitOptionsSetUseModelDelayLoad_t = uint32_t(*)(OCRInitOptionsHandle initOptionsHandle, char value);
using OcrProcessOptionsGetMaxRecognitionLineCount_t = uint32_t(*)(OCRProcessOptionsHandle processOptionsHandle, uint32_t* outLineCount);
using OcrProcessOptionsGetResizeResolution_t = uint32_t(*)(OCRProcessOptionsHandle processOptionsHandle, uint64_t* outWidth, uint64_t* outHeight); // It's actually writing 8 bytes (uint64_t) output here, idk why
using OcrProcessOptionsSetMaxRecognitionLineCount_t = uint32_t(*)(OCRProcessOptionsHandle processOptionsHandle, uint32_t lineCount);
using OcrProcessOptionsSetResizeResolution_t = uint32_t(*)(OCRProcessOptionsHandle processOptionsHandle, uint32_t width, uint32_t height);
using ReleaseOcrInitOptions_t = uint32_t(*)(OCRInitOptionsHandle initOptionsHandle);
using ReleaseOcrPipeline_t = uint32_t(*)(OCRPipelineHandle pipelineHandle);
using ReleaseOcrProcessOptions_t = uint32_t(*)(OCRProcessOptionsHandle processOptionsHandle);
using ReleaseOcrResult_t = uint32_t(*)(OCRResultHandle resultHandle);
using RunOcrPipeline_t = uint32_t(*)(OCRPipelineHandle pipelineHandle, ImageInfo* imageData, OCRProcessOptionsHandle processOptionsHandle, OCRResultHandle* outResultHandle);


extern "C" {
    CreateOcrInitOptions_t CreateOcrInitOptions;
    CreateOcrPipeline_t CreateOcrPipeline;
    CreateOcrProcessOptions_t CreateOcrProcessOptions;
    GetImageAngle_t GetImageAngle;
    GetOcrLine_t GetOcrLine;
    GetOcrLineBoundingBox_t GetOcrLineBoundingBox;
    GetOcrLineContent_t GetOcrLineContent;
    GetOcrLineCount_t GetOcrLineCount;
    GetOcrLineStyle_t GetOcrLineStyle;
    GetOcrLineWordCount_t GetOcrLineWordCount;
    GetOcrWord_t GetOcrWord;
    GetOcrWordBoundingBox_t GetOcrWordBoundingBox;
    GetOcrWordConfidence_t GetOcrWordConfidence;
    GetOcrWordContent_t GetOcrWordContent;
    OcrInitOptionsSetUseModelDelayLoad_t OcrInitOptionsSetUseModelDelayLoad;
    OcrProcessOptionsGetMaxRecognitionLineCount_t OcrProcessOptionsGetMaxRecognitionLineCount;
    OcrProcessOptionsGetResizeResolution_t OcrProcessOptionsGetResizeResolution;
    OcrProcessOptionsSetMaxRecognitionLineCount_t OcrProcessOptionsSetMaxRecognitionLineCount;
    OcrProcessOptionsSetResizeResolution_t OcrProcessOptionsSetResizeResolution;
    ReleaseOcrInitOptions_t ReleaseOcrInitOptions;
    ReleaseOcrPipeline_t ReleaseOcrPipeline;
    ReleaseOcrProcessOptions_t ReleaseOcrProcessOptions;
    ReleaseOcrResult_t ReleaseOcrResult;
    RunOcrPipeline_t RunOcrPipeline;
}
#ifndef fcMP4Internal_h
#define fcMP4Internal_h

#include "fcFoundation.h"

struct fcI420Image
{
    char *y;
    char *u;
    char *v;

    fcI420Image() : y(), u(), v() {}
};


// raw video sample
struct fcVideoFrame
{
    fcTime timestamp;
    Buffer raw;
    Buffer rgba;
    fcI420Image i420;

    fcVideoFrame() : timestamp(0) {}
    ~fcVideoFrame() { deallocate(); }

    void allocate(int width, int height)
    {
        deallocate();

        rgba.resize(width * height * 4);
        int af = roundup<2>(width) * roundup<2>(height);
        i420.y = (char*)AlignedAlloc(af, 32);
        i420.u = (char*)AlignedAlloc(af >> 2, 32);
        i420.v = (char*)AlignedAlloc(af >> 2, 32);
    }

    void deallocate()
    {
        rgba.clear();
        AlignedFree(i420.y);
        AlignedFree(i420.u);
        AlignedFree(i420.v);
    }
};

// raw audio sample
struct fcAudioFrame
{
    fcTime timestamp;
    Buffer data;

    fcAudioFrame() : timestamp() {}
};

enum fcFrameType
{
    fcFrameType_Unknown,
    fcFrameType_H264,
    fcFrameType_AAC,
};

enum fcH264FrameType
{
    fcH264FrameType_Invalid,    ///< encoder not ready or parameters are invalidate
    fcH264FrameType_IDR,        ///< IDR frame in H.264
    fcH264FrameType_I,          ///< I frame type
    fcH264FrameType_P,          ///< P frame type
    fcH264FrameType_Skip,       ///< skip the frame based encoder kernel
    fcH264FrameType_IPMixed     ///< a frame where I and P slices are mixing, not supported yet
};

struct fcH264NALHeader
{
    uint8_t forbidden_zero_bit  : 1;
    uint8_t nal_ref_idc         : 2;
    uint8_t nal_unit_type       : 5; // ENalUnitType

    fcH264NALHeader() {}
    fcH264NALHeader(char c) {
        forbidden_zero_bit  = (c >> 7) & 0x01;
        nal_ref_idc         = (c >> 5) & 0x03;
        nal_unit_type       = c & 0x1f;
    }
};

struct fcFrameData
{
    fcFrameType type;
    fcTime timestamp;
    Buffer data;

    fcFrameData() : type(fcFrameType_Unknown), timestamp(0.0){}
};

struct fcH264Frame : public fcFrameData
{
    fcH264FrameType h264_type;
    std::vector<int> nal_sizes;

    fcH264Frame()
        : h264_type(fcH264FrameType_Invalid)
    {
        type = fcFrameType_H264;
    }

    void clear()
    {
        data.clear();
        nal_sizes.clear();
    }

    // Body: [](const char *nal_data, int nal_size) -> void
    template<class Body>
    void eachNALs(const Body& body) const
    {
        int total = 0;
        for (int size : nal_sizes) {
            body(&data[total], size);
            total += size;
        }
    }
};

struct fcAACFrame : public fcFrameData
{
    std::vector<int> encoded_block_sizes;
    std::vector<int> raw_block_sizes;

    fcAACFrame()
    {
        type = fcFrameType_AAC;
        clear();
    }

    void clear()
    {
        data.clear();
        encoded_block_sizes.clear();
        raw_block_sizes.clear();
    }

    // Body: [](const char *data, int encoded_block_size, int raw_block_size) -> void
    template<class Body>
    void eachBlocks(const Body& body) const
    {
        int total = 0;
        for (size_t i = 0; i < encoded_block_sizes.size(); ++i) {
            body(&data[total], encoded_block_sizes[i], raw_block_sizes[i]);
            total += encoded_block_sizes[i];
        }
    }
};


struct fcFrameInfo
{
    size_t size;
    uint64_t file_offset;
    fcTime timestamp;

    fcFrameInfo()
        : size(), file_offset(), timestamp()
    {}
};

struct fcOffsetValue
{
    uint32_t count;
    uint32_t value;
};

struct fcSampleToChunk
{
    uint32_t first_chunk_ID;
    uint32_t samples_per_chunk;
    uint32_t sample_description_ID;
};

typedef std::function<void(fcDownloadState, const char *message)> fcDownloadCallback;

const std::string& fcMP4GetModulePath();
const std::string& fcMP4GetFAACPackagePath();

#endif // fcMP4Internal_h

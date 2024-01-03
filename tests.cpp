#include <webgpu/webgpu.hpp>

struct Identify
{
    float x;
    float y;
    float z;
    
};

struct SurfaceDescriptor2 : public WGPUSurfaceDescriptor
{
public:
    typedef SurfaceDescriptor2 S;
    typedef WGPUSurfaceDescriptor W;
    SurfaceDescriptor2() : W() { nextInChain = nullptr; }
    SurfaceDescriptor2(const W& other) : W(other) { nextInChain = nullptr; }
    SurfaceDescriptor2(const wgpu::DefaultFlag&) : W() { }

    SurfaceDescriptor2& operator=(const wgpu::DefaultFlag&)
    {
        // setDefault();
        return *this;
    }

    friend auto operator<<(std::ostream& stream, const S&) -> std::ostream&
    {
        return stream << "<wgpu::" << "SurfaceDescriptor" << ">";
    }
};

void coolIdentify()
{
    Identify a{.x = 3, .z = 3};

    WGPUSurfaceDescriptor b{.label = "hello"};
    SurfaceDescriptor2 c{{.label = "wow"}};

    // wgpu::SurfaceDescriptor descriptor
}
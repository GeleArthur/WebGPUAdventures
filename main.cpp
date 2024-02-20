#include <iostream>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <string>
#include <GLFW/glfw3.h>
#define WEBGPU_CPP_IMPLEMENTATION
#include <array>
#include <webgpu/webgpu.hpp>
#include <glfw3webgpu.h>

#ifdef __EMSCRIPTEN__
#include <emscripten/emscripten.h>
#endif

using namespace wgpu;
namespace fs = std::filesystem;

SwapChain swapChain = nullptr;
Device device = nullptr;
Queue queue = nullptr;
std::vector<float> pointData;
std::vector<uint16_t> indexData;
int indexCount;
Buffer vertexBuffer = nullptr;
Buffer indexBuffer = nullptr;
RenderPipeline pipeline = nullptr;
BindGroup bindGroup = nullptr;
Buffer uniformBuffer = nullptr;
uint32_t uniformStride = 0;

struct MyUniforms {
    std::array<float, 4> color;  // or float color[4]
    float time;
    float _pad[3];
};
MyUniforms uniforms;

void Render();
bool LoadGeometry(const fs::path& path, std::vector<float>& pointData, std::vector<uint16_t>& indexData);
ShaderModule LoadShaderModule(const fs::path& path, Device device);
uint32_t ceilToNextMultiple(uint32_t value, uint32_t step);

int run()
{
    std::cout << "LOG FOR ME!!!" << std::endl;
    static_assert(sizeof(MyUniforms) % 16 == 0);


    #ifdef __EMSCRIPTEN__
	Instance instance = wgpuCreateInstance(nullptr);
    #else
	Instance instance = createInstance(InstanceDescriptor{});
    #endif

    if(!instance)
    {
        std::cerr << "Could not initialize WebGPU!" << std::endl;
        return 1;
    }
    
    if (!glfwInit()) {
        std::cerr << "Could not initialize GLFW!" << std::endl;
        return 1;
    }

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
    GLFWwindow* glfwWindow = glfwCreateWindow(640,480, "Learn WebGPU!!!", nullptr, nullptr);
    if(!glfwWindow)
    {
        std::cerr << "Could not open window!" << std::endl;
        return 1;
    }
    
    std::cout << "Requesting adapter..." << std::endl;
    Surface windowSurface = glfwGetWGPUSurface(instance, glfwWindow);

    Adapter adapter = instance.requestAdapter(
    {{
        .compatibleSurface = windowSurface,
        .powerPreference = PowerPreference::HighPerformance,
        .forceFallbackAdapter = false
    }});
    std::cout << "Got adapter: " << adapter << std::endl;

	SupportedLimits supportedLimits;
    #ifdef __EMSCRIPTEN__
    supportedLimits.limits.minStorageBufferOffsetAlignment = 256;
    supportedLimits.limits.minUniformBufferOffsetAlignment = 256;
    #else
	adapter.getLimits(&supportedLimits);
    #endif

	RequiredLimits requiredLimits = Default;
	requiredLimits.limits.maxVertexAttributes = 2;
	requiredLimits.limits.maxVertexBuffers = 1;
	requiredLimits.limits.maxBufferSize = 15 * 5 * sizeof(float);
	requiredLimits.limits.maxVertexBufferArrayStride = 5 * sizeof(float);
	requiredLimits.limits.minStorageBufferOffsetAlignment = supportedLimits.limits.minStorageBufferOffsetAlignment;
	requiredLimits.limits.minUniformBufferOffsetAlignment = supportedLimits.limits.minUniformBufferOffsetAlignment;
    requiredLimits.limits.maxTextureDimension2D = 8192;
    requiredLimits.limits.maxTextureDimension1D = 8192;
    requiredLimits.limits.maxTextureDimension1D = 2048;
    requiredLimits.limits.maxInterStageShaderComponents = 3;
    requiredLimits.limits.maxBindGroups = 3;
    requiredLimits.limits.maxUniformBuffersPerShaderStage = 1;
    requiredLimits.limits.maxUniformBufferBindingSize = 16*4;
    requiredLimits.limits.maxDynamicUniformBuffersPerPipelineLayout = 1;
    
    
    device = adapter.requestDevice(DeviceDescriptor
    {{
        .nextInChain = nullptr,
        .label = "My Device",
        .requiredFeatureCount = 0,
        .requiredLimits = &requiredLimits,
    }});
    std::cout << "Got device: " << device << std::endl;
    
    device.setUncapturedErrorCallback([](const ErrorType type, char const* message) {
        std::cout << "Uncaptured device error: type " << type << std::endl;
        if (message) std::cout << " (" << message << ")";
        std::cout << std::endl;
    });
    auto onDeviceLost = [](WGPUDeviceLostReason reason, char const * message, void * userdata)
    {
        std::cout << message << std::endl;
    };
    wgpuDeviceSetDeviceLostCallback(device, onDeviceLost, nullptr);
    
    queue = device.getQueue();

    std::cout << "Creating swapchain..." << std::endl;
    
    swapChain = device.createSwapChain(windowSurface ,SwapChainDescriptor
    {{
        .usage = TextureUsage::RenderAttachment,
        .format = TextureFormat::BGRA8Unorm,
        .width = 640,
        .height = 480,
        .presentMode = PresentMode::Fifo,
    }});
    std::cout << "Swapchain: " << swapChain << std::endl;

    ShaderModule shaderModule = LoadShaderModule(RESOURCE_DIR "/shader.wgsl", device);
    std::cout << "Shader module: " << shaderModule << std::endl;

    std::vector vertexAttributes{
        VertexAttribute
        {{
            .format = VertexFormat::Float32x2,
            .offset = 0,
            .shaderLocation = 0,
        }},
        VertexAttribute
        {{
            .format = VertexFormat::Float32x3,
            .offset = 2 * sizeof(float),
            .shaderLocation = 1
        }}
    };


    VertexBufferLayout vertexBufferLayout
	{{
        .arrayStride = 5 * sizeof(float),
        .stepMode = VertexStepMode::Vertex,
        .attributeCount = static_cast<uint32_t>(vertexAttributes.size()),
        .attributes = vertexAttributes.data(),
	}};

    BlendState blendState
    {{
        .color = {BlendComponent{{BlendOperation::Add,  BlendFactor::SrcAlpha, BlendFactor::OneMinusSrcAlpha}}},
        .alpha = {BlendComponent{{BlendOperation::Add, BlendFactor::One, BlendFactor::Zero}}}
    }};
    ColorTargetState colorTarget
    {{
        .format = TextureFormat::BGRA8Unorm,
        .blend = &blendState,
        .writeMask = ColorWriteMask::All
    }};

    FragmentState fragmentState
    {{
        .module = shaderModule,
        .entryPoint = "fs_main",
        .constantCount = 0,
        .constants = nullptr,
        .targetCount = 1,
        .targets = &colorTarget
    }};



    // Binding group
    BindGroupLayoutEntry bindingLayout = Default;
    bindingLayout.binding = 0;
    bindingLayout.visibility = ShaderStage::Vertex | ShaderStage::Fragment;
    bindingLayout.buffer.type = BufferBindingType::Uniform;
    bindingLayout.buffer.minBindingSize = sizeof(MyUniforms);
    bindingLayout.buffer.hasDynamicOffset = true;

    BindGroupLayoutDescriptor bindGroupLayoutDesc;
    bindGroupLayoutDesc.entryCount = 1;
    bindGroupLayoutDesc.entries = &bindingLayout;
    BindGroupLayout bindGroupLayout = device.createBindGroupLayout(bindGroupLayoutDesc);

    PipelineLayoutDescriptor layoutDesc;
    layoutDesc.bindGroupLayoutCount = 1;
    layoutDesc.bindGroupLayouts = (WGPUBindGroupLayout*)&bindGroupLayout;
    
    RenderPipelineDescriptor pipelineDesc
    {{
        .label = "PipeLine",
        .layout = device.createPipelineLayout(layoutDesc),
        .vertex = VertexState
        {{
            .module = shaderModule,
            .entryPoint = "vs_main",
            .constantCount = 0,
            .constants = nullptr,
            .bufferCount = 1,
            .buffers = &vertexBufferLayout,
        }},
        .primitive = PrimitiveState
        {{
            .topology = PrimitiveTopology::TriangleList,
            .stripIndexFormat = IndexFormat::Undefined,
            .frontFace = FrontFace::CCW,
            .cullMode = CullMode::None
        }},
        
        .depthStencil = nullptr,
        .multisample = MultisampleState{{.count = 1, .mask = ~0u, .alphaToCoverageEnabled = false}},
        .fragment = &fragmentState,
    }};
    
    pipeline = device.createRenderPipeline(pipelineDesc);

    bool success = LoadGeometry(RESOURCE_DIR "/webgpu.txt", pointData, indexData);
    if (!success) {
        std::cerr << "Could not load geometry!" << std::endl;
        return 1;
    }

    vertexBuffer = device.createBuffer(BufferDescriptor{{
        .usage = BufferUsage::CopyDst | BufferUsage::Vertex,
        .size = pointData.size() * sizeof(float),
        .mappedAtCreation = false
    }});
    
    indexCount = static_cast<int>(indexData.size());
    indexBuffer = device.createBuffer(BufferDescriptor
    {{
        .usage = BufferUsage::CopyDst | BufferUsage::Index,
        .size = ((indexData.size() * sizeof(uint16_t))+3) & ~3,
        .mappedAtCreation = false,
    }});
    
    queue.writeBuffer(vertexBuffer, 0, pointData.data(), pointData.size() * sizeof(float));
    queue.writeBuffer(indexBuffer, 0, indexData.data(), ((indexData.size() * sizeof(uint16_t))+3) & ~3 );

    // Uniform
    
    uniformStride = ceilToNextMultiple(
        (uint32_t)sizeof(MyUniforms),
        (uint32_t)supportedLimits.limits.minUniformBufferOffsetAlignment
    );
    
    uniformBuffer = device.createBuffer(BufferDescriptor
    {{
        .usage = BufferUsage::CopyDst | BufferUsage::Uniform,
        .size = uniformStride + sizeof(MyUniforms),
        .mappedAtCreation = false,
    }});

    uniforms.time = 1;
    uniforms.color = { 0.0f, 1.0f, 0.4f, 1.0f };
    queue.writeBuffer(uniformBuffer, 0, &uniforms, sizeof(MyUniforms));

    uniforms.time = -1;
    uniforms.color = { 1.0f, 1.0f, 1.0f, 0.7f };
    queue.writeBuffer(uniformBuffer, uniformStride, &uniforms, sizeof(MyUniforms));

    BindGroupEntry binding;
    binding.binding = 0;
    binding.buffer = uniformBuffer;
    binding.offset = 0;
    binding.size = sizeof(MyUniforms);

    bindGroup = device.createBindGroup(BindGroupDescriptor 
    {{
        .layout = bindGroupLayout,
        .entryCount = 1,
        .entries = &binding,
    }});

#ifdef __EMSCRIPTEN__
    emscripten_set_main_loop(Render, 0, false);
#else

    while (!glfwWindowShouldClose(glfwWindow))
    {
        Render();
        swapChain.present();
#ifdef WEBGPU_BACKEND_DAWN
        // Check for pending error callbacks
        device.tick();
#endif
    }

    swapChain.release();
    device.release();
    adapter.release();
    instance.release();
    windowSurface.release();
    queue.release();

	vertexBuffer.destroy();
	vertexBuffer.release();
    
    glfwDestroyWindow(glfwWindow);
    glfwTerminate();
#endif

    return 0;
}

void Render()
{
    glfwPollEvents();
    TextureView nextTexture = swapChain.getCurrentTextureView();
    // std::cout << "nextTexture: " << nextTexture << std::endl;

    uniforms.time = static_cast<float>(glfwGetTime());
    queue.writeBuffer(uniformBuffer, offsetof(MyUniforms, time), &uniforms.time, sizeof(MyUniforms::time));

    uniforms.time = -static_cast<float>(glfwGetTime());
    queue.writeBuffer(uniformBuffer, uniformStride + offsetof(MyUniforms, time), &uniforms.time, sizeof(MyUniforms::time));
    
    CommandEncoder encoder = device.createCommandEncoder({{.label = "Command Encoder"}});
    
    RenderPassColorAttachment attachment
    {{
        .view = nextTexture,
        .resolveTarget = nullptr,
        .loadOp = LoadOp::Clear,
        .storeOp = StoreOp::Store,
        .clearValue = Color{ 0.05, 0.05, 0.05, 1.0 },
    }};
    RenderPassEncoder renderPass = encoder.beginRenderPass(RenderPassDescriptor
    {{
        .colorAttachmentCount = 1,
        .colorAttachments = &attachment,
        .depthStencilAttachment = nullptr,
    }});
    renderPass.setPipeline(pipeline);
    renderPass.setVertexBuffer(0, vertexBuffer, 0, pointData.size() * sizeof(float));
    renderPass.setIndexBuffer(indexBuffer, IndexFormat::Uint16, 0, indexData.size() * sizeof(uint16_t));

    
    uint32_t dynamicOffset = 0;
    dynamicOffset = 0 * uniformStride;
    renderPass.setBindGroup(0, bindGroup, 1, &dynamicOffset);
    renderPass.drawIndexed(indexCount, 1, 0, 0, 0);

    dynamicOffset = 1 * uniformStride;
    renderPass.setBindGroup(0, bindGroup, 1, &dynamicOffset);
    renderPass.drawIndexed(indexCount, 1, 0, 0, 0);
    
    renderPass.end();
    
    CommandBuffer command = encoder.finish(CommandBufferDescriptor{});
    queue.submit(1, &command);        
    
    nextTexture.release();
    renderPass.release();
    encoder.release();
    command.release();
}

int main(int, char**)
{
	int result = run();

    return result;
}




// Util functions
bool LoadGeometry(const fs::path& path, std::vector<float>& pointData, std::vector<uint16_t>& indexData)
{
    std::ifstream file(path);
    if(!file.is_open())
    {
        return false;
    }

    pointData.clear();
    indexData.clear();

    enum class Section
    {
        None,
        Points,
        Indices,
    };
    Section currentSection = Section::None;

    float value;
    uint16_t index;
    std::string line;
    while (!file.eof())
    {
        std::getline(file, line);

        if (!line.empty() && line.back() == '\r') {
            line.pop_back();
        }

        if (line == "[points]") {
            currentSection = Section::Points;
        }
        else if (line == "[indices]") {
            currentSection = Section::Indices;
        }
        else if (line[0] == '#' || line.empty()) {
            // Do nothing, this is a comment
        }
        else if (currentSection == Section::Points) {
            std::istringstream iss(line);
            // Get x, y, r, g, b
            for (int i = 0; i < 5; ++i) {
                iss >> value;
                pointData.push_back(value);
            }
        }
        else if (currentSection == Section::Indices) {
            std::istringstream iss(line);
            // Get corners #0 #1 and #2
            for (int i = 0; i < 3; ++i) {
                iss >> index;
                indexData.push_back(index);
            }
        }
    }
    
    return true;
}

ShaderModule LoadShaderModule(const fs::path& path, Device device) 
{
    std::ifstream file(path);
    if (!file.is_open()) {
        return nullptr;
    }
    file.seekg(0, std::ios::end);
    const size_t size = file.tellg();
    std::string shaderSource(size, ' ');
    file.seekg(0);
    file.read(shaderSource.data(), size);

    ShaderModuleWGSLDescriptor shaderCodeDesc;
    shaderCodeDesc.chain.next = nullptr;
    shaderCodeDesc.chain.sType = SType::ShaderModuleWGSLDescriptor;
    shaderCodeDesc.code = shaderSource.c_str();
    ShaderModuleDescriptor shaderDesc{};
    #ifdef WEBGPU_BACKEND_WGPU
    shaderDesc.hintCount = 0;
    shaderDesc.hints = nullptr;
    #endif
    shaderDesc.nextInChain = &shaderCodeDesc.chain;
    return device.createShaderModule(shaderDesc);
}

uint32_t ceilToNextMultiple(uint32_t value, uint32_t step)
{
    uint32_t divide_and_ceil = value / step + (value % step == 0 ? 0 : 1);
    return step * divide_and_ceil;
}
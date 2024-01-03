#include <iostream>
#include <GLFW/glfw3.h>
#define WEBGPU_CPP_IMPLEMENTATION
#include <webgpu/webgpu.hpp>
#include <glfw3webgpu.h>
#include "webgpu-utils.h"

using namespace wgpu;
int Run();


int main(int, char**)
{
	int result = Run();

    return result;
}

int Run()
{
	Instance instance{createInstance(InstanceDescriptor{})};
    if(!instance)
    {
        std::cerr << "Could not initialize WebGPU!" << '\n';
        return 1;
    }
    
    if (!glfwInit()) {
        std::cerr << "Could not initialize GLFW!" << '\n';
        return 1;
    }

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
    GLFWwindow* glfwWindow = glfwCreateWindow(640,640, "Learn WebGPU!!!", nullptr, nullptr);
    if(!glfwWindow)
    {
        std::cerr << "Could not open window!" << '\n';
        return 1;
    }
    
    std::cout << "Requesting adapter..." << '\n';
    Surface windowSurface = glfwGetWGPUSurface(instance, glfwWindow);
    RequestAdapterOptions adapterOpts{};
    adapterOpts.compatibleSurface = windowSurface;
    adapterOpts.powerPreference = PowerPreference::HighPerformance;
    adapterOpts.forceFallbackAdapter = false;

    Adapter adapter = instance.requestAdapter(adapterOpts);
    std::cout << "Got adapter: " << adapter << '\n';

    std::cout << "Requesting device..." << '\n';
    DeviceDescriptor deviceDesc{};
    deviceDesc.label = "My Device";
    deviceDesc.requiredFeatureCount = 0;
    deviceDesc.requiredLimits = nullptr;
    // deviceDesc.defaultQueue.label = "The default queue";
    Device device = adapter.requestDevice(deviceDesc);
    std::cout << "Got device: " << device << '\n';

    SurfaceCapabilities capabilities;
    windowSurface.getCapabilities(adapter, &capabilities);
    // for (size_t i{}; i < capabilities.formatCount; ++i)
    // {
    //     std::cout << capabilities.formats[i];
    // }
    //
    SurfaceConfiguration surfaceConfig{};
    surfaceConfig.usage = wgpu::TextureUsage::RenderAttachment;
    surfaceConfig.format = TextureFormat::BGRA8UnormSrgb; // Hard coded
    surfaceConfig.width = 640;
    surfaceConfig.height = 640;
    surfaceConfig.presentMode = capabilities.presentModes[0];
    surfaceConfig.alphaMode = capabilities.alphaModes[0];
    surfaceConfig.viewFormatCount = 0;
    surfaceConfig.viewFormats = nullptr;
    surfaceConfig.device = device;
    
    windowSurface.configure(surfaceConfig);

    

    Queue queue = device.getQueue();
    auto onDeviceError = [](const ErrorType type, char const* message) {
        std::cout << "Uncaptured device error: type " << type;
        if (message) std::cout << " (" << message << ")";
        std::cout << '\n';
    };
    device.setUncapturedErrorCallback(onDeviceError);

    // SwapChainDescriptor swapChainDesc{};
    // swapChainDesc.width = 640;
    // swapChainDesc.height = 640;
    // swapChainDesc.format = surface.getPreferredFormat(adapter);
    // swapChainDesc.usage = WGPUTextureUsage_RenderAttachment;
    // swapChainDesc.presentMode = WGPUPresentMode_Fifo;
    //
    // SwapChain swapChain = device.createSwapChain(surface, swapChainDesc);
    // std::cout << "SwapChain: " << swapChain << '\n';


    RenderPipelineDescriptor pipelineDesc;
    pipelineDesc.vertex.buffers = nullptr;
    pipelineDesc.vertex.bufferCount = 0;
    // pipelineDesc.vertex.module =
    pipelineDesc.vertex.entryPoint = "vs_main";
    pipelineDesc.vertex.constantCount = 0;
    pipelineDesc.vertex.constants = nullptr;

    pipelineDesc.primitive.topology = PrimitiveTopology::TriangleStrip;
    pipelineDesc.primitive.stripIndexFormat = IndexFormat::Undefined;
    pipelineDesc.primitive.frontFace = FrontFace::CCW;
    pipelineDesc.primitive.cullMode = CullMode::None;


    FragmentState fragmentState;
    // fragmentState.module = shaderModule;
    fragmentState.entryPoint = "fs_main";
    fragmentState.constantCount = 0;
    fragmentState.constants = nullptr;
    BlendState blendState;
    blendState.color.srcFactor = BlendFactor::SrcAlpha;
    blendState.color.srcFactor = BlendFactor::OneMinusSrcAlpha;
    blendState.color.operation = BlendOperation::Add;

    blendState.alpha.srcFactor = BlendFactor::Zero;
    blendState.alpha.dstFactor = BlendFactor::One;
    blendState.alpha.operation = BlendOperation::Add;

    pipelineDesc.multisample.count = 1;
    pipelineDesc.multisample.mask = ~0u;
    pipelineDesc.multisample.alphaToCoverageEnabled = false;

    // ColorTargetState colorTarget;
    // colorTarget.format = swapchainFo

    
    fragmentState.targetCount = 1;
    // fragmentState.targets = &colorTarget;
    
    // pipelineDesc.fragment = &fragmentState;

    

    pipelineDesc.depthStencil = nullptr;


    
    // RenderPipeline pipeline = device.createRenderPipeline(pipelineDesc);
    
    
    
    while (!glfwWindowShouldClose(glfwWindow))
    {
        glfwPollEvents();
        SurfaceTexture surfaceTexture;
        windowSurface.getCurrentTexture(&surfaceTexture);
        Texture surfaceTexture2 = surfaceTexture.texture;
        
        TextureView nextTexture = surfaceTexture2.createView();
        
        // std::cout << "nextTexture: " << nextTexture << '\n';

        CommandEncoderDescriptor encoderDesc{};
        encoderDesc.label = "Command Encoder";
        CommandEncoder encoder = device.createCommandEncoder(encoderDesc);

        RenderPassDescriptor renderPassDesc;
        RenderPassColorAttachment renderPassColorAttachment;
        
        renderPassColorAttachment.view = nextTexture;
        renderPassColorAttachment.resolveTarget = nullptr;
        
        renderPassColorAttachment.loadOp = WGPULoadOp_Clear;
        renderPassColorAttachment.storeOp = WGPUStoreOp_Store;
        renderPassColorAttachment.clearValue = Color{ 0.9, 0.1, 0.2, 1.0 };
        renderPassDesc.colorAttachmentCount = 1;
        renderPassDesc.colorAttachments = &renderPassColorAttachment;
        
        renderPassDesc.depthStencilAttachment = nullptr;
        
        RenderPassEncoder renderPass = encoder.beginRenderPass(renderPassDesc);
        renderPass.end();
        
        CommandBuffer command = encoder.finish(CommandBufferDescriptor{});
        queue.submit(1, &command);

        windowSurface.present();
        
        nextTexture.release();
        renderPass.release();
        encoder.release();
        command.release();
        surfaceTexture2.release();
    }

    windowSurface.unconfigure();
    device.release();
    adapter.release();
    instance.release();
    windowSurface.release();
    queue.release();
    
    glfwDestroyWindow(glfwWindow);
    glfwTerminate();
    return 0;
}

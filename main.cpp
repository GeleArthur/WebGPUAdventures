#include <iostream>
#include <GLFW/glfw3.h>
#define WEBGPU_CPP_IMPLEMENTATION
#include <webgpu/webgpu.hpp>
#include <glfw3webgpu.h>

using namespace wgpu;
int run();


int main(int, char**)
{
	int result = run();

    return result;
}

int run()
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

    Adapter adapter = instance.requestAdapter(
    {{
        .compatibleSurface = windowSurface,
        .powerPreference = PowerPreference::HighPerformance,
        .forceFallbackAdapter = false
    }});
    std::cout << "Got adapter: " << adapter << '\n';

    std::cout << "Requesting device..." << '\n';
    Device device = adapter.requestDevice(DeviceDescriptor
    {{
        .label = "My Device",
        .requiredFeatureCount = 0,
        .requiredLimits = nullptr
    }});
    
    auto onDeviceError = [](const ErrorType type, char const* message) {
        std::cout << "Uncaptured device error: type " << type;
        if (message) std::cout << " (" << message << ")";
        std::cout << '\n';
    };
    device.setUncapturedErrorCallback(onDeviceError);

    SurfaceCapabilities capabilities;
    windowSurface.getCapabilities(adapter, &capabilities);
    
    windowSurface.configure(SurfaceConfiguration
    {{
        .device = device,
        .format = TextureFormat::BGRA8UnormSrgb,
        .usage = TextureUsage::RenderAttachment,
        .viewFormatCount = 0,
        .viewFormats = nullptr,
        .alphaMode = capabilities.alphaModes[0],
        .width = 640,
        .height = 640,
        .presentMode = capabilities.presentModes[0],
    }});

    Queue queue = device.getQueue();

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
        SurfaceTexture surfaceTextureRequest;
        windowSurface.getCurrentTexture(&surfaceTextureRequest);
        Texture drawTexture = surfaceTextureRequest.texture;
        
        TextureView viewSurfaceTexture = drawTexture.createView();
        std::cout << "nextTexture: " << viewSurfaceTexture << '\n';
        
        CommandEncoder encoder = device.createCommandEncoder({{.label = "Command Encoder"}});
        
        RenderPassColorAttachment attachment
        {{
            .view = viewSurfaceTexture,
            .resolveTarget = nullptr,
            .loadOp = LoadOp::Clear,
            .storeOp = StoreOp::Store,
            .clearValue = Color{ 0.9, 0.1, 0.2, 1.0 },
        }};
        
        RenderPassEncoder renderPass = encoder.beginRenderPass(RenderPassDescriptor
        {{
            .colorAttachmentCount = 1,
            .colorAttachments = &attachment,
            .depthStencilAttachment = nullptr,
        }});
        renderPass.end();
        
        CommandBuffer command = encoder.finish(CommandBufferDescriptor{});
        queue.submit(1, &command);

        windowSurface.present();
        
        viewSurfaceTexture.release();
        renderPass.release();
        encoder.release();
        command.release();
        drawTexture.release();
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

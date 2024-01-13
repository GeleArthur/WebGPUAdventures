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
    
    Device device = adapter.requestDevice(
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

    const char* shaderSource = R"(
    struct VertexOutput {
        @builtin(position) clip_position: vec4<f32>,
    };

    @vertex
    fn vs_main(
        @builtin(vertex_index) in_vertex_index: u32,
    ) -> VertexOutput {
        var out: VertexOutput;
        let x = f32(1 - i32(in_vertex_index)) * 0.5;
        let y = f32(i32(in_vertex_index & 1u) * 2 - 1) * 0.5;
        out.clip_position = vec4<f32>(x, y, 0.0, 1.0);
        return out;
    }

    @fragment
    fn fs_main(in: VertexOutput) -> @location(0) vec4<f32> {
        return vec4<f32>(0.1, 0.5, 1.0, 1.0);
    }
    )";

    WGPUShaderModuleWGSLDescriptor shaderCodeDesc {
        .chain = {
            .sType = WGPUSType_ShaderModuleWGSLDescriptor
        },
        .code = shaderSource
    };
    ShaderModuleDescriptor desc{};
    desc.nextInChain = &shaderCodeDesc.chain; // I can't have it in the consturctor for some reseaon
    ShaderModule shaderModule = device.createShaderModule(desc);
    
    BlendState blendState
    {{
        .color = {BlendComponent{{BlendOperation::Add,  BlendFactor::SrcAlpha, BlendFactor::OneMinusSrcAlpha}}},
        .alpha = {BlendComponent{{BlendOperation::Add, BlendFactor::One, BlendFactor::Zero}}}
    }};
    ColorTargetState colorTarget
    {{
        .format = windowSurface.getPreferredFormat(adapter),
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
    
    RenderPipelineDescriptor pipelineDesc
    {{
        .label = "PipeLine",
        .layout = device.createPipelineLayout(PipelineLayoutDescriptor{}),
        .vertex = VertexState
        {{
            .module = shaderModule,
            .entryPoint = "vs_main",
            .constantCount = 0,
            .constants = nullptr,
            .bufferCount = 0,
            .buffers = nullptr,
        }},
        .primitive = PrimitiveState
        {{
            .topology = PrimitiveTopology::TriangleStrip,
            .stripIndexFormat = IndexFormat::Undefined,
            .frontFace = FrontFace::CCW,
            .cullMode = CullMode::None
        }},
        
        .depthStencil = nullptr,
        .multisample = MultisampleState{{.count = 1, .mask = ~0u, .alphaToCoverageEnabled = false}},
        .fragment = &fragmentState,
    }};
    
    RenderPipeline pipeline = device.createRenderPipeline(pipelineDesc);
    
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
        renderPass.setPipeline(pipeline);
        renderPass.draw(3, 1, 0, 0);
        renderPass.end();
        
        CommandBuffer command = encoder.finish(CommandBufferDescriptor{});
        queue.submit(1, &command);

        windowSurface.present();
        
        
        viewSurfaceTexture.release();
        renderPass.release();
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



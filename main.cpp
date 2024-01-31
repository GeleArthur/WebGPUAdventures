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

	SupportedLimits supportedLimits;
	adapter.getLimits(&supportedLimits);

	RequiredLimits requiredLimits = Default;
	// We use at most 1 vertex attribute for now
	requiredLimits.limits.maxVertexAttributes = 2;
	// We should also tell that we use 1 vertex buffers
	requiredLimits.limits.maxVertexBuffers = 1;
	// Maximum size of a buffer is 6 vertices of 2 float each
	requiredLimits.limits.maxBufferSize = 6 * 5 * sizeof(float);
	// Maximum stride between 2 consecutive vertices in the vertex buffer
	requiredLimits.limits.maxVertexBufferArrayStride = 5 * sizeof(float);
	// This must be set even if we do not use storage buffers for now
	requiredLimits.limits.minStorageBufferOffsetAlignment = supportedLimits.limits.minStorageBufferOffsetAlignment;
	// This must be set even if we do not use uniform buffers for now
	requiredLimits.limits.minUniformBufferOffsetAlignment = supportedLimits.limits.minUniformBufferOffsetAlignment;
    requiredLimits.limits.maxTextureDimension2D = 8192;
    requiredLimits.limits.maxTextureDimension1D = 8192;
    requiredLimits.limits.maxTextureDimension1D = 2048;
    requiredLimits.limits.maxInterStageShaderComponents = 3;
    
    Device device = adapter.requestDevice(
    {{
        .label = "My Device",
        .requiredFeatureCount = 0,
        .requiredLimits = &requiredLimits
    }});
    
    auto onDeviceError = [](const ErrorType type, char const* message) {
        std::cout << "Uncaptured device error: type " << type;
        if (message) std::cout << " (" << message << ")";
        std::cout << '\n';
    };
    device.setUncapturedErrorCallback(onDeviceError);

    std::vector<float> vertexData = {
        // x0,  y0,  r0,  g0,  b0
        -0.5, -0.5, 1.0, 0.0, 0.0,

        // x1,  y1,  r1,  g1,  b1
        +0.5, -0.5, 0.0, 1.0, 0.0,

        +0.0,   +0.5, 0.0, 0.0, 1.0,
        -0.55f, -0.5, 1.0, 1.0, 0.0,
        -0.05f, +0.5, 1.0, 0.0, 1.0,
        -0.55f, +0.5, 0.0, 1.0, 1.0
    };
	int vertexCount = static_cast<int>(vertexData.size() / 5);

    Buffer vertexBuffer = device.createBuffer(BufferDescriptor{{
        .usage = BufferUsage::CopyDst | BufferUsage::Vertex,
        .size = vertexData.size() * sizeof(float),
        .mappedAtCreation = false
    }});



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
    @vertex
    fn vs_main(
        @builtin(vertex_index) in_vertex_index: u32,
		@location(0) vertex_position: vec2f
    ) -> @builtin(position) vec4f
    {
        return vec4f(vertex_position, 0.0, 1.0);
    }

    @fragment
    fn fs_main(@builtin(position) clipedpos: vec4f) -> @location(0) vec4<f32> {
        return vec4<f32>(clipedpos.x/640.0, 0, 0, 1.0);
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

    std::vector<VertexAttribute> vertexAttributes{
        VertexAttribute
        {{
            .format = VertexFormat::Float32x2,
            .offset = 0,
            .shaderLocation = 0,
        }},
        VertexAttribute
        {{
            .format = VertexFormat::Float32x3,
            .offset = 0,
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
            .bufferCount = 1,
            .buffers = &vertexBufferLayout,
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

    queue.writeBuffer(vertexBuffer, 0, vertexData.data(), vertexData.size() * sizeof(float));

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
        renderPass.setVertexBuffer(0, vertexBuffer, 0, vertexData.size() * sizeof(float));
        renderPass.draw(vertexCount, 1, 0, 0);
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

	vertexBuffer.destroy();
	vertexBuffer.release();
    
    glfwDestroyWindow(glfwWindow);
    glfwTerminate();
    return 0;
}



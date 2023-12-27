#include <iostream>
#include <GLFW/glfw3.h>
#include <webgpu/webgpu.h>
#include <glfw3webgpu.h>
#include "webgpu-utils.h"

int main(int, char**){
    if (!glfwInit()) {
        std::cerr << "Could not initialize GLFW!" << '\n';
        return 1;
    }

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
    GLFWwindow* window = glfwCreateWindow(640,640, "Learn WebGPU!!!", nullptr, nullptr);
    if(!window)
    {
        std::cerr << "Could not open window!" << '\n';
        glfwTerminate();
        return 1;
    }

    WGPUInstanceDescriptor desc{};
    desc.nextInChain = nullptr;

    WGPUInstance instance{wgpuCreateInstance(&desc)};
    wgpuInstanceReference(instance);
    if(!instance)
    {
        std::cerr << "Could not initialize WebGPU!" << '\n';
        return 1;
    }

    std::cout << "WGPU instance: " << instance << '\n';

    std::cout << "Requesting adapter..." << '\n';

    const WGPUSurface surface = glfwGetWGPUSurface(instance, window);

    WGPURequestAdapterOptions adapterOpts{nullptr, surface, WGPUPowerPreference_HighPerformance, false};
    WGPUAdapter adapter = requestAdapter(instance, &adapterOpts);
    std::cout << "Got adapter: " << adapter << '\n';
	inspectAdapter(adapter);
    
    WGPUDeviceDescriptor deviceDesc{};
    deviceDesc.nextInChain = nullptr;
    deviceDesc.label = "My Device";
    deviceDesc.requiredFeaturesCount = 0;
    deviceDesc.requiredLimits = nullptr; 
    deviceDesc.defaultQueue.nextInChain = nullptr;
    deviceDesc.defaultQueue.label = "The default queue";
    WGPUDevice device = requestDevice(adapter, &deviceDesc);
    std::cout << "Got device: " << device << '\n';

    auto onDeviceError = [](WGPUErrorType type, char const* message, void* /* pUserData */) {
        std::cout << "Uncaptured device error: type " << type;
        if (message) std::cout << " (" << message << ")";
        std::cout << '\n';
    };
    wgpuDeviceSetUncapturedErrorCallback(device, onDeviceError, nullptr /* pUserData */);


    WGPUQueue queue = wgpuDeviceGetQueue(device);
    auto onQueueWorkDone = [](WGPUQueueWorkDoneStatus status, void* /* pUserData */) {
        std::cout << "Queued work finished with status: " << status << '\n';
    };
    wgpuQueueOnSubmittedWorkDone(queue, onQueueWorkDone, nullptr /* pUserData */);

    WGPUSwapChainDescriptor swapChainDesc{};
    swapChainDesc.nextInChain = nullptr;
    swapChainDesc.width = 640;
    swapChainDesc.height = 480;
    swapChainDesc.format = wgpuSurfaceGetPreferredFormat(surface, adapter);
    swapChainDesc.usage = WGPUTextureUsage_RenderAttachment;
    swapChainDesc.presentMode = WGPUPresentMode_Fifo;

    WGPUSwapChain swapChain = wgpuDeviceCreateSwapChain(device, surface, &swapChainDesc);
    std::cout << "SwapChain: " << swapChain << '\n';
    
    while (!glfwWindowShouldClose(window))
    {
        glfwPollEvents();

        WGPUTextureView nextTexture = wgpuSwapChainGetCurrentTextureView(swapChain);
        if (!nextTexture) {
            std::cerr << "Cannot acquire next swap chain texture" << '\n';
            break;
        }
        std::cout << "nextTexture: " << nextTexture << '\n';

        WGPUCommandEncoderDescriptor encoderDesc{};
        encoderDesc.nextInChain = nullptr;
        encoderDesc.label = "Command Encoder";
        WGPUCommandEncoder encoder = wgpuDeviceCreateCommandEncoder(device, &encoderDesc);

        WGPURenderPassDescriptor renderPassDesc{};
        WGPURenderPassColorAttachment renderPassColorAttachment{};

        renderPassColorAttachment.view = nextTexture;
        renderPassColorAttachment.resolveTarget = nullptr;

        renderPassColorAttachment.loadOp = WGPULoadOp_Clear;
        renderPassColorAttachment.storeOp = WGPUStoreOp_Store;
        renderPassColorAttachment.clearValue = WGPUColor{ 0.9, 0.1, 0.2, 1.0 };
        renderPassDesc.colorAttachmentCount = 1;
        renderPassDesc.colorAttachments = &renderPassColorAttachment;
        
        renderPassDesc.depthStencilAttachment = nullptr;
        renderPassDesc.timestampWriteCount = 0;
        renderPassDesc.timestampWrites = nullptr;
        renderPassDesc.nextInChain = nullptr;
        WGPURenderPassEncoder renderPass = wgpuCommandEncoderBeginRenderPass(encoder, &renderPassDesc);
        wgpuRenderPassEncoderEnd(renderPass);
        
        wgpuTextureViewRelease(nextTexture);
        
        WGPUCommandBufferDescriptor cmdBufferDescriptor{};
        cmdBufferDescriptor.nextInChain = nullptr;
        cmdBufferDescriptor.label = "Command buffer";
        WGPUCommandBuffer command = wgpuCommandEncoderFinish(encoder, &cmdBufferDescriptor);
        wgpuQueueSubmit(queue, 1, &command);

        
        wgpuSwapChainPresent(swapChain);
    }

    wgpuSwapChainRelease(swapChain);
    wgpuInstanceRelease(instance);
    wgpuSurfaceRelease(surface);
    wgpuAdapterRelease(adapter);
    wgpuDeviceRelease(device);
    
    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}
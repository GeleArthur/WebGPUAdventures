#include <iostream>
#include <GLFW/glfw3.h>
#define WEBGPU_CPP_IMPLEMENTATION
#include <webgpu/webgpu.hpp>
#include <glfw3webgpu.h>
#include "webgpu-utils.h"

using namespace wgpu;

int main(int, char**){
    InstanceDescriptor desc;
    desc.nextInChain = nullptr;
    Instance instance{createInstance(desc)};
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
    GLFWwindow* window = glfwCreateWindow(640,640, "Learn WebGPU!!!", nullptr, nullptr);
    if(!window)
    {
        std::cerr << "Could not open window!" << '\n';
        return 1;
    }
    
    std::cout << "Requesting adapter..." << '\n';
    Surface surface = glfwGetWGPUSurface(instance, window);
    RequestAdapterOptions adapterOpts{};
    adapterOpts.nextInChain = nullptr;
    adapterOpts.compatibleSurface = surface;
    // adapterOpts.powerPreference = wgpu::PowerPreference_HighPerformance;

    Adapter adapter = instance.requestAdapter(adapterOpts);
    std::cout << "Got adapter: " << adapter << '\n';

    std::cout << "Requesting device..." << '\n';
    DeviceDescriptor deviceDesc{};
    deviceDesc.nextInChain = nullptr;
    deviceDesc.label = "My Device";
    deviceDesc.requiredFeaturesCount = 0;
    deviceDesc.requiredLimits = nullptr; 
    deviceDesc.defaultQueue.nextInChain = nullptr;
    deviceDesc.defaultQueue.label = "The default queue";
    Device device = adapter.requestDevice(deviceDesc);
    std::cout << "Got device: " << device << '\n';

    Queue queue = device.getQueue();

    auto onDeviceError = [](const ErrorType type, char const* message) {
        std::cout << "Uncaptured device error: type " << type;
        if (message) std::cout << " (" << message << ")";
        std::cout << '\n';
    };
    device.setUncapturedErrorCallback(onDeviceError);

    SwapChainDescriptor swapChainDesc{};
    swapChainDesc.nextInChain = nullptr;
    swapChainDesc.width = 640;
    swapChainDesc.height = 640;
    swapChainDesc.format = surface.getPreferredFormat(adapter);
    swapChainDesc.usage = WGPUTextureUsage_RenderAttachment;
    swapChainDesc.presentMode = WGPUPresentMode_Fifo;

    SwapChain swapChain = device.createSwapChain(surface, swapChainDesc);
    std::cout << "SwapChain: " << swapChain << '\n';
    
    while (!glfwWindowShouldClose(window))
    {
        glfwPollEvents();

        TextureView nextTexture = swapChain.getCurrentTextureView();
        if (!nextTexture) {
            std::cerr << "Cannot acquire next swap chain texture" << '\n';
            break;
        }
        // std::cout << "nextTexture: " << nextTexture << '\n';

        CommandEncoderDescriptor encoderDesc{};
        encoderDesc.nextInChain = nullptr;
        encoderDesc.label = "Command Encoder";
        CommandEncoder encoder = device.createCommandEncoder(encoderDesc);

        RenderPassDescriptor renderPassDesc{};
        RenderPassColorAttachment renderPassColorAttachment{};

        renderPassColorAttachment.view = nextTexture;
        renderPassColorAttachment.resolveTarget = nullptr;

        renderPassColorAttachment.loadOp = WGPULoadOp_Clear;
        renderPassColorAttachment.storeOp = WGPUStoreOp_Store;
        renderPassColorAttachment.clearValue = Color{ 0.9, 0.1, 0.2, 1.0 };
        renderPassDesc.colorAttachmentCount = 1;
        renderPassDesc.colorAttachments = &renderPassColorAttachment;
        
        renderPassDesc.depthStencilAttachment = nullptr;
        renderPassDesc.timestampWriteCount = 0;
        renderPassDesc.timestampWrites = nullptr;
        renderPassDesc.nextInChain = nullptr;

        RenderPassEncoder renderPass = encoder.beginRenderPass(renderPassDesc);
        renderPass.end();

        // wgpu::TextureView::release
        nextTexture.release();

        CommandBufferDescriptor cmdBufferDescriptor{};
        cmdBufferDescriptor.nextInChain = nullptr;
        cmdBufferDescriptor.label = "Command buffer";
        CommandBuffer command = wgpuCommandEncoderFinish(encoder, &cmdBufferDescriptor);
        queue.submit(1, &command);

        
        swapChain.present();
    }

    swapChain.release();
    device.release();
    adapter.release();
    instance.release();
    surface.release();
    
    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}
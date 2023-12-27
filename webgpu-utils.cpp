#include "webgpu-utils.h"
#include <cassert>
#include <iostream>
#include <vector>

WGPUAdapter requestAdapter(WGPUInstance instance, const WGPURequestAdapterOptions* options)
{
    struct UserData
    {
        WGPUAdapter adapter{};
        bool requestEnded{false};
    };

    UserData userData;

    auto onAdapterRequestEnded = [](WGPURequestAdapterStatus status, WGPUAdapter adapter, const char* message, void* pUserData) {
        UserData& userData = *reinterpret_cast<UserData*>(pUserData);
        if (status == WGPURequestAdapterStatus_Success) {
            userData.adapter = adapter;
        } else {
            std::cout << "Could not get WebGPU adapter: " << message << '\n';
        }
        userData.requestEnded = true;
    };

    wgpuInstanceRequestAdapter(instance, options, onAdapterRequestEnded, &userData);

    assert(userData.requestEnded);

    return userData.adapter;
}

WGPUDevice requestDevice(WGPUAdapter adapter, WGPUDeviceDescriptor const * descriptor)
{
	struct UserData {
		WGPUDevice device = nullptr;
		bool requestEnded = false;
	};
	UserData userData;

	auto onDeviceRequestEnded = [](WGPURequestDeviceStatus status, WGPUDevice device, char const * message, void * pUserData) {
		UserData& userData = *reinterpret_cast<UserData*>(pUserData);
		if (status == WGPURequestDeviceStatus_Success) {
			userData.device = device;
		} else {
			std::cout << "Could not get WebGPU device: " << message << std::endl;
		}
		userData.requestEnded = true;
	};

	wgpuAdapterRequestDevice(
		adapter,
		descriptor,
		onDeviceRequestEnded,
		&userData
	);

	assert(userData.requestEnded);

	return userData.device;
}

void inspectAdapter(WGPUAdapter adapter) {
	std::vector<WGPUFeatureName> features;
	size_t featureCount = wgpuAdapterEnumerateFeatures(adapter, nullptr);
	features.resize(featureCount);
	wgpuAdapterEnumerateFeatures(adapter, features.data());

	std::cout << "Adapter features:" << '\n';
	for (auto f : features) {
		std::cout << " - " << f << '\n';
	}

	WGPUSupportedLimits limits = {};
	limits.nextInChain = nullptr;
	bool success = wgpuAdapterGetLimits(adapter, &limits);
	if (success) {
		std::cout << "Adapter limits:" << '\n';
		std::cout << " - maxTextureDimension1D: " << limits.limits.maxTextureDimension1D << '\n';
		std::cout << " - maxTextureDimension2D: " << limits.limits.maxTextureDimension2D << '\n';
		std::cout << " - maxTextureDimension3D: " << limits.limits.maxTextureDimension3D << '\n';
		std::cout << " - maxTextureArrayLayers: " << limits.limits.maxTextureArrayLayers << '\n';
		std::cout << " - maxBindGroups: " << limits.limits.maxBindGroups << '\n';
		std::cout << " - maxDynamicUniformBuffersPerPipelineLayout: " << limits.limits.maxDynamicUniformBuffersPerPipelineLayout <<
			'\n';
		std::cout << " - maxDynamicStorageBuffersPerPipelineLayout: " << limits.limits.maxDynamicStorageBuffersPerPipelineLayout <<
			'\n';
		std::cout << " - maxSampledTexturesPerShaderStage: " << limits.limits.maxSampledTexturesPerShaderStage << '\n';
		std::cout << " - maxSamplersPerShaderStage: " << limits.limits.maxSamplersPerShaderStage << '\n';
		std::cout << " - maxStorageBuffersPerShaderStage: " << limits.limits.maxStorageBuffersPerShaderStage << '\n';
		std::cout << " - maxStorageTexturesPerShaderStage: " << limits.limits.maxStorageTexturesPerShaderStage << '\n';
		std::cout << " - maxUniformBuffersPerShaderStage: " << limits.limits.maxUniformBuffersPerShaderStage << '\n';
		std::cout << " - maxUniformBufferBindingSize: " << limits.limits.maxUniformBufferBindingSize << '\n';
		std::cout << " - maxStorageBufferBindingSize: " << limits.limits.maxStorageBufferBindingSize << '\n';
		std::cout << " - minUniformBufferOffsetAlignment: " << limits.limits.minUniformBufferOffsetAlignment << '\n';
		std::cout << " - minStorageBufferOffsetAlignment: " << limits.limits.minStorageBufferOffsetAlignment << '\n';
		std::cout << " - maxVertexBuffers: " << limits.limits.maxVertexBuffers << '\n';
		std::cout << " - maxVertexAttributes: " << limits.limits.maxVertexAttributes << '\n';
		std::cout << " - maxVertexBufferArrayStride: " << limits.limits.maxVertexBufferArrayStride << '\n';
		std::cout << " - maxInterStageShaderComponents: " << limits.limits.maxInterStageShaderComponents << '\n';
		std::cout << " - maxComputeWorkgroupStorageSize: " << limits.limits.maxComputeWorkgroupStorageSize << '\n';
		std::cout << " - maxComputeInvocationsPerWorkgroup: " << limits.limits.maxComputeInvocationsPerWorkgroup <<
			'\n';
		std::cout << " - maxComputeWorkgroupSizeX: " << limits.limits.maxComputeWorkgroupSizeX << '\n';
		std::cout << " - maxComputeWorkgroupSizeY: " << limits.limits.maxComputeWorkgroupSizeY << '\n';
		std::cout << " - maxComputeWorkgroupSizeZ: " << limits.limits.maxComputeWorkgroupSizeZ << '\n';
		std::cout << " - maxComputeWorkgroupsPerDimension: " << limits.limits.maxComputeWorkgroupsPerDimension << '\n';
	}

	WGPUAdapterProperties properties = {};
	properties.nextInChain = nullptr;
	wgpuAdapterGetProperties(adapter, &properties);
	std::cout << "Adapter properties:" << '\n';
	std::cout << " - vendorID: " << properties.vendorID << '\n';
	std::cout << " - deviceID: " << properties.deviceID << '\n';
	std::cout << " - name: " << properties.name << '\n';
	if (properties.driverDescription) {
		std::cout << " - driverDescription: " << properties.driverDescription << '\n';
	}
	std::cout << " - adapterType: " << properties.adapterType << '\n';
	std::cout << " - backendType: " << properties.backendType << '\n';
}
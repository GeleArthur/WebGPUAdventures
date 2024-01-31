#pragma once

#include <webgpu/webgpu.h>

WGPUDevice requestDevice(WGPUAdapter adapter, WGPUDeviceDescriptor const * descriptor);
WGPUAdapter requestAdapter(WGPUInstance instance, const WGPURequestAdapterOptions* options);
void inspectAdapter(WGPUAdapter adapter);
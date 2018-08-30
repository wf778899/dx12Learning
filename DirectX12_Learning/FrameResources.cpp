#include "stdafx.h"
#include "FrameResources.h"

FrameResources::FrameResources(ID3D12Device *device, UINT objectCB_count, UINT passCB_count)
{
	device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&cmdAllocator));
	ObjectCB = std::make_unique<UploadBuffer<ObjectConstants>>(device, objectCB_count, true);
	PassCB = std::make_unique<UploadBuffer<PassConstants>>(device, passCB_count, true);
}



FrameResources::~FrameResources() {}

#include "stdafx.h"
#include "FrameResources.h"

FrameResources::FrameResources(ID3D12Device *device, UINT passCB_count, UINT objectCB_count)
{
	device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&cmdAllocator));
	PassCB = std::make_unique<UploadBuffer<PassConstants>>(device, passCB_count, true);
	ObjectCB = std::make_unique<UploadBuffer<ObjectConstants>>(device, objectCB_count, true);
}



FrameResources::~FrameResources() {}

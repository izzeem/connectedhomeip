/*
 *
 *    Copyright (c) 2022 Project CHIP Authors
 *    All rights reserved.
 *
 *    Licensed under the Apache License, Version 2.0 (the "License");
 *    you may not use this file except in compliance with the License.
 *    You may obtain a copy of the License at
 *
 *        http://www.apache.org/licenses/LICENSE-2.0
 *
 *    Unless required by applicable law or agreed to in writing, software
 *    distributed under the License is distributed on an "AS IS" BASIS,
 *    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *    See the License for the specific language governing permissions and
 *    limitations under the License.
 */
#include "TargetVideoPlayerInfo.h"

using namespace chip;

CASEClientPool<CHIP_CONFIG_DEVICE_MAX_ACTIVE_CASE_CLIENTS> gCASEClientPool;

CHIP_ERROR TargetVideoPlayerInfo::Initialize(NodeId nodeId, FabricIndex fabricIndex)
{
    ChipLogProgress(NotSpecified, "TargetVideoPlayerInfo nodeId=0x" ChipLogFormatX64 " fabricIndex=%d", ChipLogValueX64(nodeId),
                    fabricIndex);
    mNodeId      = nodeId;
    mFabricIndex = fabricIndex;
    for (auto & endpointInfo : mEndpoints)
    {
        endpointInfo.Reset();
    }

    Server * server           = &(chip::Server::GetInstance());
    chip::FabricInfo * fabric = server->GetFabricTable().FindFabricWithIndex(fabricIndex);
    if (fabric == nullptr)
    {
        ChipLogError(AppServer, "Did not find fabric for index %d", fabricIndex);
        return CHIP_ERROR_INVALID_FABRIC_INDEX;
    }

    PeerId peerID = fabric->GetPeerIdForNode(nodeId);

    CHIP_ERROR err =
        server->GetCASESessionManager()->FindOrEstablishSession(peerID, &mOnConnectedCallback, &mOnConnectionFailureCallback);
    if (err != CHIP_NO_ERROR)
    {
        ChipLogError(AppServer, "Could not establish a session to the peer");
        return err;
    }

    if (mOperationalDeviceProxy == nullptr)
    {
        ChipLogError(AppServer, "Failed to find an existing instance of OperationalDeviceProxy to the peer");
        return CHIP_ERROR_INVALID_ARGUMENT;
    }
    ChipLogProgress(AppServer, "Created an instance of OperationalDeviceProxy");

    mInitialized = true;
    return CHIP_NO_ERROR;
}

TargetEndpointInfo * TargetVideoPlayerInfo::GetOrAddEndpoint(EndpointId endpointId)
{
    if (!mInitialized)
    {
        return nullptr;
    }
    TargetEndpointInfo * endpoint = GetEndpoint(endpointId);
    if (endpoint != nullptr)
    {
        return endpoint;
    }
    for (auto & endpointInfo : mEndpoints)
    {
        if (!endpointInfo.IsInitialized())
        {
            endpointInfo.Initialize(endpointId);
            return &endpointInfo;
        }
    }
    return nullptr;
}

TargetEndpointInfo * TargetVideoPlayerInfo::GetEndpoint(EndpointId endpointId)
{
    if (!mInitialized)
    {
        return nullptr;
    }
    for (auto & endpointInfo : mEndpoints)
    {
        if (endpointInfo.IsInitialized() && endpointInfo.GetEndpointId() == endpointId)
        {
            return &endpointInfo;
        }
    }
    return nullptr;
}

bool TargetVideoPlayerInfo::HasEndpoint(EndpointId endpointId)
{
    if (!mInitialized)
    {
        return false;
    }
    for (auto & endpointInfo : mEndpoints)
    {
        if (endpointInfo.IsInitialized() && endpointInfo.GetEndpointId() == endpointId)
        {
            return true;
        }
    }
    return false;
}

void TargetVideoPlayerInfo::PrintInfo()
{
    ChipLogProgress(NotSpecified, " TargetVideoPlayerInfo nodeId=0x" ChipLogFormatX64 " fabric index=%d", ChipLogValueX64(mNodeId),
                    mFabricIndex);
    for (auto & endpointInfo : mEndpoints)
    {
        if (endpointInfo.IsInitialized())
        {
            endpointInfo.PrintInfo();
        }
    }
}

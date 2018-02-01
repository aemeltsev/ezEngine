﻿#include <PCH.h>
#include <ProceduralPlacementPlugin/Resources/ProceduralPlacementResource.h>
#include <Core/Assets/AssetFileHeader.h>
#include <Foundation/IO/ChunkStream.h>

namespace ezPPInternal
{
  extern Pattern* GetPattern(ezTempHashedString sName);
}

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezProceduralPlacementResource, 1, ezRTTIDefaultAllocator<ezProceduralPlacementResource>);
EZ_END_DYNAMIC_REFLECTED_TYPE

ezProceduralPlacementResource::ezProceduralPlacementResource()
  : ezResource<ezProceduralPlacementResource, ezProceduralPlacementResourceDescriptor>(DoUpdate::OnAnyThread, 1)
{

}

ezProceduralPlacementResource::~ezProceduralPlacementResource()
{

}

const ezDynamicArray<ezSharedPtr<const ezPPInternal::Layer>>& ezProceduralPlacementResource::GetLayers() const
{
  return m_Layers;
}

ezResourceLoadDesc ezProceduralPlacementResource::UnloadData(Unload WhatToUnload)
{
  m_Layers.Clear();

  ezResourceLoadDesc res;
  res.m_uiQualityLevelsDiscardable = 0;
  res.m_uiQualityLevelsLoadable = 0;
  res.m_State = ezResourceState::Unloaded;

  return res;
}

ezResourceLoadDesc ezProceduralPlacementResource::UpdateContent(ezStreamReader* Stream)
{
  EZ_LOG_BLOCK("ezProceduralPlacementResource::UpdateContent", GetResourceDescription().GetData());

  ezResourceLoadDesc res;
  res.m_uiQualityLevelsDiscardable = 0;
  res.m_uiQualityLevelsLoadable = 0;

  if (Stream == nullptr)
  {
    res.m_State = ezResourceState::LoadedResourceMissing;
    return res;
  }

  // skip the absolute file path data that the standard file reader writes into the stream
  {
    ezString sAbsFilePath;
    (*Stream) >> sAbsFilePath;
  }

  ezAssetFileHeader AssetHash;
  AssetHash.Read(*Stream);

  // load
  {
    ezChunkStreamReader chunk(*Stream);
    chunk.SetEndChunkFileMode(ezChunkStreamReader::EndChunkFileMode::JustClose);

    chunk.BeginStream();

    ezStringBuilder sTemp;

    // skip all chunks that we don't know
    while (chunk.GetCurrentChunk().m_bValid)
    {
      if (chunk.GetCurrentChunk().m_sChunkName == "Layers")
      {
        ezUInt32 uiNumLayers = 0;
        chunk >> uiNumLayers;

        m_Layers.Reserve(uiNumLayers);
        for (ezUInt32 uiLayerIndex = 0; uiLayerIndex < uiNumLayers; ++uiLayerIndex)
        {
          auto pLayer = EZ_DEFAULT_NEW(ezPPInternal::Layer);

          chunk >> sTemp;
          pLayer->m_sName.Assign(sTemp.GetData());

          ezUInt32 uiNumObjectsToPlace = 0;
          chunk >> uiNumObjectsToPlace;

          for (ezUInt32 uiObjectIndex = 0; uiObjectIndex < uiNumObjectsToPlace; ++uiObjectIndex)
          {
            chunk >> sTemp;
            pLayer->m_ObjectsToPlace.ExpandAndGetRef().Assign(sTemp.GetData());
          }

          pLayer->m_pPattern = ezPPInternal::GetPattern("Bayer");

          chunk >> pLayer->m_fFootprint;

          chunk >> pLayer->m_vMinOffset;
          chunk >> pLayer->m_vMaxOffset;

          chunk >> pLayer->m_fAlignToNormal;

          chunk >> pLayer->m_vMinScale;
          chunk >> pLayer->m_vMaxScale;

          chunk >> pLayer->m_fCullDistance;

          m_Layers.PushBack(pLayer);
        }
      }

      chunk.NextChunk();
    }

    chunk.EndStream();
  }

  res.m_State = ezResourceState::Loaded;
  return res;
}

void ezProceduralPlacementResource::UpdateMemoryUsage(MemoryUsage& out_NewMemoryUsage)
{
  out_NewMemoryUsage.m_uiMemoryCPU = 0;
  out_NewMemoryUsage.m_uiMemoryGPU = 0;
}

ezResourceLoadDesc ezProceduralPlacementResource::CreateResource(const ezProceduralPlacementResourceDescriptor& descriptor)
{
  //EZ_REPORT_FAILURE("This resource type does not support creating data.");

  // Missing resource

  auto pLayer = EZ_DEFAULT_NEW(ezPPInternal::Layer);
  pLayer->m_sName.Assign("MissingLayer");
  pLayer->m_ObjectsToPlace.PushBack(ezMakeHashedString("Missing Prefab"));
  pLayer->m_pPattern = ezPPInternal::GetPattern("Bayer");
  pLayer->m_fFootprint = 3.0f;
  pLayer->m_vMinOffset.Set(-1.0f, -1.0f, -0.5f);
  pLayer->m_vMaxOffset.Set(1.0f, 1.0f, 0.0f);
  pLayer->m_vMinScale.Set(1.0f, 1.0f, 1.0f);
  pLayer->m_vMaxScale.Set(1.5f, 1.5f, 2.0f);

  m_Layers.PushBack(pLayer);
  //

  ezResourceLoadDesc res;
  res.m_uiQualityLevelsDiscardable = 0;
  res.m_uiQualityLevelsLoadable = 0;
  res.m_State = ezResourceState::Loaded;

  return res;
}

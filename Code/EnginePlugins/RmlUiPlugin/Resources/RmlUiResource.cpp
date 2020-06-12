#include <RmlUiPluginPCH.h>

#include <Core/Assets/AssetFileHeader.h>
#include <Foundation/IO/FileSystem/FileReader.h>
#include <RmlUiPlugin/Resources/RmlUiResource.h>

static ezTypeVersion s_RmlUiDescVersion = 1;

ezResult ezRmlUiResourceDescriptor::Save(ezStreamWriter& stream)
{
  // write this at the beginning so that the file can be read as an ezDependencyFile
  m_DependencyFile.StoreCurrentTimeStamp();
  m_DependencyFile.WriteDependencyFile(stream);

  stream.WriteVersion(s_RmlUiDescVersion);

  stream << m_sRmlFile;

  return EZ_SUCCESS;
}

ezResult ezRmlUiResourceDescriptor::Load(ezStreamReader& stream)
{
  m_DependencyFile.ReadDependencyFile(stream);

  ezTypeVersion uiVersion = stream.ReadVersion(s_RmlUiDescVersion);

  stream >> m_sRmlFile;

  return EZ_SUCCESS;
}

//////////////////////////////////////////////////////////////////////////

// clang-format off
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezRmlUiResource, 1, ezRTTIDefaultAllocator<ezRmlUiResource>)
EZ_END_DYNAMIC_REFLECTED_TYPE;

EZ_RESOURCE_IMPLEMENT_COMMON_CODE(ezRmlUiResource);
// clang-format on

ezRmlUiResource::ezRmlUiResource()
  : ezResource(DoUpdate::OnAnyThread, 1)
{
}

ezResourceLoadDesc ezRmlUiResource::UnloadData(Unload WhatToUnload)
{
  ezResourceLoadDesc res;
  res.m_uiQualityLevelsDiscardable = 0;
  res.m_uiQualityLevelsLoadable = 0;
  res.m_State = ezResourceState::Unloaded;

  return res;
}

ezResourceLoadDesc ezRmlUiResource::UpdateContent(ezStreamReader* Stream)
{
  ezRmlUiResourceDescriptor desc;
  ezResourceLoadDesc res;
  res.m_uiQualityLevelsDiscardable = 0;
  res.m_uiQualityLevelsLoadable = 0;

  if (Stream == nullptr)
  {
    res.m_State = ezResourceState::LoadedResourceMissing;
    return res;
  }

  ezStringBuilder sAbsFilePath;
  (*Stream) >> sAbsFilePath;

  // Direct loading of rml file
  if (sAbsFilePath.GetFileExtension() == "rml")
  {
    m_sRmlFile = sAbsFilePath;

    res.m_State = ezResourceState::Loaded;
    return res;
  }

  ezAssetFileHeader assetHeader;
  assetHeader.Read(*Stream);

  if (desc.Load(*Stream).Failed())
  {
    res.m_State = ezResourceState::LoadedResourceMissing;
    return res;
  }

  return CreateResource(std::move(desc));
}

void ezRmlUiResource::UpdateMemoryUsage(MemoryUsage& out_NewMemoryUsage)
{
  // TODO
  out_NewMemoryUsage.m_uiMemoryCPU = sizeof(*this);
  out_NewMemoryUsage.m_uiMemoryGPU = 0;
}

EZ_RESOURCE_IMPLEMENT_CREATEABLE(ezRmlUiResource, ezRmlUiResourceDescriptor)
{
  m_sRmlFile = descriptor.m_sRmlFile;

  ezResourceLoadDesc res;
  res.m_uiQualityLevelsDiscardable = 0;
  res.m_uiQualityLevelsLoadable = 0;
  res.m_State = ezResourceState::Loaded;

  return res;
}

//////////////////////////////////////////////////////////////////////////

bool ezRmlUiResourceLoader::IsResourceOutdated(const ezResource* pResource) const
{
  if (ezResourceLoaderFromFile::IsResourceOutdated(pResource))
    return true;

  ezStringBuilder sId = pResource->GetResourceID();
  if (sId.GetFileExtension() == "rml")
    return false;

  ezFileReader stream;
  if (stream.Open(pResource->GetResourceID()).Failed())
    return true;

  // skip asset header
  ezAssetFileHeader assetHeader;
  assetHeader.Read(stream);

  ezDependencyFile dep;
  if (dep.ReadDependencyFile(stream).Failed())
    return true;

  return dep.HasAnyFileChanged();
}
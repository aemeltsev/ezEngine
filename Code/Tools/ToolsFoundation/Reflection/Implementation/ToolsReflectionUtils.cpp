#include <ToolsFoundation/PCH.h>
#include <ToolsFoundation/Reflection/ReflectedType.h>
#include <ToolsFoundation/Reflection/ToolsReflectionUtils.h>
#include <ToolsFoundation/Reflection/PhantomRttiManager.h>
#include <Foundation/Configuration/Startup.h>
#include <Foundation/IO/ExtendedJSONWriter.h>
#include <Foundation/IO/ExtendedJSONReader.h>
#include <Foundation/Reflection/ReflectionSerializer.h>
#include <ToolsFoundation/Reflection/IReflectedTypeAccessor.h>
#include <ToolsFoundation/Reflection/ObjectReflectionAdapter.h>
#include <ToolsFoundation/Object/DocumentObjectBase.h>


////////////////////////////////////////////////////////////////////////
// ezToolsReflectionUtils public functions
////////////////////////////////////////////////////////////////////////

ezVariant ezToolsReflectionUtils::GetDefaultVariantFromType(ezVariant::Type::Enum type)
{
  switch (type)
  {
  case ezVariant::Type::Invalid:
    return ezVariant();
  case ezVariant::Type::Bool:
    return ezVariant(false);
  case ezVariant::Type::Int8:
    return ezVariant((ezInt8)0);
  case ezVariant::Type::UInt8:
    return ezVariant((ezUInt8)0);
  case ezVariant::Type::Int16:
    return ezVariant((ezInt16)0);
  case ezVariant::Type::UInt16:
    return ezVariant((ezUInt16)0);
  case ezVariant::Type::Int32:
    return ezVariant((ezInt32)0);
  case ezVariant::Type::UInt32:
    return ezVariant((ezUInt32)0);
  case ezVariant::Type::Int64:
    return ezVariant((ezInt64)0);
  case ezVariant::Type::UInt64:
    return ezVariant((ezUInt64)0);
  case ezVariant::Type::Float:
    return ezVariant(0.0f);
  case ezVariant::Type::Double:
    return ezVariant(0.0);
  case ezVariant::Type::Color:
    return ezVariant(ezColor(1.0f, 1.0f, 1.0f));
  case ezVariant::Type::Vector2:
    return ezVariant(ezVec2(0.0f, 0.0f));
  case ezVariant::Type::Vector3:
    return ezVariant(ezVec3(0.0f, 0.0f, 0.0f));
  case ezVariant::Type::Vector4:
    return ezVariant(ezVec4(0.0f, 0.0f, 0.0f, 0.0f));
  case ezVariant::Type::Quaternion:
    return ezVariant(ezQuat(0.0f, 0.0f, 0.0f, 1.0f));
  case ezVariant::Type::Matrix3:
    return ezVariant(ezMat3::IdentityMatrix());
  case ezVariant::Type::Matrix4:
    return ezVariant(ezMat4::IdentityMatrix());
  case ezVariant::Type::String:
    return ezVariant("");
  case ezVariant::Type::Time:
    return ezVariant(ezTime());
  case ezVariant::Type::Uuid:
    return ezVariant(ezUuid());
  case ezVariant::Type::VariantArray:
    return ezVariant();
  case ezVariant::Type::VariantDictionary:
    return ezVariant();
  case ezVariant::Type::ReflectedPointer:
    return ezVariant();
  case ezVariant::Type::VoidPointer:
    return ezVariant();
  case ezVariant::Type::ENUM_COUNT:
    EZ_REPORT_FAILURE("Invalid case statement");
    return ezVariant();
  }
  return ezVariant();
}

void ezToolsReflectionUtils::GetReflectedTypeDescriptorFromRtti(const ezRTTI* pRtti, ezReflectedTypeDescriptor& out_desc)
{
  EZ_ASSERT_DEV(pRtti != nullptr, "Type to process must not be null!");
  out_desc.m_sTypeName = pRtti->GetTypeName();
  out_desc.m_sPluginName = pRtti->GetPluginName();
  out_desc.m_Flags = pRtti->GetTypeFlags();
  out_desc.m_uiTypeSize = pRtti->GetTypeSize();
  out_desc.m_uiTypeVersion = pRtti->GetTypeVersion();
  const ezRTTI* pParentRtti = pRtti->GetParentType();
  out_desc.m_sParentTypeName = pParentRtti ? pParentRtti->GetTypeName() : nullptr;

  out_desc.m_Properties.Clear();
  const ezArrayPtr<ezAbstractProperty*>& rttiProps = pRtti->GetProperties();
  const ezUInt32 uiCount = rttiProps.GetCount();
  out_desc.m_Properties.Reserve(uiCount);

  for (ezUInt32 i = 0; i < uiCount; ++i)
  {
    ezAbstractProperty* prop = rttiProps[i];

    switch (prop->GetCategory())
    {
    case ezPropertyCategory::Constant:
      {
        ezAbstractConstantProperty* constantProp = static_cast<ezAbstractConstantProperty*>(prop);
        const ezRTTI* pPropRtti = constantProp->GetSpecificType();

        if (ezReflectionUtils::IsBasicType(pPropRtti))
        {
          ezVariant value = constantProp->GetConstant();
          out_desc.m_Properties.PushBack(ezReflectedPropertyDescriptor(constantProp->GetPropertyName(), pPropRtti->GetVariantType(), value));
        }
        else
        {
          EZ_ASSERT_DEV(false, "Non-pod constants are not supported yet!");
        }
      }
      break;

    case ezPropertyCategory::Member:
    case ezPropertyCategory::Array:
    case ezPropertyCategory::Set:
      {
        const ezRTTI* pPropRtti = prop->GetSpecificType();
        out_desc.m_Properties.PushBack(ezReflectedPropertyDescriptor(prop->GetCategory(), prop->GetPropertyName(), pPropRtti->GetTypeName(), pPropRtti->GetVariantType(), prop->GetFlags()));
      }
      break;

    case ezPropertyCategory::Function:
      break;
    }
  }
}

void ezToolsReflectionUtils::RegisterType(const ezRTTI* pRtti, bool bIncludeDerived)
{
  EZ_ASSERT_DEV(pRtti != nullptr, "Invalid type !!");

  ezSet<const ezRTTI*> types;
  types.Insert(pRtti);
  if (bIncludeDerived)
  {
    ezReflectionUtils::GatherTypesDerivedFromClass(pRtti, types, true);
  }
  else
  {
    ezReflectionUtils::GatherDependentTypes(pRtti, types);
  }

  ezDynamicArray<const ezRTTI*> sortedTypes;
  ezReflectionUtils::CreateDependencySortedTypeArray(types, sortedTypes);

  for (auto type : sortedTypes)
  {
    ezReflectedTypeDescriptor desc;
    ezToolsReflectionUtils::GetReflectedTypeDescriptorFromRtti(type, desc);
    ezPhantomRttiManager::RegisterType(desc);
  }
}

ezPropertyPath ezToolsReflectionUtils::CreatePropertyPath(const char* pData1, const char* pData2, const char* pData3, const char* pData4, const char* pData5, const char* pData6)
{
  ezPropertyPath path;
  const ezUInt32 uiMaxParams = 6;
  const char* pStrings[uiMaxParams] = { pData1, pData2, pData3, pData4, pData5, pData6 };
  ezUInt32 uiUsedParams = 0;
  for (ezUInt32 i = 0; i < uiMaxParams; ++i)
  {
    if (ezStringUtils::IsNullOrEmpty(pStrings[i]))
      break;

    path.PushBack(pStrings[i]);
  }

  return path;
}

ezAbstractProperty* ezToolsReflectionUtils::GetPropertyByPath(const ezRTTI* pRtti, const ezPropertyPath& path)
{
  ezAbstractMemberProperty* pCurrentProp = ezReflectionUtils::GetMemberProperty(pRtti, path[0]);
  if (path.GetCount() == 1)
    return pCurrentProp;

  ezPropertyPath pathCopy = path;
  pathCopy.RemoveAt(0);

  return GetPropertyByPath(pCurrentProp->GetSpecificType(), pathCopy);
}

ezVariant ezToolsReflectionUtils::GetMemberPropertyValueByPath(const ezRTTI* pRtti, void* pObject, const ezPropertyPath& path)
{
  EZ_ASSERT_DEV(path.GetCount() > 0, "ezReflectedTypeDirectAccessor: the given property path is empty!");

  ezAbstractMemberProperty* pCurrentProp = ezReflectionUtils::GetMemberProperty(pRtti, path[0]);

  if (pCurrentProp == nullptr)
    return ezVariant();

  if (path.GetCount() == 1)
    return ezReflectionUtils::GetMemberPropertyValue(pCurrentProp, pObject);

  if (pCurrentProp->GetPropertyPointer(pObject) != nullptr)
  {
    ezPropertyPath pathCopy = path;
    pathCopy.RemoveAt(0);

    return GetMemberPropertyValueByPath(pCurrentProp->GetSpecificType(), pCurrentProp->GetPropertyPointer(pObject), pathCopy);
  }
  else if (pCurrentProp->GetSpecificType()->GetAllocator()->CanAllocate())
  {
    void* pValue = pCurrentProp->GetSpecificType()->GetAllocator()->Allocate();
    pCurrentProp->GetValuePtr(pObject, pValue);

    ezPropertyPath pathCopy = path;
    pathCopy.RemoveAt(0);

    ezVariant res = GetMemberPropertyValueByPath(pCurrentProp->GetSpecificType(), pValue, pathCopy);

    pCurrentProp->GetSpecificType()->GetAllocator()->Deallocate(pValue);

    return res;
  }

  return ezVariant();
}

bool ezToolsReflectionUtils::SetMemberPropertyValueByPath(const ezRTTI* pRtti, void* pObject, const ezPropertyPath& path, const ezVariant& value)
{
  EZ_ASSERT_DEV(path.GetCount() > 0, "ezReflectedTypeDirectAccessor: the given property path is empty!");

  ezAbstractMemberProperty* pCurrentProp = ezReflectionUtils::GetMemberProperty(pRtti, path[0]);

  if (pCurrentProp == nullptr)
    return false;

  if (path.GetCount() == 1)
  {
    ezReflectionUtils::SetMemberPropertyValue(pCurrentProp, pObject, value);
    return true;
  }

  if (pCurrentProp->GetPropertyPointer(pObject) != nullptr)
  {
    ezPropertyPath pathCopy = path;
    pathCopy.RemoveAt(0);

    return SetMemberPropertyValueByPath(pCurrentProp->GetSpecificType(), pCurrentProp->GetPropertyPointer(pObject), pathCopy, value);
  }
  else if (pCurrentProp->GetSpecificType()->GetAllocator()->CanAllocate())
  {
    void* pValue = pCurrentProp->GetSpecificType()->GetAllocator()->Allocate();
    pCurrentProp->GetValuePtr(pObject, pValue);

    ezPropertyPath pathCopy = path;
    pathCopy.RemoveAt(0);

    const bool res = SetMemberPropertyValueByPath(pCurrentProp->GetSpecificType(), pValue, pathCopy, value);

    pCurrentProp->SetValuePtr(pObject, pValue);
    pCurrentProp->GetSpecificType()->GetAllocator()->Deallocate(pValue);

    return res;
  }

  return false;
}

void ezToolsReflectionUtils::WriteObjectToJSON(ezStreamWriterBase& stream, const ezDocumentObjectBase* pObject, ezJSONWriter::WhitespaceMode::Enum WhitespaceMode)
{
  ezExtendedJSONWriter writer;
  writer.SetOutputStream(&stream);
  writer.SetWhitespaceMode(WhitespaceMode);

  ezObjectSerializationContext context;
  ezObjectReflectionAdapter adapter(&context);
  ezReflectionSerializer serializer(&adapter);

  writer.BeginObject();
  {
    struct TocData
    {
      ezUuid m_Guid;
      ezString m_sType;
    };

    ezDeque<TocData> AllObjects;

    const ezUuid rootGuid = context.EnqueObject((void*)(pObject), pObject->GetTypeAccessor().GetType());

    writer.AddVariableUuid("RootObjectGuid", rootGuid);

    writer.BeginObject("Objects");
    {
      ezReflectedObjectWrapper object = context.DequeueObject();

      while (object.m_pObject != nullptr)
      {
        auto& td = AllObjects.ExpandAndGetRef();
        td.m_Guid = context.GetObjectGUID(object.m_pObject);
        td.m_sType = adapter.GetTypeInfo(object.m_pType).m_szName;

        serializer.WriteJSONObject(writer, object, ezConversionUtils::ToString(td.m_Guid));

        object = context.DequeueObject();
      }

    }
    writer.EndObject();

    writer.BeginArray("TOC");
    {
      for (const auto& td : AllObjects)
      {
        writer.BeginObject();
        writer.AddVariableUuid("guid", td.m_Guid);
        writer.AddVariableString("type", td.m_sType);
        writer.EndObject();
      }
    }
    writer.EndArray();
  }
  writer.EndObject();

}

void ezToolsReflectionUtils::ReadObjectPropertiesFromJSON(ezStreamReaderBase& stream, ezIReflectedTypeAccessor& accessor)
{
  EZ_ASSERT_NOT_IMPLEMENTED;

  //ezExtendedJSONReader reader;
  //if (reader.Parse(stream).Failed())
  //  return;

  //const ezVariantDictionary& root = reader.GetTopLevelObject();

  //ezPropertyPath path;
  //ReadJSONObject(root, accessor, path);
}


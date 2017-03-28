#include <PCH.h>
#include <GameEngine/VisualScript/Nodes/VisualScriptReferenceNodes.h>
#include <GameEngine/VisualScript/VisualScriptInstance.h>
#include <Core/World/GameObject.h>

//////////////////////////////////////////////////////////////////////////

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezVisualScriptNode_FindChildObject, 1, ezRTTIDefaultAllocator<ezVisualScriptNode_FindChildObject>)
{
  EZ_BEGIN_ATTRIBUTES
  {
    new ezCategoryAttribute("References")
  }
  EZ_END_ATTRIBUTES
  EZ_BEGIN_PROPERTIES
  {
    // Data Pins (Output)
    EZ_CONSTANT_PROPERTY("Object", 0)->AddAttributes(new ezVisScriptDataPinOutAttribute(0, ezVisualScriptDataPinType::GameObjectHandle)),
    // Exposed Properties
    EZ_MEMBER_PROPERTY("Name", m_sObjectName),
  }
  EZ_END_PROPERTIES
}
EZ_END_DYNAMIC_REFLECTED_TYPE

ezVisualScriptNode_FindChildObject::ezVisualScriptNode_FindChildObject() { }

void ezVisualScriptNode_FindChildObject::Execute(ezVisualScriptInstance* pInstance, ezUInt8 uiExecPin)
{
  if (m_hObject.IsInvalidated() && !m_sObjectName.IsEmpty())
  {
    const ezTempHashedString name(m_sObjectName.GetData());
    ezGameObject* pChild = pInstance->GetOwner()->FindChildByName(name, true);

    if (pChild != nullptr)
    {
      m_hObject = pChild->GetHandle();
    }
    else
    {
      // make sure we don't try this again
      m_sObjectName.Clear();
      m_hObject.Invalidate();
    }
  }

  pInstance->SetOutputPinValue(this, 0, &m_hObject);
}

//////////////////////////////////////////////////////////////////////////

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezVisualScriptNode_FindComponent, 1, ezRTTIDefaultAllocator<ezVisualScriptNode_FindComponent>)
{
  EZ_BEGIN_ATTRIBUTES
  {
    new ezCategoryAttribute("References")
  }
    EZ_END_ATTRIBUTES
    EZ_BEGIN_PROPERTIES
  {
    // Data Pins (Input)
    EZ_CONSTANT_PROPERTY("Object", 0)->AddAttributes(new ezVisScriptDataPinInAttribute(0, ezVisualScriptDataPinType::GameObjectHandle)),
    // Data Pins (Output)
    EZ_CONSTANT_PROPERTY("Component", 0)->AddAttributes(new ezVisScriptDataPinOutAttribute(0, ezVisualScriptDataPinType::ComponentHandle)),
    // Exposed Properties
    EZ_MEMBER_PROPERTY("Type", m_sType),
  }
  EZ_END_PROPERTIES
}
EZ_END_DYNAMIC_REFLECTED_TYPE

ezVisualScriptNode_FindComponent::ezVisualScriptNode_FindComponent() { }

void ezVisualScriptNode_FindComponent::Execute(ezVisualScriptInstance* pInstance, ezUInt8 uiExecPin)
{
  if (m_hComponent.IsInvalidated() && !m_sType.IsEmpty())
  {
    ezGameObject* pObject = pInstance->GetOwner();

    if (!m_hObject.IsInvalidated())
    {
      if (!pObject->GetWorld()->TryGetObject(m_hObject, pObject))
        goto fail;
    }

    const ezRTTI* pRtti = ezRTTI::FindTypeByName(m_sType);
    if (pRtti == nullptr)
      goto fail;

    ezComponent* pComponent = nullptr;
    if (!pObject->TryGetComponentOfBaseType(pRtti, pComponent))
      goto fail;

    m_hComponent = pComponent->GetHandle();
  }

  pInstance->SetOutputPinValue(this, 0, &m_hComponent);
  return;

fail:
  m_hComponent.Invalidate();
  m_sType.Clear();
}

void* ezVisualScriptNode_FindComponent::GetInputPinDataPointer(ezUInt8 uiPin)
{
  switch (uiPin)
  {
  case 0:
    return &m_hObject;
  }

  return nullptr;
}

//////////////////////////////////////////////////////////////////////////
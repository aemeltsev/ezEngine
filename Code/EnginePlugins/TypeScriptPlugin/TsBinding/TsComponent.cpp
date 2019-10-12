#include <TypeScriptPluginPCH.h>

#include <Core/World/Component.h>
#include <Duktape/duktape.h>
#include <Foundation/Types/ScopeExit.h>
#include <TypeScriptPlugin/TsBinding/TsBinding.h>

static int __CPP_Component_GetOwner(duk_context* pDuk);
static int __CPP_Component_SetActive(duk_context* pDuk);

ezResult ezTypeScriptBinding::Init_Component()
{
  m_Duk.RegisterFunction("__CPP_Component_GetOwner", __CPP_Component_GetOwner, 1);
  m_Duk.RegisterFunction("__CPP_Component_SetActive", __CPP_Component_SetActive, 2);

  return EZ_SUCCESS;
}

ezResult ezTypeScriptBinding::CreateTsComponent(duk_context* pDuk, const char* szTypeName, const ezComponentHandle& hCppComponent, const char* szDebugString)
{
  ezDuktapeStackValidator validator(pDuk, 0);
  ezDuktapeWrapper duk(pDuk);

  ezStringBuilder sTypeName = szTypeName;

  duk.OpenGlobalObject(); // [ global ]

  bool bCloseAllComps = false;
  if (sTypeName.TrimWordStart("ez"))
  {
    EZ_SUCCEED_OR_RETURN(duk.OpenObject("__AllComponents")); // [ global __AllComponents ]
    bCloseAllComps = true;
  }

  duk_get_prop_string(duk, -1, sTypeName); // [ global __AllComponents sTypeName ]
  duk_new(duk, 0);                         // [ global __AllComponents instance ]

  // store C++ side component handle in obj as property
  {
    ezComponentHandle* pBuffer = reinterpret_cast<ezComponentHandle*>(duk_push_fixed_buffer(duk, sizeof(ezComponentHandle))); // [ global __AllComponents instance buffer ]
    *pBuffer = hCppComponent;
    duk_put_prop_index(duk, -2, ezTypeScriptBindingIndexProperty::ComponentHandle); // [ global __AllComponents instance ]
  }

  // store reference to component in the global stash
  {
    const ezUInt32 uiComponentReference = hCppComponent.GetInternalID().m_Data;

    duk.OpenGlobalStashObject();                                 // [ global __AllComponents instance gstash]
    duk_push_uint(duk, uiComponentReference);                    // [ global __AllComponents instance gstash uint ]
    duk_dup(duk, -3);                                            // [ global __AllComponents instance gstash uint instance ]
    EZ_VERIFY(duk_put_prop(duk, -3), "Storing property failed"); // [ global __AllComponents instance gstash ]
    duk.CloseObject();                                           // [ global __AllComponents instance ]
  }


  if (bCloseAllComps)
  {
    duk_pop_3(duk); // [ global __AllComponents instance ] -> [ ]
  }
  else
  {
    duk_pop_2(duk); // [ global instance ] -> [ ]
  }

  return EZ_SUCCESS;
}

void ezTypeScriptBinding::DukPutComponentObject(duk_context* pDuk, const ezComponentHandle& hComponent)
{
  ezDuktapeStackValidator validator(pDuk, +1);

  duk_push_global_stash(pDuk);

  const ezUInt32 uiComponentReference = hComponent.GetInternalID().m_Data;
  duk_push_uint(pDuk, uiComponentReference);
  if (!duk_get_prop(pDuk, -2))
  {
    // remove 'undefined' result from stack, replace it with null
    duk_pop(pDuk);
    duk_push_null(pDuk);
  }
  else
  {
    // remove stash object, keep result on top
    duk_replace(pDuk, -2);
  }
}

void ezTypeScriptBinding::DukPutComponentObject(duk_context* pDuk, ezComponent* pComponent)
{
  if (pComponent == nullptr)
  {
    duk_push_null(pDuk);
  }
  else
  {
    CreateTsComponent(pDuk, pComponent->GetDynamicRTTI()->GetTypeName(), pComponent->GetHandle(), "");
    DukPutComponentObject(pDuk, pComponent->GetHandle());
  }
}


void ezTypeScriptBinding::DeleteTsComponent(const ezComponentHandle& hCppComponent)
{
  const ezUInt32 uiComponentReference = hCppComponent.GetInternalID().m_Data;

  ezDuktapeStackValidator validator(m_Duk);

  m_Duk.OpenGlobalStashObject();
  duk_push_uint(m_Duk, uiComponentReference);
  EZ_VERIFY(duk_del_prop(m_Duk, -2), "Could not delete property");
  m_Duk.CloseObject();
}

ezComponentHandle ezTypeScriptBinding::RetrieveComponentHandle(duk_context* pDuk, ezInt32 iObjIdx /*= 0 */)
{
  ezDuktapeStackValidator validator(pDuk);

  duk_get_prop_index(pDuk, iObjIdx, ezTypeScriptBindingIndexProperty::ComponentHandle);
  ezComponentHandle hComponent = *reinterpret_cast<ezComponentHandle*>(duk_get_buffer(pDuk, -1, nullptr));
  duk_pop(pDuk);

  return hComponent;
}

static int __CPP_Component_GetOwner(duk_context* pDuk)
{
  ezDuktapeFunction duk(pDuk);

  ezComponent* pComponent = ezTypeScriptBinding::ExpectComponent<ezComponent>(pDuk);

  ezTypeScriptBinding::DukPutGameObject(duk, pComponent->GetOwner()->GetHandle());

  return duk.ReturnCustom();
}

static int __CPP_Component_SetActive(duk_context* pDuk)
{
  ezDuktapeFunction duk(pDuk);

  ezComponent* pComponent = ezTypeScriptBinding::ExpectComponent<ezComponent>(pDuk);

  pComponent->SetActive(duk.GetBoolParameter(1, true));

  return duk.ReturnVoid();
}

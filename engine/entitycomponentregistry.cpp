///////////////////////////////////////////////////////////////////////////////
// $Id$

#include "stdhdr.h"

#include "entitycomponentregistry.h"

#ifdef HAVE_UNITTESTPP
#include "UnitTest++.h"
#endif

#include <tinyxml.h>

#include "tech/dbgalloc.h" // must be last header

using namespace std;

///////////////////////////////////////////////////////////////////////////////

extern void RegisterBuiltInComponents();


///////////////////////////////////////////////////////////////////////////////
//
// CLASS: cEntityComponentRegistry
//

///////////////////////////////////////

cEntityComponentRegistry::cEntityComponentRegistry()
{
}

///////////////////////////////////////

cEntityComponentRegistry::~cEntityComponentRegistry()
{
   RevokeAll(); // Will be done in Term() usually, but unit tests will not typically call Term()
}

////////////////////////////////////////

tResult cEntityComponentRegistry::Init()
{
   RegisterBuiltInComponents();
   return S_OK;
}

///////////////////////////////////////

tResult cEntityComponentRegistry::Term()
{
   RevokeAll();
   return S_OK;
}

///////////////////////////////////////

void cEntityComponentRegistry::RevokeAll()
{
   tComponentFactoryContainer::iterator
      iter = m_componentFactoryContainer.begin(),
      end = m_componentFactoryContainer.end();
   for (; iter != end; ++iter)
   {
      SafeRelease(iter->pFactory);
   }
   m_componentFactoryContainer.clear();
}

///////////////////////////////////////

tResult cEntityComponentRegistry::RegisterComponentFactory(const tChar * pszComponent,
                                                           IEntityComponentFactory * pFactory)
{
   if (pszComponent == NULL || pFactory == NULL)
   {
      return E_POINTER;
   }

   sRegisteredComponentFactory factory;
   factory.name.assign(pszComponent);
   factory.cid = 0;
   factory.pFactory = pFactory;

   pair<tComponentFactoryContainer::iterator, bool> result =
      m_componentFactoryContainer.insert(factory);
   if (result.second)
   {
      pFactory->AddRef();
      return S_OK;
   }

   WarnMsg1("Failed to register entity component factory \"%s\"\n", pszComponent);
   return E_FAIL;
}

///////////////////////////////////////

tResult cEntityComponentRegistry::RevokeComponentFactory(const tChar * pszComponent)
{
   if (pszComponent == NULL)
   {
      return E_POINTER;
   }

   tComponentFactoryContainer::index<name>::type & nameIndex = m_componentFactoryContainer.get<name>();
   tComponentFactoryContainer::iterator f = nameIndex.find(pszComponent);
   if (f == nameIndex.end())
   {
      return S_FALSE;
   }

   f->pFactory->Release();
   m_componentFactoryContainer.erase(f);
   return S_OK;
}

///////////////////////////////////////

tResult cEntityComponentRegistry::CreateComponent(const tChar * pszComponent,
                                                  const TiXmlElement * pTiXmlElement,
                                                  IEntity * pEntity,
                                                  IEntityComponent * * ppComponent)
{
   if (pszComponent == NULL || pEntity == NULL || ppComponent == NULL)
   {
      return E_POINTER;
   }

   cAutoIPtr<IEntityComponentFactory> pFactory;
   if (FindFactory(pszComponent, &pFactory) != S_OK)
   {
      return E_FAIL;
   }

   return pFactory->CreateComponent(pTiXmlElement, pEntity, ppComponent);
}

///////////////////////////////////////

tResult cEntityComponentRegistry::CreateComponent(const tChar * pszComponent,
                                                  IEntity * pEntity, IEntityComponent * * ppComponent)
{
   if (pszComponent == NULL || ppComponent == NULL)
   {
      return E_POINTER;
   }

   cAutoIPtr<IEntityComponentFactory> pFactory;
   if (FindFactory(pszComponent, &pFactory) != S_OK)
   {
      return E_FAIL;
   }

   return pFactory->CreateComponent(NULL, pEntity, ppComponent);
}

///////////////////////////////////////

tResult cEntityComponentRegistry::CreateComponent(const TiXmlElement * pTiXmlElement,
                                                  IEntity * pEntity, IEntityComponent * * ppComponent)
{
   if (pTiXmlElement == NULL)
   {
      return E_POINTER;
   }

#ifdef _UNICODE
   cMultiVar temp(pTiXmlElement->Value());
   const wchar_t * pszComponent = temp.ToWideString();
#else
   const char * pszComponent = pTiXmlElement->Value();
#endif

   return CreateComponent(pszComponent, pTiXmlElement, pEntity, ppComponent);
}

///////////////////////////////////////

tResult cEntityComponentRegistry::FindFactory(const tChar * pszComponent, IEntityComponentFactory * * ppFactory)
{
   Assert(pszComponent != NULL);
   Assert(ppFactory != NULL);

   tComponentFactoryContainer::index<name>::type & nameIndex = m_componentFactoryContainer.get<name>();
   tComponentFactoryContainer::iterator f = nameIndex.find(pszComponent);
   if (f == nameIndex.end())
   {
      return E_FAIL;
   }

   *ppFactory = CTAddRef(f->pFactory);
   return S_OK;
}

///////////////////////////////////////

tResult EntityComponentRegistryCreate()
{
   cAutoIPtr<IEntityComponentRegistry> p(static_cast<IEntityComponentRegistry*>(new cEntityComponentRegistry));
   if (!p)
   {
      return E_OUTOFMEMORY;
   }
   return RegisterGlobalObject(IID_IEntityComponentRegistry, p);
}

///////////////////////////////////////////////////////////////////////////////

#ifdef HAVE_UNITTESTPP

extern tResult TestEntityCreate(IEntity * * ppEntity);

namespace
{
   class cTestComponent : public cComObject<IMPLEMENTS(IEntityComponent)>
   {
   public:
      static const tChar gm_name[];
      static const tEntityComponentID gm_cid;
   };
   const tChar cTestComponent::gm_name[] = _T("TestComponent");
   const tEntityComponentID cTestComponent::gm_cid = GenerateEntityComponentID(gm_name);

   template <class T>
   class cTestComponentFactory : public cComObject<IMPLEMENTS(IEntityComponentFactory)>
   {
   public:
      tResult CreateComponent(const TiXmlElement *, IEntity *, IEntityComponent * * ppComponent)
      {
         *ppComponent = static_cast<IEntityComponent*>(new T);
         return S_OK;
      }
   };
}

TEST(EntityComponentRegisterRevoke)
{
   cAutoIPtr<IEntityComponentRegistry> pECR(static_cast<IEntityComponentRegistry*>(new cEntityComponentRegistry));

   cAutoIPtr<IEntityComponentFactory> pECF(static_cast<IEntityComponentFactory*>(new cTestComponentFactory<cTestComponent>));

   {
      cAutoIPtr<IEntity> pEntity;
      CHECK_EQUAL(S_OK, TestEntityCreate(&pEntity));
      cAutoIPtr<IEntityComponent> pComponent;
      CHECK(FAILED(pECR->CreateComponent(cTestComponent::gm_name, pEntity, &pComponent)));
   }

   CHECK_EQUAL(S_FALSE, pECR->RevokeComponentFactory(cTestComponent::gm_name));

   CHECK_EQUAL(S_OK, pECR->RegisterComponentFactory(cTestComponent::gm_name, pECF));

   {
      cAutoIPtr<IEntity> pEntity;
      CHECK_EQUAL(S_OK, TestEntityCreate(&pEntity));
      cAutoIPtr<IEntityComponent> pComponent;
      CHECK_EQUAL(S_OK, pECR->CreateComponent(cTestComponent::gm_name, pEntity, &pComponent));
   }

   CHECK_EQUAL(S_OK, pECR->RevokeComponentFactory(cTestComponent::gm_name));
   CHECK_EQUAL(S_FALSE, pECR->RevokeComponentFactory(cTestComponent::gm_name));
}

#endif

///////////////////////////////////////////////////////////////////////////////

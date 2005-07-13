///////////////////////////////////////////////////////////////////////////////
// $Id$

#ifndef INCLUDED_RESOURCEMANAGER_H
#define INCLUDED_RESOURCEMANAGER_H

#include "resourceapi.h"
#include "filepath.h"
#include "globalobjdef.h"

#include <map>

#define ZLIB_WINAPI
#include <unzip.h>

#ifdef _MSC_VER
#pragma once
#endif

class cFileSpec;

const uint kNoIndex = ~0;
const ulong kNoIndexL = ~0;

///////////////////////////////////////////////////////////////////////////////
//
// CLASS: cResourceManager
//

class cResourceManager : public cComObject2<IMPLEMENTS(IResourceManager), IMPLEMENTS(IGlobalObject)>
{
public:
   cResourceManager();
   virtual ~cResourceManager();

   DECLARE_NAME_STRING(kResourceManagerName)
   DECLARE_NO_CONSTRAINTS()

   virtual tResult Init();
   virtual tResult Term();

   // IResourceManager methods
   virtual tResult AddDirectory(const tChar * pszDir);
   virtual tResult AddDirectoryTreeFlattened(const tChar * pszDir);
   virtual tResult AddArchive(const tChar * pszArchive);
   virtual tResult LoadUncached(const tChar * pszName, tResourceType type, void * param, void * * ppData, ulong * pDataSize);
   virtual tResult Load(const tChar * pszName, tResourceType type, void * param, void * * ppData);
   virtual tResult Unload(const tChar * pszName, tResourceType type);
   virtual tResult Lock(const tChar * pszName, tResourceType type);
   virtual tResult Unlock(const tChar * pszName, tResourceType type);
   virtual tResult RegisterFormat(tResourceType type,
                                  tResourceType typeDepend,
                                  const tChar * pszExtension,
                                  tResourceLoad pfnLoad,
                                  tResourcePostload pfnPostload,
                                  tResourceUnload pfnUnload);

private:
   struct sFormat;
   struct sResource;

   sResource * FindResourceWithFormat(const tChar * pszName, tResourceType type, sFormat * pFormat);
   tResult DoLoadFromFile(const cFileSpec & file, sFormat * pFormat, void * param, ulong * pDataSize, void * * ppData);
   tResult DoLoadFromArchive(uint archiveId, ulong offset, ulong index, sFormat * pFormat, void * param, ulong * pDataSize, void * * ppData);
   tResult DoLoadFromReader(IReader * pReader, sFormat * pFormat, ulong dataSize, void * param, void * * ppData);

   uint DeduceFormats(const tChar * pszName, tResourceType type, sFormat * * ppFormats, uint nMaxFormats);

   uint GetExtensionId(const tChar * pszExtension);
   uint GetExtensionIdForName(const tChar * pszName);
   uint GetDirectoryId(const tChar * pszDir);
   uint GetArchiveId(const tChar * pszArchive);

   std::vector<cStr> m_extensions;
   std::vector<cFilePath> m_dirs;

   struct sArchiveInfo
   {
      cStr archive;
      unzFile handle;
   };
   typedef std::vector<sArchiveInfo> tArchives;
   tArchives m_archives;

   struct sFormat
   {
      tResourceType type;
      tResourceType typeDepend; // if not NULL, this format loads 'type' by converting from 'typeDepend'
      uint extensionId;
      tResourceLoad pfnLoad;
      tResourcePostload pfnPostload;
      tResourceUnload pfnUnload;
   };
   typedef std::vector<sFormat> tFormats;
   tFormats m_formats;

   typedef std::multimap<tResourceType, uint> tExtsForType;
   tExtsForType m_extsForType;

   struct sResource
   {
      sResource()
      {
         extensionId = kNoIndex;
         pFormat = NULL;
         dirId = archiveId = kNoIndex;
         offset = index = kNoIndexL;
         lockCount = 0;
         pData = NULL;
         dataSize = 0;
      }
      cStr name;
      uint extensionId;
      sFormat * pFormat;
      uint dirId;
      uint archiveId;
      ulong offset;
      ulong index;
      ulong lockCount;
      void * pData;
      ulong dataSize;
   };
   typedef std::vector<sResource> tResources;
   tResources m_resources;
};


///////////////////////////////////////////////////////////////////////////////

#endif // !INCLUDED_RESOURCEMANAGER_H

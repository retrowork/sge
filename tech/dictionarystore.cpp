///////////////////////////////////////////////////////////////////////////////
// $Id$

#include "stdhdr.h"

#include "dictionarystore.h"

#include "tech/readwriteapi.h"

#ifdef HAVE_UNITTESTPP
#include "UnitTest++.h"
#endif

#include <locale>
#include <list>

#include "tech/dbgalloc.h" // must be last header

using namespace std;

///////////////////////////////////////////////////////////////////////////////

LOG_DEFINE_CHANNEL(DictionaryStore);

static const tChar kCommentChar = _T('#');
static const tChar kSepChar = _T('=');
static const tChar kWhitespace[] = _T(" \t\r\n");

///////////////////////////////////////////////////////////////////////////////

static bool SplitString(const tChar * psz, tChar split, cStr * pLeft, cStr * pRight)
{
   Assert(psz != NULL);
   const tChar * pszSplit = _tcsrchr(psz, split);
   if (pszSplit != NULL)
   {
      if (pLeft != NULL)
      {
         *pLeft = cStr(psz, pszSplit - psz).c_str();
      }
      if (pRight != NULL)
      {
         *pRight = pszSplit + 1;
      }
      return true;
   }
   return false;
}

inline void TrimSpace(cStr * pStr)
{
   TrimLeadingSpace(pStr);
   TrimTrailingSpace(pStr);
}

bool ParseDictionaryLine(const tChar * psz, cStr * pKey, cStr * pValue, cStr * pComment)
{
   Assert(psz != NULL);
   Assert(pKey != NULL);
   Assert(pValue != NULL);
   cStr temp;
   if (SplitString(psz, kCommentChar, &temp, pComment))
   {
      psz = temp.c_str();
      if (pComment != NULL)
      {
         TrimSpace(pComment);
      }
   }
   if (SplitString(psz, kSepChar, pKey, pValue))
   {
      TrimSpace(pKey);
      TrimSpace(pValue);
      return true;
   }
   else
   {
      *pKey = psz;
      cStr::size_type index = pKey->find_first_of(kWhitespace);
      if (index == cStr::npos)
      {
         return true;
      }
      pKey->clear();
   }
   return false;
}


///////////////////////////////////////////////////////////////////////////////
//
// CLASS: cDictionaryTextStore
//

///////////////////////////////////////

cDictionaryTextStore::cDictionaryTextStore(const cFileSpec & file)
 : m_file(file)
 , m_pReader(NULL)
{
}

///////////////////////////////////////

cDictionaryTextStore::cDictionaryTextStore(IReader * pReader)
 : m_file(_T(""))
 , m_pReader(CTAddRef(pReader))
{
}

///////////////////////////////////////

tResult cDictionaryTextStore::Load(IDictionary * pDictionary)
{
   Assert(pDictionary != NULL);

   tResult result = E_FAIL;

   cAutoIPtr<IReader> pReader;

   if (!!m_pReader)
   {
      pReader = m_pReader;
   }
   else
   {
      result = FileReaderCreate(m_file, kFileModeBinary, &pReader);
   }

   if (!!pReader)
   {
      cStr line;
      while (pReader->ReadLine(&line) == S_OK)
      {
         cStr key, value, comment;
         if (ParseDictionaryLine(line.c_str(), &key, &value, &comment)
             && !key.empty() && !value.empty())
         {
            DebugMsgEx2(DictionaryStore, "Read dictionary entry '%s' = '%s'\n", key.c_str(), value.c_str());
            pDictionary->Set(key.c_str(), value.c_str());
         }
         line.clear();
      }
      result = S_OK;
   }

   return result;
}

///////////////////////////////////////

tResult cDictionaryTextStore::Save(IDictionary * pDictionary)
{
   if (m_file.IsEmpty())
   {
      return E_FAIL;
   }

   cAutoIPtr<IWriter> pWriter;
   if (FileWriterCreate(m_file, kFileModeText, &pWriter) != S_OK)
   {
      return E_FAIL;
   }

   tResult result = S_FALSE;

   cStr temp;
   list<cStr> keys;
   if (pDictionary->GetKeys(&keys) == S_OK)
   {
      list<cStr>::iterator iter = keys.begin(), end = keys.end();
      for (; iter != end; ++iter)
      {
         cStr value;
         tPersistence persist;
         if (pDictionary->Get(iter->c_str(), &value, &persist) == S_OK)
         {
            if (persist == kPermanent)
            {
               Sprintf(&temp, _T("%s=%s\n"), iter->c_str(), value.c_str());
               pWriter->Write(temp.c_str(), temp.length() * sizeof(cStr::value_type));
            }
         }
      }

      result = S_OK;
   }

   return result;
}

///////////////////////////////////////

tResult cDictionaryTextStore::MergeSave(IDictionary * pDictionary)
{
   return E_NOTIMPL; // TODO
}

///////////////////////////////////////////////////////////////////////////////

IDictionaryStore * DictionaryStoreCreate(const cFileSpec & file)
{
   return static_cast<IDictionaryStore *>(new cDictionaryTextStore(file));
}

///////////////////////////////////////////////////////////////////////////////

IDictionaryStore * DictionaryStoreCreate(IReader * pReader)
{
   return static_cast<IDictionaryStore *>(new cDictionaryTextStore(pReader));
}


///////////////////////////////////////////////////////////////////////////////
//
// CLASS: cDictionaryIniStore
//

///////////////////////////////////////

static bool ParseIniSectionLine(const tChar * pszBuffer, cStr * pSection, cStr * pComment)
{
   if (pszBuffer == NULL || pSection == NULL)
   {
      return false;
   }

   const tChar * pszCommentStart = _tcsrchr(pszBuffer, kCommentChar);
   if (pszCommentStart != NULL)
   {
      if (pComment != NULL)
      {
         *pComment = _tcsinc(pszCommentStart);
         TrimLeadingSpace(pComment);
         TrimTrailingSpace(pComment);
      }
   }

   const tChar * pszSectionStart = _tcschr(pszBuffer, _T('['));
   const tChar * pszSectionEnd = _tcschr(pszBuffer, _T(']'));

   if ((pszSectionStart != NULL) && (pszSectionEnd != NULL) && (pszSectionEnd > pszSectionStart))
   {
      pszSectionStart = _tcsinc(pszSectionStart);
      *pSection = cStr(pszSectionStart, pszSectionEnd - pszSectionStart);
      TrimLeadingSpace(pSection);
      TrimTrailingSpace(pSection);
      return true;
   }

   return false;
}

///////////////////////////////////////

cDictionaryIniStore::cDictionaryIniStore(const cFileSpec & file,
                                         const tChar * pszSection)
 : m_file(file)
 , m_section((pszSection != NULL) ? pszSection : _T(""))
{
}

///////////////////////////////////////

tResult cDictionaryIniStore::Load(IDictionary * pDictionary)
{
   if (pDictionary == NULL)
   {
      return E_POINTER;
   }

   cAutoIPtr<IReader> pReader;
   if (FileReaderCreate(m_file, kFileModeText, &pReader) != S_OK)
   {
      return E_FAIL;
   }

   cStr line;
   bool bInSection = false;

   while (pReader->ReadLine(&line) == S_OK)
   {
      cStr section;
      if (ParseIniSectionLine(line.c_str(), &section, NULL))
      {
         bInSection = (_tcsicmp(section.c_str(), m_section.c_str()) == 0);
      }
      else if (bInSection)
      {
         cStr key, value, comment;
         if (ParseDictionaryLine(line.c_str(), &key, &value, &comment)
             && !key.empty() && !value.empty())
         {
            DebugMsgEx2(DictionaryStore, "Read dictionary entry '%s' = '%s'\n", key.c_str(), value.c_str());
            pDictionary->Set(key.c_str(), value.c_str());
         }
      }
   }

   return S_OK;
}

///////////////////////////////////////

tResult cDictionaryIniStore::Save(IDictionary * pDictionary)
{
   return E_NOTIMPL; // TODO
}

///////////////////////////////////////

tResult cDictionaryIniStore::MergeSave(IDictionary * pDictionary)
{
   return E_NOTIMPL; // TODO
}

///////////////////////////////////////

tResult DictionaryIniStoreCreate(const cFileSpec & file,
                                 const tChar * pszSection,
                                 IDictionaryStore * * ppStore)
{
   if (pszSection == NULL || ppStore == NULL)
   {
      return E_POINTER;
   }

   cAutoIPtr<IDictionaryStore> pStore(static_cast<IDictionaryStore *>(new cDictionaryIniStore(file, pszSection)));
   if (!pStore)
   {
      return E_OUTOFMEMORY;
   }

   return pStore.GetPointer(ppStore);
}

///////////////////////////////////////////////////////////////////////////////

#ifdef HAVE_UNITTESTPP

///////////////////////////////////////

TEST(SplitString)
{
   cStr l, r;
   CHECK(SplitString("key=value", '=', &l, &r));
   CHECK(l == "key");
   CHECK(r == "value");
}

///////////////////////////////////////

TEST(ParseDictionaryLine)
{
   cStr key, value, comment;

   key.clear();
   value.clear();
   comment.clear();
   CHECK(ParseDictionaryLine("key=value", &key, &value, &comment));
   CHECK(key == "key");
   CHECK(value == "value");
   CHECK(comment.empty());

   key.clear();
   value.clear();
   comment.clear();
   CHECK(ParseDictionaryLine("  key  =  value  # this is a comment   ", &key, &value, &comment));
   CHECK(key == "key");
   CHECK(value == "value");
   CHECK(comment == "this is a comment");

   key.clear();
   value.clear();
   comment.clear();
   CHECK(ParseDictionaryLine("definition", &key, &value, &comment));
   CHECK(key == "definition");
   CHECK(value.empty());
   CHECK(comment.empty());
}

///////////////////////////////////////

TEST(ParseIniSectionLine)
{
   // without comment
   {
      cStr section, comment;
      CHECK(ParseIniSectionLine(_T("[IniSection]"), &section, &comment));
      CHECK(section.compare(_T("IniSection")) == 0);
      CHECK(comment.empty());
   }

   // with comment
   {
      cStr section, comment;
      CHECK(ParseIniSectionLine(_T("[IniSection]   # the comment"), &section, &comment));
      CHECK(section.compare(_T("IniSection")) == 0);
      CHECK(comment.compare(_T("the comment")) == 0);
   }

   // with lots of extra whitespace
   {
      cStr section, comment;
      CHECK(ParseIniSectionLine(_T("  [   IniSection  ]   #   the comment   "), &section, &comment));
      CHECK(section.compare(_T("IniSection")) == 0);
      CHECK(comment.compare(_T("the comment")) == 0);
   }
}

///////////////////////////////////////////////////////////////////////////////

#endif // HAVE_UNITTESTPP

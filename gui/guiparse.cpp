///////////////////////////////////////////////////////////////////////////////
// $Id$

#include "stdhdr.h"

#include "guiparse.h"
#include "guistrings.h"

#include "gui/guistyleapi.h"

#ifdef HAVE_UNITTESTPP
#include "UnitTest++.h"
#endif

#include <boost/lexical_cast.hpp>
#include <boost/tokenizer.hpp>

#include <vector>

#include "tech/dbgalloc.h" // must be last header

using namespace boost;
using namespace std;

///////////////////////////////////////////////////////////////////////////////

const tChar * SkipSpaceFwd(const tChar * psz)
{
   Assert(psz != NULL);
   while (_istspace(*psz))
      psz++;
   return psz;
}

const tChar * SkipSpaceBack(const tChar * psz)
{
   Assert(psz != NULL);
   while (_istspace(*(psz - 1)))
      psz--;
   return psz;
}

///////////////////////////////////////////////////////////////////////////////

const tChar * SkipSingleComment(const tChar * psz)
{
   Assert(psz != NULL);

   if (*psz == _T('/'))
   {
      if (*(psz+1) == _T('*'))
      {
         const tChar * pszStop = _tcsstr(psz+2, _T("*/"));
         if (pszStop != NULL)
         {
            psz = pszStop + 2;
         }
         else
         {
            psz = psz + _tcslen(psz);
         }
      }
      else if (*(psz+1) == _T('/'))
      {
         const tChar * pszStop = _tcschr(psz+2, _T('\n'));
         if (pszStop != NULL)
         {
            psz = pszStop + 1;
         }
         else
         {
            psz = psz + _tcslen(psz);
         }
      }
   }

   return psz;
}

///////////////////////////////////////////////////////////////////////////////

tResult GUIParseStyleDimension(const tChar * psz, int * pDimension, eGUIDimensionSpec * pSpec)
{
   if (psz == NULL || pDimension == NULL || pSpec == NULL)
   {
      return E_POINTER;
   }

   int dimension = -1;
   tChar szSpec[32] = {0};

   // The 32 must match the size of szSpec. Not using "%*s" and passing in the buffer
   // length because then scanf won't recognize a single percent character as a valid
   // value for the string. That is, this case will not parse as desired: "25%".
#if _MSC_VER > 1310
   int nParsed = _stscanf_s(psz, _T("%d%32s"), &dimension, szSpec, sizeof(szSpec));
#else
   int nParsed = _stscanf(psz, _T("%d%32s"), &dimension, szSpec);
#endif
   if (nParsed == 2)
   {
      if (_tcscmp(szSpec, _T("%")) == 0)
      {
         *pDimension = dimension;
         *pSpec = kGUIDimensionPercent;
         return S_OK;
      }
      else if (_tcsicmp(szSpec, _T("px")) == 0)
      {
         *pDimension = dimension;
         *pSpec = kGUIDimensionPixels;
         return S_OK;
      }
   }
   else if (nParsed == 1)
   {
      *pDimension = dimension;
      *pSpec = kGUIDimensionPixels;
      return S_OK;
   }

   return E_FAIL;
}

///////////////////////////////////////////////////////////////////////////////

tResult GUIParseStyleFontSize(const tChar * psz, int * pSize, eGUIFontSizeType * pSizeType)
{
   if (psz == NULL || pSize == NULL || pSizeType == NULL)
   {
      return E_POINTER;
   }

   int size = 0;
   tChar szType[32] = {0};

   // The 32 must match the size of szSpec. Not using "%*s" and passing in the buffer
   // length because then scanf won't recognize a single percent character as a valid
   // value for the string. That is, this case will not parse as desired: "25%".
#if _MSC_VER > 1310
   int nParsed = _stscanf_s(psz, _T("%d%32s"), &size, szType, sizeof(szType));
#else
   int nParsed = _stscanf(psz, _T("%d%32s"), &size, szType);
#endif
   if (nParsed == 2)
   {
      static const struct
      {
         const tChar * pszType;
         eGUIFontSizeType type;
      }
      sizeTypeTable[] =
      {
         { _T("%"),     kGUIFontSizePercent },
         { _T("em"),    kGUIFontSizeEm },
         { _T("ex"),    kGUIFontSizeEx },
         { _T("px"),    kGUIFontSizePixels },
         { _T("in"),    kGUIFontSizeInches },
         { _T("cm"),    kGUIFontSizeCentimeters },
         { _T("mm"),    kGUIFontSizeMillimeters },
         { _T("pt"),    kGUIFontSizePoints },
         { _T("pc"),    kGUIFontSizePicas },
      };
      for (int i = 0; i < _countof(sizeTypeTable); i++)
      {
         if (_tcscmp(szType, sizeTypeTable[i].pszType) == 0)
         {
            *pSize = size;
            *pSizeType = sizeTypeTable[i].type;
            return S_OK;
         }
      }
   }
   else if (nParsed == 0)
   {
      static const struct
      {
         const tChar * pszMatch;
         int size;
         eGUIFontSizeType type;
      }
      absRelTable[] =
      {
         { _T("xx-small"),    kGUIFontXXSmall,     kGUIFontSizeAbsolute },
         { _T("x-small"),     kGUIFontXSmall,      kGUIFontSizeAbsolute },
         { _T("small"),       kGUIFontSmall,       kGUIFontSizeAbsolute },
         { _T("medium"),      kGUIFontMedium,      kGUIFontSizeAbsolute },
         { _T("large"),       kGUIFontLarge,       kGUIFontSizeAbsolute },
         { _T("x-large"),     kGUIFontXLarge,      kGUIFontSizeAbsolute },
         { _T("xx-large"),    kGUIFontXXLarge,     kGUIFontSizeAbsolute },
         { _T("larger"),      kGUIFontSizeLarger,  kGUIFontSizeRelative },
         { _T("smaller"),     kGUIFontSizeSmaller, kGUIFontSizeRelative },
      };
      for (int i = 0; i < _countof(absRelTable); i++)
      {
         if (_tcscmp(psz, absRelTable[i].pszMatch) == 0)
         {
            *pSize = absRelTable[i].size;
            *pSizeType = absRelTable[i].type;
            return S_OK;
         }
      }
   }

   return E_FAIL;
}

///////////////////////////////////////////////////////////////////////////////

tResult GUIParseColor(const tChar * pszColor, tGUIColor * pColor)
{
   if (pszColor == NULL || pColor == NULL)
   {
      return E_POINTER;
   }
   static const struct sNamedColor
   {
      sNamedColor(const tChar * p, const tGUIColor & c) : pszName(p), color(c) {}
      const tChar * pszName;
      tGUIColor color;
   }
   g_namedColorTable[] =
   {
      sNamedColor(kValueColorBlack,       GUIStandardColors::Black),
      sNamedColor(kValueColorRed,         GUIStandardColors::Red),
      sNamedColor(kValueColorGreen,       GUIStandardColors::Green),
      sNamedColor(kValueColorYellow,      GUIStandardColors::Yellow),
      sNamedColor(kValueColorBlue,        GUIStandardColors::Blue),
      sNamedColor(kValueColorMagenta,     GUIStandardColors::Magenta),
      sNamedColor(kValueColorCyan,        GUIStandardColors::Cyan),
      sNamedColor(kValueColorDarkGray,    GUIStandardColors::DarkGray),
      sNamedColor(kValueColorGray,        GUIStandardColors::Gray),
      sNamedColor(kValueColorLightGray,   GUIStandardColors::LightGray),
      sNamedColor(kValueColorWhite,       GUIStandardColors::White),
   };

   vector<float> floats;

   try
   {
      cStr color(pszColor);
      typedef tokenizer<char_separator<tChar> > tokenizer;
      static const char_separator<tChar> sep(_T("(),"));
      tokenizer tokens(color, sep);
      tokenizer::iterator iter = tokens.begin(), end = tokens.end();
      for (; iter != end; ++iter)
      {
         float f = lexical_cast<float>(*iter);
         floats.push_back(f);
      }
   }
   catch (const bad_lexical_cast &)
   {
   }

   if (floats.size() == 3)
   {
      if ((floats[0] > 1) && (floats[1] > 1) && (floats[2] > 1))
      {
         *pColor = tGUIColor(floats[0]/255.0f, floats[1]/255.0f, floats[2]/255.0f);
      }
      else
      {
         *pColor = tGUIColor(floats[0], floats[1], floats[2]);
      }
      return S_OK;
   }
   else if (floats.size() == 4)
   {
      if ((floats[0] > 1) && (floats[1] > 1) && (floats[2] > 1) && (floats[3] > 1))
      {
         *pColor = tGUIColor(floats[0]/255.0f, floats[1]/255.0f, floats[2]/255.0f, floats[3]/255.0f);
      }
      else
      {
         *pColor = tGUIColor(floats[0], floats[1], floats[2], floats[3]);
      }
      return S_OK;
   }
   else
   {
      for (int i = 0; i < _countof(g_namedColorTable); ++i)
      {
         if (_tcsicmp(g_namedColorTable[i].pszName, pszColor) == 0)
         {
            *pColor = g_namedColorTable[i].color;
            return S_OK;
         }
      }
   }
   return S_FALSE;
}

///////////////////////////////////////////////////////////////////////////////

tResult GUIParseBool(const tChar * pszBool, bool * pBool)
{
   if (pszBool == NULL || pBool == NULL)
   {
      return E_POINTER;
   }

   // empty string means false
   if (*pszBool == 0)
   {
      *pBool = false;
      return S_OK;
   }

   int n = 0;
#if _MSC_VER > 1310
   if (_stscanf_s(pszBool, _T("%d"), &n) == 1)
#else
   if (_stscanf(pszBool, _T("%d"), &n) == 1)
#endif
   {
      *pBool = (n != 0);
      return S_OK;
   }

   if (_tcsicmp(pszBool, _T("true")) == 0)
   {
      *pBool = true;
      return S_OK;
   }
   else if (_tcsicmp(pszBool, _T("false")) == 0)
   {
      *pBool = false;
      return S_OK;
   }

   return E_INVALIDARG;
}

///////////////////////////////////////////////////////////////////////////////

#ifdef HAVE_UNITTESTPP

///////////////////////////////////////

TEST(GUIParseDimension)
{
   int dim;
   eGUIDimensionSpec spec;

   CHECK(GUIParseStyleDimension("150", &dim, &spec) == S_OK);
   CHECK((dim == 150) && (spec == kGUIDimensionPixels));
   CHECK(GUIParseStyleDimension("250px", &dim, &spec) == S_OK);
   CHECK((dim == 250) && (spec == kGUIDimensionPixels));
   CHECK(GUIParseStyleDimension("350 px", &dim, &spec) == S_OK);
   CHECK((dim == 350) && (spec == kGUIDimensionPixels));
   CHECK(GUIParseStyleDimension("25%", &dim, &spec) == S_OK);
   CHECK((dim == 25) && (spec == kGUIDimensionPercent));
   CHECK(GUIParseStyleDimension("55  %", &dim, &spec) == S_OK);
   CHECK((dim == 55) && (spec == kGUIDimensionPercent));

   // try a value too big to fit in a 32-bit int--should get truncated
   CHECK(GUIParseStyleDimension("3232323232323232323232323232323232", &dim, &spec) == S_OK);
   CHECK((dim == 347068064) && (spec == kGUIDimensionPixels));

   // try a completely bogus value
   CHECK(FAILED(GUIParseStyleDimension("there is no way to parse this as a style dimension", &dim, &spec)));

   // try a value with a small enough string but still bogus
   CHECK(FAILED(GUIParseStyleDimension("still no way", &dim, &spec)));

   // only partially bogus--valid integer but invalid string after it
   CHECK(FAILED(GUIParseStyleDimension("99 units", &dim, &spec)));
}

///////////////////////////////////////

TEST(GUIParseFontSize)
{
   int size;
   eGUIFontSizeType type;

   CHECK(GUIParseStyleFontSize(NULL, NULL, NULL) == E_POINTER);
   CHECK(GUIParseStyleFontSize(_T("test"), NULL, NULL) == E_POINTER);
   CHECK(GUIParseStyleFontSize(_T("test"), &size, NULL) == E_POINTER);

   CHECK(GUIParseStyleFontSize("xx-small", &size, &type) == S_OK);
   CHECK_EQUAL(kGUIFontXXSmall, size);
   CHECK_EQUAL(kGUIFontSizeAbsolute, type);

   CHECK(GUIParseStyleFontSize("larger", &size, &type) == S_OK);
   CHECK_EQUAL(kGUIFontSizeLarger, size);
   CHECK_EQUAL(kGUIFontSizeRelative, type);

   CHECK(GUIParseStyleFontSize("78%", &size, &type) == S_OK);
   CHECK_EQUAL(78, size);
   CHECK_EQUAL(kGUIFontSizePercent, type);

   CHECK(GUIParseStyleFontSize("14pt", &size, &type) == S_OK);
   CHECK_EQUAL(14, size);
   CHECK_EQUAL(kGUIFontSizePoints, type);

   CHECK(GUIParseStyleFontSize("9mm", &size, &type) == S_OK);
   CHECK_EQUAL(9, size);
   CHECK_EQUAL(kGUIFontSizeMillimeters, type);
}

///////////////////////////////////////

TEST(GUIParseColor)
{
   tGUIColor color;
   CHECK(GUIParseColor("black", &color) == S_OK);
   CHECK(color == GUIStandardColors::Black);
   CHECK(GUIParseColor("red", &color) == S_OK);
   CHECK(color == GUIStandardColors::Red);
   CHECK(GUIParseColor("green", &color) == S_OK);
   CHECK(color == GUIStandardColors::Green);
   CHECK(GUIParseColor("yellow", &color) == S_OK);
   CHECK(color == GUIStandardColors::Yellow);
   CHECK(GUIParseColor("blue", &color) == S_OK);
   CHECK(color == GUIStandardColors::Blue);
   CHECK(GUIParseColor("magenta", &color) == S_OK);
   CHECK(color == GUIStandardColors::Magenta);
   CHECK(GUIParseColor("cyan", &color) == S_OK);
   CHECK(color == GUIStandardColors::Cyan);
   CHECK(GUIParseColor("darkgray", &color) == S_OK);
   CHECK(color == GUIStandardColors::DarkGray);
   CHECK(GUIParseColor("gray", &color) == S_OK);
   CHECK(color == GUIStandardColors::Gray);
   CHECK(GUIParseColor("lightgray", &color) == S_OK);
   CHECK(color == GUIStandardColors::LightGray);
   CHECK(GUIParseColor("white", &color) == S_OK);
   CHECK(color == GUIStandardColors::White);
   CHECK_EQUAL(S_OK, GUIParseColor("1,0,0", &color));
   CHECK(color == GUIStandardColors::Red);
}

///////////////////////////////////////

TEST(GUIParseParseBool)
{
   bool b;
   CHECK(GUIParseBool("1", NULL) == E_POINTER);
   CHECK(GUIParseBool(NULL, &b) == E_POINTER);
   CHECK(GUIParseBool("", &b) == S_OK);
   CHECK(!b);
   CHECK(GUIParseBool("1", &b) == S_OK);
   CHECK(b);
   CHECK(GUIParseBool("0", &b) == S_OK);
   CHECK(!b);
   CHECK(GUIParseBool("true", &b) == S_OK);
   CHECK(b);
   CHECK(GUIParseBool("tRuE", &b) == S_OK);
   CHECK(b);
   CHECK(GUIParseBool("false", &b) == S_OK);
   CHECK(!b);
   CHECK(GUIParseBool("fAlSe", &b) == S_OK);
   CHECK(!b);
   CHECK(GUIParseBool("arbitrary string", &b) == E_INVALIDARG);
}

#endif // HAVE_UNITTESTPP

///////////////////////////////////////////////////////////////////////////////

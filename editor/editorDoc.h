/////////////////////////////////////////////////////////////////////////////
// $Id$

#if !defined(INCLUDED_EDITORDOC_H)
#define INCLUDED_EDITORDOC_H

#include "comtools.h"
#include "editorapi.h"

#include <stack>

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

class cHeightMap;
class cTerrain;
struct sTerrainVertex;

F_DECLARE_INTERFACE(IMaterial);

typedef std::stack<IEditorCommand *> tCommandStack;

/////////////////////////////////////////////////////////////////////////////
//
// CLASS: cEditorDoc
//

class cEditorDoc : public cComObject<IMPLEMENTS(IEditorModel)>
{
protected: // create from serialization only

// Attributes
public:
	cEditorDoc();
	virtual ~cEditorDoc();

   const sTerrainVertex * GetVertexPointer() const;
   size_t GetVertexCount() const;

   IMaterial * AccessMaterial();

   cTerrain * AccessTerrain();

   virtual tResult AddCommand(IEditorCommand * pCommand);

// Operations
public:

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(cEditorDoc)
	public:
	virtual BOOL OnNewDocument();
	virtual void Serialize(CArchive& ar);
	virtual BOOL OnOpenDocument(LPCTSTR lpszPathName);
	virtual BOOL OnSaveDocument(LPCTSTR lpszPathName);
	virtual void DeleteContents();
	//}}AFX_VIRTUAL

// Implementation
public:
#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif

// Generated message map functions
protected:
	//{{AFX_MSG(cEditorDoc)
	afx_msg void OnEditUndo();
	afx_msg void OnUpdateEditUndo(CCmdUI* pCmdUI);
	afx_msg void OnEditRedo();
	afx_msg void OnUpdateEditRedo(CCmdUI* pCmdUI);
	//}}AFX_MSG

private:
   cHeightMap * m_pHeightMap;

   cAutoIPtr<IMaterial> m_pMaterial;

   cTerrain * m_pTerrain;

   tCommandStack m_undoStack, m_redoStack;

   CString m_originalUndoText, m_originalRedoText;
};

////////////////////////////////////////

inline IMaterial * cEditorDoc::AccessMaterial()
{
   return m_pMaterial;
}

////////////////////////////////////////

inline cTerrain * cEditorDoc::AccessTerrain()
{
   return m_pTerrain;
}

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(INCLUDED_EDITORDOC_H)

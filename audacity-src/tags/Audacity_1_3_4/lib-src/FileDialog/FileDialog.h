/**********************************************************************

  Audacity: A Digital Audio Editor

  FileDialog.h

  Leland Lucius

*******************************************************************//**

\class FileDialog
\brief Dialog used to present platform specific "Save As" dialog with
custom controls.

*//*******************************************************************/

#ifndef _FILE_DIALOG_H_
#define _FILE_DIALOG_H_

#if defined(__WXGTK__)
#include "config.h"
#endif

#ifdef WXUSINGDLL
#define DLL_LINKAGE WXEXPORT
#else
#define DLL_LINKAGE
#endif

#include "wx/defs.h"
#include "wx/filedlg.h"

typedef void (*fdCallback)(void *, int);

#if defined(__WXMAC__)
#include "mac/FileDialogPrivate.h"
#elif defined(__WXMSW__)
#include "win/FileDialogPrivate.h"
#elif defined(__WXGTK__) && defined(HAVE_GTK)
#include "gtk/FileDialogPrivate.h"
#else
#include "generic/FileDialogPrivate.h"
#endif

// JKC hack for dll linkage...
//#define FileDialog wxFileDialog

/////////////////////////////////////////////////////////////////////////////
// Name:        filedlg.h
// Purpose:     wxFileDialog base header
// Author:      Robert Roebling
// Modified by: Leland Lucius
// Created:     8/17/99
// Copyright:   (c) Robert Roebling
// RCS-ID:      $Id: FileDialog.h,v 1.8 2007-06-18 16:10:50 jamescrook Exp $
// Licence:     wxWindows licence
//
// Modified for Audacity to support an additional button on Save dialogs
//
/////////////////////////////////////////////////////////////////////////////

//----------------------------------------------------------------------------
// wxFileDialog convenience functions
//----------------------------------------------------------------------------

// File selector - backward compatibility
extern DLL_LINKAGE wxString 
FileSelector(const wxChar *message = wxFileSelectorPromptStr,
             const wxChar *default_path = NULL,
             const wxChar *default_filename = NULL,
             const wxChar *default_extension = NULL,
             const wxChar *wildcard = wxFileSelectorDefaultWildcardStr,
             int flags = 0,
             wxWindow *parent = NULL,
             wxString label = wxEmptyString,
             fdCallback cb = NULL,
             void *cbdata = NULL);

//#define FileSelector wxFileSelector

#endif


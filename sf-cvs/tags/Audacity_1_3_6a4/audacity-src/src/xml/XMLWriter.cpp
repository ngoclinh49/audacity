/**********************************************************************

  Audacity: A Digital Audio Editor

  XMLWriter.cpp

  Leland Lucius

*******************************************************************//**

\class XMLWriter
\brief Base class for XMLFileWriter and XMLStringWriter that provides
the general functionality for creating XML in UTF8 encoding.

*//****************************************************************//**

\class XMLFileWriter
\brief Wrapper to output XML data to files.

*//****************************************************************//**

\class XMLStringWriter
\brief Wrapper to output XML data to strings.

*//*******************************************************************/

#include <wx/defs.h>
#include <wx/ffile.h>
#include <wx/intl.h>

#include <string.h>

#include "../Internat.h"
#include "XMLWriter.h"
#include "XMLTagHandler.h"

///
/// XMLWriter base class
///
XMLWriter::XMLWriter()
{
   mDepth = 0;
   mInTag = false;
   mHasKids.Add(false);
}

XMLWriter::~XMLWriter()
{
}

void XMLWriter::StartTag(const wxString &name)
{
   int i;

   if (mInTag) {
      Write(wxT(">\n"));
      mInTag = false;
   }
   
   for (i = 0; i < mDepth; i++) {
      Write(wxT("\t"));
   }

   Write(wxString::Format(wxT("<%s"), name.c_str()));

   mTagstack.Insert(name, 0);
   mHasKids[0] = true;
   mHasKids.Insert(false, 0);
   mDepth++;
   mInTag = true;
}

void XMLWriter::EndTag(const wxString &name)
{
   int i;

   if (mTagstack.GetCount() > 0) {
      if (mTagstack[0] == name) {
         if (mHasKids[1]) {  // There will always be at least 2 at this point
            if (mInTag) {
               Write(wxT("/>\n"));
            }
            else {               
               for (i = 0; i < mDepth - 1; i++) {
                  Write(wxT("\t"));
               }
               Write(wxString::Format(wxT("</%s>\n"), name.c_str()));
            }
         }
         else {
            Write(wxT(">\n"));
         }
         mTagstack.RemoveAt(0);
         mHasKids.RemoveAt(0);
      }
   }

   mDepth--;
   mInTag = false;
}

void XMLWriter::WriteAttr(const wxString &name, const wxString &value)
{
   Write(wxString::Format(wxT(" %s=\"%s\""),
      name.c_str(),
      XMLTagHandler::XMLEsc(value).c_str()));
}

void XMLWriter::WriteAttr(const wxChar *name, const wxChar *value)
{
   WriteAttr(wxString(name), wxString(value));
}

void XMLWriter::WriteAttr(const wxString &name, const wxChar *value)
{
   WriteAttr(name, wxString(value));
}

void XMLWriter::WriteAttr(const wxChar *name, const wxString &value)
{
   WriteAttr(wxString(name), value);
}

void XMLWriter::WriteAttr(const wxString &name, int value)
{
   Write(wxString::Format(wxT(" %s=\"%d\""),
      name.c_str(),
      value));
}

void XMLWriter::WriteAttr(const wxChar *name, int value)
{
   WriteAttr(wxString(name), value);
}

void XMLWriter::WriteAttr(const wxString &name, bool value)
{
   Write(wxString::Format(wxT(" %s=\"%d\""),
      name.c_str(),
      value));
}

void XMLWriter::WriteAttr(const wxChar *name, bool value)
{
   WriteAttr(wxString(name), value);
}

void XMLWriter::WriteAttr(const wxString &name, long value)
{
   Write(wxString::Format(wxT(" %s=\"%ld\""),
      name.c_str(),
      value));
}

void XMLWriter::WriteAttr(const wxChar *name, long value)
{
   WriteAttr(wxString(name), value);
}

void XMLWriter::WriteAttr(const wxString &name, long long value)
{
   Write(wxString::Format(wxT(" %s=\"%lld\""),
      name.c_str(),
      value));
}

void XMLWriter::WriteAttr(const wxChar *name, long long value)
{
   WriteAttr(wxString(name), value);
}

void XMLWriter::WriteAttr(const wxString &name, size_t value)
{
   Write(wxString::Format(wxT(" %s=\"%ld\""),
      name.c_str(),
      value));
}

void XMLWriter::WriteAttr(const wxChar *name, size_t value)
{
   WriteAttr(wxString(name), value);
}

void XMLWriter::WriteAttr(const wxString &name, float value, int digits)
{
   Write(wxString::Format(wxT(" %s=\"%s\""),
      name.c_str(),
      Internat::ToString(value, digits).c_str()));
}

void XMLWriter::WriteAttr(const wxChar *name, float value, int digits)
{
   WriteAttr(wxString(name), value, digits);
}

void XMLWriter::WriteAttr(const wxString &name, double value, int digits)
{
   Write(wxString::Format(wxT(" %s=\"%s\""),
      name.c_str(),
      Internat::ToString(value, digits).c_str()));
}

void XMLWriter::WriteAttr(const wxChar *name, double value, int digits)
{
   WriteAttr(wxString(name), value, digits);
}

void XMLWriter::WriteData(const wxString &value)
{
   int i;

   for (i = 0; i < mDepth; i++) {
      Write(wxT("\t"));
   }

   Write(XMLTagHandler::XMLEsc(value));
}

void XMLWriter::WriteData(const wxChar *value)
{
   WriteData(wxString(value));
}

void XMLWriter::WriteSubTree(const wxString &value)
{
   if (mInTag) {
      Write(wxT(">\n"));
      mInTag = false;
      mHasKids[0] = true;
   }

   Write(value.c_str());
}

void XMLWriter::WriteSubTree(const wxChar *value)
{
   WriteSubTree(wxString(value));
}

void XMLWriter::Write(const wxChar *value)
{
   Write(wxString(value));
}

///
/// XMLFileWriter class
///
XMLFileWriter::XMLFileWriter()
{
}

XMLFileWriter::~XMLFileWriter()
{
   if (IsOpened()) {
      Close();
   }
}

bool XMLFileWriter::Open(const wxString &name, const wxString &mode)
{
   return wxFFile::Open(name, mode);
}

bool XMLFileWriter::Close()
{
   while (mTagstack.GetCount()) {
      EndTag(mTagstack[0]);
   }

   return wxFFile::Close();
}

void XMLFileWriter::Write(const wxString &data)
{
   wxFFile::Write(data);
}

///
/// XMLStringWriter class
///
XMLStringWriter::XMLStringWriter()
{
}

XMLStringWriter::~XMLStringWriter()
{
}

void XMLStringWriter::Write(const wxString &data)
{
   Append(data);
}

// Indentation settings for Vim and Emacs and unique identifier for Arch, a
// version control system. Please do not modify past this point.
//
// Local Variables:
// c-basic-offset: 3
// indent-tabs-mode: nil
// End:
//
// vim: et sts=3 sw=3
// arch-tag: 391b08e6-61f4-43ea-8431-c835c31ba86d

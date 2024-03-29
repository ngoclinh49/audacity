/**********************************************************************

  Audacity: A Digital Audio Editor

  XMLTagHandler.cpp

  Dominic Mazzoni
  Vaughan Johnson


*//****************************************************************//**

\class XMLTagHandler
\brief This class is an interface which should be implemented by
  classes which wish to be able to load and save themselves
  using XML files.

\class XMLValueChecker
\brief XMLValueChecker implements static bool methods for checking 
  input values from XML files.

*//*******************************************************************/

#include "XMLTagHandler.h"

#include "../Audacity.h"
#include "../Internat.h"

#ifdef _WIN32
   #include <windows.h>
#endif

#include <wx/defs.h>
#include <wx/arrstr.h>
#include <wx/filename.h>

#include "../SampleFormat.h"
#include "../Track.h"

bool XMLValueChecker::IsGoodString(const wxString str)
{
   size_t len = str.Length();
   int nullIndex = str.Find('\0', false);
   if ((len < 2048) && // Shouldn't be any reason for longer strings, except intentional file corruption.
         (nullIndex == -1)) // No null characters except terminator.
      return true;
   else
      return false; // good place for a breakpoint
}

// "Good" means the name is well-formed and names an existing file or folder.
bool XMLValueChecker::IsGoodFileName(const wxString strFileName, const wxString strDirName /* = "" */)
{
   // Test strFileName.
   if (!IsGoodFileString(strFileName)) 
      return false;

   #ifdef _WIN32
      if (strFileName.Length() + 1 + strDirName.Length() > MAX_PATH)
         return false;
   #endif

   // Test the corresponding wxFileName.
   wxFileName fileName(strDirName, strFileName);
   return (fileName.IsOk() && fileName.FileExists());
}

bool XMLValueChecker::IsGoodSubdirName(const wxString strSubdirName, const wxString strDirName /* = "" */)
{
   // Test strSubdirName. 
   // Note this prevents path separators, and relative path to parents (strDirName), 
   // so fixes vulnerability #3 in the NGS report for UmixIt, 
   // where an attacker could craft an AUP file with relative pathnames to get to system files, for example.
   if (!IsGoodFileString(strSubdirName) || (strSubdirName == wxT(".")) || (strSubdirName == wxT("..")))
      return false;

   #ifdef _WIN32
      if (strSubdirName.Length() + 1 + strDirName.Length() > MAX_PATH)
         return false;
   #endif

   // Test the corresponding wxFileName.
   wxFileName fileName(strDirName, strSubdirName);
   return (fileName.IsOk() && fileName.DirExists());
}

bool XMLValueChecker::IsGoodPathName(const wxString strPathName)
{
   // Test the corresponding wxFileName.
   wxFileName fileName(strPathName);
   return XMLValueChecker::IsGoodFileName(fileName.GetFullName(), fileName.GetPath(wxPATH_GET_VOLUME));
}

bool XMLValueChecker::IsGoodFileString(wxString str)
{
   return (IsGoodString(str) && 
            !str.IsEmpty() && 
            (str.Length() <= 260) && // FILENAME_MAX is 260 in MSVC, but inconsistent across platforms, sometimes huge.
            (str.Find(wxFileName::GetPathSeparator()) == -1)); // No path separator characters. //vvv (this won't work on CVS HEAD)
}

bool XMLValueChecker::IsGoodInt(const wxString strInt)
{
   if (!IsGoodString(strInt))
      return false;

   // Check that the value won't overflow.
   // Signed long: -2,147,483,648 to +2,147,483,647, i.e., -2^31 to 2^31-1
   // We're strict about disallowing spaces and commas, and requiring minus sign to be first char for negative.
   const size_t lenMAXABS = strlen("2147483647");
   const size_t lenStrInt = strInt.Length();

   if (lenStrInt > (lenMAXABS + 1))
      return false;
   else if ((lenStrInt == (lenMAXABS + 1)) && (strInt[0] == '-'))
   {
      const int digitsMAXABS[] = {2, 1, 4, 7, 4, 8, 3, 6, 4, 9};
      unsigned int i;
      for (i = 0; i < lenMAXABS; i++)
         if (strInt[i+1] < '0' || strInt[i+1] > '9')
            return false; // not a digit
            
      for (i = 0; i < lenMAXABS; i++)
         if (strInt[i+1] - '0' < digitsMAXABS[i])
            return true; // number is small enough
      return false;
   }
   else if (lenStrInt == lenMAXABS)
   {
      const int digitsMAXABS[] = {2, 1, 4, 7, 4, 8, 3, 6, 4, 8};
      unsigned int i;
      for (i = 0; i < lenMAXABS; i++)
         if (strInt[i] < '0' || strInt[i+1] > '9')
            return false; // not a digit
      for (i = 0; i < lenMAXABS; i++)
         if (strInt[i] - '0' < digitsMAXABS[i])
            return true; // number is small enough
      return false;
   }
   return true;
}

bool XMLValueChecker::IsValidChannel(const int nValue)
{
   return (nValue >= Track::LeftChannel) && (nValue <= Track::MonoChannel);
}

bool XMLValueChecker::IsValidSampleFormat(const int nValue)
{
   return (nValue == int16Sample) || (nValue == int24Sample) || (nValue == floatSample);
}

bool XMLTagHandler::ReadXMLTag(const char *tag, const char **attrs)
{
   wxArrayString tmp_attrs;

   while (*attrs) {
      const char *s = *attrs++;
      tmp_attrs.Add(UTF8CTOWX(s));
   }

// JKC: Previously the next line was:
// const char **out_attrs = new char (const char *)[tmp_attrs.GetCount()+1];
// however MSVC doesn't like the constness in this position, so this is now 
// added by a cast after creating the array of pointers-to-non-const chars.
   const wxChar **out_attrs = (const wxChar**)new wxChar *[tmp_attrs.GetCount()+1];
   for (size_t i=0; i<tmp_attrs.GetCount(); i++) {
      out_attrs[i] = tmp_attrs[i].c_str();
   }
   out_attrs[tmp_attrs.GetCount()] = 0;

   bool result = HandleXMLTag(UTF8CTOWX(tag).c_str(), out_attrs);

   delete[] out_attrs;
   return result;
}

void XMLTagHandler::ReadXMLEndTag(const char *tag)
{
   HandleXMLEndTag(UTF8CTOWX(tag).c_str());
}

void XMLTagHandler::ReadXMLContent(const char *s, int len)
{
   HandleXMLContent(wxString(s, wxConvUTF8, len));
}

XMLTagHandler *XMLTagHandler::ReadXMLChild(const char *tag)
{
   return HandleXMLChild(UTF8CTOWX(tag).c_str());
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
// arch-tag: 6aabae58-19bd-4b3a-aa6c-08432a7e106e


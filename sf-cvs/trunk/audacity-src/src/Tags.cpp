/**********************************************************************

  Audacity: A Digital Audio Editor

  Tags.cpp

  Dominic Mazzoni
 
*******************************************************************//**

\class Tags
\brief ID3 Tags (for MP3)

  This class started as an ID3 tag 

  This class holds a few informational tags, such as Title, Author,
  etc. that can be associated with a project or other audio file.
  It is modeled after the ID3 format for MP3 files, and it can
  both import ID3 tags from MP3 files, and export them as well.

  It can present the user with a dialog for editing this information.

  It only keeps track of the fields that are standard in ID3v1
  (title, author, artist, track num, year, genre, and comments),
  but it can export both ID3v1 or the newer ID3v2 format.  The primary
  reason you would want to export ID3v2 tags instead of ID3v1,
  since we're not supporting any v2 fields, is that ID3v2 tags are
  inserted at the BEGINNING of an mp3 file, which is far more
  useful for streaming.
  
  Use of this functionality requires that libid3tag be compiled in
  with Audacity.

*//****************************************************************//**

\class TagsEditor
\brief Derived from ExpandingToolBar, this dialog allows editing of Tags.

*//*******************************************************************/

#include "Tags.h"

#include "Audacity.h"
#include "FileDialog.h"
#include "FileNames.h"
#include "Internat.h"
#include "Prefs.h"
#include "xml/XMLFileReader.h"

#include <wx/button.h>
#include <wx/choice.h>
#include <wx/filename.h>
#include <wx/intl.h>
#include <wx/msgdlg.h>
#include <wx/radiobox.h>
#include <wx/sizer.h>
#include <wx/stattext.h>
#include <wx/string.h>
#include <wx/textctrl.h>

#ifdef USE_LIBID3TAG 
   #include <id3tag.h>
   // DM: the following functions were supposed to have been
   // included in id3tag.h - should be fixed in the next release
   // of mad.
   extern "C" {
      struct id3_frame *id3_frame_new(char const *);
      id3_length_t id3_latin1_length(id3_latin1_t const *);
      void id3_latin1_decode(id3_latin1_t const *, id3_ucs4_t *);
   } 
#endif

Tags::Tags()
{
   mTrackNum = -1;
   mGenre = -1;
   
   mID3V2 = true;

   mEditTitle = true;
   mEditTrackNumber = true;

   mTagsEditor = NULL;
   mTagsEditorFrame = NULL;

#ifdef USE_LIBFLAC
   mFLACMeta = new ::FLAC__StreamMetadata*[1];
   mFLACMeta[0] = NULL;
#endif

   LoadDefaults();
}

Tags::~Tags()
{
   if (mTagsEditorFrame) {
      mTagsEditorFrame->Destroy();
   }
#ifdef USE_LIBFLAC
   if (mFLACMeta[0])
   {
      ::FLAC__metadata_object_delete(mFLACMeta[0]);
      mFLACMeta[0] = NULL;
   }
   delete[] mFLACMeta;
#endif

}

void Tags::LoadDefaults()
{
   wxString path;
   wxString name;
   wxString value;
   long ndx;
   bool cont;

   // Set the parent group
   path = gPrefs->GetPath();
   gPrefs->SetPath(wxT("/Tags"));

   // Process all entries in the group
   cont = gPrefs->GetFirstEntry(name, ndx);
   while (cont) {
      gPrefs->Read(name, &value, wxT(""));

      if (name == wxT("ID3V2")) {
         mID3V2 = wxAtoi(value) != 0;
      }
      else if (name == wxT("Title")) {
         mTitle = value;
      }
      else if (name == wxT("Artist")) {
         mArtist = value;
      }
      else if (name == wxT("Album")) {
         mAlbum = value;
      }
      else if (name == wxT("Year")) {
         mYear = value;
      }
      else if (name == wxT("Comments")) {
         mComments = value;
      }
      else if (name == wxT("TrackNumber")) {
         long num;
         value.ToLong(&num);
         mTrackNum = num;
      }
      else if (name == wxT("Genre")) {
         long num;
         value.ToLong(&num);
         mGenre = num;
      }
      else {
         mExtraNames.Add(name);
         mExtraValues.Add(value);
      }

      cont = gPrefs->GetNextEntry(name, ndx);
   }

   // Restore original group
   gPrefs->SetPath(path);
}

bool Tags::IsEmpty()
{
   // At least one of these should be filled in, otherwise
   // it's assumed that the tags have not been set...
   if (mTitle.Length()==0 &&
       mArtist.Length()==0 &&
       mAlbum.Length()==0)
      return true;
   else
      return false;
}

void Tags::AllowEditTitle(bool editTitle)
{
   mEditTitle = editTitle;
}

void Tags::SetTitle(wxString title)
{
   mTitle = title;
}

wxString Tags::GetTitle()
{
   return mTitle;
}

void Tags::AllowEditTrackNumber(bool editTrackNumber)
{
   mEditTrackNumber = editTrackNumber;
}

void Tags::SetTrackNumber(int num)
{
   mTrackNum = num;
}

int Tags::GetTrackNumber()
{
   return mTrackNum;
}

bool Tags::HandleXMLTag(const wxChar *tag, const wxChar **attrs)
{
   if (wxStrcmp(tag, wxT("tags")) == 0) {
      // loop through attrs, which is a null-terminated list of
      // attribute-value pairs
      long nValue;
      while(*attrs) {
         const wxChar *attr = *attrs++;
         const wxChar *value = *attrs++;

         if (!value)
            break;

         const wxString strValue = value;
         if (!wxStrcmp(attr, wxT("title")) && XMLValueChecker::IsGoodString(strValue))
            mTitle = strValue;
         else if (!wxStrcmp(attr, wxT("artist")) && XMLValueChecker::IsGoodString(strValue))
            mArtist = strValue;
         else if (!wxStrcmp(attr, wxT("album")) && XMLValueChecker::IsGoodString(strValue))
            mAlbum = strValue;
         else if (!wxStrcmp(attr, wxT("track")) && XMLValueChecker::IsGoodInt(strValue) && strValue.ToLong(&nValue))
            mTrackNum = nValue;
         else if (!wxStrcmp(attr, wxT("year")) && XMLValueChecker::IsGoodString(strValue))
            mYear = strValue;
         else if (!wxStrcmp(attr, wxT("genre")) && XMLValueChecker::IsGoodInt(strValue) && strValue.ToLong(&nValue))
            mGenre = nValue;
         else if (!wxStrcmp(attr, wxT("comments")) && XMLValueChecker::IsGoodString(strValue))
            mComments = strValue;
         else if (!wxStrcmp(attr, wxT("id3v2")) && XMLValueChecker::IsGoodInt(strValue) && strValue.ToLong(&nValue))
            mID3V2 = (nValue != 0);
         else if (XMLValueChecker::IsGoodString(strValue) && 
                  XMLValueChecker::IsGoodString(wxString(attr))) {
            mExtraNames.Add(wxString(attr));
            mExtraValues.Add(strValue);
         }
      } // while

      return true;
   }

   if (wxStrcmp(tag, wxT("tag")) == 0) {
      // loop through attrs, which is a null-terminated list of
      // attribute-value pairs
      while(*attrs) {
         const wxChar *attr = *attrs++;
         const wxChar *value = *attrs++;

         if (!value)
            break;

         const wxString strValue = value;
         if (!wxStrcmp(attr, wxT("name")) && XMLValueChecker::IsGoodString(strValue))
            mExtraNames.Add(strValue);
         else if (!wxStrcmp(attr, wxT("value")) && XMLValueChecker::IsGoodString(strValue))
            mExtraValues.Add(strValue);
      } // while

      return true;
   }

   return false;
}

XMLTagHandler *Tags::HandleXMLChild(const wxChar *tag)
{
   if (wxStrcmp(tag, wxT("tag")) == 0)
      return this;

   return NULL;
}

void Tags::WriteXML(XMLWriter &xmlFile)
{
   int j;

   xmlFile.StartTag(wxT("tags"));
   xmlFile.WriteAttr(wxT("title"), mTitle);
   xmlFile.WriteAttr(wxT("artist"), mArtist);
   xmlFile.WriteAttr(wxT("album"), mAlbum);
   xmlFile.WriteAttr(wxT("track"), mTrackNum);
   xmlFile.WriteAttr(wxT("year"), mYear);
   xmlFile.WriteAttr(wxT("genre"), mGenre);
   xmlFile.WriteAttr(wxT("comments"), mComments);
   xmlFile.WriteAttr(wxT("id3v2"), mID3V2);

   if (mExtraNames.GetCount() == 0) {
      xmlFile.EndTag(wxT("tags"));
      return;
   }

   for(j=0; j<(int)mExtraNames.GetCount(); j++) {
      xmlFile.StartTag(wxT("tag"));
      xmlFile.WriteAttr(wxT("name"), mExtraNames[j]);
      xmlFile.WriteAttr(wxT("value"), mExtraValues[j]);
      xmlFile.EndTag(wxT("tag"));
   }

   xmlFile.EndTag(wxT("tags"));
}

bool Tags::ShowEditDialog(wxWindow *parent, wxString title, bool modal)
{
   // If the window is already open, just bring it to front
   if (mTagsEditor && mTagsEditorFrame) {
      if (!modal) {
         mTagsEditorFrame->Raise();
         return true;
      }

      // We must kill an existing modeless editor since we don't want
      // to have two on the screen at once.
      mTagsEditorFrame->Destroy();
      mTagsEditor = NULL;
      mTagsEditorFrame = NULL;
   }

   if (modal) {
      ToolBarDialog *tbd = new ToolBarDialog(parent, -1, title);
      new TagsEditor(tbd, -1, this, mEditTitle, mEditTrackNumber);
      tbd->CentreOnParent();
      return (tbd->ShowModal() != wxID_CANCEL);
      // No need to delete...see TagsEditor::OnClose()
   }
   else {
      mTagsEditorFrame = new ToolBarFrame(parent, -1, title);
      mTagsEditor = new TagsEditor(mTagsEditorFrame, -1, this,
                                   mEditTitle, mEditTrackNumber);
      mTagsEditorFrame->CentreOnParent();
      mTagsEditorFrame->Show();
   }

   return true;
}

void Tags::EditorIsClosing()
{
   mTagsEditor = NULL;
   mTagsEditorFrame = NULL;
}

#ifdef USE_LIBID3TAG

/* Declare Static functions */
static wxString GetID3FieldStr(struct id3_tag *tp, const char *name);
static int GetNumGenres();
static wxString GetGenreNum(int i);

wxString GetID3FieldStr(struct id3_tag *tp, const char *name)
{
   struct id3_frame *frame;

   frame = id3_tag_findframe(tp, name, 0);
   if (frame) {
      const id3_ucs4_t *ustr;

      if (strcmp(name, ID3_FRAME_COMMENT) == 0)
	 ustr = id3_field_getfullstring(&frame->fields[3]);
      else
	 ustr = id3_field_getstrings(&frame->fields[1], 0);

      if (ustr) {
	 char *str = (char *)id3_ucs4_utf8duplicate(ustr);
	 wxString s = UTF8CTOWX(str);
	 free(str);
	 return s;
      }
   }

   return wxT("");
}

#endif // ifdef USE_LIBID3TAG 

int GetNumGenres()
{
   return 148;
}

wxString GetGenreNum(int i)
{
#ifdef USE_LIBID3TAG
  id3_latin1_t      i_latin1[50];
  id3_ucs4_t       *i_ucs4;
  const id3_ucs4_t *genre_ucs4;

  sprintf((char *)i_latin1, "%d", i);
  i_ucs4 =
     (id3_ucs4_t *)malloc((id3_latin1_length(i_latin1) + 1) *
                          sizeof(*i_ucs4));
  if (i_ucs4) {
    id3_latin1_decode(i_latin1, i_ucs4);
    genre_ucs4 = id3_genre_name(i_ucs4);
    char *genre_char = (char *)id3_ucs4_utf8duplicate(genre_ucs4);
    wxString genreStr = UTF8CTOWX(genre_char);
    free(genre_char);
    free(i_ucs4);
    return genreStr;    
  }   

#endif // ifdef USE_LIBID3TAG 

  return wxT("");
}

void Tags::ImportID3(wxString fileName)
{
#ifdef USE_LIBID3TAG 

   struct id3_file *fp = id3_file_open(OSFILENAME(fileName),
                                          ID3_FILE_MODE_READONLY);
   if (!fp) return;

   struct id3_tag *tp = id3_file_tag(fp);
   if (!tp) return;

   mTitle = GetID3FieldStr(tp, ID3_FRAME_TITLE);
   mArtist = GetID3FieldStr(tp, ID3_FRAME_ARTIST);
   mAlbum = GetID3FieldStr(tp, ID3_FRAME_ALBUM);   
   mYear = GetID3FieldStr(tp, ID3_FRAME_YEAR);
   mComments = GetID3FieldStr(tp, ID3_FRAME_COMMENT);

   long l;
   wxString s;
   if ((s = GetID3FieldStr(tp, ID3_FRAME_TRACK)).ToLong(&l))
      mTrackNum = l;

   mID3V2 = ( tp->options & ID3_TAG_OPTION_ID3V1 ) ? false : true;
   
   s = GetID3FieldStr(tp, ID3_FRAME_GENRE);

   if( mID3V2 ) {
      int numGenres = GetNumGenres();
      for(int i=0; i<numGenres; i++)
         if (0 == s.CmpNoCase(GetGenreNum(i)))
            mGenre = i;
   }
   else {
      if( s.ToLong( &l ) )
         mGenre = l;
   }

   // Loop through all remaining frames
   int i;
   for(i=0; i<(int)tp->nframes; i++) {
      struct id3_frame *frame = tp->frames[i];

      //printf("ID: %08x '%4s'\n", (int) *(int *)frame->id, frame->id);
      //printf("Desc: %s\n", frame->description);
      //printf("Num fields: %d\n", frame->nfields);
      
      if (!strcmp(frame->id, ID3_FRAME_TITLE) ||
          !strcmp(frame->id, ID3_FRAME_ARTIST) ||
          !strcmp(frame->id, ID3_FRAME_ALBUM) ||
          !strcmp(frame->id, ID3_FRAME_YEAR) ||
          !strcmp(frame->id, ID3_FRAME_COMMENT) ||
          !strcmp(frame->id, ID3_FRAME_GENRE) ||
          !strcmp(frame->id, ID3_FRAME_TRACK)) {
         continue;
      }

      const id3_ucs4_t *ustr;

      if (frame->nfields>=2) {
         ustr = id3_field_getstrings(&frame->fields[1], 0);
         if (ustr) {
            wxString name = UTF8CTOWX(frame->description);
            
            char *str = (char *)id3_ucs4_utf8duplicate(ustr);
            wxString value = UTF8CTOWX(str);
            free(str);
            
            mExtraNames.Add(name);
            mExtraValues.Add(value);
         }
      }

      if (frame->nfields==3) {
         wxString name, value;

         ustr = id3_field_getstring(&frame->fields[2]);
         if (ustr) {
            char *str = (char *)id3_ucs4_utf8duplicate(ustr);
            value = UTF8CTOWX(str);
            free(str);
         }

         ustr = id3_field_getstring(&frame->fields[1]);
         if (ustr) {
            char *str = (char *)id3_ucs4_utf8duplicate(ustr);
            name = UTF8CTOWX(str);
            free(str);
         }

         mExtraNames.Add(name);
         mExtraValues.Add(value);
      }

   }

   id3_file_close(fp);
#endif // ifdef USE_LIBID3TAG 
}

#ifdef USE_LIBID3TAG 

/* Declare Static functions */
static struct id3_frame *MakeID3Frame(const char *name, const char *data0, const char *data1);

struct id3_frame *MakeID3Frame(const char *name, const char *data0, const char *data1)
{
  struct id3_frame *frame;
  id3_latin1_t     *latin1;
  id3_ucs4_t       *ucs4;

  frame = id3_frame_new(name);

  latin1 = (id3_latin1_t *)data1;
  ucs4 = (id3_ucs4_t *)malloc((id3_latin1_length(latin1) + 1) * sizeof(*ucs4));
  id3_latin1_decode(latin1, ucs4);     
  
  if (strcmp(name, ID3_FRAME_COMMENT) == 0) {
     id3_field_setfullstring(&frame->fields[3], ucs4);
  }
  else if (strcmp(name, "TXXX") == 0) {
     id3_field_setstring(&frame->fields[2], ucs4);
     if (data0) {
        free(ucs4);
        latin1 = (id3_latin1_t *)data0;
        ucs4 = (id3_ucs4_t *)malloc((id3_latin1_length(latin1) + 1) * sizeof(*ucs4));
        
        id3_latin1_decode(latin1, ucs4);     
        
        id3_field_setstring(&frame->fields[1], ucs4);
     }
  }
  else
     id3_field_setstrings(&frame->fields[1], 1, &ucs4);

  free(ucs4);
  
  return frame;
} 

#endif //ifdef USE_LIBID3TAG 

// returns buffer len; caller frees
int Tags::ExportID3(char **buffer, bool *endOfFile)
{
#ifdef USE_LIBID3TAG 
   struct id3_tag *tp = id3_tag_new();

   int i;

   if (IsEmpty())
      return 0;
   
   if (mTitle != wxT(""))
      id3_tag_attachframe(tp, MakeID3Frame(ID3_FRAME_TITLE, NULL, mTitle.mb_str()));

   if (mArtist != wxT(""))
      id3_tag_attachframe(tp, MakeID3Frame(ID3_FRAME_ARTIST, NULL, mArtist.mb_str()));

   if (mAlbum != wxT(""))
      id3_tag_attachframe(tp, MakeID3Frame(ID3_FRAME_ALBUM, NULL, mAlbum.mb_str()));

   if (mYear != wxT(""))
      id3_tag_attachframe(tp, MakeID3Frame(ID3_FRAME_YEAR, NULL, mYear.mb_str()));

   if (mComments != wxT(""))
      id3_tag_attachframe(tp, MakeID3Frame(ID3_FRAME_COMMENT, NULL, mComments.mb_str()));

   if (mTrackNum >= 0) {
      wxString trackNumStr;
      trackNumStr.Printf(wxT("%d"), mTrackNum);
      id3_tag_attachframe(tp, MakeID3Frame(ID3_FRAME_TRACK, NULL, trackNumStr.mb_str()));
   }

   if (mGenre >= 0) {
      if (mID3V2) {
         wxString genreStr = GetGenreNum(mGenre);
         id3_tag_attachframe(tp, MakeID3Frame(ID3_FRAME_GENRE, NULL, genreStr.mb_str()));
      }
      else {
         wxString genreStr;
         genreStr.Printf(wxT("%d"), mGenre);
         id3_tag_attachframe(tp, MakeID3Frame(ID3_FRAME_GENRE, NULL, genreStr.mb_str()));
      }
   }

   for(i=0; i<(int)mExtraNames.GetCount(); i++) {
      id3_tag_attachframe(tp, 
                          MakeID3Frame("TXXX", /* Unknown text field */
                                       mExtraNames[i].mb_str(),
                                       mExtraValues[i].mb_str()));
   }

   if (mID3V2) {
      tp->options &= (~ID3_TAG_OPTION_COMPRESSION); // No compression

      // If this version of libid3tag supports it, use v2.3 ID3
      // tags instead of the newer, but less well supported, v2.4
      // that libid3tag uses by default.
      #ifdef ID3_TAG_HAS_TAG_OPTION_ID3V2_3
      tp->options |= ID3_TAG_OPTION_ID3V2_3;
      #endif

      *endOfFile = false;
   }
   else {
      tp->options |= ID3_TAG_OPTION_ID3V1;
      *endOfFile = true;
   }

   id3_length_t len;
   
   len = id3_tag_render(tp, 0);
   *buffer = (char *)malloc(len);
   len = id3_tag_render(tp, (id3_byte_t *)*buffer);

   id3_tag_delete(tp);

   return len;
#else //ifdef USE_LIBID3TAG 
   return 0;
#endif
}

#ifdef USE_LIBFLAC
void Tags::ExportFLACTags(FLAC::Encoder::File *encoder)
{
   /* Somehow I can't get the hang of the libFLAC++ API for metadata, 
      so we mix things a little with the 'pure' C API instead - JAPJ */

   if (mFLACMeta[0])
   {
      /* first cleanup any previous metadata */
      ::FLAC__metadata_object_delete(mFLACMeta[0]);
      mFLACMeta[0] = NULL;
   }

   if (mFLACMeta[0] == NULL)
   {
      mFLACMeta[0] = ::FLAC__metadata_object_new(FLAC__METADATA_TYPE_VORBIS_COMMENT);
   }

   if (mTitle != wxT(""))
   {
      FLAC::Metadata::VorbisComment::Entry entry("TITLE", mTitle.mb_str());
      ::FLAC__metadata_object_vorbiscomment_append_comment(mFLACMeta[0], entry.get_entry(), true);
   }

   if (mArtist != wxT(""))
   {
      FLAC::Metadata::VorbisComment::Entry entry("ARTIST", mArtist.mb_str());
      ::FLAC__metadata_object_vorbiscomment_append_comment(mFLACMeta[0], entry.get_entry(), true);
   }

   if (mAlbum != wxT(""))
   {
      FLAC::Metadata::VorbisComment::Entry entry("ALBUM", mAlbum.mb_str());
      ::FLAC__metadata_object_vorbiscomment_append_comment(mFLACMeta[0], entry.get_entry(), true);
   }

   if (mTrackNum >= 0) {
      wxString trackNumStr;
      trackNumStr.Printf(wxT("%d"), mTrackNum);
      FLAC::Metadata::VorbisComment::Entry entry("TRACKNUMBER", trackNumStr.mb_str());
      ::FLAC__metadata_object_vorbiscomment_append_comment(mFLACMeta[0], entry.get_entry(), true);
   }

   if (mGenre >= 0) {
      wxString genreStr = GetGenreNum(mGenre);
      FLAC::Metadata::VorbisComment::Entry entry("GENRE", genreStr.mb_str());
      ::FLAC__metadata_object_vorbiscomment_append_comment(mFLACMeta[0], entry.get_entry(), true);
   }

   encoder->set_metadata(mFLACMeta,1);
}
#endif

#ifdef USE_LIBVORBIS
void Tags::ExportOGGTags(vorbis_comment *comment)
{
   vorbis_comment_init(comment);
// ((char *) (const char *)(X).mb_str())
   if (mTitle != wxT("")) {
      vorbis_comment_add_tag(comment, "TITLE", (char *) (const char *) mTitle.mb_str());
   }

   if (mArtist != wxT("")) {
      vorbis_comment_add_tag(comment, "ARTIST", (char *) (const char *) mArtist.mb_str());
   }

   if (mAlbum != wxT("")) {
      vorbis_comment_add_tag(comment, "ALBUM", (char *) (const char *) mAlbum.mb_str());
   }

   if (mYear != wxT("")) {
      vorbis_comment_add_tag(comment, "YEAR", (char *) (const char *) mYear.mb_str());
   }

   if (mComments != wxT("")) {
      vorbis_comment_add_tag(comment, "COMMENT", (char *) (const char *) mComments.mb_str());
   }

   if (mTrackNum >= 0) {
      wxString trackNumStr;
      trackNumStr.Printf(wxT("%d"), mTrackNum);
      vorbis_comment_add_tag(comment, "TRACKNUMBER", (char *) (const char *) trackNumStr.mb_str());
   }

   if (mGenre >= 0) {
      vorbis_comment_add_tag(comment, "GENRE", (char *) (const char *) GetGenreNum(mGenre).mb_str());
   }

   for (size_t i = 0; i < mExtraNames.GetCount(); i++) {
      vorbis_comment_add_tag(comment,
                             (char *) (const char *) mExtraNames[i].mb_str(),
                             (char *) (const char *) mExtraValues[i].mb_str());
   }
}
#endif

//
// TagsEditor
//

enum {
   CancelID = wxID_CANCEL,
   CloseID = wxID_OK,
   StaticTextID = 10000,
   TitleTextID,
   ArtistTextID,
   AlbumTextID,
   TrackNumTextID,
   YearTextID,
   CommentsTextID,
   GenreID,
   FormatID,
   MoreID,
   FewerID,
   ClearID,
   LoadID,
   SaveID,
   SaveDefaultsID,
   FirstExtraID
};

BEGIN_EVENT_TABLE(TagsEditor, ExpandingToolBar)
   EVT_TEXT(TitleTextID, TagsEditor::OnChange)
   EVT_TEXT(ArtistTextID, TagsEditor::OnChange)
   EVT_TEXT(AlbumTextID, TagsEditor::OnChange)
   EVT_TEXT(TrackNumTextID, TagsEditor::OnChange)
   EVT_TEXT(YearTextID, TagsEditor::OnChange)
   EVT_TEXT(CommentsTextID, TagsEditor::OnChange)
   EVT_CHOICE(GenreID, TagsEditor::OnChange)
   EVT_RADIOBOX(FormatID, TagsEditor::OnChange)
   EVT_BUTTON(CancelID, TagsEditor::OnCancel)
   EVT_BUTTON(CloseID, TagsEditor::OnClose)
   EVT_BUTTON(MoreID, TagsEditor::OnMore)
   EVT_BUTTON(FewerID, TagsEditor::OnFewer)
   EVT_BUTTON(ClearID, TagsEditor::OnClear)
   EVT_BUTTON(LoadID, TagsEditor::OnLoad)
   EVT_BUTTON(SaveID, TagsEditor::OnSave)
   EVT_BUTTON(SaveDefaultsID, TagsEditor::OnSaveDefaults)
   EVT_COMMAND_RANGE(FirstExtraID, FirstExtraID+1000,
                     wxEVT_COMMAND_TEXT_UPDATED, TagsEditor::OnChange)
END_EVENT_TABLE()

TagsEditor::TagsEditor(wxWindow * parent, wxWindowID id,
                       Tags * tags,
                       bool editTitle, bool editTrackNumber):
   ExpandingToolBar(parent, id),
   mTags(tags)
{
   mTransfering = true; // avoid endless update loop

   // Make local backup copies of metadata
   mTitle       = mTags->mTitle;
   mArtist      = mTags->mArtist;
   mAlbum       = mTags->mAlbum;
   mTrackNum    = mTags->mTrackNum;
   mYear        = mTags->mYear;
   mGenre       = mTags->mGenre;
   mComments    = mTags->mComments;
   mID3V2       = mTags->mID3V2;
   mExtraNames  = mTags->mExtraNames;
   mExtraValues = mTags->mExtraValues;

   BuildMainPanel();
   BuildExtraPanel();
   GetMainPanel()->Layout();
   GetMainPanel()->Fit();
   Layout();
   Fit();

   GetExtraPanel()->SetSize(GetMainPanel()->GetSize().GetWidth(), -1);

   if (!editTitle)
      mTitleText->Enable(false);

   if (!editTrackNumber)
      mTrackNumText->Enable(false);

   TransferDataToWindow();

   mTransfering = false;
}

TagsEditor::~TagsEditor()
{
   if (mExtraNameTexts) {
      delete[] mExtraNameTexts;
   }

   if (mExtraValueTexts) {
      delete[] mExtraValueTexts;
   }

   mTags->EditorIsClosing();
}

bool TagsEditor::Validate()
{
   wxString errorString =
      _("Maximum length of attribute '%s' is %d characters. Data was truncated.");

   if(!mTags->mID3V2)
   {
      if(mTags->mTitle.Length() > 30)
      {
         wxMessageBox(wxString::Format(errorString, _("Title"), 30));

         mTags->mTitle = mTags->mTitle.Left(30);
         TransferDataToWindow();

         return FALSE;
      }

      if(mTags->mArtist.Length() > 30)
      {
         wxMessageBox(wxString::Format(errorString, _("Artist"), 30));

         mTags->mArtist = mTags->mArtist.Left(30);
         TransferDataToWindow();

         return FALSE;
      }

      if(mTags->mAlbum.Length() > 30)
      {
         wxMessageBox(wxString::Format(errorString, _("Album"), 30));

         mTags->mAlbum = mTags->mAlbum.Left(30);
         TransferDataToWindow();

         return FALSE;
      }

      if(mTags->mYear.Length() > 4)
      {
         wxMessageBox(wxString::Format(errorString, _("Year"), 4));

         mTags->mYear = mTags->mYear.Left(4);
         TransferDataToWindow();

         return FALSE;
      }

      if(mTags->mComments.Length() > 30)
      {
         wxMessageBox(wxString::Format(errorString, _("Comments"), 30));

         mTags->mComments = mTags->mComments.Left(30);
         TransferDataToWindow();

         return FALSE;
      }
   }

   return TRUE;
}

bool TagsEditor::TransferDataToWindow()
{
   mTitleText->SetValue(mTags->mTitle);
   mArtistText->SetValue(mTags->mArtist);   
   mAlbumText->SetValue(mTags->mAlbum);
   mYearText->SetValue(mTags->mYear);
   mCommentsText->SetValue(mTags->mComments);
   
   if (mTags->mTrackNum != -1) {
      wxString numStr;
      numStr.Printf(wxT("%d"), mTags->mTrackNum);
      mTrackNumText->SetValue(numStr);
   }
   
   if (mTags->mGenre >= 0 && mTags->mGenre < GetNumGenres())
   	mGenreChoice->SetStringSelection(GetGenreNum(mTags->mGenre));
   
   mFormatRadioBox->SetSelection((int)mTags->mID3V2);

   int i;

   for(i=0; i<(int)mTags->mExtraNames.GetCount(); i++) {
      mExtraNameTexts[i]->SetValue(mTags->mExtraNames[i]);
      mExtraValueTexts[i]->SetValue(mTags->mExtraValues[i]);
   }

   return TRUE;
}

bool TagsEditor::TransferDataFromWindow()
{
   mTags->mTitle = mTitleText->GetValue();
   mTags->mArtist = mArtistText->GetValue();
   mTags->mAlbum = mAlbumText->GetValue();

   wxString str = mTrackNumText->GetValue();
   if (str == wxT(""))
      mTags->mTrackNum = -1;
   else {
      long i;
      str.ToLong(&i);
      mTags->mTrackNum = i;
   }

   mTags->mYear = mYearText->GetValue();
   mTags->mComments = mCommentsText->GetValue();
   mTags->mID3V2 = (mFormatRadioBox->GetSelection())?true:false;

   int i;
   
   for(i=0; i<GetNumGenres(); i++)
   	if (GetGenreNum(i) == mGenreChoice->GetStringSelection()) {
   		mTags->mGenre = i;
   		break;
   	}
	
   for(i=0; i<(int)mTags->mExtraNames.GetCount(); i++) {
      mTags->mExtraNames[i] = mExtraNameTexts[i]->GetValue();
      mTags->mExtraValues[i] = mExtraValueTexts[i]->GetValue();
   }

   return TRUE;
}

void TagsEditor::OnChange(wxCommandEvent & event)
{
   if (!mTransfering) { // avoid endless update loop
      mTransfering = true;
      TransferDataFromWindow();
      mTransfering = false;
   }
}

void TagsEditor::OnCancel(wxCommandEvent & event)
{
   // Restore original values
   mTags->mExtraNames.Clear();
   mTags->mExtraValues.Clear();
   mTags->mTitle       = mTitle;
   mTags->mArtist      = mArtist;
   mTags->mAlbum       = mAlbum;
   mTags->mTrackNum    = mTrackNum;
   mTags->mYear        = mYear;
   mTags->mGenre       = mGenre;
   mTags->mComments    = mComments;
   mTags->mID3V2       = mID3V2;
   mTags->mExtraNames  = mExtraNames;
   mTags->mExtraValues = mExtraValues;

   GetParent()->Destroy();
}

void TagsEditor::OnClose(wxCommandEvent & event)
{
   if (!TransferDataFromWindow()) {
      return;
   }

   GetParent()->Destroy();
}

void TagsEditor::OnMore(wxCommandEvent & event)
{
   TransferDataFromWindow();

   mTags->mExtraNames.Add(wxT(""));
   mTags->mExtraValues.Add(wxT(""));

   RebuildMainPanel();
}

void TagsEditor::OnFewer(wxCommandEvent & event)
{
   TransferDataFromWindow();

   int len = (int)mTags->mExtraNames.GetCount();
   if (len > 0) {
      mTags->mExtraNames.RemoveAt(len-1);
      mTags->mExtraValues.RemoveAt(len-1);

      RebuildMainPanel();
   }
}

void TagsEditor::OnClear(wxCommandEvent & event)
{
   mTags->mTitle.Clear();
   mTags->mArtist.Clear();
   mTags->mAlbum.Clear();
   mTags->mTrackNum = -1;
   mTags->mYear.Clear();
   mTags->mGenre = -1;
   mTags->mComments.Clear();
   mTags->mID3V2 = true;;
   mTags->mExtraNames.Clear();
   mTags->mExtraValues.Clear();

   RebuildMainPanel();
}

void TagsEditor::OnLoad(wxCommandEvent & event)
{
   wxString fn;

   // For drawer to remain visible while dialog is displayed since the mouse
   // position is still tracked.
   Expand();

   // Ask the user for the real name
   fn = FileSelector(_("Save Metadata As:"),
                     FileNames::DataDir(),
                     wxT("Tags.xml"),
                     wxT("xml"),
                     wxT("*.xml"),
                     wxOPEN,
                     this);

   // Now, allow the drawer to be active again.
   Collapse();
   TryAutoExpand();
   Fit();

   // User canceled...
   if (fn.IsEmpty()) {
      return;
   }

   // Clear current contents
   mTags->mTitle.Clear();
   mTags->mArtist.Clear();
   mTags->mAlbum.Clear();
   mTags->mTrackNum = -1;
   mTags->mYear.Clear();
   mTags->mGenre = -1;
   mTags->mComments.Clear();
   mTags->mID3V2 = true;;
   mTags->mExtraNames.Clear();
   mTags->mExtraValues.Clear();

   // Load the metadata
   XMLFileReader reader;
   if (!reader.Parse(mTags, fn)) {
      // Inform user of load failure
      wxMessageBox(reader.GetErrorStr(),
                   _("Error loading metadata"),
                   wxOK | wxCENTRE,
                   this);
   }

   // Refresh the dialog
   RebuildMainPanel();

   return;
}

void TagsEditor::OnSave(wxCommandEvent & event)
{
   wxString fn;

   // For drawer to remain visible while dialog is displayed since the mouse
   // position is still tracked.
   Expand();

   // Ask the user for the real name
   fn = FileSelector(_("Save Metadata As:"),
                     FileNames::DataDir(),
                     wxT("Tags.xml"),
                     wxT("xml"),
                     wxT("*.xml"),
                     wxSAVE | wxOVERWRITE_PROMPT,
                     this);

   // Now, allow the drawer to be active again.
   Collapse();
   TryAutoExpand();
   Fit();

   // User canceled...
   if (fn.IsEmpty()) {
      return;
   }

   // Create/Open the file
   XMLFileWriter writer;
   writer.Open(fn, wxT("wb"));

   // Complain if open failed
   if (!writer.IsOpened())
   {
      // Constructor will emit message
      return;
   }

   // Write the metadata
   mTags->WriteXML(writer);

   // Close the file
   writer.Close();

   return;
}

void TagsEditor::OnSaveDefaults(wxCommandEvent & event)
{
   gPrefs->DeleteGroup(wxT("/Tags"));
   gPrefs->Write(wxT("/Tags/ID3V2"), mTags->mID3V2);
   gPrefs->Write(wxT("/Tags/Title"), mTags->mTitle);
   gPrefs->Write(wxT("/Tags/Artist"), mTags->mArtist);
   gPrefs->Write(wxT("/Tags/Album"), mTags->mAlbum);
   gPrefs->Write(wxT("/Tags/Year"), mTags->mYear);
   gPrefs->Write(wxT("/Tags/Comments"), mTags->mComments);
   gPrefs->Write(wxT("/Tags/TrackNumber"), mTags->mTrackNum);
   gPrefs->Write(wxT("/Tags/Genre"), mTags->mGenre);

   for (size_t i = 0; i < mTags->mExtraNames.GetCount(); i++) {
      gPrefs->Write(wxT("/Tags/") + mTags->mExtraNames[i],
                    mTags->mExtraValues[i]);
   }
}

void TagsEditor::RebuildMainPanel()
{
   GetMainPanel()->DestroyChildren();

   if (mExtraNameTexts) {
      delete[] mExtraNameTexts;
      mExtraNameTexts = NULL;
   }

   if (mExtraNameTexts) {
      delete[] mExtraValueTexts;
      mExtraValueTexts = NULL;
   }

   BuildMainPanel();
   GetMainPanel()->Layout();
   GetMainPanel()->Fit();
   Layout();
   Fit();

   GetExtraPanel()->SetSize(GetMainPanel()->GetSize().GetWidth(), -1);

   TryAutoExpand();

   mTransfering = true;
   TransferDataToWindow();
   mTransfering = false;
}

void TagsEditor::BuildMainPanel()
{
   wxPanel *parent = GetMainPanel();

   wxBoxSizer *mainSizer = new wxBoxSizer(wxVERTICAL);

   /***/

   wxString formats[2];
   formats[0] = _("ID3v1 (more compatible)");
   formats[1] = _("ID3v2 (more flexible)");

   mFormatRadioBox = new wxRadioBox(parent, FormatID, wxString(_("Format")) + wxT(":"),
                                    wxDefaultPosition, wxDefaultSize,
                                    2, formats,
                                    0, wxRA_VERTICAL);
   mainSizer->Add(mFormatRadioBox, 1, wxEXPAND | wxALL, 7);

   /***/
   
   wxFlexGridSizer *gridSizer = new wxFlexGridSizer(2, 0, 0);

   wxStaticText *item3 =
       new wxStaticText(parent, StaticTextID, wxString(_("Title")) + wxT(":"),
                        wxDefaultPosition, wxDefaultSize, 0);
   gridSizer->Add(item3, 0, wxALIGN_RIGHT | wxALIGN_CENTER_VERTICAL | wxALL, 3);

   mTitleText =
       new wxTextCtrl(parent, TitleTextID, wxT(""), wxDefaultPosition,
                      wxSize(200, -1), 0);
   gridSizer->Add(mTitleText, 1, wxEXPAND | wxALL, 3);

   wxStaticText *item5 =
       new wxStaticText(parent, StaticTextID, wxString(_("Artist")) + wxT(":"),
                        wxDefaultPosition, wxDefaultSize, 0);
   gridSizer->Add(item5, 0, wxALIGN_RIGHT | wxALIGN_CENTER_VERTICAL | wxALL, 3);

   mArtistText =
       new wxTextCtrl(parent, ArtistTextID, wxT(""), wxDefaultPosition,
                      wxSize(200, -1), 0);
   gridSizer->Add(mArtistText, 1, wxEXPAND | wxALL, 3);

   wxStaticText *item7 =
       new wxStaticText(parent, StaticTextID, wxString(_("Album")) + wxT(":"),
                        wxDefaultPosition, wxDefaultSize, 0);
   gridSizer->Add(item7, 0, wxALIGN_RIGHT | wxALIGN_CENTER_VERTICAL | wxALL, 3);

   mAlbumText =
       new wxTextCtrl(parent, AlbumTextID, wxT(""), wxDefaultPosition,
                      wxSize(200, -1), 0);
   gridSizer->Add(mAlbumText, 1, wxEXPAND | wxALL, 3);

   mainSizer->Add(gridSizer, 0, wxALIGN_CENTRE | wxALL, 7);

   /***/
   
   wxBoxSizer *hSizer = new wxBoxSizer(wxHORIZONTAL);

   wxStaticText *item9 =
       new wxStaticText(parent, StaticTextID, wxString(_("Track Number")) + wxT(":"),
                        wxDefaultPosition, wxDefaultSize, 0);
   hSizer->Add(item9, 0, wxALIGN_CENTRE | wxALL, 3);

   mTrackNumText = 
       new wxTextCtrl(parent, TrackNumTextID, wxT(""), wxDefaultPosition,
                      wxSize(40, -1), 0);
   hSizer->Add(mTrackNumText, 0, wxALIGN_CENTRE | wxALL, 3);

   wxStaticText *item11 =
       new wxStaticText(parent, StaticTextID, wxString(_("Year")) + wxT(":"),
                        wxDefaultPosition, wxDefaultSize, 0);
   hSizer->Add(item11, 0, wxALIGN_CENTRE | wxALL, 3);

   mYearText =
       new wxTextCtrl(parent, YearTextID, wxT(""), wxDefaultPosition,
                      wxSize(40, -1), 0);
   hSizer->Add(mYearText, 0, wxALIGN_CENTRE | wxALL, 3);
   
   mainSizer->Add(hSizer, 0, wxALIGN_CENTRE | wxALL, 7);

   /***/

   gridSizer = new wxFlexGridSizer(2, 0, 0);

   wxStaticText *item20 =
       new wxStaticText(parent, StaticTextID, wxString(_("Genre")) + wxT(":"),
                        wxDefaultPosition, wxDefaultSize, 0);
   gridSizer->Add(item20, 0, wxALIGN_RIGHT | wxALIGN_CENTER_VERTICAL | wxALL, 3);

	wxArrayString sortedGenres;
	for(int i=0; i<GetNumGenres(); i++)
		sortedGenres.Add(GetGenreNum(i));
	sortedGenres.Sort();

   mGenreChoice =
       new wxChoice(parent, GenreID,
                    wxDefaultPosition, wxSize(-1, -1),
                    sortedGenres);
   mGenreChoice->SetStringSelection(wxT("Pop")); // sensible default
   gridSizer->Add(mGenreChoice, 1, wxEXPAND | wxALL, 3);
   
   wxStaticText *item22 =
       new wxStaticText(parent, StaticTextID, wxString(_("Comments")) + wxT(":"),
                        wxDefaultPosition, wxDefaultSize, 0);
   gridSizer->Add(item22, 0, wxALIGN_RIGHT | wxALIGN_CENTER_VERTICAL | wxALL, 3);

   mCommentsText =
       new wxTextCtrl(parent, CommentsTextID, wxT(""), wxDefaultPosition,
                      wxSize(200, -1), 0);
   gridSizer->Add(mCommentsText, 1, wxEXPAND | wxALL, 3);
   
   mainSizer->Add(gridSizer, 0, wxALIGN_CENTRE | wxALL, 7);

   /***/

   int len = (int)mTags->mExtraNames.GetCount();
   int i;

   mExtraNameTexts = new wxTextCtrl*[len];
   mExtraValueTexts = new wxTextCtrl*[len];

   gridSizer = new wxFlexGridSizer(2, 0, 0);
   
   for(i=0; i<len; i++) {
      mExtraNameTexts[i] =
         new wxTextCtrl(parent, FirstExtraID+(2*i)+0, wxT(""),
                        wxDefaultPosition, wxSize(100, -1));
      gridSizer->Add(mExtraNameTexts[i], 0, wxEXPAND | wxALL, 3);

      mExtraValueTexts[i] =
         new wxTextCtrl(parent, FirstExtraID+(2*i)+1, wxT(""),
                        wxDefaultPosition, wxSize(200, -1));
      gridSizer->Add(mExtraValueTexts[i], 1, wxEXPAND | wxALL, 3);
   }

   mainSizer->Add(gridSizer, 0, wxEXPAND | wxALL, 7);

   parent->SetSizer(mainSizer);
   parent->Layout();
   parent->Fit();
}

void TagsEditor::BuildExtraPanel()
{
   wxPanel *parent = GetExtraPanel();

   wxBoxSizer *mainSizer = new wxBoxSizer(wxVERTICAL);

      /***/

   wxBoxSizer *hSizer = new wxBoxSizer(wxHORIZONTAL);

   wxButton *fewer =
       new wxButton(parent, FewerID, _("&Fewer"), wxDefaultPosition,
                    wxDefaultSize, 0);
   hSizer->Add(fewer, 0, wxALIGN_CENTRE | wxALL, 3);

   wxButton *more =
       new wxButton(parent, MoreID, _("&More"), wxDefaultPosition,
                    wxDefaultSize, 0);
   hSizer->Add(more, 0, wxALIGN_CENTRE | wxALL, 3);

   hSizer->Add(10, 1, wxEXPAND);

   wxButton *clear =
       new wxButton(parent, ClearID, _("&Clear"), wxDefaultPosition,
                    wxDefaultSize, 0);
   hSizer->Add(clear, 0, wxALIGN_CENTRE | wxALL, 3);

   mainSizer->Add(hSizer, 0, wxEXPAND | wxALL, 7);

      /***/
   
   wxStaticBoxSizer *staticBoxSizer = new wxStaticBoxSizer(wxHORIZONTAL,
                                                     parent,
                                                     _("Template"));

   wxButton *load =
       new wxButton(parent, LoadID, _("&Load..."), wxDefaultPosition,
                    wxDefaultSize, 0);
   staticBoxSizer->Add(load, 0, wxALIGN_CENTRE | wxALL, 3);

   wxButton *save =
       new wxButton(parent, SaveID, _("&Save..."), wxDefaultPosition,
                    wxDefaultSize, 0);
   staticBoxSizer->Add(save, 0, wxALIGN_CENTRE | wxALL, 3);

   staticBoxSizer->Add(1, 1, wxEXPAND);

   wxButton *defaultButton =
       new wxButton(parent, SaveDefaultsID, _("S&et default"),
                    wxDefaultPosition, wxDefaultSize, 0);
   staticBoxSizer->Add(defaultButton, 0, wxALIGN_CENTRE | wxALL, 3);

   mainSizer->Add(staticBoxSizer, 0, wxEXPAND | wxALL, 7);

      /***/
   
   hSizer = new wxBoxSizer(wxHORIZONTAL);

#if defined(__WXGTK20__) || defined(__WXMAC__)
   wxButton *cancel =
       new wxButton(parent, CancelID, _("&Cancel"), wxDefaultPosition,
                    wxDefaultSize, 0);
   hSizer->Add(cancel, 0, wxALIGN_CENTRE | wxALL, 3);
   wxButton *close =
       new wxButton(parent, CloseID, _("&Done"), wxDefaultPosition,
                    wxDefaultSize, 0);
   hSizer->Add(close, 0, wxALIGN_CENTRE | wxALL, 3);
#else
   wxButton *close =
       new wxButton(parent, CloseID, _("&Done"), wxDefaultPosition,
                    wxDefaultSize, 0);
   hSizer->Add(close, 0, wxALIGN_CENTRE | wxALL, 3);
   wxButton *cancel =
       new wxButton(parent, CancelID, _("&Cancel"), wxDefaultPosition,
                    wxDefaultSize, 0);
   hSizer->Add(cancel, 0, wxALIGN_CENTRE | wxALL, 3);
#endif

   mainSizer->Add(hSizer, 0, wxALIGN_RIGHT | wxALL, 7);

   mainSizer->Add(1, 5, wxEXPAND);

   parent->SetSizer(mainSizer);
   parent->Layout();
   parent->Fit();
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
// arch-tag: 94f72c32-970b-4f4e-bbf3-3880fce7b965



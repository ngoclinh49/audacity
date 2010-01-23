/**********************************************************************

  Audacity: A Digital Audio Editor

  CommandManager.h

  Brian Gunlogson
  Dominic Mazzoni

**********************************************************************/

#ifndef __AUDACITY_COMMAND_MANAGER__
#define __AUDACITY_COMMAND_MANAGER__

#include <wx/string.h>
#include <wx/dynarray.h>
#include <wx/menu.h>
#include <wx/hashmap.h>

#include "../xml/XMLTagHandler.h"

class CommandFunctor
{
public:
   virtual void operator()(int index = 0) = 0;
};

struct MenuBarListEntry
{
   wxString name;
   wxMenuBar *menubar;
};

struct SubMenuListEntry
{
   wxString name;
   wxMenu *menu;
};

struct CommandListEntry
{
   int id;
   wxString name;
   wxString key;
   wxString defaultKey;
   wxString label;
   wxMenu *menu;
   CommandFunctor *callback;
   bool multi;
   int index;
   int count;
};

WX_DEFINE_ARRAY(MenuBarListEntry *, MenuBarList);
WX_DEFINE_ARRAY(SubMenuListEntry *, SubMenuList);
WX_DEFINE_ARRAY(CommandListEntry *, CommandList);

WX_DECLARE_STRING_HASH_MAP(CommandListEntry *, CommandNameHash);
WX_DECLARE_HASH_MAP(int, CommandListEntry *, wxIntegerHash, wxIntegerEqual, CommandIDHash);

class CommandManager: public XMLTagHandler
{
 public:

   //
   // Constructor / Destructor
   //

   CommandManager();
   virtual ~CommandManager();

   void PurgeData();

   //
   // Creating menus and adding commands
   //

   wxMenuBar *AddMenuBar(wxString sMenu);

   void BeginMenu(wxString tName);
   void EndMenu();

   void BeginSubMenu(wxString tName);
   void EndSubMenu();

   void AddItem(wxString name, wxString label, CommandFunctor *callback);
   void AddItemList(wxString name, wxArrayString labels,
                    CommandFunctor *callback, bool plugins = false);

   void AddSeparator();

   // A command doesn't actually appear in a menu but might have a
   // keyboard shortcut.
   void AddCommand(wxString name, wxString label, CommandFunctor *callback);

   //
   // Modifying menus
   //

   void Enable(wxString name, bool enabled);
   void Modify(wxString name, wxString newLabel);

   //
   // Executing commands
   //

   bool HandleMenuID(int id);

   bool HandleKey(wxKeyEvent &evt);

   //
   // Accessing
   //

   void GetAllCommandNames(wxArrayString &names, bool includeMultis);

   wxString GetLabelFromName(wxString name);
   wxString GetKeyFromName(wxString name);
   wxString GetDefaultKeyFromName(wxString name);

   //
   // Loading/Saving
   //

   virtual bool HandleXMLTag(const char *tag, const char **attrs);
   virtual void HandleXMLEndTag(const char *tag);
   virtual XMLTagHandler *HandleXMLChild(const char *tag);
   virtual void WriteXML(int depth, FILE *fp);

 protected:

   wxMenuBar * CurrentMenuBar();
   wxMenuBar * GetMenuBar(wxString sMenu);
   wxMenu * CurrentSubMenu();
   wxMenu * CurrentMenu();

   int NextIdentifier(int ID);
   int NewIdentifier(wxString name, wxString label, wxMenu *menu,
                     CommandFunctor *callback,
                     bool multi, int index, int count);

   wxString GetKey(wxString label);

private:
   MenuBarList  mMenuBarList;
   SubMenuList  mSubMenuList;
   CommandList  mCommandList;
   CommandNameHash  mCommandNameHash;
   CommandNameHash  mCommandKeyHash;
   CommandIDHash  mCommandIDHash;
   int mCurrentID;
   int mHiddenID;
   int mXMLKeysRead;
   wxMenu * mCurrentMenu;
};

#endif

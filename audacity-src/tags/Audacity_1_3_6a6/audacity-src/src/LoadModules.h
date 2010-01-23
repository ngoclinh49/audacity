/**********************************************************************

  Audacity: A Digital Audio Editor

  LoadModules.h

  Dominic Mazzoni
  James Crook

**********************************************************************/

#ifndef __AUDACITY_LOADMODULES_H__
#define __AUDACITY_LOADMODULES_H__

#include <wx/dynlib.h>
#include <wx/module.h>

void LoadModules();
wxWindow *  MakeHijackPanel();

//
// Module Manager
//
// wxPluginManager would be MUCH better, but it's an "undocumented" framework.
//
#define ModuleDispatchName "ModuleDispatch"

typedef enum
{
   ModuleInitialize,
   ModuleTerminate,
   AppInitialized,
   ProjectInitialized
} ModuleDispatchTypes;

typedef int AUDACITY_DLL_API (*fnModuleDispatch)(ModuleDispatchTypes type);

class Module
{
public:
   Module(const wxString & name);
   virtual ~Module();

   bool Load();
   void Unload();
   int Dispatch(ModuleDispatchTypes type);

private:
   wxString mName;
   wxDynamicLibrary *mLib;
   fnModuleDispatch mDispatch;
};

class ModuleManager:public wxModule
{
public:
   ModuleManager() {};
   virtual ~ModuleManager() {};

   virtual bool OnInit();
   virtual void OnExit();

   static void Initialize();
   static int Dispatch(ModuleDispatchTypes type);

private:
   static ModuleManager *mInstance;

   wxArrayPtrVoid mModules;

   DECLARE_DYNAMIC_CLASS(ModuleManager);
};

#endif /* __AUDACITY_LOADMODULES_H__ */

// Indentation settings for Vim and Emacs and unique identifier for Arch, a
// version control system. Please do not modify past this point.
//
// Local Variables:
// c-basic-offset: 3
// indent-tabs-mode: nil
// End:
//
// vim: et sts=3 sw=3


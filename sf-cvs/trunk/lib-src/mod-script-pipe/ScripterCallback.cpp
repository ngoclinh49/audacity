// ScripterCallback.cpp : 
//
// A loadable module that connects a windows named pipe
// to a registered service function that is able to
// process a single command at a time.
//
// The service function is provided by the application
// and not by libscript.  mod_script_pipe was developed for
// Audacity.  Because it forwards commands
// rather than handling them itself it can be used in
// other projects too.  

#include <wx/wx.h>
#include "ScripterCallback.h"
//#include "../lib_widget_extra/ShuttleGuiBase.h"
#include "../../src/ShuttleGui.h"

#ifdef NOT_DEFINED

// This is 'traditional code' for DLLs, which we're not
// using.

//#include "stdafx.h"

BOOL APIENTRY DllMain( HANDLE hModule, 
                       DWORD  ul_reason_for_call, 
                       LPVOID lpReserved
					 )
{
    switch (ul_reason_for_call)
	{
		case DLL_PROCESS_ATTACH:
		case DLL_THREAD_ATTACH:
		case DLL_THREAD_DETACH:
		case DLL_PROCESS_DETACH:
			break;
    }
    return TRUE;
}

#endif


extern void PipeServer();


extern "C" {

   // This is an example of an exported variable

typedef SCRIPT_PIPE_DLL_IMPORT int (*tpExecScriptServerFunc)( wxString * pIn);


static tpExecScriptServerFunc pScriptServerFn=NULL;

wxString Str2;
wxArrayString aStr;
int iSent;

// Do serv sends the command back to the script server function,
// and creates a list of response lines to be sent.
int DoSrv( char * pIn )
{
   wxString Str1(pIn, wxConvISO8859_1);
   Str1.Replace( wxT("\r"), wxT(""));
   Str1.Replace( wxT("\n"), wxT(""));
   (*pScriptServerFn)( &Str1 );

   /* Responses are disabled for now.
   size_t l = Str2.Length();
   Str2+= wxT('\n');
   aStr.Clear();
   iSent = -1;
   size_t iStart = 0;
   size_t i;
   for(i=0;i<=l;i++)
   {
      if( Str2[i] == wxT('\n') )
      {
         aStr.Add( Str2.Mid( iStart, i-iStart) );
         iStart = i+1;
      }
   }
   */
//   wxLogDebug("Added %i Strings", aStr.GetCount());
   return 1;
}

// DoSrvMore yields one script line at a time.
int DoSrvMore( char * pOut, int nMax )
{

   wxString Temp;

   if( aStr.GetCount() == 1 )
   {
      if( iSent != -1 )
         return -1;
      iSent++;
      Temp = aStr[0];
   }
   else if( iSent == -1 )
   {
      Temp = wxString::Format(wxT("Lines:%i"), aStr.GetCount());
      iSent++;
   }
   else
   {
      if( iSent >= (int)aStr.GetCount() )
         return -2;
      Temp = aStr[iSent];
      iSent++;
   }
   size_t l;
   l = Temp.Length();
   if( l>= (nMax-3) )
      l=(nMax-4);
   memcpy( pOut, Temp.mb_str(), l );
   pOut[l] = '\n';
   pOut[l+1] = '\0';
   return l+2;
}


int SCRIPT_PIPE_DLL_API RegScriptServerFunc( tpExecScriptServerFunc pFn )
{
   if( pFn )
   {
      pScriptServerFn = pFn;
      PipeServer();
   }
   return 4;
}

int SCRIPT_PIPE_DLL_API ScriptServerResponseFunc( wxString * pOut )
{
   //wxLogDebug(wxT(*pOut));
   return 1;
}

// This is an example of an exported function.
int SCRIPT_PIPE_DLL_API ExtensionModuleInit(int ix)
{
//   pExecFunc = NULL;
   ix;// compiler food.

#if defined(_DEBUG)
   wxLogDebug(wxT("Got into DLL"));
#endif

   // Here is proof that the DLL was dynamically loaded and this Init function
   // called.
   //wxDialog Dlg( (wxWindow*)NULL, (wxWindowID)-1, wxT("mod-script-pipe - Dialog Loaded by Plug In"), wxPoint(0,0));


#if 0
   ShuttleGui S( &Dlg, eIsCreating );
   S.StartStatic( "Scripter Initialising" );
   S.StartHorizontalLay();
   S.Id(wxID_CANCEL).AddButton( "Cancel" ); 
   S.Id(wxID_OK).AddButton( "OK" )->SetFocus(); 
   S.EndHorizontalLay();
   S.EndStatic();
#endif

   /*
   Dlg.Fit();
   Dlg.Move( 100,100 );
   int id = Dlg.ShowModal();
   */
//printf("id = %d\n", id);
   // return -1 for cancel, anything else for OK.
//   return (id==wxID_CANCEL)?-1:42;
   return 0;
}


}


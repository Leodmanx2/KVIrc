//=============================================================================
//
//   File : kvi_kvs_coresimplecommands_af.cpp
//   Created on Fri 31 Oct 2003 00:04:25 by Szymon Stefanek
//
//   This file is part of the KVIrc IRC client distribution
//   Copyright (C) 2003 Szymon Stefanek <pragma at kvirc dot net>
//
//   This program is FREE software. You can redistribute it and/or
//   modify it under the terms of the GNU General Public License
//   as published by the Free Software Foundation; either version 2
//   of the License, or (at your opinion) any later version.
//
//   This program is distributed in the HOPE that it will be USEFUL,
//   but WITHOUT ANY WARRANTY; without even the implied warranty of
//   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
//   See the GNU General Public License for more details.
//
//   You should have received a copy of the GNU General Public License
//   along with this program. If not, write to the Free Software Foundation,
//   Inc. ,59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
//
//=============================================================================

#define __KVIRC__

#include "kvi_kvs_coresimplecommands.h"


#include "kvi_window.h"
#include "kvi_out.h"
#include "kvi_locale.h"
#include "kvi_app.h"
#include "kvi_options.h"
#include "kvi_ircview.h" // this is only for KviIrcView::NoTimestamp
#include "kvi_debugwindow.h"
#include "kvi_console.h"
#include "kvi_scriptbutton.h"
#include "kvi_iconmanager.h"

#include "kvi_kvs_popupmanager.h"
#include "kvi_kvs_eventmanager.h"
#include "kvi_kvs_kernel.h"
#include "kvi_kvs_object_controller.h"

#ifndef COMPILE_NO_X_BELL
	#include "kvi_xlib.h" // XBell : THIS SHOULD BE INCLUDED AS LAST!
	#include <unistd.h>   // for usleep();
#endif

#include "kvi_tal_tooltip.h"

// kvi_app.cpp
extern KviAsciiDict<KviWindow> * g_pGlobalWindowDict;

namespace KviKvsCoreSimpleCommands
{
	///////////////////////////////////////////////////////////////////////////////////////////////////////

	/*
		@doc: admin
		@type:
			command
		@title:
			admin
		@syntax:
			admin [target server]
		@short:
			Requests the admin info from a server
		@description:
			Requests admin information from the specified server or the current server if no [target server] is specified.[br]
			This command is a [doc:rfc2821wrappers]RFC2821 command wrapper[/doc]; see that document for more information.[br]
	*/
	// RFC2821 wrapper

	///////////////////////////////////////////////////////////////////////////////////////////////////////

	/*
		@doc: away
		@type:
			command
		@title:
			away
		@syntax:
			away [-a | --all-networks] [<reason:string>]
		@short:
			Puts you into 'away' state
		@switches:
			!sw: -a | --all-networks
			Set away on all networks
		@description:
			Puts you into 'away' state in the connection associated to the
			current [b]IRC context[/b].[br] This command is "server based";
			this means that the effects will be visible only after the
			server has acknowledged the change.[br]
			When you use this command, other people will know that you are 
			away from the keyboard, and they will know why you're not here.[br]
			To return from being away you must use [cmd]back[/cmd].[br]
			This command is [doc:connection_dependant_commands]connection dependant[/doc].[br]
		@examples:
			[example]
			away I'm asleep. Don't wake me up.
			[/example]
	*/

	KVSCSC(away)
	{
		QString szReason;
		KVSCSC_PARAMETERS_BEGIN
			KVSCSC_PARAMETER("reason",KVS_PT_STRING,KVS_PF_OPTIONAL | KVS_PF_APPENDREMAINING,szReason)
		KVSCSC_PARAMETERS_END

		KVSCSC_REQUIRE_CONNECTION

		if(szReason.isEmpty())szReason = KVI_OPTION_STRING(KviOption_stringAwayMessage);
	
		if(KVSCSC_pSwitches->find('a',"all-networks"))
		{
			KviAsciiDictIterator<KviWindow> it(*g_pGlobalWindowDict);
			while(KviWindow * wnd = it.current())
			{
				if(wnd->type()==KVI_WINDOW_TYPE_CONSOLE)
				{
					KviConsole* pConsole=(KviConsole*)wnd;
					if(pConsole->isConnected())
						pConsole->connection()->sendFmtData("AWAY :%s",
								pConsole->connection()->encodeText(szReason).data()
								);
				}
				++it;
			}
		} else  {
			QCString szR = KVSCSC_pConnection->encodeText(szReason);
			if(!(KVSCSC_pConnection->sendFmtData("AWAY :%s",szR.data())))
				return KVSCSC_pContext->warningNoIrcConnection();
		}
	
		return true;
	}

	///////////////////////////////////////////////////////////////////////////////////////////////////////

	/*
		@doc: back
		@type:
			command
		@title:
			back
		@syntax:
			back [-a | --all-networks]
		@switches:
			!sw: -a | --all-networks
			Set back on all networks
		@short:
			Allows you to return from being away
		@description:
			Using this command makes you return from being [cmd]away[/cmd] in the connection associated to the
			current [b]IRC context[/b].[br] This command is "server based";
			this means that the effects will be visible only after the
			server has acknowledged the change.[br]
			This command is [doc:connection_dependant_commands]connection dependant[/doc].[br]
		@examples:
			[example]
			back
			[/example]
	*/

	KVSCSC(back)
	{
		
		if(KVSCSC_pSwitches->find('a',"all-networks"))
		{
			KviAsciiDictIterator<KviWindow> it(*g_pGlobalWindowDict);
			while(KviWindow * wnd = it.current())
			{
				if(wnd->type()==KVI_WINDOW_TYPE_CONSOLE)
				{
					KviConsole* pConsole=(KviConsole*)wnd;
					if(pConsole->isConnected())
						pConsole->connection()->sendFmtData("AWAY");
				}
				++it;
			}
		} else {
			KVSCSC_REQUIRE_CONNECTION
	
			if(!(KVSCSC_pConnection->sendFmtData("AWAY")))
				return KVSCSC_pContext->warningNoIrcConnection();
		}
	
		return true;
	}
	
	///////////////////////////////////////////////////////////////////////////////////////////////////////

	/*
		@doc: ban
		@type:
			command
		@title:
			ban
		@syntax:
			ban <mask_list>
		@short:
			Sets ban masks for the channel
		@description:
			Sets the ban masks specified in the <mask_list>,
			which is a comma separated list of nicknames. 
			This command works only if executed in a channel window.
			The command is translated to a set of MODE messages containing
			a variable number of +b flags.
			This command is [doc:connection_dependant_commands]connection dependant[/doc].
		@examples:
			[example]
			ban Maxim,Gizmo!*@*,*!root@*
			[/example]
		@seealso:
			[cmd]op[/cmd],[cmd]deop[/cmd],[cmd]voice[/cmd],[cmd]devoice[/cmd],[cmd]unban[/cmd]
	*/

	KVSCSC(ban)
	{
		return multipleModeCommand(__pContext,__pParams,__pSwitches,'+','b');
	}

	///////////////////////////////////////////////////////////////////////////////////////////////////////

	/*
	 	@doc: beep
		@type:
			command
		@title:
			beep
		@syntax:
			beep [-p=<pitch:uint>] [-d=<duration:uint>] [-s] [volume:uint]
		@short:
			Beep beep!
		@switches:
			!sw: -p=<pitch:uint> | --pitch=<pitch:uint>
			Sets the bell to the specified pitch, if possible
			!sw: -d=<duration:uint> | --duration=<duration:uint>
			Sets the duration of the beep to <duration> milliseconds
			!sw: -s | --synchronous
			Causes KVIrc to wait for completion of the beeping before
			returning from this command
		@description:
			Beeps (when possible :D)[br]
			...[br]
			No , really..[br]
			This command rings the bell on the keyboard (the PC speaker).
			The volume must be in range 0-100; the default is 100.[br]
			The pitch is specified in Hz and must be positive.[br]
			The duration is specified in milliseconds.[br]
			An invalid (or unspecified) pitch, volume or duration
			makes KVIrc to use the default values set by the system.[br]
			The duration of the bell is only indicative and
			can be shortened by a subsequent call to /beep (that
			will override the currently playing one).[br]
			On Windows, the bell is always synchronous and it is not
			event granted that the bell will be a bell at all... you might
			get the system default sound instead.. so be careful if you
			want to write portable scripts :)[br]
			If the -s switch is specified the bell becomes synchronous:
			KVIrc waits the bell to complete before continuing.[br]
			Obviously -s is senseless on Windows.[br]
			(WARNING : the main KVIrc thread is stopped in that case
			so if you play long notes (duration > 100)
			the entire application will appear to freeze for a while).[br]
			The precision of the bell pitch, duration and
			volume is strongly dependant on the system and the underlying hardware.[br]
	*/

	KVSCSC(beep)
	{
		kvs_uint_t uVolume;
		KVSCSC_PARAMETERS_BEGIN
			KVSCSC_PARAMETER("volume",KVS_PT_UINT,KVS_PF_OPTIONAL,uVolume)
		KVSCSC_PARAMETERS_END

		kvs_int_t pitch    = -1;
		kvs_int_t duration = -1;
		bool bSync   = (KVSCSC_pSwitches->find('s',"sync") != 0);
		bool bOk = false;

		KviKvsVariant * pPitch = KVSCSC_pSwitches->find('p',"pitch");
		if(pPitch)
		{
			if(!pPitch->asInteger(pitch))
			{
				KVSCSC_pContext->warning(__tr2qs("Invalid pitch value: using default"));
				pitch = -1;
			}
		}

		KviKvsVariant * pDuration = KVSCSC_pSwitches->find('d',"duration");
		if(pDuration)
		{
			if(!pDuration->asInteger(duration))
			{
				KVSCSC_pContext->warning(__tr2qs("Invalid duration value: using default"));
				duration = -1;
			}
		}
	
		if((uVolume > 100) || (uVolume < 1))uVolume = 100;

#ifdef COMPILE_ON_WINDOWS
		Beep(pitch,duration);
#else
	#ifndef COMPILE_NO_X_BELL

		XKeyboardState st;
		XKeyboardControl ctl;

		XGetKeyboardControl(qt_xdisplay(),&st);

		unsigned long mask = KBBellPercent;
		ctl.bell_percent = uVolume;
		if(pitch >= 0)
		{
			ctl.bell_pitch    = pitch;
			mask             |= KBBellPitch;
		}
		if(duration >= 0)
		{
			ctl.bell_duration = duration;
			mask             |= KBBellDuration;
		}
		XChangeKeyboardControl(qt_xdisplay(),mask,&ctl);

		XBell(qt_xdisplay(),100);

		if(bSync)
		{
			if(duration >= 0)usleep(duration * 1000);
			else usleep(st.bell_duration * 1000);
		}

		ctl.bell_pitch = st.bell_pitch;
		ctl.bell_duration = st.bell_duration;
		ctl.bell_percent = st.bell_percent;

		XChangeKeyboardControl(qt_xdisplay(),mask,&ctl);

	#endif //COMPILE_NO_X_BELL
#endif
		return true;
	}

	///////////////////////////////////////////////////////////////////////////////////////////////////////

	/*
		@doc: buttonctl
		@title:
			buttonctl
		@type:
			command
		@short:
			changes user definable buttons
		@syntax:
			buttonctl [-q] <type:string> <name:string> <operation:string> [parameter:string]
		@switches:
			!sw: -q | --quiet
			Run quietly
		@description:
			Changes an existing user defined button.[br]
			<type_unused> is ignored and present only for backward compatibility.[br]
			<name> is the name of the button.[br]
			<operation> may be one of the constant strings "enable", "disable", "image",
			"text".[br]
			Operations "enable" and "disable" do not require the fourth [parameter] and have
			the obvious meaning.[br] Operation "image" requires the [parameter] to be
			a valid [doc:image_id]image_id[/doc] and sets the button image.[br]
			Operation "text" requires the [parameter] (and in fact all the following ones)
			to be a string containing the button text label.[br]
			The <operation> constants may be abbreviated, even to the single letters 'e','d','i' and 't'.[br]
			The -q switch causes the command to be quiet about errors and warnings.[br]
		@seealso:
			[cmd]button[/cmd]
	*/

//#warning	"ALSO /HELP must NOT interpret the identifiers!"
//#warning	"ALSO /DEBUG that relays to the DEBUG WINDOW"
	
	KVSCSC(buttonctl)
	{
		QString tbTypeUnused,tbName,tbOp,tbPar;
		KVSCSC_PARAMETERS_BEGIN
			KVSCSC_PARAMETER("type",KVS_PT_STRING,0,tbTypeUnused)
			KVSCSC_PARAMETER("name",KVS_PT_STRING,0,tbName)
			KVSCSC_PARAMETER("operation",KVS_PT_STRING,0,tbOp)
			KVSCSC_PARAMETER("parameter",KVS_PT_STRING,KVS_PF_OPTIONAL,tbPar)
		KVSCSC_PARAMETERS_END

		KviScriptUserButton * pButton = 0;

		if(!KVSCSC_pWindow->buttonContainer())
		{
			if(!KVSCSC_pSwitches->find('q',"quiet"))KVSCSC_pContext->warning(__tr2qs("The specified window has no button containers"));
			return true;
		}
	
		pButton = (KviScriptUserButton *)(KVSCSC_pWindow->buttonContainer())->child(tbName,"KviWindowScriptButton");

		if(!pButton)
		{
			if(!KVSCSC_pSwitches->find('q',"quiet"))KVSCSC_pContext->warning(__tr2qs("No button with type %Q named %Q"),&tbTypeUnused,&tbName);
			return true;
		}
		QChar o;
		if (tbOp.length() > 0) o=tbOp[0];
			else o=QChar('x');

	//	QChar o = tbOp.length() > 0 ? tbOp[0] : QChar('x');

		switch(o.unicode())
		{
			case 't':
				KviTalToolTip::remove(pButton);
				KviTalToolTip::add(pButton,tbPar);
				pButton->setButtonText(tbPar);
			break;
			case 'i':
				if(!tbPar.isEmpty())
				{
					QPixmap * pix = g_pIconManager->getImage(tbPar);
					if(pix)
					{
						pButton->setButtonPixmap(*pix);
					} else {
						if(!KVSCSC_pSwitches->find('q',"quiet"))KVSCSC_pContext->warning(__tr2qs("Can't find the icon '%Q'"),&tbPar);
					}
				}
			break;
			case 'e':
				pButton->setEnabled(true);
			break;
			case 'd':
				pButton->setEnabled(false);
			break;
		}
		return true;
	
	}

	///////////////////////////////////////////////////////////////////////////////////////////////////////

	/*
		@doc: codepage
		@type:
			command
		@title:
			codepage
		@syntax:
			codepage <encoding name>
		@short:
			Tries to set the codepage on server
		@description:
			This is a not-widely implemented extension
			that allows the user to set the codepage mapping
			on server.
			This command is a [doc:rfc2821wrappers]RFC2821 command wrapper[/doc]; see that document for more information.[br]
	*/
	// RFC2821 wrapper

	///////////////////////////////////////////////////////////////////////////////////////////////////////

	/*
		@doc: ctcp
		@type:
			command
		@title:
			ctcp
		@syntax:
			ctcp [-n] <target:string> <ctcp_data:string>
		@short:
			Sends a CTCP message
		@description:
			Sends a CTCP message to the specified <target>.[br]
			The target may be a nickname , a channel, or a comma separated list of nicknames.[br]
			The <ctcp_data> is a string containing the ctcp type followed by the ctcp parameters.[br]
			For more info take a look at the [doc:ctcp_handling]ctcp protocol implementation notes[/doc].[br]
			The CTCP message will be a request (sent through a PRIVMSG) unless the -n switch
			specified: in that case it will be a reply (sent through a NOTICE).[br]
			If <ctcp_data> is the single string "ping" then a trailing time string argument
			is added in order to determine the round trip time when the ping reply comes back.
			To override this behaviour simply specify your own time string parameter.[br]
			This command is [doc:connection_dependant_commands]connection dependant[/doc].[br]
		@examples:
			[example]
			ctcp Pragma VERSION
			[/example]
	*/

	KVSCSC(ctcp)
	{
		QString szTarget,szCtcpData;
		KVSCSC_PARAMETERS_BEGIN
			KVSCSC_PARAMETER("target",KVS_PT_NONEMPTYSTRING,0,szTarget)
			KVSCSC_PARAMETER("ctcp_data",KVS_PT_STRING,KVS_PF_OPTIONAL | KVS_PF_APPENDREMAINING,szCtcpData)
		KVSCSC_PARAMETERS_END

		KVSCSC_REQUIRE_CONNECTION
		
		if(KviQString::equalCI(szCtcpData,"PING"))
		{
			struct timeval tv;
			kvi_gettimeofday(&tv,0);
			KviQString::appendFormatted(szCtcpData," %d.%d",tv.tv_sec,tv.tv_usec);
		}

		QCString szT = KVSCSC_pConnection->encodeText(szTarget);
		QCString szD = KVSCSC_pConnection->encodeText(szCtcpData);
	
		if(!(KVSCSC_pConnection->sendFmtData("%s %s :%c%s%c",
				KVSCSC_pSwitches->find('n',"notice") ? "NOTICE" : "PRIVMSG",szT.data(),0x01,szD.data(),0x01)))
			return KVSCSC_pContext->warningNoIrcConnection();

		return true;
	}

	///////////////////////////////////////////////////////////////////////////////////////////////////////

	/*
		@doc: debug
		@type:
			command
		@title:
			debug
		@syntax:
			debug <text>
		@short:
			Outputs text to the debug window
		@switches:
		@description:
			Outputs the &lt;text&gt; to the debug window.[br]
		@seealso:
	*/

	KVSCSC(debug)
	{
		QString szAll;
		KVSCSC_pParams->allAsString(szAll);
		KviWindow * pWnd = KviDebugWindow::getInstance();
		pWnd->outputNoFmt(KVI_OUT_NONE,szAll);
		return true;
	}

	///////////////////////////////////////////////////////////////////////////////////////////////////////

	/*
		@doc: delete
		@type:
			command
		@title:
			delete
		@syntax:
			delete [-q] [-i] <objectHandle>
		@short:
			Destroys an object
		@switches:
			!sw: -q | --quiet
			Causes the command to run quietly
			!sw: -i | --immediate
			Causes the object to be destroyed immediately
			instead of simply scheduling its later deletion.
		@description:
			Schedules for destruction the object designed by <objectHandle>.
			This command is internally aliased to [cmd]destroy[/cmd].
			Please note that the object is NOT immediately destroyed:
			it will be destroyed when KVIrc returns to the main event loop,
			so after the current script code part has been executed.
			This behaviour makes the object destruction safe in any
			part of the script, but may lead to problems when
			using signals and slots.[br]
			For example, when you delete an object that emits some signals,
			the signals may be still emitted after the delete call.
			You have to disconnect the signals explicitly if you don't want it
			to happen.[br]
			Alternatively you can use the -i switch: it causes the object
			to be destructed immediately but is intrinsicly unsafe:
			in complex script scenarios it may lead to a SIGSEGV;
			usually when called from one of the deleted object function
			handlers, or from a slot connected to one of the deleted object
			signals. Well, it actually does not SIGSEGV, but I can't guarantee it;
			so, if use the -i switch, test your script 10 times before releasing it.
			The -q switch causes the command to run a bit more silently: it still
			complains if the parameter passed is not an object reference, but
			it fails silently if the reference just points to an inexisting object (or is null).
		@examples:
			[example]
			[/example]
	*/

	/*
		@doc: destroy
		@type:
			command
		@title:
			destroy
		@syntax:
			destroy [-q] [-i] <objectHandle>
		@short:
			Destroys an object
		@description:
			This is a builtin alias for the command [cmd]delete[/cmd]
	*/

	KVSCSC(deleteCKEYWORDWORKAROUND)
	{
		kvs_hobject_t hObject;
		KVSCSC_PARAMETERS_BEGIN
			KVSCSC_PARAMETER("objectHandle",KVS_PT_HOBJECT,0,hObject)
		KVSCSC_PARAMETERS_END
		if(hObject == (kvs_hobject_t)0)
		{
			if(!KVSCSC_pSwitches->find('q',"quiet"))
				KVSCSC_pContext->warning(__tr2qs("Can't delete a null object reference"));
		} else {
			KviKvsObject * o = KviKvsKernel::instance()->objectController()->lookupObject(hObject);
			if(!o)
			{
				if(!KVSCSC_pSwitches->find('q',"quiet"))
					KVSCSC_pContext->warning(__tr2qs("Can't delete an inexisting object"));
			} else {
				if(KVSCSC_pSwitches->find('i',"immediate"))
					o->dieNow();
				else
					o->die();
			}
		}
		return true;
	}

	///////////////////////////////////////////////////////////////////////////////////////////////////////

	/*
		@doc: delpopupitem
		@type:
			command
		@title:
			delpopupitem
		@syntax:
			delpopupitem [-d] [-q] <popupname:string> <item_id:string>
		@short:
			Deletes an item from a popup
		@switches:
			!sw: -q | --quiet
			Run quietly
			!sw: -d | --deep
			Search the whole popup tree instead of only the first level
		@description:
			Deletes the item specified by <id> from the poup <popupname>.
			If the -d flag is specified then the item with the specified
			<id> is seached in the whole popup tree (containing submenus)
			otherwise it is searched only in the first level.[br]
			If the -q flag is specified the command does not complain
			about inexisting items or inexisting popup menus.[br]
			See [cmd]defpopup[/cmd] for more informations.[br]
		@seealso:
			[cmd]defpopup[/cmd],[cmd]popup[/cmd]
	*/
	// FIXME: #warning "Separator should have the expression too ?"


	KVSCSC(delpopupitem)
	{
		QString szPopupName,szItemId;
		KVSCSC_PARAMETERS_BEGIN
			KVSCSC_PARAMETER("popupname",KVS_PT_NONEMPTYSTRING,0,szPopupName)
			KVSCSC_PARAMETER("item_id",KVS_PT_NONEMPTYSTRING,0,szItemId)
		KVSCSC_PARAMETERS_END

		KviKvsPopupMenu * p = KviKvsPopupManager::instance()->lookup(szPopupName);
		if(!p)
		{
			if(!KVSCSC_pSwitches->find('q',"quiet"))
				KVSCSC_pContext->warning(__tr2qs("Inexisting popup \"%Q\""),&szPopupName);
			return true;
		}
	
		if(p->isLocked())
		{
			KVSCSC_pContext->error(__tr2qs("Popup menu self-modification is not allowed (the popup is probably open)"));
			return false;
		}
	
		if(!p->removeItemByName(szItemId,KVSCSC_pSwitches->find('d',"deep")))
		{
			if(!KVSCSC_pSwitches->find('q',"quiet"))
				KVSCSC_pContext->warning(__tr2qs("The menu item with id \"%Q\" does not exist in popup \"%Q\""),&szItemId,&szPopupName);
		}
	
		return true;
	}

	///////////////////////////////////////////////////////////////////////////////////////////////////////

	/*
		@doc: deop
		@type:
			command
		@title:
			deop
		@syntax:
			deop <nickname_list>
		@short:
			Removes chanop status from the specified users
		@description:
			Removes channel operator status to the users specified in <nickname_list>,
			which is a comma separated list of nicknames. 
			This command works only if executed in a channel window.
			The command is translated to a set of MODE messages containing
			a variable number of -o flags.
			This command is [doc:connection_dependant_commands]connection dependant[/doc].
		@examples:
			[example]
			deop Pragma,Crocodile
			[/example]
		@seealso:
			[cmd]op[/cmd],[cmd]voice[/cmd],[cmd]devoice[/cmd]
	*/

	KVSCSC(deop)
	{
		return multipleModeCommand(__pContext,__pParams,__pSwitches,'-','o');
	}

	///////////////////////////////////////////////////////////////////////////////////////////////////////

	/*
		@doc: devoice
		@type:
			command
		@title:
			devoice
		@syntax:
			devoice <nickname_list>
		@short:
			Removes voice flag for the specified users
		@description:
			Removes the voice flag for the users specified in <nickname_list>,
			which is a comma separated list of nicknames. 
			This command works only if executed in a channel window.
			The command is translated to a set of MODE messages containing
			a variable number of -v flags.
			This command is [doc:connection_dependant_commands]connection dependant[/doc].
		@examples:
			[example]
			devoice Pragma,Crocodile
			[/example]
		@seealso:
			[cmd]op[/cmd],[cmd]deop[/cmd],[cmd]voice[/cmd]
	*/

	KVSCSC(devoice)
	{
		return multipleModeCommand(__pContext,__pParams,__pSwitches,'-','v');
	}

	///////////////////////////////////////////////////////////////////////////////////////////////////////

	/*
		@doc: die
		@type:
			command
		@title:
			die
		@syntax:
			die <message:text>
		@short:
			Prints an error message and stops the script
		@description:
			Prints an error message and stops the current script
			This command is equivalent to [cmd]error[/cmd]
	*/
	// Internally aliased to error

	///////////////////////////////////////////////////////////////////////////////////////////////////////

	/*
		@doc: echo
		@type:
			command
		@title:
			echo
		@syntax:
			echo [-d] [-w=<window_id>] [-i=<icon_number>] [-n] <text>
		@short:
			Outputs text to a KVirc window
		@switches:
			!sw: -w=<window_id> | --window=<window_id>
			Causes the output to be redirected to the window specified by &lt;window_id&gt
			!sw: -i=<icon_number> | --icon=<icon_number>
			Causes the output to use the icon & color scheme specified by &lt;icon_number&gt
			!sw: -n | --no-timestamp
			Disables the message timestamping
			!sw: -d | --debug
			Send the output to the debug window (takes precedence over -w)
		@description:
			Outputs the &lt;text&gt; to the current window.[br]
			If the 'w' switch is present , outputs the &lt;text&gt;
			to the specified window instead of the current one.
			The <window_id&> parameter is the [doc:window_naming_conventions]global ID[/doc] of the window
			that has to be used.[br]
			If the 'i' switch is given , it uses the specified
			icon scheme (icon and colors) , otherwise it uses
			the default one (0).[br]
			If the -d switch is used then the output is sent to a special
			window called "Debug" (the window is created if not existing yet).
			This is useful for script debugging purposes (you get the output 
			in Debug regardless of the window that the executed command is attacched to).
			The KVIrc view widgets support clickable links that can be realized by using special [doc:escape_sequences]escape sequences[/doc].[br]
			The 'n' switch disables timestamping so you can output your own timestamp
			or not timestamp at all.[br]
		@examples:
			[example]
			echo Hey! this is my first echo test!
			echo -i=10 This text has a specified icon and colors
			echo --icon=[fnc]$icon[/fnc](parser error) this has the colors of the parser error messages
			[/example]
		@seealso:
			[fnc]$window[/fnc],
			[doc:window_naming_conventions]window naming conventions documentation[/doc]
	*/

	KVSCSC(echo)
	{
		QString szAll;
		KVSCSC_pParams->allAsString(szAll);

		kvs_int_t iMsgType = KVI_OUT_NONE;
		KviWindow * pWnd = KVSCSC_pContext->window();

		if(!KVSCSC_pSwitches->isEmpty())
		{
			KviKvsVariant * v;
			if((v = KVSCSC_pSwitches->find('w',"window")))
			{
				QString szWnd;
				v->asString(szWnd);
	//#warning "FIXME: the window database is not unicode! (we even could keep integer window id's at this point!)"
				pWnd = g_pApp->findWindow(szWnd.utf8().data());
				if(!pWnd)
				{
					KVSCSC_pContext->warning(__tr2qs("The argument of the -w switch did not evaluate to a valid window id: using default"));
					pWnd = KVSCSC_pContext->window();
				}
			}
				
			if((v = KVSCSC_pSwitches->find('i',"icon")))
			{
				if(!v->asInteger(iMsgType))
				{
					KVSCSC_pContext->warning(__tr2qs("The argument of the i switch did not evaluate to a number: using default"));
					iMsgType = KVI_OUT_NONE;
				} else {
					iMsgType = iMsgType % KVI_NUM_MSGTYPE_OPTIONS;
				}
			}
			
			if(KVSCSC_pSwitches->find('d',"debug"))
			{
				pWnd = KviDebugWindow::getInstance();
			}
		}

		int iFlags = KVSCSC_pSwitches->find('n',"no-timestamp") ? KviIrcView::NoTimestamp : 0;
		pWnd->outputNoFmt(iMsgType,szAll,iFlags);
		return true;
	}

	///////////////////////////////////////////////////////////////////////////////////////////////////////

	/*
		@doc: echoprivmsg
		@type:
			command
		@title:
			echoprivmsg
		@syntax:
			echoprivmsg [switches] <nick:string> <user:string> <host:string> <text:string>
		@short:
			Outputs text to a KVIrc window
		@switches:
			!sw: -p[=<nick_prefix>] | --prefix[=<prefix>]
			The message is printed with the specified custom nickname prefix.
			If <nick_prefix> is omitted then an empty string is assumed.
			!sw: -s[=<nick_suffix>] | --suffix[=<nicksuffix>]
			The message is printed with the specified custom nickname suffix.
			If <nick_suffix> is omitted then an empty string is assumed.
			!sw: -w=<window_id> | --window=<window_id>
			The message is printed to the window specified by [doc:window_naming_conventions]window_id[/doc]
			!sw: -i=<color_set> | --color-set=<color_set>
			Causes the message to use the specified icon scheme (icon and colors).
			!sw: -n | --no-highlighting
			Do not apply the highlighting rules
			!sw: -x | --no-notifier
			Never cause the notifier window to pop up
			!sw: -f | --no-flashing
			Never cause the window taskbar entry to flash (this works only on some platforms)
		@description:
			Outputs a <text> to the current window in the privmsg format.[br]
			The <nick> <user> and <host> parameters are formatted
			as specified by the user preferences (for example
			the nick may use smart colorisation).
			If you don't know the username and host then just use '*' for
			that parameters.
			The message will also get the highlighting rules applied.
			If the 'w' switch is present, outputs <text>
			to the specified window instead of the current one.
			The <window_id> parameter is the [doc:window_naming_conventions]global ID[/doc] of the window
			that has to be used.[br]
			Please note that this is not the same as the standard
			[doc:command_rebinding]-r rebinding switch[/doc]:
			-w causes the specified window to be used only for output,
			but the command parameters are evaluated in the current window.[br]
			If the 'i' switch is given , it uses the specified
			icon scheme (icon and colors) , otherwise it uses
			the default one (0).[br]
			If the -n switch is present then the highlighting rules
			are not applied.[br]
			If the -x switch is present then the message will never cause
			the notifier window to be popped up.[br]
			If the -f switch is present then the message will never cause
			the system taskbar to flash.[br]
			Actually -x and -f have a sense only if highlighting is used and thus -n is not present.[br]
			For more informations about the icon/color schemes see [fnc]$msgtype[/fnc].
			The KVIrc view widgets support clickable sockets that can be realized by using special [doc:escape_sequences]escape sequences[/doc].[br]
		@examples:
			[example]
			echoprivmsg Test * * This is a test message
			echoprivmsg -i=$msgtype(ChanPrivmsgCrypted) Pragma pragma staff.kvirc.net Hi people! :)
			[/example]
		@seealso:
			[fnc]$window[/fnc],
			[fnc]$window.caption[/fnc],
			[fnc]$msgtype[/fnc],
			[cmd]echo[/cmd],
			[doc:window_naming_conventions]Window Naming Conventions[/doc]
	*/

	KVSCSC(echoprivmsg)
	{
		QString szNick,szUser,szHost,szText;
		KVSCSC_PARAMETERS_BEGIN
			KVSCSC_PARAMETER("nick",KVS_PT_NONEMPTYSTRING,0,szNick)
			KVSCSC_PARAMETER("user",KVS_PT_STRING,0,szUser)
			KVSCSC_PARAMETER("host",KVS_PT_STRING,0,szHost)
			KVSCSC_PARAMETER("text",KVS_PT_STRING,KVS_PF_OPTIONAL | KVS_PF_APPENDREMAINING,szText)
		KVSCSC_PARAMETERS_END

		int type = KVI_OUT_NONE;
		KviWindow * pWnd = KVSCSC_pWindow;
		KviConsole * pConsole = pWnd->console();
		if(!pConsole)pConsole = g_pApp->activeConsole();

		KviKvsVariant * v;

		if(v = KVSCSC_pSwitches->find('i',"color-set"))
		{
			kvs_int_t msgType;
			if(v->asInteger(msgType))
			{
				if(msgType < 0)msgType = -msgType;
				type = (int)(msgType % KVI_NUM_MSGTYPE_OPTIONS);
			} else KVSCSC_pContext->warning(__tr2qs("Invalid color-set specification, using default"));
		}

		if(v = KVSCSC_pSwitches->find('w',"window"))
		{
			QString szWin;
			v->asString(szWin);
			KviStr window = szWin;
			pWnd = g_pApp->findWindow(window.ptr());
			if(!pWnd)
			{
				KVSCSC_pContext->warning(__tr2qs("Window '%s' not found, using current one"),window.ptr());
				pWnd = KVSCSC_pWindow;
			}
		}

		QString szPrefix,szSuffix;
		bool bPrefix = false;
		bool bSuffix = false;

		if(v = KVSCSC_pSwitches->find('p',"prefix"))
		{
			v->asString(szPrefix);
			bPrefix = true;
		}
		if(v = KVSCSC_pSwitches->find('s',"suffix"))
		{
			v->asString(szSuffix);
			bSuffix = true;
		}

		int iFlags = 0;
		if(KVSCSC_pSwitches->find('n',"no-highlighting"))iFlags |= KviConsole::NoHighlighting;
		if(KVSCSC_pSwitches->find('f',"no-flashing"))iFlags |= KviConsole::NoWindowFlashing;
		if(KVSCSC_pSwitches->find('x',"no-notifier"))iFlags |= KviConsole::NoNotifier;
		
		pConsole->outputPrivmsg(pWnd,type,
				szNick,szUser,szHost,szText,
				iFlags,
				bPrefix ? szPrefix : QString::null,bSuffix ? szSuffix : QString::null);

		return true;
	}

	///////////////////////////////////////////////////////////////////////////////////////////////////////

	/*
		@doc: error
		@type:
			command
		@title:
			error
		@syntax:
			error <message:text>
		@short:
			Prints an error message and stops the script
		@description:
			Prints an error message and stops the current script
			This command is equivalent to [cmd]die[/cmd]
		@seealso:
			[cmd]warning[/cmd]
	*/

	KVSCSC(error)
	{
#ifdef COMPILE_NEW_KVS
		QString szAll;
		KVSCSC_pParams->allAsString(szAll);
		KVSCSC_pContext->error("%Q",&szAll);
#endif
		return false;
	}

	///////////////////////////////////////////////////////////////////////////////////////////////////////

	/*
		@doc: eval
		@type:
			command
		@title:
			eval
		@syntax:
			eval [-q] [-r=<window>] <command>
		@switches:
			!sw: -q | --quiet
			Disable any error output
			!sw: -f | --force
			Continue execution even if <command> fails with an error
		@short:
			Change the behaviour of a set of commands
		@description:
			This command is useful to execute variable command sequences.[br]
			<command> is first evaluated as a normal parameter (thus identifiers
			and variables are substituted) then the evaluated string is executed
			as a command sequence.[br]
			-q causes eval to run quietly and don't display any errors in the inner command.[br]
			-f causes eval to ignore the errors inside <command> and continue execution.[br]
			This command may be used to rebind the <command> to a specified window.
			<command> shares the local variables with this command scope so you
			can easily exchange data with it.
			Remember that <command> is still a normal parameter and it must be
			enclosed in quotes if youwant it to be a complex command sequence.
			eval propagates the <command> return value.[br]
		@examples:
			[example]
				[comment]# evaluate a variable command[/comment]
				[cmd]if[/cmd](%somecondition)%tmp = "echo yeah"
				else %tmp = "echo -i=10 yeah"
				eval %tmp
				[comment]# Rebind the command to the #linux channel to get the user list[/comment]
				eval -r=[fnc]$channel[/fnc](#linux) "%Nicks[]=$chan.array;"
				[comment]# A nice alias that allows iterating commands through all the consoles[/comment]
				[comment]# This is by LatinSuD :)[/comment]
				[cmd]alias[/cmd](iterate)
				{
					%ctxt[]=[fnc]$window.list[/fnc](console,all)
					[cmd]for[/cmd](%i=0;%i<%ctxt[]#;%i++)
					{
						[cmd]eval[/cmd] -r=%ctxt[%i] $0-
					}
				}
				iterate [cmd]echo[/cmd] Hi ppl! :)
				[comment]# A little bit shorter (but less "colorful") version might be...[/comment]
				[cmd]alias[/cmd](iterate)
				{
					[cmd]foreach[/cmd](%x,[fnc]$window.list[/fnc](console,all))
							[cmd]eval[/cmd] -r=%x $0-;
				}
				iterate [cmd]echo[/cmd] Hi again!
				[comment]# Evaluate a command block[/comment]
				eval "{ echo First command!; echo Second command!; }"
			[/example]
	*/

	KVSCSC(eval)
	{
		QString szCommands;
		KVSCSC_PARAMETERS_BEGIN
			KVSCSC_PARAMETER("commands",KVS_PT_STRING,KVS_PF_APPENDREMAINING,szCommands)
		KVSCSC_PARAMETERS_END

		KviKvsScript s("eval::inner",szCommands);
		int iRunFlags = 0;
		if(KVSCSC_pContext->reportingDisabled() || KVSCSC_pSwitches->find('q',"quiet"))
			iRunFlags |= KviKvsScript::Quiet;
		bool bRet = s.run(KVSCSC_pContext,iRunFlags) ? true : false;
		if(!bRet)
		{
			if(!KVSCSC_pSwitches->find('f',"force"))
				return false;
			KVSCSC_pContext->clearError();
		}
		return true;
	}

	///////////////////////////////////////////////////////////////////////////////////////////////////////

	/*
		@doc: eventctl
		@title:
			eventctl
		@type:
			command
		@short:
			Controls the execution of event handlers
		@syntax:
			eventctl [-u] [-e] [-d] [-q] <event_name:string> <handler_name:string> [parameters]
		@switches:
			!sw: -u | --unregister
			Unregisters the specified handler
			!sw: -e | --enable
			Enables the specified handler
			!sw: -d | --disable
			Disables the specified handler
			!sw: -q | --quiet
			Do not print any warnings
		@description:
			Performs control actions on the handler <handler_name> for
			the event <event_name>.
			Without any switch it triggers the handler for testing purposes,
			eventually passing [parameters].[br]
			With the -u switch the handler <handler_name> is unregistered.[br]
			With the -d swtich is is disabled (so it is never executed)
			and with -e is enabled again.[br]
			The <event_name> may be one of the kvirc-builtin event names
			or a numeric code (from 0 to 999) of a raw server message.[br]
		@seealso:
			[cmd]event[/cmd]
	*/
	KVSCSC(eventctl)
	{
		QString szEventName,szHandlerName;
		KviKvsVariantList vList;
		KVSCSC_PARAMETERS_BEGIN
			KVSCSC_PARAMETER("event_name",KVS_PT_NONEMPTYSTRING,0,szEventName)
			KVSCSC_PARAMETER("handler_name",KVS_PT_NONEMPTYSTRING,0,szHandlerName)
			KVSCSC_PARAMETER("parameters",KVS_PT_VARIANTLIST,KVS_PF_OPTIONAL,vList)
		KVSCSC_PARAMETERS_END
		
		bool bOk;
		int iNumber = szEventName.toInt(&bOk);
		bool bIsRaw = (bOk && (iNumber >= 0) && (iNumber < 1000));

		if(bIsRaw)
		{
			if(!KviKvsEventManager::instance()->isValidRawEvent(iNumber))
			{
				if(!KVSCSC_pSwitches->find('q',"quiet"))
					KVSCSC_pContext->warning(__tr2qs("No such event (%Q)"),&szEventName);
				return true;
			}
		} else {
			iNumber = KviKvsEventManager::instance()->findAppEventIndexByName(szEventName);
			if(!KviKvsEventManager::instance()->isValidAppEvent(iNumber))
			{
				if(!KVSCSC_pSwitches->find('q',"quiet"))
					KVSCSC_pContext->warning(__tr2qs("No such event (%Q)"),&szEventName);
				return true;
			}
		}

		if(KVSCSC_pSwitches->find('u',"unregister"))
		{
			// unregister it
			if(bIsRaw)
			{
				if(!KviKvsEventManager::instance()->removeScriptRawHandler(iNumber,szHandlerName))
					if(!KVSCSC_pSwitches->find('q',"quiet"))
						KVSCSC_pContext->warning(__tr2qs("No handler '%Q' for raw numeric event '%d'"),&szHandlerName,iNumber);
			} else {
				if(!KviKvsEventManager::instance()->removeScriptAppHandler(iNumber,szHandlerName))
					if(!KVSCSC_pSwitches->find('q',"quiet"))
						KVSCSC_pContext->warning(__tr2qs("No handler '%Q' for event '%Q'"),&szHandlerName,&szEventName);
			}
		} else if(KVSCSC_pSwitches->find('e',"enable") || KVSCSC_pSwitches->find('d',"disable"))
		{
			// enable it
			if(bIsRaw)
			{
				if(!KviKvsEventManager::instance()->enableScriptRawHandler(iNumber,szHandlerName,KVSCSC_pSwitches->find('e',"enable")))
					if(!KVSCSC_pSwitches->find('q',"quiet"))
						KVSCSC_pContext->warning(__tr2qs("No handler '%Q' for raw numeric event '%d'"),&szHandlerName,iNumber);
			} else {
				if(!KviKvsEventManager::instance()->enableScriptAppHandler(iNumber,szHandlerName,KVSCSC_pSwitches->find('e',"enable")))
					if(!KVSCSC_pSwitches->find('q',"quiet"))
						KVSCSC_pContext->warning(__tr2qs("No handler '%Q' for event '%Q'"),&szHandlerName,&szEventName);
			}
		} else {
			// trigger it
			KviKvsScriptEventHandler * h;
			QString code;
			
			if(bIsRaw)
			{
				h = KviKvsEventManager::instance()->findScriptRawHandler(iNumber,szHandlerName);
			} else {
				h = KviKvsEventManager::instance()->findScriptAppHandler(iNumber,szHandlerName);
			}
	
			if(h)
			{
				KviKvsScript * s = h->script();
				KviKvsScript copy(*s);
				KviKvsVariant retVal;
				copy.run(KVSCSC_pWindow,&vList,0,KviKvsScript::PreserveParams);
			} else {
				if(!KVSCSC_pSwitches->find('q',"quiet"))
					KVSCSC_pContext->warning(__tr2qs("No handler '%Q' for event '%Q'"),&szHandlerName,&szEventName);
			}
		}

		return true;
	}
	///////////////////////////////////////////////////////////////////////////////////////////////////////

	/*
		@doc: exit
		@type:
			command
		@title:
			exit
		@syntax:
			exit
		@switches:
		@short:
			Closes KVIrc
		@description:
			It closes KVirc application
	*/

	KVSCSC(exit)
	{
		g_pApp->quit();
		return true;
	}

};

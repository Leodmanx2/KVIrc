//=============================================================================
//
//   File : kvi_kvs_eventmanager.cpp
//   Created on Thu Aug 17 2000 13:59:12 by Szymon Stefanek
//
//   This file is part of the KVIrc IRC client distribution
//   Copyright (C) 2000-2004 Szymon Stefanek <pragma at kvirc dot net>
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

#include "kvi_kvs_eventmanager.h"
#include "kvi_config.h"
#include "kvi_kvs_script.h"
#include "kvi_kvs_variant.h"
#include "kvi_options.h"
#include "kvi_locale.h"
#include "kvi_out.h"
#include "kvi_module.h"
#include "kvi_window.h"
#include "kvi_kvs_variantlist.h"

/*
	@doc: events
	@type:
		language
	@keyterms:
		events,event handlers,event
	@title:
		Events
	@short:
		Events: user reactions
	@body:
		KVIrc triggers an event when a particular situation occurs (hehe :D).[br]
		You can define a set of event handlers for each event type.[br]
		An event handler is a snippet of user-defined code that gets executed when the event is triggered.[br]
		Event handlers can be created or destroyed by using the scriptcenter (graphic interface)
		or even from the commandline (or script) by using the [cmd]event[/cmd] command.[br]
		For example, the [event:onirc]OnIRC[/event] is triggered when the login operations have
		been terminated and you can consider yourself "completely" on IRC. For example , you might
		want to "auto-join" some channels. Nothing easier! The following snippet of code
		adds a handler to the OnIRC event that joins three channels:[br]
		[example]
		[cmd]event[/cmd](OnIRC,autojoin)
		{
			[cmd]echo[/cmd] Auto-joining my preferred channels...
			[cmd]join[/cmd] #kvirc,#siena,#linux
		}
		[/example]
		Now try to connect to a server and you'll see that it joins automatically the three channels!.[br]
		You might also want to do some other actions just after the connection has been established,
		for example you might want to look immediately for a friend of yours by issuing a [cmd]whois[/cmd]
		to the server (well.. you could use the notify list for that, but well, this is an example).[br]
		You can add the [cmd]whois[/cmd] request to the handler above or just create a new one:[br]
		[example]
		[cmd]event[/cmd](OnIRC,lookforfred)
		{
			[cmd]echo[/cmd] Looking for fred...
			[cmd]whois[/cmd] fred
		}
		[/example]
		(An even nicer idea would be to use the [cmd]awhois[/cmd] command...but that's left to the reader as exercise.[br]
		To remove an event handler you still use the [cmd]event[/cmd] command , but with an empty code block:[br]
		[example]
		[cmd]event[/cmd](OnIRC,lookforfred){}[br]
		[/example]
		[br]
		Certain events will pass you some data in the positional parameters.[br]
		For example, when you are being banned from a channel, KVIrc triggers the [event:onmeban]OnMeBan[/event]
		event: you might be interested in WHO has banned you. KVIrc will pass the "ban source" informations
		in the positional parameters $0,$1 and $2.[br]
		(Please note that the parameters started from $1 in KVIrc versions older than 3.0.0!).[br]
		You may take a look at the list of available [doc:event_index_all]events[/doc].[br]
*/


KviKvsEventManager * KviKvsEventManager::m_pInstance = 0;

KviKvsEventManager::KviKvsEventManager()
{
	m_pInstance = this;
	for(int i=0;i<KVI_KVS_NUM_RAW_EVENTS;i++)
		m_rawEventTable[i] = 0;
}

KviKvsEventManager::~KviKvsEventManager()
{
	clear();
}

void KviKvsEventManager::init()
{
	if(KviKvsEventManager::instance())
	{
		debug("WARNING: Trying to create KviKvsEventManager twice!");
		return;
	}
	(void) new KviKvsEventManager();
}

void KviKvsEventManager::done()
{
	if(!KviKvsEventManager::instance())
	{
		debug("WARNING: Trying to destroy the KviKvsEventManager twice!");
		return;
	}
	delete KviKvsEventManager::instance();
}

unsigned int KviKvsEventManager::findAppEventIndexByName(const QString &szName)
{
	for(unsigned int u = 0;u < KVI_KVS_NUM_APP_EVENTS;u++)
	{
		if(KviQString::equalCI(szName,m_appEventTable[u].name()))return u;
		//Backwards compatibility >_<
		if((u == 4) && KviQString::equalCI(szName,"OnIrcConnectionEstabilished"))
			return u;
	}
	return KVI_KVS_NUM_APP_EVENTS; // <-- invalid event number
}

KviKvsEvent * KviKvsEventManager::findAppEventByName(const QString &szName)
{
	for(unsigned int u = 0;u < KVI_KVS_NUM_APP_EVENTS;u++)
	{
		if(KviQString::equalCI(szName,m_appEventTable[u].name()))return &(m_appEventTable[u]);
		//Backwards compatibility >_<
		if((u == 4) && KviQString::equalCI(szName,"OnIrcConnectionEstabilished"))
			return &(m_appEventTable[u]);
	}
	return 0;
}

bool KviKvsEventManager::addAppHandler(unsigned int uEvIdx,KviKvsEventHandler * h)
{
	if(uEvIdx >= KVI_KVS_NUM_APP_EVENTS)return false;
	m_appEventTable[uEvIdx].addHandler(h);
	return true;
}

bool KviKvsEventManager::addRawHandler(unsigned int uRawIdx,KviKvsEventHandler * h)
{
	if(uRawIdx >= KVI_KVS_NUM_RAW_EVENTS)return false;
	if(!m_rawEventTable[uRawIdx])
	{
		m_rawEventTable[uRawIdx] = new KviPointerList<KviKvsEventHandler>();
		m_rawEventTable[uRawIdx]->setAutoDelete(true);
	}
	m_rawEventTable[uRawIdx]->append(h);
	return true;
}

bool KviKvsEventManager::removeScriptAppHandler(unsigned int uEvIdx,const QString &szName)
{
	if(uEvIdx >= KVI_KVS_NUM_APP_EVENTS)return false;
	KviKvsEventHandler * h;
	if(!(m_appEventTable[uEvIdx].handlers()))return false;
	for(h = m_appEventTable[uEvIdx].handlers()->first();h;h = m_appEventTable[uEvIdx].handlers()->next())
	{
		if(h->type() == KviKvsEventHandler::Script)
		{
			if(KviQString::equalCI(((KviKvsScriptEventHandler *)h)->name(),szName))
			{
				m_appEventTable[uEvIdx].removeHandler(h);
				return true;
			}
		}
	}
	return false;
}

KviKvsScriptEventHandler * KviKvsEventManager::findScriptRawHandler(unsigned int uEvIdx,const QString &szName)
{
	if(uEvIdx >= KVI_KVS_NUM_RAW_EVENTS)return 0;
	if(!m_rawEventTable[uEvIdx])return 0;
	KviKvsEventHandler * h;
	for(h = m_rawEventTable[uEvIdx]->first();h;h = m_rawEventTable[uEvIdx]->next())
	{
		if(h->type() == KviKvsEventHandler::Script)
		{
			if(KviQString::equalCI(((KviKvsScriptEventHandler *)h)->name(),szName))
			{
				return (KviKvsScriptEventHandler *)h;
			}
		}
	}
	return 0;
}

KviKvsScriptEventHandler * KviKvsEventManager::findScriptAppHandler(unsigned int uEvIdx,const QString &szName)
{
	if(uEvIdx >= KVI_KVS_NUM_APP_EVENTS)return 0;
	KviKvsEventHandler * h;
	if(!(m_appEventTable[uEvIdx].handlers()))return 0;
	for(h = m_appEventTable[uEvIdx].handlers()->first();h;h = m_appEventTable[uEvIdx].handlers()->next())
	{
		if(h->type() == KviKvsEventHandler::Script)
		{
			if(KviQString::equalCI(((KviKvsScriptEventHandler *)h)->name(),szName))
			{
				return (KviKvsScriptEventHandler *)h;
			}
		}
	}
	return 0;
}

bool KviKvsEventManager::enableScriptAppHandler(unsigned int uEvIdx,const QString &szName,bool bEnable)
{
	KviKvsScriptEventHandler * h = findScriptAppHandler(uEvIdx,szName);
	if(!h)return false;
	h->setEnabled(bEnable);
	return true;
}


bool KviKvsEventManager::removeModuleAppHandler(unsigned int uEvIdx,KviKvsModuleInterface *i)
{
	if(uEvIdx >= KVI_KVS_NUM_APP_EVENTS)return false;
	KviKvsEventHandler * h;
	if(!(m_appEventTable[uEvIdx].handlers()))return false;
	for(h = m_appEventTable[uEvIdx].handlers()->first();h;h = m_appEventTable[uEvIdx].handlers()->next())
	{
		if(h->type() == KviKvsEventHandler::Module)
		{
			if(((KviKvsModuleEventHandler *)h)->moduleInterface() == i)
			{
				m_appEventTable[uEvIdx].removeHandler(h);
				return true;
			}
		}
		// COMPAT
		/*
		} else if(h->type() == KviKvsEventHandler::OldModule)
		{
			if(((KviKvsOldModuleEventHandler *)h)->module() == i)
			{
				m_appEventTable[uEvIdx].removeHandler(h);
				return true;
			}
		}
		*/
		// END COMPAT
	}
	return false;
}

void KviKvsEventManager::removeAllModuleAppHandlers(KviKvsModuleInterface *pIface)
{
	KviKvsEventHandler * h;
	for(unsigned int i =0;i< KVI_KVS_NUM_APP_EVENTS;i++)
	{
		if(!m_appEventTable[i].handlers())continue;
	
		KviPointerList<KviKvsEventHandler> l;
		l.setAutoDelete(false);
		for(h = m_appEventTable[i].handlers()->first();h;h = m_appEventTable[i].handlers()->next())
		{
			if(h->type() == KviKvsEventHandler::Module)
			{
				if(((KviKvsModuleEventHandler *)h)->moduleInterface() == pIface)
				{
					l.append(h);
				}
			}
			// COMPAT
			/*
			} else if(h->type() == KviKvsEventHandler::OldModule)
			{
				if(((KviKvsOldModuleEventHandler *)h)->module() == pIface)
				{
					l.append(h);
				}
			}
			*/
			// END COMPAT
	
		}
		for(h = l.first();h;h = l.next())m_appEventTable[i].removeHandler(h);
	}
}

void KviKvsEventManager::removeAllModuleRawHandlers(KviKvsModuleInterface *pIface)
{
	KviKvsEventHandler * h;
	for(unsigned int i =0;i< KVI_KVS_NUM_RAW_EVENTS;i++)
	{
		if(!m_rawEventTable[i])continue;

		KviPointerList<KviKvsEventHandler> l;
		l.setAutoDelete(false);
		for(h = m_rawEventTable[i]->first();h;h = m_rawEventTable[i]->next())
		{
			if(h->type() == KviKvsEventHandler::Module)
			{
				if(((KviKvsModuleEventHandler *)h)->moduleInterface() == pIface)
				{
					l.append(h);
				}
			}
			// COMPAT
			/*
			} else if(h->type() == KviKvsEventHandler::OldModule)
			{
				if(((KviKvsOldModuleEventHandler *)h)->module() == pIface)
				{
					l.append(h);
				}
			}
			*/
			// END COMPAT

		}
		for(h = l.first();h;h = l.next())m_rawEventTable[i]->removeRef(h);
		if(m_rawEventTable[i]->isEmpty())
		{
			delete m_rawEventTable[i];
			m_rawEventTable[i] = 0;
		}
	}
}

bool KviKvsEventManager::removeScriptRawHandler(unsigned int uEvIdx,const QString &szName)
{
	if(uEvIdx >= KVI_KVS_NUM_RAW_EVENTS)return false;
	if(!m_rawEventTable[uEvIdx])return false;
	KviKvsEventHandler * h;
	for(h = m_rawEventTable[uEvIdx]->first();h;h = m_rawEventTable[uEvIdx]->next())
	{
		if(h->type() == KviKvsEventHandler::Script)
		{
			if(KviQString::equalCI(((KviKvsScriptEventHandler *)h)->name(),szName))
			{
				m_rawEventTable[uEvIdx]->removeRef(h);
				if(m_rawEventTable[uEvIdx]->isEmpty())
				{
					delete m_rawEventTable[uEvIdx];
					m_rawEventTable[uEvIdx] = 0;
				}
				return true;
			}
		}
	}
	return false;
}


bool KviKvsEventManager::enableScriptRawHandler(unsigned int uEvIdx,const QString &szName,bool bEnable)
{
	KviKvsScriptEventHandler * h = findScriptRawHandler(uEvIdx,szName);
	if(!h)return false;
	h->setEnabled(bEnable);
	return true;
}


bool KviKvsEventManager::removeModuleRawHandler(unsigned int uRawIdx,KviKvsModuleInterface *i)
{
	if(uRawIdx >= KVI_KVS_NUM_RAW_EVENTS)return false;
	if(!m_rawEventTable[uRawIdx])return false;
	KviKvsEventHandler * h;
	for(h = m_rawEventTable[uRawIdx]->first();h;h = m_rawEventTable[uRawIdx]->next())
	{
		if(h->type() == KviKvsEventHandler::Module)
		{
			if(((KviKvsModuleEventHandler *)h)->moduleInterface() == i)
			{
				m_rawEventTable[uRawIdx]->removeRef(h);
				if(m_rawEventTable[uRawIdx]->isEmpty())
				{
					delete m_rawEventTable[uRawIdx];
					m_rawEventTable[uRawIdx] = 0;
				}
				return true;
			}
		}
		// COMPAT
		/*
		} else if(h->type() == KviKvsEventHandler::OldModule)
		{
			if(((KviKvsOldModuleEventHandler *)h)->module() == i)
			{
				m_rawEventTable[uRawIdx]->removeRef(h);
				if(m_rawEventTable[uRawIdx]->isEmpty())
				{
					delete m_rawEventTable[uRawIdx];
					m_rawEventTable[uRawIdx] = 0;
				}
				return true;
			}
		}
		*/
		// END COMPAT

	}
	return false;
}

void KviKvsEventManager::removeAllModuleHandlers(KviKvsModuleInterface * pIface)
{
	removeAllModuleAppHandlers(pIface);
	removeAllModuleRawHandlers(pIface);
}

void KviKvsEventManager::removeAllScriptAppHandlers()
{
	for(int i=0;i< KVI_KVS_NUM_APP_EVENTS;i++)
	{
		m_appEventTable[i].clearScriptHandlers();
	}
}

void KviKvsEventManager::removeAllScriptRawHandlers()
{
	for(int i=0;i< KVI_KVS_NUM_RAW_EVENTS;i++)
	{
		if(m_rawEventTable[i])
		{
			KviPointerList<KviKvsEventHandler> dl;
			dl.setAutoDelete(false);
			KviKvsEventHandler * e;
			for(e = m_rawEventTable[i]->first();e;e = m_rawEventTable[i]->next())
			{
				if(e->type() == KviKvsEventHandler::Script)dl.append(e);
			}
			
			for(e = dl.first();e;e = dl.next())
			{
				m_rawEventTable[i]->removeRef(e);
			}

			if(m_rawEventTable[i]->isEmpty())
			{
				delete m_rawEventTable[i];
				m_rawEventTable[i] = 0;
			}
		}
	}
}

void KviKvsEventManager::clearRawEvents()
{
	for(int i=0;i<KVI_KVS_NUM_RAW_EVENTS;i++)
	{
		if(m_rawEventTable[i])delete m_rawEventTable[i];
		m_rawEventTable[i] = 0;
	}
}

void KviKvsEventManager::clearAppEvents()
{
	for(int i=0;i<KVI_KVS_NUM_APP_EVENTS;i++)
	{
		m_appEventTable[i].clear();
	}
}

void KviKvsEventManager::clear()
{
	clearRawEvents();
	clearAppEvents();
}

bool KviKvsEventManager::triggerHandlers(KviPointerList<KviKvsEventHandler> * pHandlers,KviWindow *pWnd,KviKvsVariantList *pParams)
{
	if(!pHandlers)return false;

	bool bGotHalt = false;
	for(KviKvsEventHandler * h = pHandlers->first();h;h = pHandlers->next())
	{
		switch(h->type())
		{
			case KviKvsEventHandler::Script:
			{
				if(((KviKvsScriptEventHandler *)h)->isEnabled())
				{
					KviKvsScript * s = ((KviKvsScriptEventHandler *)h)->script();
					KviKvsScript copy(*s);
					KviKvsVariant retVal;
					int iRet = copy.run(pWnd,pParams,&retVal,KviKvsScript::PreserveParams);
					if(!iRet)
					{
						// error! disable the handler if it's broken
						if(KVI_OPTION_BOOL(KviOption_boolDisableBrokenEventHandlers))
						{
							((KviKvsScriptEventHandler *)h)->setEnabled(false);
							pWnd->output(KVI_OUT_PARSERERROR,__tr2qs("Event handler %Q is broken: disabling"),&(s->name()));
						}
					}
					if(!bGotHalt)bGotHalt = (iRet & KviKvsScript::HaltEncountered);
				}
			}
			break;
			case KviKvsEventHandler::Module:
			{
				KviModule * m = (KviModule *)((KviKvsModuleEventHandler *)h)->moduleInterface();
				KviKvsModuleEventHandlerRoutine * proc = ((KviKvsModuleEventHandler *)h)->handlerRoutine();
				KviKvsVariant retVal;
				KviKvsRunTimeContext ctx(0,pWnd,pParams,&retVal);
				KviKvsModuleEventCall call(m,&ctx,pParams);
				if(!(*proc)(&call))bGotHalt = true;
			}
			break;
		}
	}
	return bGotHalt;
}


void KviKvsEventManager::loadRawEvents(const QString &szFileName)
{
	KviConfig cfg(szFileName,KviConfig::Read);
	removeAllScriptRawHandlers();
	int i;

	for(i=0;i<KVI_KVS_NUM_RAW_EVENTS;i++)
	{
		QString tmp;
		KviQString::sprintf(tmp,"RAW%d",i);
		if(cfg.hasGroup(tmp))
		{
			cfg.setGroup(tmp);
			unsigned int nHandlers = cfg.readUIntEntry("NHandlers",0);
			if(nHandlers)
			{
				m_rawEventTable[i] = new KviPointerList<KviKvsEventHandler>();
				m_rawEventTable[i]->setAutoDelete(true);
				for(unsigned int index = 0;index < nHandlers;index++)
				{
					KviQString::sprintf(tmp,"Name%u",index);
					QString szName = cfg.readQStringEntry(tmp,"unnamed");
					KviQString::sprintf(tmp,"Buffer%u",index);
					QString szCode = cfg.readQStringEntry(tmp,"");
					KviQString::sprintf(tmp,"RawEvent%u::%Q",index,&szName);
					KviKvsScriptEventHandler * s = new KviKvsScriptEventHandler(szName,tmp,szCode);
					KviQString::sprintf(tmp,"Enabled%u",index);
					s->setEnabled(cfg.readBoolEntry(tmp,false));
					m_rawEventTable[i]->append(s);
				}
			}
		}
	}
}

void KviKvsEventManager::saveRawEvents(const QString &szFileName)
{
	KviConfig cfg(szFileName,KviConfig::Write);
	cfg.clear();
	int i;

	for(i=0;i<KVI_KVS_NUM_RAW_EVENTS;i++)
	{
		if(m_rawEventTable[i])
		{
			QString tmp;
			KviQString::sprintf(tmp,"RAW%d",i);
			cfg.setGroup(tmp);

			int index = 0;
			for(KviKvsEventHandler * s = m_rawEventTable[i]->first();s; s = m_rawEventTable[i]->next())
			{
				if(s->type() == KviKvsEventHandler::Script)
				{
					KviQString::sprintf(tmp,"Name%d",index);
					cfg.writeEntry(tmp,((KviKvsScriptEventHandler *)s)->name());
					KviQString::sprintf(tmp,"Buffer%d",index);
					cfg.writeEntry(tmp,((KviKvsScriptEventHandler *)s)->code());
					KviQString::sprintf(tmp,"Enabled%d",index);
					cfg.writeEntry(tmp,((KviKvsScriptEventHandler *)s)->isEnabled());
					index++;
				}
			}
			cfg.writeEntry("NHandlers",index);
		}
	}

}

void KviKvsEventManager::loadAppEvents(const QString &szFileName)
{
	KviConfig cfg(szFileName,KviConfig::Read);
	removeAllScriptAppHandlers();

	int i;
	for(i=0;i<KVI_KVS_NUM_APP_EVENTS;i++)
	{
		QString szEventName(m_appEventTable[i].name());
		// Backwards compatibility >_<
		if((i == 4) && !cfg.hasGroup(szEventName))
			szEventName = "OnIrcConnectionEstabilished";
		if(cfg.hasGroup(szEventName))
		{
			cfg.setGroup(szEventName);
			unsigned int nHandlers = cfg.readUIntEntry("NHandlers",0);
			if(nHandlers)
			{
				for(unsigned int index = 0;index < nHandlers;index++)
				{
					QString tmp;
					KviQString::sprintf(tmp,"Name%u",index);
					QString szName = cfg.readQStringEntry(tmp,"unnamed");
					KviQString::sprintf(tmp,"Buffer%u",index);
					QString szCode = cfg.readQStringEntry(tmp,"");
					KviQString::sprintf(tmp,"Enabled%u",index);
					bool bEnabled = cfg.readBoolEntry(tmp,false);
					QString szCntx;
					KviQString::sprintf(szCntx,"%Q::%Q",&(m_appEventTable[i].name()),&szName);
					KviKvsScriptEventHandler *s = new KviKvsScriptEventHandler(szName,szCntx,szCode,bEnabled);
					m_appEventTable[i].addHandler(s);
				}
			}
		}
	}

}

void KviKvsEventManager::saveAppEvents(const QString &szFileName)
{
	KviConfig cfg(szFileName,KviConfig::Write);
	cfg.clear();
	int i;
	bool bCompat = FALSE;

	for(i=0;i<KVI_KVS_NUM_APP_EVENTS;i++)
	{
		if(m_appEventTable[i].hasHandlers())
		{
			QString szEventName(m_appEventTable[i].name());
			// Backwards compatibility >_<
			if((i == 4) && cfg.hasGroup(szEventName))
			{
				szEventName = "OnIRCConnectionEstabilished";
				bCompat = TRUE;
			}
			cfg.setGroup(szEventName);
			int index = 0;
			for(KviKvsEventHandler* s = m_appEventTable[i].handlers()->first();s; s = m_appEventTable[i].handlers()->next())
			{
				if(s->type() == KviKvsEventHandler::Script)
				{
					QString tmp;
					KviQString::sprintf(tmp,"Name%d",index);
					cfg.writeEntry(tmp,((KviKvsScriptEventHandler *)s)->name());
					KviQString::sprintf(tmp,"Buffer%d",index);
					cfg.writeEntry(tmp,((KviKvsScriptEventHandler *)s)->code());
					KviQString::sprintf(tmp,"Enabled%d",index);
					cfg.writeEntry(tmp,((KviKvsScriptEventHandler *)s)->isEnabled());
					index++;
				}
			}
			cfg.writeEntry("NHandlers",index);

			// Backwards compatibility >_<
			if((i == 4) && !bCompat)
				i--;
		}
	}
}

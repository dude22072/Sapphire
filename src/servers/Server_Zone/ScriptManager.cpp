#include <Server_Common/Logger.h>
#include <Server_Common/ExdData.h>
#include <chaiscript/chaiscript.hpp>

#include <Server_Common/ChaiscriptStdLib.h>
#include "ScriptManager.h"

#include "Zone.h"
#include "Player.h"
#include "BattleNpc.h"
#include "ServerZone.h"
#include "Event.h"
#include "EventHelper.h"
#include "ScriptManager.h"

#include "ServerNoticePacket.h"

#include <boost/lexical_cast.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/make_shared.hpp>
#include <boost/format.hpp>
#include <boost/foreach.hpp>

extern Core::Logger g_log;
extern Core::Data::ExdData g_exdData;
extern Core::ServerZone g_serverZone;
extern Core::Scripting::ScriptManager g_scriptManager;

Core::Scripting::ScriptManager::ScriptManager()
{
   m_pChaiHandler = create_chaiscript();
}

Core::Scripting::ScriptManager::~ScriptManager()
{

}

void Core::Scripting::ScriptManager::loadDir( std::string dirname, std::set<std::string>& chaiFiles )
{

   boost::filesystem::path targetDir( dirname );

   boost::filesystem::recursive_directory_iterator iter( targetDir ), eod;

   BOOST_FOREACH( boost::filesystem::path const& i, make_pair( iter, eod ) )
   {

      if( is_regular_file( i ) && boost::filesystem::extension( i.string() ) == ".chai" ||
          boost::filesystem::extension( i.string() ) == ".inc" )
      {
         chaiFiles.insert( i.string() );
      }
   }

}

void Core::Scripting::ScriptManager::onPlayerFirstEnterWorld( Core::Entity::PlayerPtr pPlayer )
{
   try
   {
      std::string test = m_onFirstEnterWorld( *pPlayer );
   }
   catch( const std::exception &e )
   {
      std::string what = e.what();
      g_log.Log( LoggingSeverity::error, what );
   }
}

bool Core::Scripting::ScriptManager::registerBnpcTemplate( std::string templateName, uint32_t bnpcBaseId, uint32_t bnpcNameId, uint32_t modelId, std::string aiName )
{
   return g_serverZone.registerBnpcTemplate( templateName, bnpcBaseId, bnpcNameId, modelId, aiName );
}

void Core::Scripting::ScriptManager::reload()
{
   auto handler = create_chaiscript();
   m_pChaiHandler.swap( handler );
   init();
}

const boost::shared_ptr<chaiscript::ChaiScript>& Core::Scripting::ScriptManager::getHandler() const
{
   return m_pChaiHandler;
}


bool Core::Scripting::ScriptManager::onTalk( Core::Entity::PlayerPtr pPlayer, uint64_t actorId, uint32_t eventId )
{
   std::string eventName = "onTalk";
   std::string objName = Event::getEventName( eventId );

   pPlayer->sendDebug("Actor: " +
                              std::to_string( actorId ) +
                              " \neventId: " +
                              std::to_string( eventId ) +
                              " (0x" + boost::str( boost::format( "%|08X|" ) 
                                                   % static_cast< uint64_t >( eventId & 0xFFFFFFF ) ) + ")" );

   uint16_t eventType = eventId >> 16;

   try
   {
      // Get object from engine
      auto obj = m_pChaiHandler->eval( objName );
      pPlayer->sendDebug( "Calling: " + objName + "." + eventName );

      pPlayer->eventStart( actorId, eventId, Event::Event::Talk, 0, 0 );

      auto fn = m_pChaiHandler->eval< std::function< void( chaiscript::Boxed_Value &, uint32_t, Entity::Player&, uint64_t ) > >( eventName );
      fn( obj, eventId, *pPlayer, actorId );

      pPlayer->checkEvent( eventId );
   }
   catch( std::exception& e )
   {

      if( eventType == Common::EventType::Quest )
      {
         auto questInfo = g_exdData.getQuestInfo( eventId );
         if( questInfo )
         {
            pPlayer->sendDebug( "Quest not implemented: " + questInfo->name + "\n" + e.what() );
            return false;
         }
      }

      pPlayer->sendDebug( e.what() );
      return false;
   }
   return true;
}

bool Core::Scripting::ScriptManager::onEnterTerritory( Core::Entity::PlayerPtr pPlayer, uint32_t eventId,
                                                       uint16_t param1, uint16_t param2 )
{
   std::string eventName = "onEnterTerritory";
   std::string objName = Event::getEventName( eventId );

   try
   {
      // Get object from engine
      auto obj = m_pChaiHandler->eval( objName );

      pPlayer->sendDebug( "Calling: " + objName + "." + eventName );

      pPlayer->eventStart( pPlayer->getId(), eventId, Event::Event::EnterTerritory, 0, pPlayer->getZoneId() );

      auto fn = m_pChaiHandler->eval< std::function< void( chaiscript::Boxed_Value &, uint32_t, Entity::Player&, uint16_t, uint16_t ) > >( eventName );
      fn( obj, eventId, *pPlayer, param1, param2 );

      pPlayer->checkEvent( eventId );
   }
   catch( std::exception& e )
   {
      pPlayer->sendDebug( e.what() );
      return false;
   }
   return true;
}

bool Core::Scripting::ScriptManager::onWithinRange( Entity::PlayerPtr pPlayer, uint32_t eventId, uint32_t param1, float x, float y, float z )
{
   std::string eventName = "onWithinRange";
   std::string objName = Event::getEventName( eventId );

   try
   {
      // Get object from engine
      auto obj = m_pChaiHandler->eval( Event::getEventName( eventId )  );

      pPlayer->sendDebug( "Calling: " + objName + "." + eventName );

      pPlayer->eventStart( pPlayer->getId(), eventId, Event::Event::WithinRange, 1, param1 );

      auto fn = m_pChaiHandler->eval< std::function< void( chaiscript::Boxed_Value &, uint32_t, Entity::Player&, uint32_t, float, float, float ) > >( eventName );
      fn( obj, eventId, *pPlayer, param1, x, y, z );

      pPlayer->checkEvent( eventId );
   }
   catch( std::exception& e )
   {
      pPlayer->sendDebug( e.what() );
      return false;
   }
   return true;
}

bool Core::Scripting::ScriptManager::onOutsideRange( Entity::PlayerPtr pPlayer, uint32_t eventId, uint32_t param1, float x, float y, float z )
{
   std::string eventName = "onOutsideRange";
   std::string objName = Event::getEventName( eventId );

   try
   {
      // Get object from engine
      auto obj = m_pChaiHandler->eval( Event::getEventName( eventId ) );

      pPlayer->sendDebug( "Calling: " + objName + "." + eventName );

      pPlayer->eventStart( pPlayer->getId(), eventId, Event::Event::OutsideRange, 1, param1 );

      auto fn = m_pChaiHandler->eval< std::function< void( chaiscript::Boxed_Value &, uint32_t, Entity::Player&, uint32_t, float, float, float ) > >( eventName );
      fn( obj, eventId, *pPlayer, param1, x, y, z );

      pPlayer->checkEvent( eventId );
   }
   catch( std::exception& e )
   {
      pPlayer->sendDebug( e.what() );
      return false;
   }
   return true;
}

bool Core::Scripting::ScriptManager::onEmote( Core::Entity::PlayerPtr pPlayer, uint64_t actorId,
                                              uint32_t eventId, uint8_t emoteId )
{
   std::string eventName = "onEmote";
   std::string objName = Event::getEventName( eventId );

   try
   {
      auto obj = m_pChaiHandler->eval( Event::getEventName( eventId ) );

      pPlayer->sendDebug( "Calling: " + objName + "." + eventName );

      pPlayer->eventStart( actorId, eventId, Event::Event::Emote, 0, emoteId );

      auto fn = m_pChaiHandler->eval< std::function< void( chaiscript::Boxed_Value &, uint32_t, Entity::Player&, uint64_t, uint8_t ) > >( eventName );
      fn( obj, eventId, *pPlayer, actorId, emoteId );

      pPlayer->checkEvent( eventId );
   }
   catch( std::exception& e )
   {
      uint16_t eventType = eventId >> 16;

      if( eventType == Common::EventType::Quest )
      {
         auto questInfo = g_exdData.getQuestInfo( eventId );
         if( questInfo )
         {
            pPlayer->sendDebug( "Quest not implemented: " + questInfo->name + "\n" + e.what() );
            return false;
         }
      }
      return false;
   }
   return true;
}

bool Core::Scripting::ScriptManager::onEventHandlerReturn( Core::Entity::PlayerPtr pPlayer, uint32_t eventId, uint16_t subEvent,
                                                           uint16_t param1, uint16_t param2, uint16_t param3 )
{

   pPlayer->sendDebug("eventId: " +
                        std::to_string( eventId ) +
                        " ( 0x" + boost::str( boost::format( "%|08X|" ) % ( uint64_t ) ( eventId & 0xFFFFFFF ) ) + " ) " +
                        " scene: " + std::to_string( subEvent ) +
                        " p1: " + std::to_string( param1 ) +
                        " p2: " + std::to_string( param2 ) +
                        " p3: " + std::to_string( param3 ) );

   try
   {
      auto pEvent = pPlayer->getEvent( eventId );
      if( pEvent )
      {
         pEvent->setPlayedScene( false );
         // try to retrieve a stored callback
         auto eventCallback = pEvent->getEventReturnCallback();
         // if there is one, proceed to call it
         if( eventCallback )
         {
            eventCallback( *pPlayer, eventId, param1, param2, param3 );
            if( !pEvent->hasPlayedScene() )
               pPlayer->eventFinish( eventId, 1 );
            else
               pEvent->setPlayedScene( false );
         }
         // else, finish the event.
         else
            pPlayer->eventFinish( eventId, 1 );
      }
   }
   catch( std::exception& e )
   {
      pPlayer->sendNotice( e.what() );
      return false;
   }

   return true;
}

bool Core::Scripting::ScriptManager::onEventHandlerTradeReturn( Core::Entity::PlayerPtr pPlayer, uint32_t eventId,
                                                                uint16_t subEvent, uint16_t param, uint32_t catalogId )
{
   std::string eventName = Event::getEventName( eventId ) + "_TRADE";

   try
   {
      auto fn = m_pChaiHandler->eval< std::function< void( Entity::Player&, uint32_t, uint16_t, uint16_t, uint32_t ) > >( eventName );
      fn( *pPlayer, eventId, subEvent, param, catalogId );
   }
   catch( ... )
   {
      return false;
   }

   return true;
}

bool Core::Scripting::ScriptManager::onEventItem( Entity::PlayerPtr pPlayer, uint32_t eventItemId,
                                                     uint32_t eventId, uint32_t castTime, uint64_t targetId )
{
   std::string eventName = "onEventItem";
   std::string objName = Event::getEventName( eventId );

   try
   {
      auto obj = m_pChaiHandler->eval( Event::getEventName( eventId ) );

      pPlayer->sendDebug( "Calling: " + objName + "." + eventName );

      pPlayer->eventStart( targetId, eventId, Event::Event::Item, 0, 0 );
      
      auto fn = m_pChaiHandler->eval< std::function< void( chaiscript::Boxed_Value &, uint32_t, Entity::Player&, uint32_t, uint32_t, uint64_t ) > >( eventName );
      fn( obj, eventId, *pPlayer, eventItemId, castTime, targetId );
   }
   catch( std::exception& e )
   {
      pPlayer->sendNotice( e.what() );
      return false;
   }

   return true;

}

bool Core::Scripting::ScriptManager::onMobKill( Entity::PlayerPtr pPlayer, uint16_t nameId )
{
   std::string eventName = "onBnpcKill_" + std::to_string( nameId );

   
   // loop through all active quests and try to call available onMobKill callbacks
   for( size_t i = 0; i < 30; i++ )
   {
      auto activeQuests = pPlayer->getQuestActive( i );
      if( !activeQuests )
         continue;

      uint16_t questId = activeQuests->c.questId;
      if( questId != 0 )
      {
         auto obj = m_pChaiHandler->eval( Event::getEventName( 0x00010000 | questId ) );
         std::string objName = Event::getEventName( 0x00010000 | questId );

         pPlayer->sendDebug("Calling: " + objName + "." + eventName);

         try
         {
            auto fn = m_pChaiHandler->eval< std::function< void( chaiscript::Boxed_Value &, Entity::Player& ) > >(eventName);
            fn( obj, *pPlayer );
         }
         catch( std::exception& e )
         {
            g_log.info( e.what() );
         }

      }
   }

   return true;
}

bool Core::Scripting::ScriptManager::onCastFinish( Entity::PlayerPtr pPlayer, Entity::ActorPtr pTarget, uint32_t actionId )
{
    std::string eventName = "onFinish";

    try 
    {
        auto obj = m_pChaiHandler->eval( "skillDef_" + std::to_string( actionId ) );
        std::string objName = "skillDef_" + std::to_string( actionId );

        pPlayer->sendDebug( "Calling: " + objName + "." + eventName );
        auto fn = m_pChaiHandler->eval< std::function< void( chaiscript::Boxed_Value &, Entity::Player& ) > >( eventName );
        fn( obj, *pPlayer );
    }
    catch( std::exception& e )
    {
        pPlayer->sendUrgent( e.what() );
    }

    return true;
}

bool Core::Scripting::ScriptManager::onZoneInit( ZonePtr pZone )
{
   std::string eventName = "onZoneInit_" + pZone->getInternalName();

   try
   {
      auto fn = m_pChaiHandler->eval< std::function< void( Zone& ) > >( eventName );
      fn( *pZone );
   }
   catch( std::exception& e )
   {
      g_log.info( e.what() );
      return false;
   }

   return true;

}


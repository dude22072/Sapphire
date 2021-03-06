#include <Server_Common/Util.h>
#include <Server_Common/UtilMath.h>
#include <Server_Common/PacketContainer.h>

#include "Forwards.h"
#include "Action.h"
#include "Actor.h"
#include "Zone.h"
#include <Server_Common/GamePacket.h>
#include "GameConnection.h"
#include "ActorControlPacket142.h"
#include "ActorControlPacket143.h"
#include "ActorControlPacket144.h"
#include "UpdateHpMpTpPacket.h"

#include "StatusEffectContainer.h"
#include "ServerZone.h"
#include "Session.h"
#include "Player.h"

extern Core::ServerZone g_serverZone;

using namespace Core::Common;
using namespace Core::Network::Packets;
using namespace Core::Network::Packets::Server;

Core::Entity::Actor::Actor()
{
}

Core::Entity::Actor::~Actor()
{
}

/*! \return the id of the actor */
uint32_t Core::Entity::Actor::getId() const
{
   return m_id;
}

/*! \return the actors position object */
Core::Common::FFXIVARR_POSITION3& Core::Entity::Actor::getPos()
{
   return m_pos;
}
/*! \return the actors name */
std::string Core::Entity::Actor::getName() const
{
   return std::string( m_name );
}

/*! \return true if the actor is of type player */
bool Core::Entity::Actor::isPlayer() const
{
   return ( m_type == ActorType::Player ? true : false );
}

/*! \return true if the actor is of type mob */
bool Core::Entity::Actor::isMob() const
{
   return ( m_type == ActorType::BattleNpc ? true : false );
}

/*! \return list of actors currently in range */
std::set< Core::Entity::ActorPtr > Core::Entity::Actor::getInRangeActors( bool includeSelf )
{
    auto tempInRange = m_inRangeActors;

    if( includeSelf )
        tempInRange.insert( shared_from_this() );

    return tempInRange;
}

/*! \return current stance of the actors */
Core::Entity::Actor::Stance Core::Entity::Actor::getStance() const
{
   return m_currentStance;
}

/*! \return current HP */
uint32_t Core::Entity::Actor::getHp() const
{
   return m_hp;
}

/*! \return current MP */
uint32_t Core::Entity::Actor::getMp() const
{
   return m_mp;
}

/*! \return current TP */
uint16_t Core::Entity::Actor::getTp() const
{
   return m_tp;
}

/*! \return current GP */
uint16_t Core::Entity::Actor::getGp() const
{
   return m_gp;
}

/*! \return current class or job */
Core::Common::ClassJob Core::Entity::Actor::getClass() const
{
   return m_class;
}

/*! \return current class or job as int ( this feels pointless ) */
uint8_t Core::Entity::Actor::getClassAsInt() const
{
   return static_cast< uint8_t >( m_class );
}

/*! \param ClassJob to set */
void Core::Entity::Actor::setClass( Common::ClassJob classJob )
{
   m_class = classJob;
}

/*! \param Id of the target to set */
void Core::Entity::Actor::setTargetId( uint64_t targetId )
{
   m_targetId = targetId;
}

/*! \return Id of the current target */
uint64_t Core::Entity::Actor::getTargetId() const
{
   return m_targetId;
}

/*! \return True if the actor is alive */
bool Core::Entity::Actor::isAlive() const
{
   return ( m_hp > 0 );
}

/*! \return max hp for the actor */
uint32_t Core::Entity::Actor::getMaxHp() const
{
   return m_baseStats.max_hp;
}

/*! \return max mp for the actor */
uint32_t Core::Entity::Actor::getMaxMp() const
{
   return m_baseStats.max_mp;
}

/*! \return reset hp to current max hp */
void Core::Entity::Actor::resetHp()
{
   m_hp = getMaxHp();
}

/*! \return reset mp to current max mp */
void Core::Entity::Actor::resetMp()
{
   m_mp = getMaxMp();
}

/*! \param hp amount to set ( caps to maxHp ) */
void Core::Entity::Actor::setHp( uint32_t hp )
{
   m_hp = hp < getMaxHp() ? hp : getMaxHp();
}

/*! \param mp amount to set ( caps to maxMp ) */
void Core::Entity::Actor::setMp( uint32_t mp )
{
   m_mp = mp < getMaxMp() ? mp : getMaxMp();
}

/*! \param mp amount to set ( caps to maxMp ) */
void Core::Entity::Actor::setGp( uint32_t gp )
{
   m_gp = gp;
}

/*! \return current status of the actor */
Core::Entity::Actor::ActorStatus Core::Entity::Actor::getStatus() const
{
   return m_status;
}

/*! \param status to set */
void Core::Entity::Actor::setStatus( ActorStatus status )
{
   m_status = status;
}

/*!
Performs necessary steps to mark an actor dead.
Sets hp/mp/tp, sets status, plays animation and fires onDeath event
*/
void Core::Entity::Actor::die()
{
   m_status = ActorStatus::Dead;
   m_hp = 0;
   m_mp = 0;
   m_tp = 0;

   // fire onDeath event
   onDeath();

   bool selfNeedsUpdate = false;

   // if the actor is a player, the update needs to be send to himself too
   if( isPlayer() )
      selfNeedsUpdate = true;

   sendToInRangeSet( ActorControlPacket142( m_id, SetStatus, static_cast< uint8_t>( ActorStatus::Dead ) ), selfNeedsUpdate );

   // TODO: not all actor show the death animation when they die, some quest npcs might just despawn
   //       although that might be handled by setting the HP to 1 and doing some script magic
   sendToInRangeSet( ActorControlPacket142( m_id, DeathAnimation, 0, 0, 0, 0x20 ), selfNeedsUpdate );

}

/*!
Calculates and sets the rotation to look towards a specified
position

\param Position to look towards
*/
bool Core::Entity::Actor::face( const Common::FFXIVARR_POSITION3& p )
{
   float oldRot = getRotation();
   float rot = Math::Util::calcAngFrom( getPos().x, getPos().z, p.x, p.z );
   float newRot = PI - rot + ( PI / 2 );

   m_pCell = nullptr;

   setRotation( newRot );

   if( oldRot != newRot )
      return true;

   return false;
}

/*!
Sets the actors position and notifies the zone to propagate the change

\param Position to set
*/
void Core::Entity::Actor::setPosition( const Common::FFXIVARR_POSITION3& pos )
{
   m_pos = pos;
   m_pCurrentZone->changeActorPosition( shared_from_this() );
}

void Core::Entity::Actor::setPosition( float x, float y, float z )
{
   m_pos.x = x;
   m_pos.y = y;
   m_pos.z = z;
   m_pCurrentZone->changeActorPosition( shared_from_this() );
}

/*!
Set and propagate the actor stance to in range players
( not the actor himself )

\param stance to set
*/
void Core::Entity::Actor::setStance( Stance stance )
{
   m_currentStance = stance;

   sendToInRangeSet( ActorControlPacket142( m_id, ToggleAggro, stance, 1 ) );
}

/*!
Check if an action is queued for execution, if so update it
and if fully performed, clean up again.

\return true if a queued action has been updated
*/
bool Core::Entity::Actor::checkAction()
{

   if( m_pCurrentAction == nullptr )
      return false;

   if( m_pCurrentAction->update() )
      m_pCurrentAction.reset();

   return true;

}

/*!
Change the current target and propagate to in range players

\param target actor id
*/
void Core::Entity::Actor::changeTarget( uint64_t targetId )
{
   setTargetId( targetId );

   sendToInRangeSet( ActorControlPacket144( m_id, SetTarget,
                                            0, 0, 0, 0, targetId ) );
}

/*!
Dummy function \return 0
*/
uint8_t Core::Entity::Actor::getLevel() const
{
   return 0;
}

/*!
Let an actor take damage and perform necessary steps
according to resulting hp, propagates new hp value to players
in range
TODO: eventually this needs to distinguish between physical and
magical dmg and take status effects into account

\param amount of damage to be taken
*/
void Core::Entity::Actor::takeDamage( uint32_t damage )
{
   if( damage >= m_hp )
   {
      m_hp = 0;
      die();
   }
   else
      m_hp -= damage;

   sendStatusUpdate( false );
}

/*!
Send an HpMpTp update to players in range ( and potentially to self )
TODO: poor naming, should be changed. Status is not HP. Also should be virtual
so players can have their own version and we can abolish the param.

\param true if the update should also be sent to the actor ( player ) himself
*/
void Core::Entity::Actor::sendStatusUpdate( bool toSelf )
{
   UpdateHpMpTpPacket updateHpPacket( shared_from_this() );
   sendToInRangeSet( updateHpPacket );
}

/*! \return pointer to this instance as PlayerPtr */
Core::Entity::PlayerPtr Core::Entity::Actor::getAsPlayer()
{
   return boost::dynamic_pointer_cast< Entity::Player, Entity::Actor >( shared_from_this() );
}

/*! \return pointer to this instance as BattleNpcPtr */
Core::Entity::BattleNpcPtr Core::Entity::Actor::getAsBattleNpc()
{
   return boost::reinterpret_pointer_cast< Entity::BattleNpc, Entity::Actor >( shared_from_this() );
}

/*! \return ActionPtr of the currently registered action, or nullptr */
Core::Action::ActionPtr Core::Entity::Actor::getCurrentAction() const
{
   return m_pCurrentAction;
}

/*! \param ActionPtr of the action to be registered */
void Core::Entity::Actor::setCurrentAction( Core::Action::ActionPtr pAction )
{
   m_pCurrentAction = pAction;
}

/*!
check if a given actor is in the actors in range set

\param ActorPtr to be checked for
\return true if the actor was found
*/
bool Core::Entity::Actor::isInRangeSet( ActorPtr pActor ) const
{
   return !( m_inRangeActors.find( pActor ) == m_inRangeActors.end() );
}

/*! \return ActorPtr of the closest actor in range, if none, nullptr */
Core::Entity::ActorPtr Core::Entity::Actor::getClosestActor()
{
   if( m_inRangeActors.empty() )
      // no actors in range, don't bother
      return nullptr;

   ActorPtr tmpActor = nullptr;

   // arbitrary high number
   float minDistance = 10000;

   for( auto pCurAct : m_inRangeActors )
   {
      float distance = Math::Util::distance( getPos().x,
                                             getPos().y,
                                             getPos().z,
                                             pCurAct->getPos().x,
                                             pCurAct->getPos().y,
                                             pCurAct->getPos().z );

      if( distance < minDistance )
      {
         minDistance = distance;
         tmpActor = pCurAct;
      }
   }

   return tmpActor;
}

/*!
Send a packet to all players in range, potentially to self if set and is player

\param GamePacketPtr to send
\param bool should be send to self?
*/
void Core::Entity::Actor::sendToInRangeSet( Network::Packets::GamePacketPtr pPacket, bool bToSelf )
{

   if( bToSelf && isPlayer() )
   {
      auto pPlayer = getAsPlayer();

      auto pSession = g_serverZone.getSession( pPlayer->getId() );

      // it might be that the player DC'd in which case the session would be invalid
      if( pSession )
         pSession->getZoneConnection()->queueOutPacket( pPacket );
   }

   if( m_inRangePlayers.empty() )
      return;

   for( auto pCurAct : m_inRangePlayers )
   {
      assert( pCurAct );
      pPacket->setValAt<uint32_t>( 0x04, m_id );
      pPacket->setValAt<uint32_t>( 0x08, pCurAct->m_id );
      // it might be that the player DC'd in which case the session would be invalid
      pCurAct->queuePacket( pPacket );
   }
}

/*!
Add a given actor to the fitting in range set according to type
but also to the global actor map

\param ActorPtr to add
*/
void Core::Entity::Actor::addInRangeActor( ActorPtr pActor )
{

   // if this is null, something went wrong
   assert( pActor );

   // add actor to in range set
   m_inRangeActors.insert( pActor );

   if( pActor->isPlayer() )
   {
      auto pPlayer = pActor->getAsPlayer();

      // if actor is a player, add it to the in range player set
      m_inRangePlayers.insert( pPlayer );
   }

   m_inRangeActorMap[pActor->getId()] = pActor;
}

/*!
Remove a given actor from the fitting in range set according to type
but also to the global actor map

\param ActorPtr to remove
*/
void Core::Entity::Actor::removeInRangeActor( ActorPtr pActor )
{
   // if this is null, something went wrong
   assert( pActor );

   // call virtual event
   onRemoveInRangeActor( pActor );

   // remove actor from in range actor set
   m_inRangeActors.erase( pActor );

   // if actor is a player, despawn ourself for him
   // TODO: move to virtual onRemove?
   if( isPlayer() )
      pActor->despawn( shared_from_this() );

   if( pActor->isPlayer() )
   {
      auto pPlayer = pActor->getAsPlayer();
      m_inRangePlayers.erase( pPlayer );
   }

   m_inRangeActorMap.erase( pActor->getId() );
}

/*! \return true if there is at least one actor in the in range set */
bool Core::Entity::Actor::hasInRangeActor() const
{
   return ( m_inRangeActors.size() > 0 );
}

/*! Clear the whole in range set, this does no cleanup */
void Core::Entity::Actor::clearInRangeSet()
{
   m_inRangeActors.clear();
   m_inRangePlayers.clear();
   m_inRangeActorMap.clear();
}

/*! \return ZonePtr to the current zone, nullptr if not set */
Core::ZonePtr Core::Entity::Actor::getCurrentZone() const
{
   return m_pCurrentZone;
}

/*! \param ZonePtr to the zone to be set as current */
void Core::Entity::Actor::setCurrentZone( ZonePtr currZone )
{
   m_pCurrentZone = currZone;
}

/*!
Get the current cell of a region the actor is in

\return Cell*
*/
Core::Cell * Core::Entity::Actor::getCell() const
{
   return m_pCell;
}

/*!
Set the current cell the actor is in

\param Cell* for the cell to be set
*/
void Core::Entity::Actor::setCell( Cell * pCell )
{
   m_pCell = pCell;
}

/*!
Autoattack prototype implementation
TODO: move the check if the autoAttack can be performed to the callee
also rename autoAttack to autoAttack as that is more elaborate
On top of that, this only solves attacks from melee classes.
Will have to be extended for ranged attacks.

\param ActorPtr the autoAttack is performed on
*/
void Core::Entity::Actor::autoAttack( ActorPtr pTarget )
{

   uint64_t tick = Util::getTimeMs();

   if( ( tick - m_lastAttack ) > 2500 )
   {
      pTarget->onActionHostile( shared_from_this() );
      m_lastAttack = tick;
      srand( static_cast< uint32_t >( tick ) );

      uint32_t damage = 10 + rand() % 12;
      uint32_t variation = 0 + rand() % 3;

      GamePacketNew< FFXIVIpcEffect > effectPacket( getId() );
      effectPacket.data().targetId = pTarget->getId();
      effectPacket.data().actionAnimationId = 0x366;
      effectPacket.data().unknown_2 = variation;
//      effectPacket.data().unknown_3 = 1;
      effectPacket.data().actionTextId = 0x366;
      effectPacket.data().numEffects = 1;
      effectPacket.data().rotation = Math::Util::floatToUInt16Rot( getRotation() );
      effectPacket.data().effectTarget = pTarget->getId();
      effectPacket.data().effects[0].param1 = damage;
      effectPacket.data().effects[0].unknown_1 = 3;
      effectPacket.data().effects[0].unknown_2 = 1;
      effectPacket.data().effects[0].unknown_3 = 7;

      sendToInRangeSet( effectPacket );

      if( this->isPlayer() )
         this->getAsPlayer()->queuePacket( effectPacket );

      pTarget->takeDamage( damage );
   }
}

/*! \param StatusEffectPtr to be applied to the actor */
void Core::Entity::Actor::addStatusEffect( StatusEffect::StatusEffectPtr pEffect )
{
   m_pStatusEffectContainer->addStatusEffect( pEffect );
}


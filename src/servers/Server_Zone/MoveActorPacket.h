#ifndef _MOVEACTORPACKET_H
#define _MOVEACTORPACKET_H

#include <Server_Common/GamePacketNew.h>
#include <Server_Common/ServerPacketDef.h>
#include <Server_Common/UtilMath.h>
#include "Player.h"
#include "Forwards.h"


namespace Core {
namespace Network {
namespace Packets {
namespace Server {

/**
* @brief The Client UI Initialization packet. This must be sent to the client
* once upon connection to configure the UI.
*/
class MoveActorPacket :
   public GamePacketNew<FFXIVIpcActorMove>
{
public:
   MoveActorPacket( Entity::ActorPtr actor, uint8_t unk1, uint8_t unk2, uint8_t unk3, uint16_t unk4 ) :
      GamePacketNew<FFXIVIpcActorMove>( actor->getId(), actor->getId() )
   {
      initialize( actor, unk1, unk2, unk3, unk4 );
   };

private:
   void initialize( Entity::ActorPtr actor, uint8_t unk1, uint8_t unk2, uint8_t unk3, uint16_t unk4 )
   {
      
      m_data.rotation = Math::Util::floatToUInt8Rot( actor->getRotation() );
      m_data.unknown_1 = unk1;
      m_data.unknown_2 = unk2;
      m_data.unknown_3 = unk3;
      m_data.unknown_4 = unk4;
      m_data.posX = Math::Util::floatToUInt16( actor->getPos().x );
      m_data.posY = Math::Util::floatToUInt16( actor->getPos().y );
      m_data.posZ = Math::Util::floatToUInt16( actor->getPos().z );

   };
};

}
}
}
}

#endif /*_MOVEACTORPACKET_H*/
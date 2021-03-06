#ifndef _CHATPACKET_H
#define _CHATPACKET_H

#include <Server_Common/GamePacketNew.h>
#include <Server_Common/ServerPacketDef.h>
#include "Forwards.h"


namespace Core {
namespace Network {
namespace Packets {
namespace Server {

/**
* @brief The Chat packet.
*/
class ChatPacket :
   public GamePacketNew< FFXIVIpcChat >
{
public:
   ChatPacket( Entity::PlayerPtr player, Common::ChatType chatType, const std::string& msg ) :
      GamePacketNew<FFXIVIpcChat>( player->getId(), player->getId() )
   {
      initialize( player, chatType, msg );
   };

private:
   void initialize( Entity::PlayerPtr player, Common::ChatType chatType, const std::string& msg )
   {
      m_data.chatType = chatType;
      strcpy( m_data.name, player->getName().c_str() );
      strcpy( m_data.msg, msg.c_str() );
   };
};

}
}
}
}

#endif /*_CHATPACKET_H*/
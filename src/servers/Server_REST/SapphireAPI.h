#ifndef _SAPPHIREAPI_H_
#define _SAPPHIREAPI_H_

#include <string>
#include <vector>
#include <map>
#include <boost/shared_ptr.hpp>

#include "PlayerMinimal.h"

namespace Core
{
   class Session;

   namespace Network
   {

      class SapphireAPI
      {
      public:
         SapphireAPI();
         ~SapphireAPI();

         typedef std::map< std::string, boost::shared_ptr< Session > > SessionMap;

         bool login( const std::string& username, const std::string& pass, std::string& sId );

         bool createAccount( const std::string& username, const std::string& pass, std::string& sId );

         int createCharacter( const int& accountId, const std::string& name, const std::string& infoJson );
         
         void deleteCharacter( std::string name, uint32_t accountId );

		 bool insertSession( const uint32_t& accountId, std::string& sId );

         std::vector<Core::PlayerMinimal> getCharList( uint32_t accountId );

         bool checkNameTaken( std::string name );

         uint32_t getNextCharId();

         uint64_t getNextContentId();

         int checkSession( const std::string& sId );

         bool removeSession( const std::string& sId );

         SessionMap m_sessionMap;

      };
   }
}

#endif
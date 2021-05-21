/*
// Copyright (c) 2017 Intel Corporation
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
*/
#include <boost/algorithm/string/case_conv.hpp>
#include <boost/algorithm/string/predicate.hpp>
#include <boost/algorithm/string/replace.hpp>
#include <boost/container/flat_set.hpp>
#include <filesystem>
#include <fstream>
#include <optional>
#include <regex>
#include <sdbusplus/asio/connection.hpp>
#include <sdbusplus/asio/object_server.hpp>
#include <nlohmann/json.hpp>
#include <iostream>
#include <syslog.h>
#include <sys/inotify.h>
#include <unistd.h>

#include <boost/container/flat_map.hpp>
#include <sdbusplus/bus/match.hpp>
#include <sdbusplus/message/types.hpp>

#include <mutex>              // std::mutex, std::unique_lock
#include <condition_variable> // std::condition_variable


#define DEBUG

#ifdef DEBUG
#define DBGPRINT(X)  cout  << (X) << endl;
#else
#define DBGPRINT(X)  /* */
#endif



using namespace std;

std::shared_ptr<sdbusplus::asio::connection> systemBus;
std::string dhcpobjectpath;

int onPropertyUpdate(sd_bus_message* m, void* /* userdata */,

                            sd_bus_error* retError)
{
    DBGPRINT("PropertyUpdated  \n");	
    if (retError == nullptr || sd_bus_error_is_set(retError))
    {
        //BMCWEB_LOG_ERROR << "Got sdbus error on match";
	DBGPRINT("Got sdbus error on match \n");
        return 0;
    }

    sdbusplus::message::message message(m);
    std::string iface;
    boost::container::flat_map<std::string, std::variant<std::string>>
        changedProperties;

    message.read(iface, changedProperties);
    auto it = changedProperties.find("Address");

    if (it == changedProperties.end())

    {
        return 0;
    }

    std::string* newip = std::get_if<std::string>(&it->second);
    if (newip == nullptr)
    {
	DBGPRINT("Unable to read newip\n");
        return 0;
    }

    DBGPRINT("#####New IP: \n");
    DBGPRINT( *newip);
    DBGPRINT("#############\n");


    return 0;
}


void registerDCHPMonSignal()
{

	DBGPRINT("~~~~~~~~~~~~~Register DHCP IP PropertiesChanged Signal\n");
	DBGPRINT(dhcpobjectpath);
	DBGPRINT("~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~\n");

	static std::unique_ptr<sdbusplus::bus::match::match> DHCPIPMonitor;
	//std::vector<std::unique_ptr<sdbusplus::bus::match::match>> matches;

	std::string propertiesMatchString =
		("type='signal',"
		 "interface='org.freedesktop.DBus.Properties',"
		 "path='" + dhcpobjectpath + "',"
		 "arg0='xyz.openbmc_project.Network.IP',"
		 "member='PropertiesChanged'");


	/*std::string propertiesMatchString1 =
	  ("type='signal',"
	  "interface='org.freedesktop.DBus.Properties',"
	  "path_namespace='/xyz/openbmc_project/network/config',"
	  "arg0namespace='xyz.openbmc_project.Network.IP',"
	  "member='PropertiesChanged'");


	  std::string propertiesMatchString2 =
	  ("type='signal',"
	  "interface='org.freedesktop.DBus.Properties',"
	  "path_namespace='/xyz/openbmc_project/network/config/dhcp',"
	  "arg0namespace='xyz.openbmc_project.Network.DHCPConfiguration',"
	  "member='PropertiesChanged'");


	//cerating matches for each interface 
	auto matchold = std::make_unique<sdbusplus::bus::match::match>(*systemBus,propertiesMatchString1,onPropertyUpdate,nullptr);
	//ading to matches vector
	matches.emplace_back(std::move(matchold));

	auto matchnew = std::make_unique<sdbusplus::bus::match::match>(*systemBus,propertiesMatchString2,onPropertyUpdate,nullptr);	    
	matches.emplace_back(std::move(matchnew));*/

	DHCPIPMonitor = std::make_unique<sdbusplus::bus::match::match>(*systemBus, propertiesMatchString, onPropertyUpdate,nullptr);

	return;
}



void get_dhcp_object_paths()
{

	constexpr const std::array<const char*, 1> NetworkIP = {"xyz.openbmc_project.Network.IP"};

	systemBus->async_method_call(
			[](const boost::system::error_code ec,
				const std::vector<std::string>& objects) {
			DBGPRINT("(((((((SUBTREE  PATHS CALLBACK BEGIN )))))))\n");


			if (ec)
			{
				DBGPRINT("error in GetSubtreePaths function \n");

			}

			for (const std::string& objectpath : objects)
			{
			         DBGPRINT("########LOOP BEGIN########\n");
			         DBGPRINT("!!!!!!!!object_path: \n");
				 DBGPRINT(objectpath);
			 	 DBGPRINT("!!!!!!!!!!!!!!!!!!!!!\n");	 

			         systemBus->async_method_call(
					[objectpath](const boost::system::error_code ec2,
						const std::variant<std::string>& origin) 
					{
					DBGPRINT("&&&&&&&&CALLBACK BEGIN &&&&&&&\n");

					int err = 0;
					if (ec2)
					{
					DBGPRINT("$$$$$$$ ERROR IN CALLBACK $$$$$$$\n");
					err = 1;	
					}

			                if(err !=1 )
					{
				       
					const std::string* addrtype = std::get_if<std::string>(&origin);

					if(*addrtype == "xyz.openbmc_project.Network.IP.AddressOrigin.DHCP")
					{
					//register for this object
					DBGPRINT("*****DHCP PBJECT FOUND ******\n");
					DBGPRINT(objectpath);

				        dhcpobjectpath = objectpath;

					DBGPRINT("*****************************\n");

					}


					}
					DBGPRINT("&&&&&&& CALLBACK END &&&&&&&&&&\n");
					}
					,
				"xyz.openbmc_project.Network",
				objectpath,
				"org.freedesktop.DBus.Properties", "Get",
				"xyz.openbmc_project.Network.IP", "Origin");
				
				DBGPRINT("########LOOP END########\n");		

				}
			DBGPRINT("(((((((SUBTREE  PATHS CALLBACK END )))))))\n");
			},
	  "xyz.openbmc_project.ObjectMapper",
	  "/xyz/openbmc_project/object_mapper",
          "xyz.openbmc_project.ObjectMapper", "GetSubTreePaths",
	  "/xyz/openbmc_project/network", 0,
	  NetworkIP );

}


int main()
{
        //boost::asio::io_service io;
	auto io = std::make_shared<boost::asio::io_context>();
        systemBus = std::make_shared<sdbusplus::asio::connection>(*io);

	//det dhcp objects
	get_dhcp_object_paths();
	sleep(20);

        //registering for property change
        registerDCHPMonSignal();
	
	io->run();

}

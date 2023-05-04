
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

constexpr auto MAPPER_BUSNAME = "xyz.openbmc_project.ObjectMapper";
constexpr auto MAPPER_OBJ_PATH = "/xyz/openbmc_project/object_mapper";
constexpr auto MAPPER_IFACE = "xyz.openbmc_project.ObjectMapper";
constexpr auto DBUS_PROPERTY_IFACE = "org.freedesktop.DBus.Properties";

using namespace std;
using PropertyValue = std::variant<uint8_t, uint16_t, std::string,
                                   std::vector<std::string>, bool>;

std::shared_ptr<sdbusplus::asio::connection> systemBus;
std::string dhcpobjectpath;

// call back handler for property changed signal
int onPropertyUpdate(sd_bus_message* m, void* /* userdata */, sd_bus_error* retError)
{

    DBGPRINT("PropertyUpdated  \n");
    if (retError == nullptr || sd_bus_error_is_set(retError))
    {
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

// Regsiter property changed signal 
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

// Get service name
const std::string getService(const std::string& path,
                                          const std::string& interface)
{
    std::unordered_map<std::string, std::vector<std::string>> mapperResponse;
    using InterfaceList = std::vector<std::string>;

    auto mapper = systemBus->new_method_call(MAPPER_BUSNAME, MAPPER_OBJ_PATH,
                                      MAPPER_IFACE, "GetObject");

    mapper.append(path, InterfaceList({interface}));

    auto mapperResponseMsg = systemBus->call(mapper);
    mapperResponseMsg.read(mapperResponse);
    if (mapperResponse.empty())
    {
	std::cerr << "empty response fo get service" << "\n";    
        return "";
    }

    // the value here will be the service name
    return mapperResponse.cbegin()->first;
}

// Get the property name
const PropertyValue getProperty(const std::string& objectPath,
                             const std::string& interface,
                             const std::string& propertyName)
{
    PropertyValue value{};

    auto service = getService(objectPath, interface);
    if (service.empty())
    {
        return value;
    }

    auto method = systemBus->new_method_call(service.c_str(), objectPath.c_str(),
                                      DBUS_PROPERTY_IFACE, "Get");
    method.append(interface, propertyName);

    auto reply = systemBus->call(method);
    reply.read(value);

    return value;
}

void get_dhcp_async_paths_async()
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
					if(*addrtype == "xyz.openbmc_project.Network.IP.AddressOrigin.Static")
					{
					//register for this object
						dhcpobjectpath = objectpath;
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
	  "/", 0,
	  NetworkIP );

    sleep(120);	
}	

void get_dhcp_object_paths()
{

    DBGPRINT(" get_dhcp_object_paths called \n");
    std::string interface = "xyz.openbmc_project.Network.IP";
    std::string objectPath = "/";
    std::string propName = "Origin";	
    std::vector<std::string> paths;

    auto method = systemBus->new_method_call(MAPPER_BUSNAME, MAPPER_OBJ_PATH,
                                      MAPPER_IFACE, "GetSubTreePaths");
    method.append(objectPath.c_str());
    method.append(0); // Depth 0 to search all
    method.append(std::vector<std::string>({interface.c_str()}));
    auto reply = systemBus->call(method);

    reply.read(paths);

    for(auto path: paths)
    {	    
	    std::cout << "path:" << path << "\n";
	    auto val = getProperty(path,interface, propName);
	    auto origin = std::get<std::string>(val);
	    if( origin == "xyz.openbmc_project.Network.IP.AddressOrigin.Static")
            {
		    std::cout << "static ip found " << "\n";
		    dhcpobjectpath = path;
	    }			    
    }
}


int main()
{
        boost::asio::io_context io;
	systemBus = std::make_shared<sdbusplus::asio::connection>(io);
	
	// Get dhcp objects paths
	get_dhcp_object_paths();

        //register call back for property change
        registerDCHPMonSignal();
	
	io.run();
}

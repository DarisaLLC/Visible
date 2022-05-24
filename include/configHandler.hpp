
#ifndef ConfigHandler_hpp
#define ConfigHandler_hpp

#include <libconfig.h++>

using namespace std;
using namespace libconfig;


class configHandler {
protected:
    //==== SINGLETON STUFF ==============================================//
    static configHandler& GetInstance()
    {
        static configHandler instance; // Guaranteed to be destroyed.
        // Instantiated on first use.
        return instance;
    }
    //==== END SINGLETON STUFF ==============================================//
    
public:
    
    /**
     Get the config statically using fewer commands
     */
    static Config& GetConfig(){
        return GetInstance().config;
    }
    
    Config config;
    
private:
    //==== SINGLETON STUFF ==============================================//
    configHandler();
    // C++11:
    // Stop the compiler from generating copy methods for the object
    configHandler(configHandler const&) = delete;
    void operator=(configHandler const&) = delete;
    //==== END SINGLETON STUFF ==============================================//
    
    const char *CONFIG_FILE = "config/Visible.cfg";
};






#endif /* ConfigHandler_hpp */


#include <iostream>
#include "configHandler.hpp"

configHandler::configHandler(){
    
    // Read the config file, and report/exit if there's an error.
    try
    {
        config.readFile(CONFIG_FILE);
    }
    catch(const FileIOException &fioex)
    {
        std::cerr << "I/O error while reading file." << std::endl;
        exit(EXIT_FAILURE);
    }
    catch(const ParseException &pex)
    {
        std::cerr << "Parse error at " << pex.getFile() << ":" << pex.getLine()
        << " - " << pex.getError() << std::endl;
        exit(EXIT_SUCCESS);
    }
}

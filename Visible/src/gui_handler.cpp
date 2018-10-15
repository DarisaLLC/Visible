#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdelete-non-virtual-dtor"
#pragma GCC diagnostic ignored "-Wunused-private-field"
#pragma GCC diagnostic ignored "-Wcomma"

#include "gui_handler.hpp"

gui_handler::gui_handler(){
    //Make an empty window that will be hidden from view. We make this to store a single object
    //that we will use as our hook for global param definitions since Cinder's wrapper of
    //AntTweakBar does not provide one.
    globalParams = params::InterfaceGl::create( getWindow(), "globalParams", toPixels( ivec2(10, 10) ) );
    globalParams->hide();
    
    //Make a general settings window to show by default.
    AddWindow("General Settings", ivec2(200, 150));
    params::InterfaceGlRef generalWindow = GetWindow("General Settings");
    generalWindow->setPosition(ivec2(5, 50));
}

/**
 Circumvents a limitation in Cinder's wrapper for AntTweakBar and allows a direct call to TWDefine.
 See the note at http://anttweakbar.sourceforge.net/doc/tools:anttweakbar:twdefine for details on 
 why this works.
 */
void gui_handler::DefineGlobal(const string options){
    //To explain: we first give a command that does nothing. This is given to Cinder's
    //wrapper of AntTweakBar with the name of globalParam's internal representation affixed.
    //We have a newline to specify that another command is to be set at the same time; the difference
    //here is that the name is not affixed, so we can use any twBar name or the GLOBAL name here.
    string dummy = " help=' ' \n GLOBAL ";
    globalParams->setOptions("", dummy + options);
}

/**
 Adds a window to the GUI with the given name and size.
 */
void gui_handler::AddWindow(string name, ivec2 size){
    params::InterfaceGlRef newWindow = params::InterfaceGl::create( getWindow(), name, toPixels( size ) );
    windows.insert(std::make_pair(name, newWindow));
}

/**
 Returns a handle for the window with the specified name.
 Gives the default window if the name doesn't exist.
 */
params::InterfaceGlRef gui_handler::GetWindow(string name){
    if(windows.count(name) > 0){
        return windows.at(name);
    }
    else{
        return windows.at("General Settings");
    }
}

/**
 Invokes a draw call on the window with the specified name.
 */
void gui_handler::DrawWindow(string name){
    GetWindow(name)->draw();
}

/**
 Invokes a draw call on every registered window.
 */
void gui_handler::DrawAll(){
    for (const auto& i : windows)
    {
        i.second->draw();
    }
}

#pragma GCC diagnostic pop


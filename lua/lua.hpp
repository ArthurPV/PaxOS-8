#include "../src/lua.hpp"
#include <stdlib.h>
#include <string>
using namespace std;

#include "../widgets/gui.hpp"


int customPrint(lua_State* L) {
    int nargs = lua_gettop(L);

    for (int i = 1; i <= nargs; ++i) {
        const char* str = lua_tostring(L, i);
        if (str) {
            print(str);
        }
    }
    return 0;
}

bool runLuaFunction(lua_State* L, const std::string& functionName) {
    lua_getglobal(L, functionName.c_str());
    
    if (lua_isfunction(L, -1)) {
        if (lua_pcall(L, 0, LUA_MULTRET, 0) == 0) {
            // Lua function executed successfully, and output is captured in luaOutput.
            return true;
        } else {
            //std::cerr << "Error executing Lua function '" << functionName << "': " << lua_tostring(L, -1) << std::endl;
        }
    } else {
        //std::cerr << "Lua function '" << functionName << "' not found." << std::endl;
    }

    return false;
}

void addFunction(lua_State* L, std::string name, int (*function)(lua_State* L))
{
    lua_pushcfunction(L, function);
    lua_setglobal(L, name.c_str());
}

void addVariable(lua_State* L, std::string name, int value)
{
    lua_pushinteger(L, value);
    lua_setglobal(L, name.c_str());
}

void* custom_allocator(void *ud, void *ptr, size_t osize, size_t nsize) {
    if (nsize == 0) {
        // Free the block
        if (ptr != NULL) {
            free(ptr);
        }
        return NULL;
    } else {
        // Allocate or resize the block
        #ifdef BUILD_PAXO
            return ps_realloc(ptr, nsize);
        #else
            return realloc(ptr, nsize);
        #endif
    }
}

struct LuaEvent
{
    string callback;
    Gui* obj;
    bool (Gui::*func)(void);
};

class LuaInterpreter
{
    public:
    std::string conf = "";
    std::string data = "";
    static uint idcounter;
    static vector<Gui*> gui;
    static vector<LuaEvent> events;

    void loadScript(std::string filename)
    {
        storage::LFile file(filename, storage::OPEN_MODE::READ);
        data = file.read();
        file.close();
    }

    void runApp()
    {
        idcounter = 0;

        lua_State* L = luaL_newstate();
        lua_setallocf(L, custom_allocator, NULL);

        luaL_openlibs(L);

        addFunction(L, "print", customPrint);
        addFunction(L, "Gui", luaNewObject);
        
        // GUI methods
        addFunction(L, "setX", setX);
        addFunction(L, "setY", setY);
        addFunction(L, "setWidth", setWidth);
        addFunction(L, "setHeight", setHeight);
        
        addFunction(L, "getX", getX);
        addFunction(L, "getY", getY);
        addFunction(L, "getWidth", getWidth);
        addFunction(L, "getHeight", getHeight);
        
        addFunction(L, "setColor", setColor);
        addFunction(L, "setText", setText);
        addFunction(L, "onClick", onClick);
        
        
        setColorInit(L);

        if (luaL_loadstring(L, data.c_str()) == 0) {
            if (lua_pcall(L, 0, LUA_MULTRET, 0) == 0) {
                // Lua script executed successfully, and output is captured in luaOutput.
            } else {
                print("Error executing Lua script: " + std::string(lua_tostring(L, -1)));
            }
        } else {
            printf("Error loading Lua script");
        }

        runLuaFunction(L, "run");

        /*while(true)
        {
            gui[0]->updateAll();

            for (int i = 0; i < events.size(); i++)
            {
                if(((events[i].obj)->*(events[i].func))())
                {
                    runLuaFunction(L, events[i].callback);
                }
            }
        }*/

        lua_close(L);
    }

    static int luaNewObject(lua_State* L)
    {
        string type = lua_tostring(L, 1);
        uint x = lua_tonumber(L, 2);
        uint y = lua_tonumber(L, 3);
        uint w = lua_tonumber(L, 4);
        uint h = lua_tonumber(L, 5);

        if(type=="Window")
        {
            gui.push_back(new Window("lua app"));
        }else if(type=="Box")
        {
            gui.push_back(new Box(x,y,w,h));
            gui[idcounter]->enabledBackground = true;
            gui[idcounter]->setBackgroundColor(COLOR_EXTRA_LIGHT);
        }else if(type=="Label")
        {
            gui.push_back(new Label(x,y,w,h));
        }
        
        if(lua_gettop(L)==6) // has parent
        {
            uint id = lua_tonumber(L, 6);
            gui[id]->addChild(gui[idcounter]);
        }
        
        lua_pushinteger(L, idcounter);
        idcounter++;
        return 1;
    }

    static void setColorInit(lua_State* L)
    {
        addVariable(L, "COLOR_LIGHT", COLOR_LIGHT);
        addVariable(L, "COLOR_BLACK", COLOR_BLACK);
        addVariable(L, "COLOR_PRIMARY", COLOR_PRIMARY);
        addVariable(L, "COLOR_SUCCESS", COLOR_SUCCESS);
        addVariable(L, "COLOR_WHITE", COLOR_WARNING);
        addVariable(L, "COLOR_WHITE", COLOR_BLUE);
    }
    
    static int setX(lua_State* L)
    {
        Gui* g = gui[lua_tointeger(L, 1)];
        uint16_t xPos = lua_tointeger(L, 2);
        g->setX(xPos);
        return 0;
    }
    
    static int setY(lua_State* L)
    {
        Gui* g = gui[lua_tointeger(L, 1)];
        uint16_t yPos = lua_tointeger(L, 2);
        g->setY(yPos);
        return 0;
    }
    
    static int setWidth(lua_State* L)
    {
        Gui* g = gui[lua_tointeger(L, 1)];
        uint16_t width = lua_tointeger(L, 2);
        g->setWidth(width);
        return 0;
    }
    
    static int setHeight(lua_State* L)
    {
        Gui* g = gui[lua_tointeger(L, 1)];
        uint16_t height = lua_tointeger(L, 2);
        g->setHeight(height);
        return 0;
    }
    
    static int getX(lua_State* L)
    {
        Gui* g = gui[lua_tointeger(L, 1)];
        lua_pushnumber(L, g->getX());
        return 1;
    }
    
    static int getY(lua_State* L)
    {
        Gui* g = gui[lua_tointeger(L, 1)];
        lua_pushnumber(L, g->getY());
        return 1;
    }
    
    static int getWidth(lua_State* L)
    {
        Gui* g = gui[lua_tointeger(L, 1)];
        lua_pushnumber(L, g->getWidth());
        return 1;
    }
    
    static int getHeight(lua_State* L)
    {
        Gui* g = gui[lua_tointeger(L, 1)];
        lua_pushnumber(L, g->getHeight());
        return 1;
    }
    
    static int setColor(lua_State* L)
    {
        Gui* g = gui[lua_tointeger(L, 1)];
        uint color = lua_tointeger(L, 2);
        g->setBackgroundColor(color);
        return 0;
    }

    static int setText(lua_State* L)
    {
        Gui* g = gui[lua_tointeger(L, 1)];
        //string text = lua_tostring(L, 2);


        if(g->getType() == GUI_TYPE::LABEL_TYPE)
        {
            print("label");
            reinterpret_cast<Label*>(g)->setText(lua_tostring(L, 2));
        }else
            print("not label");
    }

    static int onClick(lua_State* L)
    {
        print("onClick");
        Gui* g = gui[lua_tointeger(L, 1)];
        string text = lua_tostring(L, 2);
        LuaEvent event;
        event.callback = text;
        event.func = &Gui::isTouched;
        event.obj = g;
        events.push_back(event);
    }
};

uint LuaInterpreter::idcounter;
vector<Gui*> LuaInterpreter::gui;
vector<LuaEvent> LuaInterpreter::events;

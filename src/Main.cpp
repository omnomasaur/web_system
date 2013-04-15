//Entry point file.

#include <string>
#include <algorithm>

#include <SFML\Graphics.hpp>
#include <SFML\System.hpp>

#include "WebSystem.h"

// This annoyingly long function converts sf::Keyboard::Keys that are
//  not characters into WPARAM key codes, which are what CEF takes. 
// As far as I am aware there is no workaround to make this shorter.
WPARAM sfkeyToWparam(sf::Keyboard::Key key)
{
	switch(key)
	{
	case sf::Keyboard::LControl		:		return VK_CONTROL;
	case sf::Keyboard::RControl		:		return VK_CONTROL;
	case sf::Keyboard::LSystem		:		return VK_LWIN;
	case sf::Keyboard::RSystem		:		return VK_RWIN;
	case sf::Keyboard::Menu			:		return VK_APPS;
	case sf::Keyboard::SemiColon	:		return VK_OEM_1;
    case sf::Keyboard::Slash		:		return VK_OEM_2;
    case sf::Keyboard::Equal		:		return 	VK_OEM_PLUS	;
    case sf::Keyboard::Dash			:		return 	VK_OEM_MINUS;
    case sf::Keyboard::LBracket		:		return 	VK_OEM_4;
    case sf::Keyboard::RBracket		:		return 	VK_OEM_6;
    case sf::Keyboard::Comma		:		return 	VK_OEM_COMMA;
    case sf::Keyboard::Period		:		return 	VK_OEM_PERIOD;
    case sf::Keyboard::Quote		:		return 	VK_OEM_7;
    case sf::Keyboard::BackSlash	:		return 	VK_OEM_5;
    case sf::Keyboard::Tilde		:		return 	VK_OEM_3;
    case sf::Keyboard::Escape		:		return 	VK_ESCAPE;
    case sf::Keyboard::Space		:		return 	VK_SPACE ;
    case sf::Keyboard::Return		:		return 	VK_RETURN;
    case sf::Keyboard::BackSpace	:       return 	VK_BACK;
    case sf::Keyboard::Tab			:       return 	VK_TAB;
    case sf::Keyboard::PageUp		:		return 	VK_PRIOR;
    case sf::Keyboard::PageDown		:       return 	VK_NEXT;
    case sf::Keyboard::End			:       return 	VK_END;
    case sf::Keyboard::Home			:       return 	VK_HOME;
    case sf::Keyboard::Insert		:		return 	VK_INSERT;
    case sf::Keyboard::Delete		:		return 	VK_DELETE;
    case sf::Keyboard::Add			:       return 	VK_ADD;
    case sf::Keyboard::Subtract		:		return 	VK_SUBTRACT;
    case sf::Keyboard::Multiply		:		return 	VK_MULTIPLY;
    case sf::Keyboard::Divide		:		return 	VK_DIVIDE;
    case sf::Keyboard::Pause		:		return 	VK_PAUSE ;
    case sf::Keyboard::F1			:       return 	VK_F1;
    case sf::Keyboard::F2			:       return 	VK_F2;
    case sf::Keyboard::F3			:       return 	VK_F3;
    case sf::Keyboard::F4			:       return 	VK_F4;
    case sf::Keyboard::F5			:       return 	VK_F5;
    case sf::Keyboard::F6			:       return 	VK_F6;
    case sf::Keyboard::F7			:       return 	VK_F7;
    case sf::Keyboard::F8			:       return 	VK_F8;
    case sf::Keyboard::F9			:       return 	VK_F9;
    case sf::Keyboard::F10			:       return 	VK_F10;
    case sf::Keyboard::F11			:       return 	VK_F11;
    case sf::Keyboard::F12			:       return 	VK_F12;
    case sf::Keyboard::F13			:       return 	VK_F13;
    case sf::Keyboard::F14			:       return 	VK_F14;
    case sf::Keyboard::F15			:       return 	VK_F15;
    case sf::Keyboard::Left			:       return 	VK_LEFT;
    case sf::Keyboard::Right		:		return 	VK_RIGHT;
    case sf::Keyboard::Up			:       return 	VK_UP;
    case sf::Keyboard::Down			:       return 	VK_DOWN;
    case sf::Keyboard::Numpad0		:		return 	VK_NUMPAD0;
    case sf::Keyboard::Numpad1		:		return 	VK_NUMPAD1;
    case sf::Keyboard::Numpad2		:		return 	VK_NUMPAD2;
    case sf::Keyboard::Numpad3		:		return 	VK_NUMPAD3;
    case sf::Keyboard::Numpad4		:		return 	VK_NUMPAD4;
    case sf::Keyboard::Numpad5		:		return 	VK_NUMPAD5;
    case sf::Keyboard::Numpad6		:		return 	VK_NUMPAD6;
    case sf::Keyboard::Numpad7		:		return 	VK_NUMPAD7;
    case sf::Keyboard::Numpad8		:		return 	VK_NUMPAD8;
	case sf::Keyboard::Numpad9		:		return 	VK_NUMPAD9;
	}

	return VK_NONAME;
}

// These two commands are used for getting parameters from argv in main.
// This program doesn't actually use that, but they're included here anyway.  
bool getCmd(char** begin, char** end, const std::string & option)
{
	char** itr = std::find(begin, end, option);

	if(itr != end)
	{
		return true;
	}
	return false;
}

char* getCmdOption(char** begin, char** end, const std::string & option)
{
    char** itr = std::find(begin, end, option);
    if (itr != end && ++itr != end)
    {
        return *itr;
    }
    return NULL;
}

// This is the javascript callback function that gets bound to "testCallback"
// Each JS binding gets it's own callback function which must return a bool
//  and take these parameters.  
bool jsCallback (
		CefRefPtr<CefV8Value> object,
		const CefV8ValueList& arguments,
		CefRefPtr<CefV8Value>& retval,
		CefString& exception
						  )
{
	std::string text = arguments[0]->GetStringValue();
	printf("Text: %s\n", text.c_str());
	return true;
}

int main(int argc, char* argv[])
{
	//Make the window to render things in.
	sf::RenderWindow window;
	window.create(sf::VideoMode(1280, 720), "test_base");

	//WebInterfaces are used to interact with the web system.  
	WebInterface* pWeb;

	//This starts the thread that runs CEF.
	//Said thread is a part of the WebSystem and not CEF's own multi threading.  
	//This is because CEF1's multi threading does not support creating and
	// removing browsers once it's multi threading is started.  
	//CEF3 can't be used as it does not yet support off screen rendering.  
	WebSystem::StartWeb();

	//Register our custom scheme for loading local files.  
	//"local" specifies local:// as the scheme.
	//"res" specifies /res as the domain.  
	WebSystem::RegisterScheme("local", "res", new LocalSchemeHandlerFactory());

	//Create our web interface and retrieve a pointer to it.  
	//Parameters are (width, height, staring URL, transparent background)
	pWeb = WebSystem::CreateWebInterfaceSync(1280, 720, 
		//"local://../res/index.htm"   "http://www.google.com"
		"http://www.google.com"
		, true);
	
	//Add the binding for "testCallback" to our web interface.  
	//Specifies jsCallback as the function to be called for the binding.  
	pWeb->AddJSBinding("testCallback", &jsCallback);

	//Set up the sprite which will be drawing our web texture.  
	sf::Sprite sprite;
	//printf("Setting sprite texture.\n");
	//Get the texture from pWeb to draw in texture.  This only need be done once.  
	sprite.setTexture(*pWeb->GetTexture());
	sprite.setOrigin(sprite.getTexture()->getSize().x / 2.0f, sprite.getTexture()->getSize().y / 2.0f);
	sprite.setPosition(640, 360);

	//This is acutally the example loop from SFML's tutorials.  
    // run the program as long as the window is open
    while (window.isOpen())
    {
        // check all the window's events that were triggered since the last iteration of the loop
        sf::Event event;
        while (window.pollEvent(event))
        {
            // "close requested" event: we close the window
            if (event.type == sf::Event::Closed)
			{
                window.close();
			}

			if (event.type == sf::Event::KeyPressed)
			{
				//These are just for the rotation to demonstrate the web being drawn to a sprite.  
				if(event.key.code == sf::Keyboard::Q)
				{
					sprite.rotate(1.0f);
				}
				if(event.key.code == sf::Keyboard::E)
				{
					sprite.rotate(-1.0f);
				}
				if(event.key.code == sf::Keyboard::S)
				{
					pWeb->SetSize(640, 360);
					sprite.setTexture(*pWeb->GetTexture(), true);
					sprite.setOrigin(sprite.getTexture()->getSize().x / 2.0f, sprite.getTexture()->getSize().y / 2.0f);
					sprite.setPosition(640, 360);
				}
				
				//Make sure we give focus to the browser since we are about to send it input.  
				//Since focus is never taken away in this example, this only actually needs to be done once.  
				pWeb->SendFocusEvent(true);

				//Set up and pass the key down to the web interface, which then passes to CEF.
				CefBrowser::KeyType type = KT_KEYDOWN;
				CefKeyInfo keyInfo;
				WPARAM key = sfkeyToWparam(event.key.code);
				keyInfo.key = key;

				if(key != VK_NONAME)
					pWeb->SendKeyEvent(type, keyInfo, 0);
			}

			if (event.type == sf::Event::KeyReleased)
			{
				//Same as passing the key down, but for the key up.  
				CefBrowser::KeyType type = KT_KEYUP;
				CefKeyInfo keyInfo;
				WPARAM key = sfkeyToWparam(event.key.code);
				keyInfo.key = key;

				if(key != VK_NONAME)
					pWeb->SendKeyEvent(type, keyInfo, 0);
			}

			if (event.type == sf::Event::TextEntered)
			{
				//This passes the key events for keys which have a character value.  
				pWeb->SendFocusEvent(true);
				CefBrowser::KeyType type = KT_CHAR;
				CefKeyInfo keyInfo;
				keyInfo.key = (WPARAM)(char)event.text.unicode;
				//Set the proper modifier so that shift/control/alt works.
				int modifier = 0;
				if(sf::Keyboard::isKeyPressed(sf::Keyboard::LShift))
					modifier = 1;
				else if(sf::Keyboard::isKeyPressed(sf::Keyboard::LControl))
					modifier = 2;
				else if(sf::Keyboard::isKeyPressed(sf::Keyboard::LAlt))
					modifier = 4;

				pWeb->SendKeyEvent(type, keyInfo, modifier);
			}

			if(event.type == sf::Event::MouseButtonPressed)
			{
				//Send the mouse down events.  
				pWeb->SendFocusEvent(true);
				//These two lines transform the global mouse position into coordinates of the sprite
				// which the web is being drawn to.  
				sf::Vector2f point = (sf::Vector2f)sf::Mouse::getPosition(window);
				point = sprite.getInverseTransform().transformPoint(point);
				//printf("Point: (%d, %d)\n", (int)point.x, (int)point.y);
				pWeb->SendMouseClickEvent((int)point.x, (int)point.y, MBT_LEFT, false, 1);
			}

			if(event.type == sf::Event::MouseButtonReleased)
			{
				//Send the mouse release events.
				sf::Vector2f point = (sf::Vector2f)sf::Mouse::getPosition(window);
				point = sprite.getInverseTransform().transformPoint(point);
				pWeb->SendMouseClickEvent((int)point.x, (int)point.y, MBT_LEFT, true, 1);
			}

			if(event.type == sf::Event::MouseMoved)
			{
				//Send events for when the mouse moves.  
				sf::Vector2f point = (sf::Vector2f)sf::Mouse::getPosition(window);
				point = sprite.getInverseTransform().transformPoint(point);
				pWeb->SendMouseMoveEvent((int)point.x, (int)point.y, false);
			}

			if(event.type == sf::Event::MouseWheelMoved)
			{
				//Send events for the mousewheel.  
				//CURRENTLY BROKEN AND DOES NOTHING
				//printf("Mousewheel: %d\n", event.mouseWheel.delta);
				sf::Vector2f point = (sf::Vector2f)sf::Mouse::getPosition(window);
				point = sprite.getInverseTransform().transformPoint(point);
				pWeb->SendMouseWheelEvent((int)point.x, (int)point.y, event.mouseWheel.delta, 0);
			}
        }

		//UPDATE ZONE
		//This is actually redundantly updates the texture.
		//The first draw is done when CEF calls onPaint, but that can be interrupted
		// by the sprite being drawn (which changes the texture binding in openGL).
		// So if this is not called there will be some strange display errors.  
		// Since this can't be interrupted by drawing there will not be errors here.  
		WebSystem::UpdatTextures();

		//DRAW ZONE
		//standard pre-draw back buffer clear.  
		window.clear(sf::Color::Black);

		//draw stuff
		window.draw(sprite);

		window.display();
    }

	//Release pWeb so that CEF can shutdown properly.  
	pWeb->Release();

	//Signal for the web system to end it's thread.  
	WebSystem::EndWeb();

	//Wait for the web system to finish working before ending.  
	WebSystem::WaitForWebEnd();

	return EXIT_SUCCESS;
}
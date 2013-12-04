//Entry point file.

#include <string>
#include <algorithm>

#include <SFML\Graphics.hpp>

#include "WebSystem.h"
#include "CustomScheme.h"

// This annoyingly long function converts sf::Keyboard::Keys that are
//  not characters into WPARAM key codes, which are what CEF takes. 
// As far as I am aware there is no workaround to make this shorter.
WPARAM sfkeyToWparam(sf::Keyboard::Key key)
{
	switch (key)
	{
	case sf::Keyboard::LControl:		return VK_CONTROL;
	case sf::Keyboard::RControl:		return VK_CONTROL;
	case sf::Keyboard::LSystem:		return VK_LWIN;
	case sf::Keyboard::RSystem:		return VK_RWIN;
	case sf::Keyboard::Menu:		return VK_APPS;
	case sf::Keyboard::SemiColon:		return VK_OEM_1;
	case sf::Keyboard::Slash:		return VK_OEM_2;
	case sf::Keyboard::Equal:		return 	VK_OEM_PLUS;
	case sf::Keyboard::Dash:		return 	VK_OEM_MINUS;
	case sf::Keyboard::LBracket:		return 	VK_OEM_4;
	case sf::Keyboard::RBracket:		return 	VK_OEM_6;
	case sf::Keyboard::Comma:		return 	VK_OEM_COMMA;
	case sf::Keyboard::Period:		return 	VK_OEM_PERIOD;
	case sf::Keyboard::Quote:		return 	VK_OEM_7;
	case sf::Keyboard::BackSlash:		return 	VK_OEM_5;
	case sf::Keyboard::Tilde:		return 	VK_OEM_3;
	case sf::Keyboard::Escape:		return 	VK_ESCAPE;
	case sf::Keyboard::Space:		return 	VK_SPACE;
	case sf::Keyboard::Return:		return 	VK_RETURN;
	case sf::Keyboard::BackSpace:       return 	VK_BACK;
	case sf::Keyboard::Tab:       return 	VK_TAB;
	case sf::Keyboard::PageUp:		return 	VK_PRIOR;
	case sf::Keyboard::PageDown:       return 	VK_NEXT;
	case sf::Keyboard::End:       return 	VK_END;
	case sf::Keyboard::Home:       return 	VK_HOME;
	case sf::Keyboard::Insert:		return 	VK_INSERT;
	case sf::Keyboard::Delete:		return 	VK_DELETE;
	case sf::Keyboard::Add:       return 	VK_ADD;
	case sf::Keyboard::Subtract:		return 	VK_SUBTRACT;
	case sf::Keyboard::Multiply:		return 	VK_MULTIPLY;
	case sf::Keyboard::Divide:		return 	VK_DIVIDE;
	case sf::Keyboard::Pause:		return 	VK_PAUSE;
	case sf::Keyboard::F1:       return 	VK_F1;
	case sf::Keyboard::F2:       return 	VK_F2;
	case sf::Keyboard::F3:       return 	VK_F3;
	case sf::Keyboard::F4:       return 	VK_F4;
	case sf::Keyboard::F5:       return 	VK_F5;
	case sf::Keyboard::F6:       return 	VK_F6;
	case sf::Keyboard::F7:       return 	VK_F7;
	case sf::Keyboard::F8:       return 	VK_F8;
	case sf::Keyboard::F9:       return 	VK_F9;
	case sf::Keyboard::F10:       return 	VK_F10;
	case sf::Keyboard::F11:       return 	VK_F11;
	case sf::Keyboard::F12:       return 	VK_F12;
	case sf::Keyboard::F13:       return 	VK_F13;
	case sf::Keyboard::F14:       return 	VK_F14;
	case sf::Keyboard::F15:       return 	VK_F15;
	case sf::Keyboard::Left:       return 	VK_LEFT;
	case sf::Keyboard::Right:		return 	VK_RIGHT;
	case sf::Keyboard::Up:       return 	VK_UP;
	case sf::Keyboard::Down:       return 	VK_DOWN;
	case sf::Keyboard::Numpad0:		return 	VK_NUMPAD0;
	case sf::Keyboard::Numpad1:		return 	VK_NUMPAD1;
	case sf::Keyboard::Numpad2:		return 	VK_NUMPAD2;
	case sf::Keyboard::Numpad3:		return 	VK_NUMPAD3;
	case sf::Keyboard::Numpad4:		return 	VK_NUMPAD4;
	case sf::Keyboard::Numpad5:		return 	VK_NUMPAD5;
	case sf::Keyboard::Numpad6:		return 	VK_NUMPAD6;
	case sf::Keyboard::Numpad7:		return 	VK_NUMPAD7;
	case sf::Keyboard::Numpad8:		return 	VK_NUMPAD8;
	case sf::Keyboard::Numpad9:		return 	VK_NUMPAD9;
	}

	return VK_NONAME;
}

//These two functions are for checking command line arguments.
//They aren't even used here, but are included next to the 
//entry point out of habit.
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
bool jsCallback(
	CefRefPtr<CefListValue> arguments
	)
{
	//Get argument 0 as a string.  
	std::string text = arguments->GetString(0);
	printf("Text: %s\n", text.c_str());
	return true;
}

int main(int argc, char* argv[])
{
	//This will force WebSystem to run in cef single process mode.  
	//WebSystem::SetSingleProcess(true);

	//This will set another process in the directory we launched from as the process to run for cef multi-process architecture.
	//If this is invalid, WebSystem will attempt to use duplicates of this process.
	WebSystem::SetSubprocess("test_web.exe");

	//Main must be run here if we wish to allow duplicates of this process to be used with cef multi-process architecture.
	int exit_code = WebSystem::Main();
	//If the exit code returned by main was anything other than -1, it means that this process is a child doing cef work for another process.
	//It also means that this child has closed, meaning we should exit main.
	if (exit_code >= 0)
	{
		return exit_code;
	}

	//Make the window to render things in.
	sf::RenderWindow window;
	window.create(sf::VideoMode(1280, 720), "test_base", sf::Style::Close);
	window.setFramerateLimit(60);

	//WebInterfaces are used to interact with the web system.  
	std::vector<CefRefPtr<WebInterface>> webInterfaces;
	std::vector<sf::Sprite> sprites;

	//This starts the thread that runs CEF.
	//Said thread is a part of the WebSystem and not CEF's own multi threading.  
	//This is because CEF1's multi threading does not support creating and
	// removing browsers once it's multi threading is started.  
	//CEF3 now supports offscreen rendering so technically this could be rewritten
	//to not use it's own thread, but if it aint broke don't fix it.
	WebSystem::StartWeb();

	//Register our custom scheme for loading local files.  
	//"local" specifies local:// as the scheme.
	//"res" specifies /res as the domain.  
	WebSystem::RegisterScheme("local", "res", new LocalSchemeHandlerFactory());

	int cols = 1;
	int rows = 1;
	for (int i = 0; i < cols * rows; i++)
	{
		int currCol, currRow;
		currCol = i % cols;
		currRow = (int)floor((double)i / cols);
		CefRefPtr<WebInterface> pWeb;
		//Create our web interface and retrieve a pointer to it.  
		//Parameters are (width, height, staring URL, transparent background)
		pWeb = WebSystem::CreateWebInterfaceSync(
			(int)(window.getSize().x / (float)cols)
			, (int)(window.getSize().y / (float)rows)
			//,"local://../res/index.htm"
			//,"http://www.randomwebsite.com/cgi-bin/random.pl"
			, "http://www.google.com"
			, true);

		//Add the binding for "testCallback" to our web interface.  
		//Specifies jsCallback as the function to be called for the binding.  
		pWeb->AddJSBinding("testCallback", &jsCallback);

		webInterfaces.push_back(pWeb);

		//Set up the sprite which will be drawing our web texture.  
		sf::Sprite sprite;
		//printf("Setting sprite texture.\n");
		//Get the texture from pWeb to draw in texture.  This only need be done once.  
		sprite.setTexture(*pWeb->GetTexture());
		//sprite.setOrigin(sprite.getTexture()->getSize().x / 2.0f, sprite.getTexture()->getSize().y / 2.0f);
		float x, y;
		x = ((float)currCol / cols) * window.getSize().x;
		y = ((float)currRow / rows) * window.getSize().y;
		sprite.setPosition(x, y);

		sprites.push_back(sprite);
	}

	//These variables are used to track rapid mouse clicks.
	//This is so that we can properly send double/triple clicks to WebSystem.
	sf::Clock clickClock; clickClock.restart();
	float clickTime = 0.25f;
	sf::Mouse::Button lastClickType = sf::Mouse::Left;
	int clickCount = 1;

	//This is acutally the exact example loop from SFML's tutorials.  
    // run the program as long as the window is open
    while (window.isOpen())
    {
        // check all the window's events that were triggered since the last iteration of the loop
        sf::Event event;
		while (window.pollEvent(event))
		{
			// "close requested" event: we close the window
			if (event.type == sf::Event::Closed)
				window.close();

			//if (!webInterfaces.size()) continue;
				if (event.type == sf::Event::MouseButtonPressed)
				{

					if (clickClock.getElapsedTime().asSeconds() < clickTime && event.mouseButton.button == lastClickType)
						clickCount++;
					else
						clickCount = 1;

					lastClickType = event.mouseButton.button;

					for (unsigned int i = 0; i < webInterfaces.size(); i++)
					{
						if (webInterfaces.size())
							webInterfaces[i]->SendFocusEvent(true);
						//Send the mouse down events.  
						//These two lines transform the global mouse position into coordinates of the sprite
						// which the web is being drawn to.  
						sf::Vector2f point = (sf::Vector2f)sf::Mouse::getPosition(window);
						point = sprites[i].getInverseTransform().transformPoint(point);
						//printf("Point: (%d, %d)\n", (int)point.x, (int)point.y);
						//CefBrowserHost::MouseButtonType type = event.mouseButton.button == sf::Mouse::Left ? MBT_LEFT : MBT_RIGHT;

						webInterfaces[i]->SendMouseClickEvent((int)point.x, (int)point.y, event.mouseButton.button, false, clickCount);

						clickClock.restart();
					}
				}
				if (event.type == sf::Event::MouseButtonReleased)
				{
					for (unsigned int i = 0; i < webInterfaces.size(); i++)
					{
						if (webInterfaces.size())
							webInterfaces[i]->SendFocusEvent(true);
						//Send the mouse release events.
						sf::Vector2f point = (sf::Vector2f)sf::Mouse::getPosition(window);
						point = sprites[i].getInverseTransform().transformPoint(point);
						CefBrowserHost::MouseButtonType type = event.mouseButton.button == sf::Mouse::Left ? MBT_LEFT : MBT_RIGHT;
						webInterfaces[i]->SendMouseClickEvent((int)point.x, (int)point.y, event.mouseButton.button, true, clickCount);
					}
				}
			for (unsigned int i = 0; i < webInterfaces.size(); i++)
			{
				if (webInterfaces.size())
					webInterfaces[i]->SendFocusEvent(true);
				if (event.type == sf::Event::MouseMoved)
				{
					//Send events for when the mouse moves.  
					sf::Vector2f point = (sf::Vector2f)sf::Mouse::getPosition(window);
					point = sprites[i].getInverseTransform().transformPoint(point);
					webInterfaces[i]->SendMouseMoveEvent((int)point.x, (int)point.y, false);
				}
				if (event.type == sf::Event::MouseWheelMoved)
				{
					//Send events for the mousewheel.  
					//CURRENTLY BROKEN AND DOES NOTHING
					//printf("Mousewheel: %d\n", event.mouseWheel.delta);
					sf::Vector2f point = (sf::Vector2f)sf::Mouse::getPosition(window);
					point = sprites[i].getInverseTransform().transformPoint(point);
					webInterfaces[i]->SendMouseWheelEvent((int)point.x, (int)point.y, 0, event.mouseWheel.delta * 30);
				}
				if (event.type == sf::Event::KeyPressed)
				{
					if (event.key.code == sf::Keyboard::Escape)
					{
						if (webInterfaces.size())
						{
							//webInterfaces.front()->GetBrowser()->GetHost()->ParentWindowWillClose();
							//webInterfaces.front()->GetBrowser()->GetHost()->CloseBrowser(true);
							//webInterfaces.front()->Release();

							for (unsigned int i = 0; i < webInterfaces.size(); i++)
							{
								webInterfaces[i]->Close();
								webInterfaces[i] = NULL;
							}
							webInterfaces.clear();
							sprites.clear();
						}
					}
					//if (!webInterfaces.size()) continue;
					//if (event.key.code == sf::Keyboard::A)
					//{
					//	webInterfaces.front()->ExecuteJS("document.getElementById(\"input_textbox\").value = \"c++_put_this_here\";\n");
					//}
					////These are just for the rotation to demonstrate the web being drawn to a sprite.  
					//if (event.key.code == sf::Keyboard::Q)
					//{
					//	sprites.front().rotate(1.0f);
					//}
					//if (event.key.code == sf::Keyboard::E)
					//{
					//	sprites.front().rotate(-1.0f);
					//}
					//if (event.key.code == sf::Keyboard::S)
					//{
					//	if (!webInterfaces.size()) continue;
					//	webInterfaces.front()->SetSize(640, 360);
					//	sprites.front().setTexture(*webInterfaces.front()->GetTexture(), true);
					//	sprites.front().setOrigin(sprites.front().getTexture()->getSize().x / 2.0f, sprites.front().getTexture()->getSize().y / 2.0f);
					//	sprites.front().setPosition(640, 360);
					//}

					if (!webInterfaces.size())continue;

					//Make sure we give focus to the browser since we are about to send it input.  
					//Since focus is never taken away in this example, this only actually needs to be done once.  
					//webInterfaces[i]->SendFocusEvent(true);

					//Set up and pass the key down to the web interface, which then passes to CEF.
					cef_key_event_type_t type = KEYEVENT_KEYDOWN;
					//CefKeyInfo keyInfo;
					WPARAM key = sfkeyToWparam(event.key.code);

					if (key != VK_NONAME)
						webInterfaces[i]->SendKeyEvent(type, key);
				}
				if (event.type == sf::Event::KeyReleased)
				{
					//Same as passing the key down, but for the key up.  
					cef_key_event_type_t type = KEYEVENT_KEYUP;
					//CefKeyInfo keyInfo;
					WPARAM key = sfkeyToWparam(event.key.code);
					//keyInfo.key = key;

					if (key != VK_NONAME)
						webInterfaces[i]->SendKeyEvent(type, key);
				}
				if (event.type == sf::Event::TextEntered)
				{
					//This passes the key events for keys which have a character value.  
					//webInterfaces[i]->SendFocusEvent(true);
					cef_key_event_type_t type = KEYEVENT_CHAR;
					//CefKeyInfo keyInfo;
					WPARAM key = (WPARAM)(char)event.text.unicode;

					webInterfaces[i]->SendKeyEvent(type, key);
				}
			}

			if (!webInterfaces.size())
			{
				if (event.type == sf::Event::KeyPressed)
				{
					if (event.key.code == sf::Keyboard::Escape)
					{
						webInterfaces.push_back(WebSystem::CreateWebInterfaceSync(1280, 720,
							//"local://../res/index.htm"
							"http://www.google.com"
							, true));
						webInterfaces.back()->AddJSBinding("testCallback", &jsCallback);
						//sprites.back().setTexture(*webInterfaces.front()->GetTexture());
						//sprites.back().setOrigin(sprites.front().getTexture()->getSize().x / 2.0f, sprites.front().getTexture()->getSize().y / 2.0f);
						//sprites.back().setPosition(640, 360);

						//Set up the sprite which will be drawing our web texture.  
						sf::Sprite sprite;
						//Get the texture from pWeb to draw in texture.  This only need be done once.  
						sprite.setTexture(*webInterfaces.back()->GetTexture());
						//sprite.setOrigin(sprite.getTexture()->getSize().x / 2.0f, sprite.getTexture()->getSize().y / 2.0f);
						sprites.push_back(sprite);
					}
				}
			}
        }

		WebSystem::UpdateInterfaceTextures();

		//DRAW ZONE
		window.clear();

		//draw stuff
		for (unsigned int i = 0; i < sprites.size(); i++)
			window.draw(sprites[i]);

		window.display();
    }

	WebSystem::EndWeb();
	WebSystem::WaitForWebEnd();

	return EXIT_SUCCESS;
}
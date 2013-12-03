
////////////////////////////////////////////////////////////
// Headers
////////////////////////////////////////////////////////////
#include "WebSystem.h"
#include <fstream>
#include <utility>

////////////////////////////////////////////////////////////
// Static variables
////////////////////////////////////////////////////////////
std::string WebSystem::sSubprocess = "";
bool WebSystem::sSingleProcess = false;
CefMainArgs WebSystem::sArgs;
CefRefPtr<WebApp> WebSystem::sApp = NULL;
CefRefPtr<WebSystem> WebSystem::sInstance = NULL;
sf::Thread* WebSystem::spThread = NULL;
bool WebSystem::sEndThread = false;
std::queue<WebSystem::RegScheme> WebSystem::sRegSchemeQueue;
std::queue<WebInterface*> WebSystem::sMakeWebInterfaceQueue;
std::map<int, WebInterface*> WebSystem::sWebInterfaces;
WebSystem::BindingMap WebSystem::sBindings;

////////////////////////////////////////////////////////////
//These functions are taken / some slightly modified from cefclient2010 example code.  
//They are used in order to send the parameters of javascript bindings from the render process to the browser process.
void SetList(CefRefPtr<CefV8Value> source, CefRefPtr<CefListValue> target);
void SetList(CefRefPtr<CefListValue> source, CefRefPtr<CefV8Value> target);

// Transfer a V8 value to a List index.
void SetListValue(CefRefPtr<CefListValue> list, int index,
	CefRefPtr<CefV8Value> value) {
	if (value->IsArray()) {
		CefRefPtr<CefListValue> new_list = CefListValue::Create();
		SetList(value, new_list);
		list->SetList(index, new_list);
	}
	else if (value->IsString()) {
		list->SetString(index, value->GetStringValue());
	}
	else if (value->IsBool()) {
		list->SetBool(index, value->GetBoolValue());
	}
	else if (value->IsInt()) {
		list->SetInt(index, value->GetIntValue());
	}
	else if (value->IsDouble()) {
		list->SetDouble(index, value->GetDoubleValue());
	}
}

// Transfer a V8 array to a List.
void SetList(CefRefPtr<CefV8Value> source, CefRefPtr<CefListValue> target) {
	//ASSERT(source->IsArray());

	int arg_length = source->GetArrayLength();
	if (arg_length == 0)
		return;

	// Start with null types in all spaces.
	target->SetSize(arg_length);

	for (int i = 0; i < arg_length; ++i)
		SetListValue(target, i, source->GetValue(i));
}

// Transfer a List value to a V8 array index.
void SetListValue(CefRefPtr<CefV8Value> list, int index,
	CefRefPtr<CefListValue> value) {
	CefRefPtr<CefV8Value> new_value;

	CefValueType type = value->GetType(index);
	switch (type) {
	case VTYPE_LIST: {
						 CefRefPtr<CefListValue> list = value->GetList(index);
						 new_value = CefV8Value::CreateArray(static_cast<int>(list->GetSize()));
						 SetList(list, new_value);
	} break;
	case VTYPE_BOOL:
		new_value = CefV8Value::CreateBool(value->GetBool(index));
		break;
	case VTYPE_DOUBLE:
		new_value = CefV8Value::CreateDouble(value->GetDouble(index));
		break;
	case VTYPE_INT:
		new_value = CefV8Value::CreateInt(value->GetInt(index));
		break;
	case VTYPE_STRING:
		new_value = CefV8Value::CreateString(value->GetString(index));
		break;
	default:
		break;
	}

	if (new_value.get()) {
		list->SetValue(index, new_value);
	}
	else {
		list->SetValue(index, CefV8Value::CreateNull());
	}
}

// Transfer a List to a V8 array.
void SetList(CefRefPtr<CefListValue> source, CefRefPtr<CefV8Value> target) {
	//ASSERT(target->IsArray());

	int arg_length = static_cast<int>(source->GetSize());
	if (arg_length == 0)
		return;

	for (int i = 0; i < arg_length; ++i)
		SetListValue(target, i, source);
}

//--------------------------------------------------------------------------------------------------------------------------
//Internal Methods
//--------------------------------------------------------------------------------------------------------------------------

////////////////////////////////////////////////////////////
CefRefPtr<WebSystem> WebSystem::GetInstance()
{
	if (!sInstance)
	{
		sInstance = new WebSystem();
	}

	return sInstance;
}

////////////////////////////////////////////////////////////
//This method is what runs our thread.
void WebSystem::WebThread()
{
	if (!sApp)
	{
		Main();
	}

	//Standard fare CEF initialization.
	CefSettings settings;
	settings.multi_threaded_message_loop = false;
	settings.single_process = sSingleProcess;

	//If we have a specified subrocess path, use that subprocess.
	if (sSubprocess != "")
	{
		//Check if file exists.
		std::ifstream ifile(sSubprocess.c_str());
		if (ifile)
			CefString(&settings.browser_subprocess_path).FromASCII(sSubprocess.c_str());
	}

	CefInitialize(sArgs, settings, sApp.get());

	while (!sEndThread)
	{
		CefDoMessageLoopWork();

		while (sRegSchemeQueue.size() > 0)
		{
			CefRegisterSchemeHandlerFactory(sRegSchemeQueue.front().mName, sRegSchemeQueue.front().mDomain, sRegSchemeQueue.front().mFactory);
			sRegSchemeQueue.pop();
		}

		while (sMakeWebInterfaceQueue.size() > 0)
		{
			GetInstance()->AddBrowserToInterface(sMakeWebInterfaceQueue.front());
			sMakeWebInterfaceQueue.pop();
		}
	}

	//WE SHOULD PROBABLY CHECK IF THERE ARE STILL LIVE BROWSERS HERE

	CefShutdown();
}

////////////////////////////////////////////////////////////
void WebSystem::AddBrowserToInterface(WebInterface* pWeb)
{
	AutoLock lock_scope(this);
	CefWindowInfo info;
	info.SetTransparentPainting(pWeb->IsTransparent());
	info.SetAsOffScreen(NULL); //We're drawing offscreen so pass NULL as the window handler.  
	CefBrowserSettings browserSettings;

	CefRefPtr<CefBrowser> browser = CefBrowserHost::CreateBrowserSync(info, this, pWeb->GetCurrentURL(), browserSettings);

	pWeb->mBrowser = browser;

	sWebInterfaces[browser->GetIdentifier()] = pWeb;
}

//--------------------------------------------------------------------------------------------------------------------------
//API Methods
//--------------------------------------------------------------------------------------------------------------------------

////////////////////////////////////////////////////////////
int WebSystem::Main()
{
#if(PLATFORM == WINDOWS)
	sArgs = CefMainArgs(GetModuleHandle(NULL));
#else
	sArgs = CefMainArgs();
#endif
	sApp = new WebApp();

	int exit_code = CefExecuteProcess(sArgs, sApp.get());

	if (exit_code >= 0)
	{
		return exit_code;
	}

	return -1;
}

////////////////////////////////////////////////////////////
void WebSystem::StartWeb()
{
	if (!spThread)
	{
		spThread = new sf::Thread(&WebThread);

		sEndThread = false;
		spThread->launch();
	}
}

////////////////////////////////////////////////////////////
void WebSystem::EndWeb()
{
	sEndThread = true;
}

////////////////////////////////////////////////////////////
void WebSystem::WaitForWebEnd()
{
	if (spThread)
	{
		spThread->wait();

		delete spThread;
		spThread = NULL;
	}
}

////////////////////////////////////////////////////////////
void WebSystem::RegisterScheme(std::string name, std::string domain, CefRefPtr<CefSchemeHandlerFactory> factory)
{
	sRegSchemeQueue.push(RegScheme(name, domain, factory));
}

////////////////////////////////////////////////////////////
WebInterface* WebSystem::CreateWebInterfaceSync(int width, int height, const std::string& url, bool transparent)
{
	WebInterface* pWeb = new WebInterface(width, height, url, transparent);

	sMakeWebInterfaceQueue.push(pWeb);

	while (!pWeb->mBrowser)
	{
		//Sleep for one millisecond to prevent 100% CPU usage.
		//There is probably a more elegant solution for this.
		Sleep(1); 
	}

	return pWeb;
}

//////////////////////////////////////////////////////////// 
void WebSystem::UpdateInterfaceTextures()
{
	std::map<int, WebInterface*>::iterator i;
	for (i = sWebInterfaces.begin(); i != sWebInterfaces.end(); i++)
	{
		i->second->UpdateTexture();
	}
}

//--------------------------------------------------------------------------------------------------------------------------
//Methods called by CEF
//--------------------------------------------------------------------------------------------------------------------------

////////////////////////////////////////////////////////////
void WebSystem::OnBrowserCreated(CefRefPtr<CefBrowser> browser)
{
	//Send all JsBindings
	BindingMap::iterator i = sBindings.begin();
	for (; i != sBindings.end(); i++)
	{
		if (i->first.second == browser->GetIdentifier())
		{
			CefRefPtr<CefProcessMessage> message = CefProcessMessage::Create("jsbinding_create");
			message->GetArgumentList()->SetString(0, i->first.first);
			message->GetArgumentList()->SetInt(1, i->first.second);

			browser->SendProcessMessage(PID_RENDERER, message);
		}
	}
}

////////////////////////////////////////////////////////////
void WebSystem::OnBrowserDestroyed(CefRefPtr<CefBrowser> browser)
{
	BindingMap::iterator i = sBindings.begin();
	for (; i != sBindings.end();)
	{
		if (i->first.second == browser->GetIdentifier())
			sBindings.erase(i++);
		else
			++i;
	}
}

////////////////////////////////////////////////////////////
void WebSystem::OnContextCreated(CefRefPtr<CefBrowser> browser, CefRefPtr<CefFrame> frame, CefRefPtr<CefV8Context> context)
{
	//Retrieve the context's window object.
	CefRefPtr<CefV8Value> object = context->GetGlobal();

	//Create a new callback handler.
	CefRefPtr<CefV8Handler> handler = new WebV8Handler();

	//Setup bindings for that web interface.	
	BindingMap::iterator i = sBindings.begin();
	for (; i != sBindings.end(); i++)
	{
		if (i->first.second == browser->GetIdentifier())
		{
			CefRefPtr<CefV8Value> func = CefV8Value::CreateFunction(i->first.first, handler);

			object->SetValue(i->first.first, func, V8_PROPERTY_ATTRIBUTE_NONE);
		}
	}
}

////////////////////////////////////////////////////////////
void WebSystem::OnContextReleased(CefRefPtr< CefBrowser > browser, CefRefPtr< CefFrame > frame, CefRefPtr< CefV8Context > context)
{

}

////////////////////////////////////////////////////////////
void WebSystem::OnUncaughtException(CefRefPtr< CefBrowser > browser, CefRefPtr< CefFrame > frame, CefRefPtr< CefV8Context > context, CefRefPtr< CefV8Exception > exception, CefRefPtr< CefV8StackTrace > stackTrace)
{

}

////////////////////////////////////////////////////////////
void WebSystem::OnRenderProcessThreadCreated(CefRefPtr<CefListValue> extra_info)
{

}

////////////////////////////////////////////////////////////
bool WebSystem::OnProcessMessageReceived(CefRefPtr<CefBrowser> browser, CefProcessId source_process, CefRefPtr<CefProcessMessage> message)
{
	CefString message_name = message->GetName();

	//Identify whether we are in the browser process or the render process.
	if (CefCurrentlyOn(TID_RENDERER))
	{
		//Renderer
		if (message_name == "jsbinding_create")
		{
			std::string name = message->GetArgumentList()->GetString(0);

			sBindings.insert(std::make_pair(
				std::make_pair(name, message->GetArgumentList()->GetInt(1)), 
				(JsBinding::JsCallback)NULL));

			browser->Reload();

			return true;
		}
	}
	else
	{
		//Browser
		if (message_name == "jsbinding_call")
		{
			if (sWebInterfaces.count(browser->GetIdentifier()))
			{
				WebInterface* pWeb = sWebInterfaces[browser->GetIdentifier()];

				std::string name;
				CefRefPtr<CefListValue> arguments = CefListValue::Create();
				CefRefPtr<CefV8Value> retval;
				CefString exception;

				CefRefPtr<CefListValue> margs = message->GetArgumentList();
				name = margs->GetString(0);
				int argCount = margs->GetInt(1);
				std::string test = margs->GetString(2);
				for (int i = 0; i < argCount; i++)
				{
					CefValueType type = margs->GetType(i + 2);
					switch (type) {
					case VTYPE_LIST: {
						CefRefPtr<CefListValue> list = margs->GetList(i + 2);
						arguments->SetList(i, list);
					} break;
					case VTYPE_BOOL: {
						bool b = margs->GetBool(i + 2);
						arguments->SetBool(i, b);
					} break;
					case VTYPE_DOUBLE: {
						double d = margs->GetDouble(i + 2);
						arguments->SetDouble(i, d);
					} break;
					case VTYPE_INT: {
						int integer = margs->GetInt(i + 2);
						arguments->SetInt(i, integer);
					} break;
					case VTYPE_STRING: {
						std::string s = margs->GetString(i + 2);
						arguments->SetString(i, s);
					} break;
					default:
						break;
					}
				}

				pWeb->JSCallback(name, arguments);

				return true;
			}
		}
	}

	return false;
}

////////////////////////////////////////////////////////////
void WebSystem::OnLoadStart(CefRefPtr<CefBrowser> browser, CefRefPtr<CefFrame> frame)
{

}

////////////////////////////////////////////////////////////
void WebSystem::OnLoadEnd(CefRefPtr<CefBrowser> browser, CefRefPtr<CefFrame> frame, int httpStatusCode)
{

}

////////////////////////////////////////////////////////////
void WebSystem::OnAfterCreated(CefRefPtr<CefBrowser> browser)
{
	AutoLock lock_scope(this);
}

////////////////////////////////////////////////////////////
void WebSystem::OnBeforeClose(CefRefPtr<CefBrowser> browser)
{
	AutoLock lock_scope(this);

	//Free the browser pointer so that the browser can be destroyed.
	if (sWebInterfaces.count(browser->GetIdentifier()))
		sWebInterfaces[browser->GetIdentifier()]->ClearBrowser();
}

////////////////////////////////////////////////////////////
bool WebSystem::GetViewRect(CefRefPtr<CefBrowser> browser, CefRect &rect)
{
	if (!sWebInterfaces.count(browser->GetIdentifier()))
	{
		rect = CefRect();
		return true;
	}
	WebInterface* pWeb = sWebInterfaces[browser->GetIdentifier()];
	if (!pWeb)
	{
		rect = CefRect();
		return true;
	}

	rect = CefRect(0, 0, pWeb->GetWidth(), pWeb->GetHeight());
	return true;
}

////////////////////////////////////////////////////////////
void WebSystem::OnPaint(CefRefPtr<CefBrowser> browser, PaintElementType type, const RectList& dirtyRects, const void* buffer, int width, int height)
{
	////Get the web interface we will be working with. 
	if (!sWebInterfaces.count(browser->GetIdentifier())) return;
	WebInterface* pWeb = sWebInterfaces[browser->GetIdentifier()];
	if (!pWeb)
		return;

	if (!buffer)
		return;

	if (type == PET_VIEW)
	{
		int old_width = pWeb->mTextureWidth;
		int old_height = pWeb->mTextureHeight;

		//Retrieve current size of browser view.
		pWeb->mTextureWidth = width;
		pWeb->mTextureHeight = height;

		//Check if we need to resize the texture before drawing to it.
		if (old_width != pWeb->mTextureWidth || old_height != pWeb->mTextureHeight)
		{
			//This literally has never been called since the creation of this project, thus 
			//it was never updated, and the outdated code has now been removed.
			//printf("Called resize code in onpaint.\n");
		}
		else
		{
			//We want to work on the buffer byte by byte so get a pointer with a new type.
			char* bitmap = (char*)(buffer);

			sf::Lock lock(pWeb->mMutex);
			//Update the dirty rectangles.
			CefRenderHandler::RectList::const_iterator i = dirtyRects.begin();
			for (; i != dirtyRects.end(); ++i)
			{
				const CefRect& rect = *i;
				//Create a rect sized buffer for the new rectangle data.
				char* rectBuffer = new char[rect.width * (rect.height + 1) * BYTES_PER_PIXEL];

				for (int jj = 0; jj < rect.height; jj++)
				{
					//Copy the new rectangle data out of the full size buffer into our rect sized one.  
					memcpy(
						rectBuffer + jj * rect.width * BYTES_PER_PIXEL,
						bitmap + ((rect.x + ((rect.y + jj) * pWeb->mTextureWidth)) * BYTES_PER_PIXEL),
						rect.width * BYTES_PER_PIXEL
						);
				}

				//Convert BGRA to RGBA
				unsigned int* pTmpBuf = (unsigned int*)rectBuffer;
				const int numPixels = rect.width * rect.height;
				for (int i = 0; i < numPixels; i++)
				{
					pTmpBuf[i] = (pTmpBuf[i] & 0xFF00FF00) | ((pTmpBuf[i] & 0x00FF0000) >> 16) | ((pTmpBuf[i] & 0x000000FF) << 16);
				}

				if (!rectBuffer)
					continue;
				//Update the texture with the new data.  
				//This can be interrupted if the main thread calls a draw on a sprite which uses this texture
				// as the texture is bound by openGL calls.  
				//To rectify this we have the redundancy updating system.  
				pWeb->mpTexture->update((sf::Uint8*)rectBuffer, rect.width, rect.height, rect.x, rect.y);

				//Here we need to add the data required for the update to the queue for redundancy updates.  
				pWeb->mUpdateRects.push(WebInterface::UpdateRect());
				pWeb->mUpdateRects.back().buffer = rectBuffer;
				pWeb->mUpdateRects.back().rect = rect;
			}
		}
	}
}

//--------------------------------------------------------------------------------------------------------------------------
//Web Interface Methods
//--------------------------------------------------------------------------------------------------------------------------

////////////////////////////////////////////////////////////
WebInterface::WebInterface(int width, int height, const std::string& url, bool transparent)
:mTextureWidth(width)
, mTextureHeight(height)
, mCurrentURL(url)
, mTransparent(transparent)
, mBrowser(NULL)
, mpTexture(NULL)
{
	mpTexture = new sf::Texture();
	mpTexture->create(mTextureWidth, mTextureHeight);
	mpTexture->setSmooth(true);

}

////////////////////////////////////////////////////////////
WebInterface::~WebInterface()
{

	delete mpTexture;

	while (mUpdateRects.size() > 0)
	{
		delete[] mUpdateRects.front().buffer;
		mUpdateRects.pop();
	}

	std::map<int, WebInterface*>::iterator i;
	for (i = WebSystem::sWebInterfaces.begin(); i != WebSystem::sWebInterfaces.end(); i++)
	{
		if (i->second == this)
		{
			WebSystem::sWebInterfaces.erase(i);
			break;
		}
	}

	if (mBrowser)
	{
		mBrowser->GetHost()->CloseBrowser(true);
		mBrowser = NULL;
	}
}

////////////////////////////////////////////////////////////
void WebInterface::UpdateTexture()
{
	sf::Lock lock(mMutex);
	while (mUpdateRects.size() > 0)
	{
		int size = mUpdateRects.size();
		const CefRect& rect = mUpdateRects.front().rect;
		mpTexture->update((sf::Uint8*)mUpdateRects.front().buffer, rect.width, rect.height, rect.x, rect.y);
		delete[] mUpdateRects.front().buffer;
		mUpdateRects.pop();
	}
}

////////////////////////////////////////////////////////////
void WebInterface::SendFocusEvent(bool setFocus)
{
	mBrowser->GetHost()->SendFocusEvent(setFocus);
}

////////////////////////////////////////////////////////////
int WebInterface::GetMouseModifiers()
{
	int mod = 0;
	if (sf::Keyboard::isKeyPressed(sf::Keyboard::LControl) ||
		sf::Keyboard::isKeyPressed(sf::Keyboard::RControl))
		mod |= EVENTFLAG_CONTROL_DOWN;
	if (sf::Keyboard::isKeyPressed(sf::Keyboard::LShift) ||
		sf::Keyboard::isKeyPressed(sf::Keyboard::RShift))
		mod |= EVENTFLAG_SHIFT_DOWN;
	if (sf::Keyboard::isKeyPressed(sf::Keyboard::LAlt) ||
		sf::Keyboard::isKeyPressed(sf::Keyboard::RAlt))
		mod |= EVENTFLAG_ALT_DOWN;
	if (sf::Mouse::isButtonPressed(sf::Mouse::Left))
		mod |= EVENTFLAG_LEFT_MOUSE_BUTTON;
	if (sf::Mouse::isButtonPressed(sf::Mouse::Middle))
		mod |= EVENTFLAG_MIDDLE_MOUSE_BUTTON;
	if (sf::Mouse::isButtonPressed(sf::Mouse::Right))
		mod |= EVENTFLAG_RIGHT_MOUSE_BUTTON;

	// Low bit set from GetKeyState indicates "toggled".
	//if (::GetKeyState(VK_NUMLOCK) & 1)
	//	mod |= EVENTFLAG_NUM_LOCK_ON;
	//if (::GetKeyState(VK_CAPITAL) & 1)
	//	mod |= EVENTFLAG_CAPS_LOCK_ON;

	return mod;
}

////////////////////////////////////////////////////////////
int WebInterface::GetKeyboardModifiers()
{
	int mod = 0;
	if (sf::Keyboard::isKeyPressed(sf::Keyboard::LControl) ||
		sf::Keyboard::isKeyPressed(sf::Keyboard::RControl))
		mod |= EVENTFLAG_CONTROL_DOWN;
	if (sf::Keyboard::isKeyPressed(sf::Keyboard::LShift) ||
		sf::Keyboard::isKeyPressed(sf::Keyboard::RShift))
		mod |= EVENTFLAG_SHIFT_DOWN;
	if (sf::Keyboard::isKeyPressed(sf::Keyboard::LAlt) ||
		sf::Keyboard::isKeyPressed(sf::Keyboard::RAlt))
		mod |= EVENTFLAG_ALT_DOWN;

	return mod;
}

////////////////////////////////////////////////////////////
void WebInterface::SendMouseClickEvent(int x, int y, sf::Mouse::Button button, bool mouseUp, int clickCount)
{
	if (!mBrowser)
		return;

	CefBrowserHost::MouseButtonType type = button == sf::Mouse::Left ? MBT_LEFT : 
		button == sf::Mouse::Right ? MBT_RIGHT : MBT_MIDDLE;

	CefMouseEvent e; e.x = x; e.y = y; e.modifiers = GetMouseModifiers();
	mBrowser->GetHost()->SendMouseMoveEvent(e, false);
	mBrowser->GetHost()->SendMouseClickEvent(e, type, mouseUp, clickCount);
}

//////////////////////////////////////////////////////////// 
void WebInterface::SendMouseMoveEvent(int x, int y, bool mouseLeave)
{
	if (!mBrowser)
		return;

	CefMouseEvent e; e.x = x; e.y = y; e.modifiers = GetMouseModifiers();
	mBrowser->GetHost()->SendMouseMoveEvent(e, mouseLeave);
}

////////////////////////////////////////////////////////////
void WebInterface::SendMouseWheelEvent(int x, int y, int deltaX, int deltaY)
{
	if (!mBrowser)
		return;

	CefMouseEvent e; e.x = x; e.y = y; e.modifiers = GetMouseModifiers();
	mBrowser->GetHost()->SendMouseWheelEvent(e, deltaX, deltaY);
}

////////////////////////////////////////////////////////////
void WebInterface::SendKeyEvent(cef_key_event_type_t type, WPARAM key, int modifiers)
{
	if (!mBrowser)
		return;
	CefKeyEvent e; e.type = type; e.windows_key_code = key; e.modifiers = modifiers == -1 ? GetKeyboardModifiers() : modifiers;
	mBrowser->GetHost()->SendKeyEvent(e);
}

////////////////////////////////////////////////////////////
void WebInterface::AddJSBinding(const std::string name, JsBinding::JsCallback callback)
{
	WebSystem::sBindings.insert(std::make_pair(std::make_pair(name, mBrowser->GetIdentifier()), callback));

	CefRefPtr<CefProcessMessage> message = CefProcessMessage::Create("jsbinding_create");
	message->GetArgumentList()->SetString(0, name.c_str());
	message->GetArgumentList()->SetInt(1, mBrowser->GetIdentifier());

	mBrowser->SendProcessMessage(PID_RENDERER, message);

	//We have to reload the page otherwise the bindings wont be added to the context.
	mBrowser->Reload();
}

////////////////////////////////////////////////////////////
void WebInterface::AddJSBindings(const std::vector<JsBinding> bindings)
{
	for (unsigned int i = 0; i < bindings.size(); i++)
	{
		WebSystem::sBindings.insert(std::make_pair(std::make_pair(bindings[i].mFunctionName, mBrowser->GetIdentifier()), bindings[i].mfpJSCallback));

		CefRefPtr<CefProcessMessage> message = CefProcessMessage::Create("jsbinding_create");
		message->GetArgumentList()->SetString(0, bindings[i].mFunctionName);
		message->GetArgumentList()->SetInt(1, mBrowser->GetIdentifier());

		mBrowser->SendProcessMessage(PID_RENDERER, message);
	}

	//We have to reload the page otherwise the bindings wont be added to the context.
	mBrowser->Reload();
}

////////////////////////////////////////////////////////////
void WebInterface::ExecuteJS(const CefString& code)
{
	if (!mBrowser)
		return;

	ExecuteJS(code, mBrowser->GetMainFrame());
}

////////////////////////////////////////////////////////////
void WebInterface::ExecuteJS(const CefString& code, CefRefPtr<CefFrame> frame)
{
	if (!mBrowser)
		return;

	//Should probably check to make sure the frame is from our browser here.

	ExecuteJS(code, frame, 0);
}

////////////////////////////////////////////////////////////
void WebInterface::ExecuteJS(const CefString& code, CefRefPtr<CefFrame> frame, int startLine)
{
	if (!mBrowser)
		return;

	//Should probably check to make sure the frame is from our browser here.

	frame->ExecuteJavaScript(code, frame->GetURL(), startLine);
}

////////////////////////////////////////////////////////////
bool WebInterface::JSCallback(const CefString& name,
	CefRefPtr<CefListValue> arguments
	)
{
	bool result = false;
	//Check if this is one of our bindings.

	if (WebSystem::sBindings.count(std::make_pair(name, mBrowser->GetIdentifier())))
	{
		result = WebSystem::sBindings[std::make_pair(name, mBrowser->GetIdentifier())](arguments);
	}

	//Otherwise fallthrough and return false.
	return result;
}

////////////////////////////////////////////////////////////
void WebInterface::SetSize(int width, int height)
{
	mTextureWidth = width;
	mTextureHeight = height;

	if (mBrowser)
		mBrowser->GetHost()->WasResized();

	sf::Texture* pOldTexture = mpTexture;
	mpTexture = new sf::Texture();
	mpTexture->create(mTextureWidth, mTextureHeight);
	mpTexture->setSmooth(true);
	if (pOldTexture)
		delete pOldTexture;

	sf::Lock lock(mMutex);

	//Clear the update rects
	while (mUpdateRects.size() > 0)
	{
		delete[] mUpdateRects.front().buffer;
		mUpdateRects.pop();
	}

	if (mBrowser)
		mBrowser->GetHost()->Invalidate(CefRect(0, 0, mTextureWidth, mTextureHeight), PET_VIEW);
}

////////////////////////////////////////////////////////////
bool WebV8Handler::Execute(const CefString& name,
	CefRefPtr<CefV8Value> object,
	const CefV8ValueList& arguments,
	CefRefPtr<CefV8Value>& retval,
	CefString& exception)
{
	//Send message to browser process to call function
	CefRefPtr<CefBrowser> browser = CefV8Context::GetCurrentContext()->GetBrowser();
	CefRefPtr<CefProcessMessage> message = CefProcessMessage::Create("jsbinding_call");
	message->GetArgumentList()->SetString(0, name);
	message->GetArgumentList()->SetInt(1, arguments.size());
	for (unsigned int i = 0; i < arguments.size(); i++)
	{
		SetListValue(message->GetArgumentList(), i+2, arguments[i]);
	}
	
	browser->SendProcessMessage(PID_BROWSER, message);

	return false;
}
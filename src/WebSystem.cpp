//About: Function definitions for class declared in WebInterface.h
//Author: Eric Kasper

#include "WebSystem.h"

WebSystem* WebSystem::sInstance = NULL;
sf::Thread* WebSystem::spThread = NULL;
bool WebSystem::sEndThread = false;
std::queue<WebSystem::RegScheme> WebSystem::mRegSchemeQueue;
std::queue<WebInterface*> WebSystem::mMakeWebInterfaceQueue;
std::map<int, WebInterface*> WebSystem::mWebInterfaces;

//This method is what runs in our thread.  
void WebSystem::WebThread()
{
	//Standard fare CEF initialization.  
	CefSettings settings;
	settings.multi_threaded_message_loop = false;

	CefRefPtr<CefApp> app = CefRefPtr<WebApp>();

	CefInitialize(settings, app);

	//Keep updating until we are signaled to close.  
	while(!sEndThread)
	{
		CefDoMessageLoopWork();

		while(mRegSchemeQueue.size() > 0)
		{
			CefRegisterSchemeHandlerFactory(mRegSchemeQueue.front().mName, mRegSchemeQueue.front().mDomain, mRegSchemeQueue.front().mFactory);
			mRegSchemeQueue.pop();
		}

		while(mMakeWebInterfaceQueue.size() > 0)
		{
			GetInstance()->AddBrowserToInterface(mMakeWebInterfaceQueue.front());
			mMakeWebInterfaceQueue.pop();
		}
	}

	//WE SHOULD PROBABLY CHECK IF THERE ARE STILL LIVE BROWSERS HERE

	CefShutdown();
}

//Starts the thread that CEF runs in.  
void WebSystem::StartWeb()
{
	if(!spThread)
	{
		spThread = new sf::Thread(&WebThread);
	}
	sEndThread = false;
	spThread->launch();
}

//Ends CEF activity.  
//All web interfaces must call WebInterface->Release() before this is called.  
void WebSystem::EndWeb()
{
	sEndThread = true;
}

//Waits until the thread which runs CEF has ended.  
void WebSystem::WaitForWebEnd()
{
	if(spThread)
	{
		spThread->wait();
	}
}

//Adds a custom scheme to be registered for use.
void WebSystem::RegisterScheme(std::string name, std::string domain, CefRefPtr<CefSchemeHandlerFactory> factory)
{
	mRegSchemeQueue.push(RegScheme(name, domain, factory));
}

void WebSystem::UpdatTextures()
{
	std::map<int, WebInterface*>::iterator i;
	for(i = mWebInterfaces.begin(); i != mWebInterfaces.end(); i++)
	{
		i->second->UpdateTexture();
	}
}

//This is for accessing the non-static functions of our singleton. 
//These are mostly override funcitons called by CEF.
//There is no need to call this as all API functions are static.  
WebSystem* WebSystem::GetInstance()
{
	if(!sInstance)
	{
		sInstance = new WebSystem();
	}

	return sInstance;
}

//Creates a new web interface and waits until it is ready for use,
// then returns a pointer to said interface.  
WebInterface* WebSystem::CreateWebInterfaceSync(int width, int height, const std::string& url, bool transparent)
{
	WebInterface* pWeb = new WebInterface(width, height, url, transparent);

	mMakeWebInterfaceQueue.push(pWeb);

	while(!pWeb->mBrowser)
	{
		Sleep(1);
		//printf("Waiting...\n");
	}

	return pWeb;
}

//Called from within the web thread, this creates and adds a browser to a web interface.  
void WebSystem::AddBrowserToInterface(WebInterface* pWeb)
{
	AutoLock lock_scope(this);
	CefWindowInfo info;
	info.SetTransparentPainting(pWeb->IsTransparent());
	info.SetAsOffScreen(NULL); //We're drawing offscreen so just pass 0 as the window handler.  
	CefBrowserSettings browserSettings;

	CefRefPtr<CefBrowser> browser = CefBrowser::CreateBrowserSync(info, static_cast<CefRefPtr<CefClient>>(this), pWeb->GetCurrentURL(), browserSettings);
	browser->SetSize(PET_VIEW, pWeb->mTextureWidth, pWeb->mTextureHeight);

	pWeb->mBrowser = browser;

	mWebInterfaces[browser->GetIdentifier()] = pWeb;
}

//--------------------------------------------------------------------------------------------------------------------------
//Methods called by CEF
//--------------------------------------------------------------------------------------------------------------------------

void WebSystem::OnLoadStart(CefRefPtr<CefBrowser> browser, CefRefPtr<CefFrame> frame)
{
	//WebInterface* pWeb = mWebInterfaces[browser->GetIdentifier()];
}

void WebSystem::OnLoadEnd(CefRefPtr<CefBrowser> browser, CefRefPtr<CefFrame> frame, int httpStatusCode)
{
	//printf("Finished load.\n");
}

void WebSystem::OnAfterCreated(CefRefPtr<CefBrowser> browser)
{
	AutoLock lock_scope(this);

	//printf("Browser created.\n");
}

void WebSystem::OnBeforeClose(CefRefPtr<CefBrowser> browser)
{
	AutoLock lock_scope(this);

	//Free the browser pointer so that the browser can be destroyed.
	mWebInterfaces[browser->GetIdentifier()]->ClearBrowser();
}

void WebSystem::OnContextCreated(CefRefPtr<CefBrowser> browser, CefRefPtr<CefFrame> frame, CefRefPtr<CefV8Context> context)
{
	//printf("JS Context Created.\n");
	//Retrieve the context's window object.
	CefRefPtr<CefV8Value> object = context->GetGlobal();

	//Create a new callback handler.
	CefRefPtr<CefV8Handler> handler = new WebV8Handler(mWebInterfaces[browser->GetIdentifier()]);

	//Setup bindings for that web interface.
	WebInterface* pWeb = mWebInterfaces[browser->GetIdentifier()];
	for(unsigned int i = 0; i < pWeb->mJSBindings.size(); i++)
	{
		//Create the function.
		CefRefPtr<CefV8Value> func = CefV8Value::CreateFunction(pWeb->mJSBindings[i].mFunctionName, handler);

		object->SetValue(pWeb->mJSBindings[i].mFunctionName, func, V8_PROPERTY_ATTRIBUTE_NONE);
	}
}	

void WebSystem::OnContextReleased( CefRefPtr< CefBrowser > browser, CefRefPtr< CefFrame > frame, CefRefPtr< CefV8Context > context )
{

}

void WebSystem::OnUncaughtException( CefRefPtr< CefBrowser > browser, CefRefPtr< CefFrame > frame, CefRefPtr< CefV8Context > context, CefRefPtr< CefV8Exception > exception, CefRefPtr< CefV8StackTrace > stackTrace )
{

}
	
void WebSystem::OnPaint(CefRefPtr<CefBrowser> browser, PaintElementType type, const RectList& dirtyRects, const void* buffer)
{
	//Get the web interface we will be working with. 
	WebInterface* pWeb = mWebInterfaces[browser->GetIdentifier()];

	if(type == PET_VIEW)
	{
		int old_width = pWeb->mTextureWidth;
		int old_height = pWeb->mTextureHeight;

		//Retrieve current size of browser view.
		browser->GetSize(type, pWeb->mTextureWidth, pWeb->mTextureHeight);

		//Convert BGRA to RGBA
		unsigned int* pTmpBuf = (unsigned int*)buffer;
		const int numPixels = pWeb->mTextureHeight * pWeb->mTextureWidth;
		for (int i = 0; i < numPixels; i++)
		{
			pTmpBuf[i] = (pTmpBuf[i] & 0xFF00FF00) | ((pTmpBuf[i] & 0x00FF0000) >> 16) | ((pTmpBuf[i] & 0x000000FF) << 16);
		}

		//Check if we need to resize the texture before drawing to it.
		if(old_width != pWeb->mTextureWidth || old_height != pWeb->mTextureHeight)
		{
			printf("Called resize code in onpaint.\n");
			//Update/resize the whole texture.
			//sf::Texture* pOldTexture = pWeb->mpTexture;
			//pWeb->mpTexture = new sf::Texture();
			//pWeb->mpTexture->create(pWeb->mTextureWidth, pWeb->mTextureHeight);
			//pWeb->mpTexture->setSmooth(true);

			//char* pPrevBuffer = pWeb->mpBuffer;
			//pWeb->mpBuffer = new char[pWeb->mTextureWidth * (pWeb->mTextureHeight + 1) * BYTES_PER_PIXEL];
			//delete pPrevBuffer;
			//
			//unsigned char* bitmap = (unsigned char*)(buffer);
			////Update the whole rectangle
			//for(int jj = 0; jj < pWeb->mTextureHeight; jj++)
			//{
			//	memcpy(
			//		pWeb->mpBuffer + jj * pWeb->mTextureWidth * BYTES_PER_PIXEL,
			//		bitmap + (0 + (jj + 0) * pWeb->mTextureWidth) * BYTES_PER_PIXEL,
			//		pWeb->mTextureWidth * BYTES_PER_PIXEL
			//		);
			//}

			//pWeb->mpTexture->update((sf::Uint8*)pWeb->mpBuffer, pWeb->mTextureWidth, pWeb->mTextureHeight, 0, 0);
		}
		else
		{
			//We want to work on the buffer byte by byte so get a pointer with a new type.
			char* bitmap = (char*)(buffer);

			//Update the dirty rectangles.
			CefRenderHandler::RectList::const_iterator i = dirtyRects.begin();
			for(; i != dirtyRects.end(); ++i)
			{
				const CefRect& rect = *i;
				//Create a rect sized buffer for the new rectangle data.
				char* rectBuffer = new char[rect.width * rect.height * BYTES_PER_PIXEL];

				for(int jj = 0; jj < rect.height; jj++)
				{
					//Copy the new rectangle data out of the full size buffer into our rect sized one.  
					memcpy(
						rectBuffer + jj * rect.width * BYTES_PER_PIXEL,
						bitmap + ( (rect.x + ( (rect.y + jj) * pWeb->mTextureWidth) ) * BYTES_PER_PIXEL ),
						rect.width * BYTES_PER_PIXEL
						);
				}

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



WebInterface::WebInterface(int width, int height, const std::string& url, bool transparent)
	:mTextureWidth(width)
	,mTextureHeight(height)
	,mCurrentURL(url)
	,mTransparent(transparent)
	,mBrowser(NULL)
	,mpTexture(NULL)
{	
	//printf("Constructed web interface.\n");
	mpTexture = new sf::Texture();
	mpTexture->create(mTextureWidth, mTextureHeight);
	mpTexture->setSmooth(true);

}

WebInterface::~WebInterface()
{

}

//Redundantly updates dirty rectangles in order to fix errors which appear due to multithreading.
//Should be called once before every draw or errors may appear.  
void WebInterface::UpdateTexture()
{
	while(mUpdateRects.size() > 0)
	{
		const CefRect& rect = mUpdateRects.front().rect;
		mpTexture->update((sf::Uint8*)mUpdateRects.front().buffer, rect.width, rect.height, rect.x, rect.y);
		delete mUpdateRects.front().buffer;
		mUpdateRects.pop();
	}
}

//Sends the event to focus/unfocus the browser of this interface.  
void WebInterface::SendFocusEvent(bool setFocus)
{
	mBrowser->SendFocusEvent(setFocus);
}

//Sends a mouse click event.  
void WebInterface::SendMouseClickEvent(int x, int y, CefBrowser::MouseButtonType type, bool mouseUp, int clickCount)
{
	if(!mBrowser)
		return;
	
	mBrowser->SendMouseMoveEvent(x, y, false);
	mBrowser->SendMouseClickEvent(x, y, type, mouseUp, clickCount);
}

//Sends a mouse move event.  
void WebInterface::SendMouseMoveEvent(int x, int y, bool mouseLeave)
{
	if(!mBrowser)
		return;

	mBrowser->SendMouseMoveEvent(x, y, mouseLeave);
}

//Sends a mouse wheel event.  
void WebInterface::SendMouseWheelEvent(int x, int y, int deltaX, int deltaY)
{
	if(!mBrowser)
		return;

	mBrowser->SendMouseWheelEvent(x, y, deltaX, deltaY);
}

//Sends a key event.  
void WebInterface::SendKeyEvent(CefBrowser::KeyType type, const CefKeyInfo& keyInfo, int modifiers)
{
	if(!mBrowser)
		return;

	mBrowser->SendKeyEvent(type, keyInfo, modifiers);
}

//Adds a new javascript binding to this web interface.  
//Since the browser has to be updated in order for the new binding to take effect
// for multiple bindings use the AddJSBindings function.  
void WebInterface::AddJSBinding(const std::string& name, bool (*callback)(
		CefRefPtr<CefV8Value> object,
		const CefV8ValueList& arguments,
		CefRefPtr<CefV8Value>& retval,
		CefString& exception))
{
	mJSBindings.push_back(JsBinding(name, callback));

	//We have to reload the page otherwise the bindings wont be added to the context.
	mBrowser->Reload();
}

//Use this for adding multiple bindings so that the page only needs to be reloaded once.  
void WebInterface::AddJSBindings(const std::vector<JsBinding>& bindings)
{
	for(unsigned int i = 0; i < bindings.size(); i++)
	{
		mJSBindings.push_back(bindings[i]);
	}
	
	//We have to reload the page otherwise the bindings wont be added to the context.
	mBrowser->Reload();
}

//Executes javascript code in the browser's main frame.  
void WebInterface::ExecuteJS(const CefString& code)
{
	if(!mBrowser)
		return;

	ExecuteJS(code, mBrowser->GetMainFrame());
}

//Executes javascript code in specified frame.  
void WebInterface::ExecuteJS(const CefString& code, CefRefPtr<CefFrame> frame)
{
	if(!mBrowser)
		return;

	//Should probably check to make sure the frame is from our browser here.

	ExecuteJS(code, frame, 0);
}

//Executes javascript code in specified frame, from specified starting line.  
void WebInterface::ExecuteJS(const CefString& code, CefRefPtr<CefFrame> frame, int startLine)
{
	if(!mBrowser)
		return;

	//Should probably check to make sure the frame is from our browser here.

	frame->ExecuteJavaScript(code, frame->GetURL(), startLine);
}

//Handles calling the appropriate callback function for the binding.  
bool WebInterface::JSCallback(const CefString& name,
                       CefRefPtr<CefV8Value> object,
                       const CefV8ValueList& arguments,
                       CefRefPtr<CefV8Value>& retval,
                       CefString& exception)
{
	//Check if this is one of our bindings.
	for(int i = 0; i < mJSBindings.size(); i++)
	{
		if(name == mJSBindings[i].mFunctionName)
		{
			if(mJSBindings[i].mfpJSCallback != NULL)
			{
				return mJSBindings[i].mfpJSCallback(object, arguments, retval, exception);
			}
		}
	}

	//Otherwise fallthrough and return false.
	return false;
}

//Changes the size of the interface.  
void WebInterface::SetSize(int width, int height)
{
	//printf("Set size of web interface.\n");
	mTextureWidth = width;
	mTextureHeight = height;
	if(mBrowser)
		mBrowser->SetSize(PET_VIEW, mTextureWidth, mTextureHeight);

	sf::Texture* pOldTexture = mpTexture;
	mpTexture = new sf::Texture();
	mpTexture->create(mTextureWidth, mTextureHeight);
	mpTexture->setSmooth(true);
	if(pOldTexture)
		delete pOldTexture;

	mBrowser->Invalidate(CefRect(0, 0, mTextureWidth, mTextureHeight));
}

//All this does is call the callback handling function in the web interface.  
bool WebV8Handler::Execute(const CefString& name,
                       CefRefPtr<CefV8Value> object,
                       const CefV8ValueList& arguments,
                       CefRefPtr<CefV8Value>& retval,
                       CefString& exception)
{
	return mpWebInterface->JSCallback(name, object, arguments, retval, exception);
}
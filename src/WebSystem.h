//About: A class for using CEF to embed multiple web pages into an application using SFML.
//Author: Eric Kasper

#pragma once

#include <queue>

#include <include/cef_app.h>
#include <include/cef_client.h>

#include <SFML/Graphics.hpp>
#include <SFML/System.hpp>

#include "CustomScheme.h"

#ifndef BYTES_PER_PIXEL
#define BYTES_PER_PIXEL 4
#endif

class WebApp : public CefApp
{
	WebApp(){}

	//Removed because it apparently did nothing.
	//virtual void OnRegisterCustomSchemes(CefRefPtr<CefSchemeRegistrar> registrar) override
	//{
	//	//registrar->AddCustomScheme("local", true, false, false);
	//}

	IMPLEMENT_REFCOUNTING(WebApp);
};

class WebInterface;

class WebSystem : public CefClient,
	public CefLifeSpanHandler,
	public CefLoadHandler,
	public CefRequestHandler,
	public CefDisplayHandler,
	public CefRenderHandler,
	public CefV8ContextHandler
{
public:
	//API methods
	static void StartWeb();
	static void EndWeb();
	static void WaitForWebEnd();
	static void RegisterScheme(std::string name, std::string domain, CefRefPtr<CefSchemeHandlerFactory> factory);
	static WebInterface* CreateWebInterfaceSync(int width, int height, const std::string& url, bool transparent);
	static void UpdatTextures();

	//This is for accessing the non-static functions of our singleton. 
	//These are mostly override funcitons called by CEF.
	//There is no need to call this as all API functions are static.  
	static WebSystem* GetInstance();

	//Cef methods
	virtual CefRefPtr<CefLifeSpanHandler> GetLifeSpanHandler() override { return this; }
	virtual CefRefPtr<CefLoadHandler> GetLoadHandler() override { return this; }
	virtual CefRefPtr<CefRequestHandler> GetRequestHandler() override { return this; }
	virtual CefRefPtr<CefDisplayHandler> GetDisplayHandler() override { return this; }
	virtual CefRefPtr<CefRenderHandler> GetRenderHandler() override { return this; }
	virtual CefRefPtr<CefV8ContextHandler> GetV8ContextHandler() override { return this; }

	//CefLoadHandler methods
	virtual void OnLoadStart(CefRefPtr<CefBrowser> browser, CefRefPtr<CefFrame> frame) override;
	virtual void OnLoadEnd(CefRefPtr<CefBrowser> browser, CefRefPtr<CefFrame> frame, int httpStatusCode) override;

	//CefRenderHandler methods
	virtual void OnPaint(CefRefPtr<CefBrowser> browser, PaintElementType type,
		const RectList& dirtyRects, const void* buffer) override;

	//CefLifeSpanHandler methods
	virtual void OnAfterCreated(CefRefPtr<CefBrowser> browser) override;
	virtual void OnBeforeClose(CefRefPtr<CefBrowser> browser) override;

	//CefV8ContextHandler methods
	virtual void OnContextCreated(CefRefPtr<CefBrowser> browser, CefRefPtr<CefFrame> frame, CefRefPtr<CefV8Context> context) override;
	virtual void OnContextReleased( CefRefPtr< CefBrowser > browser, CefRefPtr< CefFrame > frame, CefRefPtr< CefV8Context > context ) override;
	virtual void OnUncaughtException( CefRefPtr< CefBrowser > browser, CefRefPtr< CefFrame > frame, CefRefPtr< CefV8Context > context, CefRefPtr< CefV8Exception > exception, CefRefPtr< CefV8StackTrace > stackTrace ) override;

private:
	//Structure for holding scheme data so that it can be registered later.  
	struct RegScheme
	{
	public:
		RegScheme(std::string name, std::string domain, CefRefPtr<CefSchemeHandlerFactory> factory)
			:mName(name)
			,mDomain(domain)
			,mFactory(factory)
		{}
		std::string mName;
		std::string mDomain;
		CefRefPtr<CefSchemeHandlerFactory> mFactory;
	};

	//Instance of our singleton.
	static WebSystem* sInstance;
	
	//Thread variables.  
	static void WebThread();
	static sf::Thread* spThread;
	static bool sEndThread;

	//These queues are used to store things which need to be initialized in the WebThread
	//After initialization they are removed from the queue.  
	static std::queue<RegScheme> mRegSchemeQueue;
	static std::queue<WebInterface*> mMakeWebInterfaceQueue;

	//This references the web interfaces at thier browser ID so that they can be accessed
	// when only the ID of thier browser is available.  
	static std::map<int, WebInterface*> mWebInterfaces;

	//Creates a browser and adds it to the web interface.  
	void AddBrowserToInterface(WebInterface* pWeb);

	WebSystem(){}
	~WebSystem(){}

	//Include the default reference countign implementation.
	IMPLEMENT_REFCOUNTING(WebSystem);
	// Include the default locking implementation.
	IMPLEMENT_LOCKING(WebSystem);
};


class WebInterface
{
	friend class WebSystem;

public:
	//This struct holds the data for a javascript binding.
	struct JsBinding
	{
	public:
		JsBinding(std::string name, bool (*callback)(
		CefRefPtr<CefV8Value> object,
		const CefV8ValueList& arguments,
		CefRefPtr<CefV8Value>& retval,
		CefString& exception))
		:mFunctionName(name)
		,mfpJSCallback(callback)
		{}

		std::string mFunctionName;
		bool (*mfpJSCallback) (
			CefRefPtr<CefV8Value> object,
			const CefV8ValueList& arguments,
			CefRefPtr<CefV8Value>& retval,
			CefString& exception
							  );
	};

	//Send events
	void SendFocusEvent(bool setFocus);
	void SendMouseClickEvent(int x, int y, CefBrowser::MouseButtonType type, bool mouseUp, int clickCount);
	void SendMouseMoveEvent(int x, int y, bool mouseLeave);
	void SendMouseWheelEvent(int x, int y, int deltaX, int deltaY);
	void SendKeyEvent(CefBrowser::KeyType type, const CefKeyInfo& keyInfo, int modifiers);

	//Javascript
	void AddJSBinding(const std::string& name, bool (*callback)(
		CefRefPtr<CefV8Value> object,
		const CefV8ValueList& arguments,
		CefRefPtr<CefV8Value>& retval,
		CefString& exception));
	void AddJSBindings(const std::vector<JsBinding>& bindings);

	void ExecuteJS(const CefString& code);
	void ExecuteJS(const CefString& code, CefRefPtr<CefFrame> frame);
	void ExecuteJS(const CefString& code, CefRefPtr<CefFrame> frame, int startLine);

	bool JSCallback(const CefString& name,
                       CefRefPtr<CefV8Value> object,
                       const CefV8ValueList& arguments,
                       CefRefPtr<CefV8Value>& retval,
                       CefString& exception);

	//Our stuff
	WebInterface(int width, int height, const std::string& url, bool transparent);
	virtual ~WebInterface();

	CefRefPtr<CefBrowser> GetBrowser() { return mBrowser; }

	int GetWidth() { return mTextureWidth; }
	int GetHeight() { return mTextureHeight; }

	void UpdateTexture();
	sf::Texture* GetTexture() { return mpTexture; }

	std::string GetCurrentURL() { return mCurrentURL; }

	bool IsTransparent() { return mTransparent; }

	void SetSize(int width, int height);

private:
	//Holds data for redundantly updating rectangles on the texture.  
	struct UpdateRect
	{
	public:
		char* buffer;
		CefRect rect;
	};

	//Reference to our browser.  
	CefRefPtr<CefBrowser> mBrowser;
	void ClearBrowser() { mBrowser = NULL; }

	//List of all our javascript bindings.  
	std::vector<JsBinding> mJSBindings;

	//Our texture.  
	sf::Texture* mpTexture;

	//Width and height of our texture.  
	int mTextureWidth;
	int mTextureHeight;

	//Queue of rectangles to be retroatively updated.  
	std::queue<UpdateRect> mUpdateRects;

	//The URL of our current webpage.  
	std::string mCurrentURL;

	//Whether or not our background is transparent.  
	//This cannot be changed after creation and is merely stored for reference.  
	bool mTransparent;

	IMPLEMENT_REFCOUNTING(WebInterface);
};

//This pretty much has to be it's own class, as CEF will delete its instances
class WebV8Handler : public CefV8Handler
{
	friend class WebSystem;

public:
	WebV8Handler(WebInterface* inface) :mpWebInterface(inface) {}

	virtual bool Execute(const CefString& name,
                    CefRefPtr<CefV8Value> object,
                    const CefV8ValueList& arguments,
                    CefRefPtr<CefV8Value>& retval,
                    CefString& exception) override;

private:
	WebInterface* mpWebInterface;

	IMPLEMENT_REFCOUNTING(WebV8Handler);
};
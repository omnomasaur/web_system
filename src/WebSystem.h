
#pragma once
////////////////////////////////////////////////////////////
// Headers
////////////////////////////////////////////////////////////
#include <include/cef_app.h>
#include <include/cef_client.h>
#include <include/cef_render_handler.h>
#include <SFML\Graphics.hpp>
#include <queue>

////////////////////////////////////////////////////////////
// Pre-processor Definitions
////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////
// Default the platform to windows if it has not been set.
//
////////////////////////////////////////////////////////////
#ifndef PLATFORM
#define PLATFORM WINDOWS
#endif

////////////////////////////////////////////////////////////
// Currently only 32bit color is supported.
//
////////////////////////////////////////////////////////////
#ifndef BYTES_PER_PIXEL
#define BYTES_PER_PIXEL 4
#endif

////////////////////////////////////////////////////////////
// Web System Definitions
////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////
/// \brief Data for a javascript binding.
///
////////////////////////////////////////////////////////////
struct JsBinding
{
public:
	////////////////////////////////////////////////////////////
	/// \brief Function pointer to deal with a bound callback.
	///
	////////////////////////////////////////////////////////////
	typedef bool(*JsCallback) (
		CefRefPtr<CefListValue> arguments
		);

	////////////////////////////////////////////////////////////
	/// \brief Creates a container for javascript binding data.
	///
	/// \param name			Call sign of the function in javascript.
	/// \param callback		Pointer to the function in c++.
	///
	////////////////////////////////////////////////////////////
	JsBinding(std::string name, JsCallback callback)
		:mFunctionName(name)
		, mfpJSCallback(callback)
	{}

	////////////////////////////////////////////////////////////
	/// \brief Call sign of the function in javascript.
	///
	////////////////////////////////////////////////////////////
	std::string mFunctionName;

	////////////////////////////////////////////////////////////
	/// \brief Pointer to the function in c++.
	///
	////////////////////////////////////////////////////////////
	JsCallback mfpJSCallback;
};

//Forward declarations.
class WebApp;
class WebInterface;

class WebSystem : public CefBrowserProcessHandler,
	public CefRenderProcessHandler,
	public CefClient,
	public CefLifeSpanHandler,
	public CefLoadHandler,
	public CefRequestHandler,
	public CefDisplayHandler,
	public CefRenderHandler
{
	friend class WebInterface;
	friend class WebV8Handler;
public:
	////////////////////////////////////////////////////////////
	/// API Methods
	////////////////////////////////////////////////////////////

	////////////////////////////////////////////////////////////
	/// \brief Entry point handler for cef subprocesses
	///
	/// Must be run in the entry point function of a process which can be used as a subprocess for cef.  
	/// If the return value is anything other than -1, it means that this process was a subprocess for cef.
	/// It also means that this subprocess has closed, and is the exit_code to be returned by the entry point.
	///
	/// \return -1 if this is the main process, exit_code if this is a child process
	///
	////////////////////////////////////////////////////////////
	static int Main();

	////////////////////////////////////////////////////////////
	/// \brief Sets the executable name to be run as cef subprocesses
	///
	/// \param path		Path to the executable
	///
	////////////////////////////////////////////////////////////
	static void SetSubprocess(const std::string path){ sSubprocess = path; }

	////////////////////////////////////////////////////////////
	/// \brief Sets whether to run in single process mode.
	///
	/// \param single	True for single process
	///
	////////////////////////////////////////////////////////////
	static void SetSingleProcess(bool single) { sSingleProcess = single; }

	////////////////////////////////////////////////////////////
	/// \brief Starts the thread that runs cef.
	///
	/// The thread started by this function is part of WebSystem and not cef's own multi threading.
	/// This is because cef1's multi threading does not support creating and removing browser once
	/// multi threading has begun.  
	/// WebSystem now uses cef3, and thus could be re-written to use cef's built in multi threading.
	/// To do so would require a complete restructuring of the system and thus has not been done.
	///
	////////////////////////////////////////////////////////////
	static void StartWeb();

	////////////////////////////////////////////////////////////
	/// \brief Signals WebSystem to end
	///
	/// This should not be called until the program is ready to close.
	/// WebSystem has not been designed to close and restart multiple times.  Such behavior is undefined.
	///
	////////////////////////////////////////////////////////////
	static void EndWeb();

	////////////////////////////////////////////////////////////
	/// \brief Blocks until WebSystem has closed.
	///
	/// Should only be called after EndWeb()
	///
	////////////////////////////////////////////////////////////
	static void WaitForWebEnd();

	////////////////////////////////////////////////////////////
	/// \brief Queues a custom scheme to be registered for use in WebSystem
	///
	/// \param name		Name of the custom scheme e.g. "http" for http://
	/// \param domain	The domain that this scheme is allowed on
	/// \param factor	Factory to create scheme handlers
	///
	////////////////////////////////////////////////////////////
	static void RegisterScheme(std::string name, std::string domain, CefRefPtr<CefSchemeHandlerFactory> factory);

	////////////////////////////////////////////////////////////
	/// \brief Creates a web interface, and blocks until it is ready.
	///
	/// \param width		Width of the interface texture
	/// \param height		Height of the interface texture
	/// \param url			Url to load 
	/// \param transparent	True to use a transparent background
	///
	////////////////////////////////////////////////////////////
	static WebInterface* CreateWebInterfaceSync(int width, int height, const std::string& url, bool transparent);

	////////////////////////////////////////////////////////////
	/// \brief Redundantly updates the textures of all WebInterfaces
	///
	/// Due to the multi threaded nature of WebSystem, it is a common occurence that a call to OnPaint
	/// is interrupted by a call draw a texture.  As this causes a call to gl_bind it will break the
	/// painting of the texture.  
	/// This function is used to redundantly update textures on the draw thread, where they cannot
	/// be interrupted by another thread calling gl_bind on them.
	///
	/// \return Pointer to the created WebInterface
	///
	////////////////////////////////////////////////////////////
	static void UpdateInterfaceTextures();

	////////////////////////////////////////////////////////////
	/// \breif This is for accessing the non-static functions of our singleton.
	///
	/// These are mostly override functions called by CEF.
	/// There is no need to call this as all API functions are static.
	///
	/// \return CefRefPtr referencing the singleton.
	///
	////////////////////////////////////////////////////////////
	static CefRefPtr<WebSystem> GetInstance();

	////////////////////////////////////////////////////////////
	/// Cef Handler retrieval methods.  Called by Cef.
	///
	////////////////////////////////////////////////////////////
	virtual CefRefPtr<CefLifeSpanHandler> GetLifeSpanHandler() override { return this; }
	virtual CefRefPtr<CefLoadHandler> GetLoadHandler() override { return this; }
	virtual CefRefPtr<CefRequestHandler> GetRequestHandler() override { return this; }
	virtual CefRefPtr<CefDisplayHandler> GetDisplayHandler() override { return this; }
	virtual CefRefPtr<CefRenderHandler> GetRenderHandler() override { return this; }

	////////////////////////////////////////////////////////////
	/// CefLoadHandler methods.
	///
	////////////////////////////////////////////////////////////
	virtual void OnLoadStart(CefRefPtr<CefBrowser> browser, CefRefPtr<CefFrame> frame) override;
	virtual void OnLoadEnd(CefRefPtr<CefBrowser> browser, CefRefPtr<CefFrame> frame, int httpStatusCode) override;

	////////////////////////////////////////////////////////////
	/// CefRenderHandler methods.
	///
	////////////////////////////////////////////////////////////
	virtual bool GetViewRect(CefRefPtr<CefBrowser> browser, CefRect &rect) override;
	virtual void OnPaint(CefRefPtr<CefBrowser> browser, PaintElementType type,
		const RectList& dirtyRects, const void* buffer, int width, int height) override;

	////////////////////////////////////////////////////////////
	/// CefLifespanHandler methods.
	///
	////////////////////////////////////////////////////////////
	virtual void OnAfterCreated(CefRefPtr<CefBrowser> browser) override;
	virtual void OnBeforeClose(CefRefPtr<CefBrowser> browser) override;

	////////////////////////////////////////////////////////////
	/// CefBrowserProcessHandler methods.
	///
	////////////////////////////////////////////////////////////
	virtual void OnRenderProcessThreadCreated(CefRefPtr<CefListValue> extra_info) override;

	////////////////////////////////////////////////////////////
	/// CefRenderProcessHandler methods.
	///
	////////////////////////////////////////////////////////////
	virtual void OnBrowserCreated(CefRefPtr<CefBrowser> browser) override;
	virtual void OnBrowserDestroyed(CefRefPtr<CefBrowser> browser) override;
	virtual void OnContextCreated(CefRefPtr<CefBrowser> browser, CefRefPtr<CefFrame> frame, CefRefPtr<CefV8Context> context) override;
	virtual void OnContextReleased(CefRefPtr< CefBrowser > browser, CefRefPtr< CefFrame > frame, CefRefPtr< CefV8Context > context) override;
	virtual void OnUncaughtException(CefRefPtr< CefBrowser > browser, CefRefPtr< CefFrame > frame, CefRefPtr< CefV8Context > context, CefRefPtr< CefV8Exception > exception, CefRefPtr< CefV8StackTrace > stackTrace) override;

	////////////////////////////////////////////////////////////
	/// CefClient / CefRenderProcessHandler shared method.
	///
	////////////////////////////////////////////////////////////
	virtual bool OnProcessMessageReceived(CefRefPtr<CefBrowser> browser, CefProcessId source_process, CefRefPtr<CefProcessMessage> message) override;

private:

	////////////////////////////////////////////////////////////
	/// \brief Container for custom scheme information
	///
	////////////////////////////////////////////////////////////
	struct RegScheme
	{
	public:
		RegScheme(std::string name, std::string domain, CefRefPtr<CefSchemeHandlerFactory> factory)
			:mName(name)
			, mDomain(domain)
			, mFactory(factory)
		{}
		std::string mName;
		std::string mDomain;
		CefRefPtr<CefSchemeHandlerFactory> mFactory;
	};


	////////////////////////////////////////////////////////////
	/// Arguments
	////////////////////////////////////////////////////////////

	////////////////////////////////////////////////////////////
	/// \brief Path to the process to be used for cef subprocesses
	///
	////////////////////////////////////////////////////////////
	static std::string sSubprocess;

	////////////////////////////////////////////////////////////
	/// \brief Boolean to turn on/off single process mode
	///
	////////////////////////////////////////////////////////////
	static bool sSingleProcess;

	////////////////////////////////////////////////////////////
	/// \brief Main args for cef
	///
	////////////////////////////////////////////////////////////
	static CefMainArgs sArgs;

	////////////////////////////////////////////////////////////
	/// \brief Our custom CefApp implementation
	///
	////////////////////////////////////////////////////////////
	static CefRefPtr<WebApp> sApp;

	////////////////////////////////////////////////////////////
	/// \brief Instance of our singleton
	///
	////////////////////////////////////////////////////////////
	static CefRefPtr<WebSystem> sInstance;

	////////////////////////////////////////////////////////////
	/// Thread 
	////////////////////////////////////////////////////////////

	////////////////////////////////////////////////////////////
	/// \brief The actual function that our thread runs
	///
	////////////////////////////////////////////////////////////
	static void WebThread();

	////////////////////////////////////////////////////////////
	/// \brief A thread instance to run WebThread() on
	///
	////////////////////////////////////////////////////////////
	static sf::Thread* spThread;

	////////////////////////////////////////////////////////////
	/// \brief Boolean to signal that the thread should end
	///
	////////////////////////////////////////////////////////////
	static bool sEndThread;

	////////////////////////////////////////////////////////////
	/// \brief Queue which stores custom schemes to be initialized within the WebThread
	///
	/// After initialization they are removed from the queue.
	///
	////////////////////////////////////////////////////////////
	static std::queue<RegScheme> sRegSchemeQueue;

	////////////////////////////////////////////////////////////
	/// \brief Queue which stores WebInterfaces which are waiting to have their browser initialized
	///
	/// After initialization they are removed from the queue.
	///
	////////////////////////////////////////////////////////////
	static std::queue<WebInterface*> sMakeWebInterfaceQueue;

	////////////////////////////////////////////////////////////
	/// \brief Keeps track of existing WebInterfaces by the ID of their cef browser.
	///
	////////////////////////////////////////////////////////////
	static std::map<int, WebInterface*> sWebInterfaces;

	////////////////////////////////////////////////////////////
	/// \brief Type for the map containing all javascript bindings.  
	///
	////////////////////////////////////////////////////////////
	typedef std::map<std::pair<std::string, int>,
		 JsBinding::JsCallback >
		 BindingMap;

	////////////////////////////////////////////////////////////
	/// \brief Map containing all javascript bindings.  
	///
	////////////////////////////////////////////////////////////
	static BindingMap sBindings;

	////////////////////////////////////////////////////////////
	/// \brief Creates a browser and adds it to a WebInterface
	///
	////////////////////////////////////////////////////////////
	void AddBrowserToInterface(WebInterface* pWeb);

	WebSystem(){}
	~WebSystem(){}

public:
	////////////////////////////////////////////////////////////
	/// \brief Implement cef reference counting.
	///
	////////////////////////////////////////////////////////////
	IMPLEMENT_REFCOUNTING(WebSystem);

	////////////////////////////////////////////////////////////
	/// \brief Implement cef locking mechanism.
	///
	////////////////////////////////////////////////////////////
	IMPLEMENT_LOCKING(WebSystem);
};


////////////////////////////////////////////////////////////
/// \brief Custom CefApp which is merely used to allow for custom process handlers.
///
////////////////////////////////////////////////////////////
class WebApp : public CefApp
{
public:
	WebApp(){}

	////////////////////////////////////////////////////////////
	/// Cef process handler retrieval methods.  Called by Cef.
	///
	////////////////////////////////////////////////////////////
	virtual CefRefPtr<CefBrowserProcessHandler> GetBrowserProcessHandler() override { return WebSystem::GetInstance().get(); }
	virtual CefRefPtr<CefRenderProcessHandler> GetRenderProcessHandler() override { return WebSystem::GetInstance().get(); }

	////////////////////////////////////////////////////////////
	/// \brief Implement cef reference counting.
	///
	////////////////////////////////////////////////////////////
	IMPLEMENT_REFCOUNTING(WebApp);
};

////////////////////////////////////////////////////////////
/// \breif Interface for the program to interact with WebSystem.  
///
/// Should be held in CefRefPtr container rather than as a raw pointer.
/// Each instance functions has it's own browser and functions independently.
/// Do not delete when finished.  Instead call Close() and then release the
/// CefRefPtr used to reference this object.
///
////////////////////////////////////////////////////////////
class WebInterface : public CefBase
{
	friend class WebSystem;
public:

	////////////////////////////////////////////////////////////
	/// \brief Send a focus event to this WebInterface.
	///
	/// \param setFocus True to gain focus, false to lose focus.
	///
	////////////////////////////////////////////////////////////
	void SendFocusEvent(bool setFocus);

	////////////////////////////////////////////////////////////
	/// \brief Send a mouse click event to this WebInterface
	///
	/// \param x			Horizontal position of the mouse in local coordinates.
	/// \param y			Vertical position of the mosue in local coordinates.
	/// \param button		The mouse button for which we are sending an event.  Left | Middle | Right
	/// \param mouseUp		Whether the event is for a depression or release of the button.
	/// \param clickCount	The number of clicks.  Used for double/triple etc. clicks.
	///
	////////////////////////////////////////////////////////////
	void SendMouseClickEvent(int x, int y, sf::Mouse::Button button, bool mouseUp, int clickCount);

	////////////////////////////////////////////////////////////
	/// \brief Send a mouse move event to this WebInterface
	///
	/// \param x			Horizontal position of the mouse in local coordinates.
	/// \param y			Vertical position of the mouse in local coordinates.
	/// \param mouseLeave	Whether the mouse leaves our boundaries.  When in doubt set false.
	///
	////////////////////////////////////////////////////////////
	void SendMouseMoveEvent(int x, int y, bool mouseLeave = false);

	////////////////////////////////////////////////////////////
	/// \brief Send a mouse wheel event to this WebInterface
	///
	/// \param x		Horizontal position of the mouse in local coordinates.
	/// \param y		Vertical position of the mouse in local coordinates.
	/// \param deltaX	Change in the horizontal mouse wheel.
	/// \param deltaY	Change in the vertical mouse wheel.
	///
	////////////////////////////////////////////////////////////
	void SendMouseWheelEvent(int x, int y, int deltaX, int deltaY);

	////////////////////////////////////////////////////////////
	/// \brief Send a key event to this WebInterface
	/// 
	/// \param type			Differentiates between keyup and keydown events.
	/// \param key			The key being pressed/released.
	/// \param modifiers	Holds the state of SHIFT/CONTROL/ALT etc. for this key press.  If unspecified will default for Shift, Control, and Alt keys only.
	///
	////////////////////////////////////////////////////////////
	void SendKeyEvent(cef_key_event_type_t type, WPARAM key, int modifiers = -1);


	////////////////////////////////////////////////////////////
	/// \brief Adds a javascript binding to this WebInterface
	///
	/// \param name			Call sign of this function for javascript.
	/// \param callback		Pointer to the bound function.
	///
	////////////////////////////////////////////////////////////
	void AddJSBinding(const std::string name, JsBinding::JsCallback callback);

	////////////////////////////////////////////////////////////
	/// \brief Adds a list of javascript bindings to this WebInterface
	/// 
	/// \param bindings List of bindings to be added.
	///
	/// Calling this is more efficient than multiple calls of AddJsBinding() as it only reloads
	/// the browser once rather than once per binding.  
	///
	////////////////////////////////////////////////////////////
	void AddJSBindings(const std::vector<JsBinding> bindings);


	////////////////////////////////////////////////////////////
	/// \brief Executes javascript.  
	///
	/// \param code		Code to be executed.
	///
	////////////////////////////////////////////////////////////
	void ExecuteJS(const CefString& code);

	////////////////////////////////////////////////////////////
	/// \brief Exectues javascript in specified frame.  
	///
	/// \param code		Code to be executed.
	/// \param frame	Frame to be used for execution.
	///
	////////////////////////////////////////////////////////////
	void ExecuteJS(const CefString& code, CefRefPtr<CefFrame> frame);

	////////////////////////////////////////////////////////////
	/// \brief Executes javascript in specified frame, with specified start line.
	///
	/// \param code			Code to be executed.
	/// \param frame		Frame to be used for execution.
	/// \param startLine	Line to be used as starting position.
	///
	////////////////////////////////////////////////////////////
	void ExecuteJS(const CefString& code, CefRefPtr<CefFrame> frame, int startLine);


	////////////////////////////////////////////////////////////
	/// \brief Called internally to handle javascript callbacks.  
	///
	/// Can also be called externally to fake javascript callbacks for testing etc..
	///
	/// \return Success of the callback.
	///
	////////////////////////////////////////////////////////////
	bool JSCallback(const CefString& name,
		CefRefPtr<CefListValue> arguments
		);


	////////////////////////////////////////////////////////////
	/// \brief Construct a web interface.
	///
	/// \param width			Horizontal size of the WebInterface.
	/// \param height			Vertical size of the WebInterface.
	/// \param url				Url to load upon construction.
	/// \param transparent		Background transparency.
	///
	////////////////////////////////////////////////////////////
	WebInterface(int width, int height, const std::string& url, bool transparent);
	virtual ~WebInterface();

	////////////////////////////////////////////////////////////
	/// \brief Closes this WebInterface so that it can be released.
	///
	/// \param force	True will force the browser to close.  False will allow the user to cancel the close.
	///
	/// If the close is not forced, the WebInterface is not guaranteed to be
	/// ready for release, which can cause a memory leak if not tracked carefully.
	///
	////////////////////////////////////////////////////////////
	void Close(bool force = true) { sf::Lock lock(mMutex); mBrowser->GetHost()->CloseBrowser(force); }

	////////////////////////////////////////////////////////////
	/// \brief Accesses the browser related this WebInterface
	///
	/// \return CefRefPtr referencing the browser related to this WebInterface.
	///
	////////////////////////////////////////////////////////////
	CefRefPtr<CefBrowser> GetBrowser() { return mBrowser; }

	////////////////////////////////////////////////////////////
	/// \brief Returns the width of this WebInterface.
	///
	/// \return Width of this WebInterface.
	///
	////////////////////////////////////////////////////////////
	int GetWidth() { return mTextureWidth; }

	////////////////////////////////////////////////////////////
	/// \brief Returns the height of this WebInterface.
	///
	/// \return Height of this WebInterface.
	///
	////////////////////////////////////////////////////////////
	int GetHeight() { return mTextureHeight; }

	////////////////////////////////////////////////////////////
	/// \brief Performs the redundancy update for our texture.  
	///
	/// This will be called for all WebInterfaces by WebSystem::UpdateInterfaceTextures().
	/// It can also be called manually for each texture, but as it must be called each frame
	/// in which an OnPaint for the WebInterface, it is much safter to use WebSystem::UpdateInterfaceTextures().
	///
	////////////////////////////////////////////////////////////
	void UpdateTexture();

	////////////////////////////////////////////////////////////
	/// \brief Returns the texture of this WebInterface.
	///
	/// \return Pointer to the texture of this WebInterface.
	///
	////////////////////////////////////////////////////////////
	sf::Texture* GetTexture() { return mpTexture; }

	////////////////////////////////////////////////////////////
	/// \brief Returns the url of the currently loaded web page.
	///
	/// \return URL of the currently loaded web page.
	///
	////////////////////////////////////////////////////////////
	std::string GetCurrentURL() { return mCurrentURL; }

	////////////////////////////////////////////////////////////
	/// \brief Returns background transparency.
	///
	/// \return Boolean representing background transparency.
	///
	////////////////////////////////////////////////////////////
	bool IsTransparent() { return mTransparent; }

	////////////////////////////////////////////////////////////
	/// \brief Used to change the size of this WebInterface.
	///
	/// \param width	New horizontal size.
	/// \param height	New vertical size.
	///
	////////////////////////////////////////////////////////////
	void SetSize(int width, int height);

private:
	////////////////////////////////////////////////////////////
	/// \brief Returns the defaultly handled modifiers for mouse keys.
	///
	/// \return Integer representing current mouse modifiers.
	///
	////////////////////////////////////////////////////////////
	static int GetMouseModifiers();

	////////////////////////////////////////////////////////////
	/// \brief Returns the defaultly tracked modifiers for keyboard keys.
	///
	/// This only handles Shift, Control, and Alt.
	///
	/// \return Integer representing current defaultly tracked keyboard modifiers.
	///
	////////////////////////////////////////////////////////////
	static int GetKeyboardModifiers();

	////////////////////////////////////////////////////////////
	/// \brief Holds data for redundantly updating rectangles on the texture. 
	///
	////////////////////////////////////////////////////////////
	struct UpdateRect
	{
	public:
		char* buffer;
		CefRect rect;
	};

	////////////////////////////////////////////////////////////
	/// \brief Reference to the browser related to this WebInterface.
	///
	////////////////////////////////////////////////////////////
	CefRefPtr<CefBrowser> mBrowser;

	////////////////////////////////////////////////////////////
	/// \brief Releases the reference to the browser related to this WebInterface.
	///
	/// This allows for the browser to be safely closed.
	///
	////////////////////////////////////////////////////////////
	void ClearBrowser() { mBrowser = NULL; }

	////////////////////////////////////////////////////////////
	/// \brief The texture used for displaying this WebInterface.
	///
	////////////////////////////////////////////////////////////
	sf::Texture* mpTexture;

	////////////////////////////////////////////////////////////
	/// \brief Width of the texture, and also the WebInterface.
	///
	////////////////////////////////////////////////////////////
	int mTextureWidth;

	////////////////////////////////////////////////////////////
	/// \brief Height of the texture, and also the WebInterface.
	///
	////////////////////////////////////////////////////////////
	int mTextureHeight;

	////////////////////////////////////////////////////////////
	/// \brief Mutex used to prevent multiple threads from operating on the texture at the same time.
	///
	////////////////////////////////////////////////////////////
	sf::Mutex mMutex;

	////////////////////////////////////////////////////////////
	/// \brief Queue of rectangles to be redundantly updated.
	///
	////////////////////////////////////////////////////////////
	std::queue<UpdateRect> mUpdateRects;

	////////////////////////////////////////////////////////////
	/// \brief URL of the current web page.
	///
	//////////////////////////////////////////////////////////// 
	std::string mCurrentURL;

	////////////////////////////////////////////////////////////
	/// \brief Whether or not the background is transparent for this WebInterface.
	///
	/// This cannot be changed after creation and is merely stored for reference. 
	///
	//////////////////////////////////////////////////////////// 
	bool mTransparent;

public:
	////////////////////////////////////////////////////////////
	/// \brief Implement cef reference counting.
	///
	////////////////////////////////////////////////////////////
	IMPLEMENT_REFCOUNTING(WebInterface);

	////////////////////////////////////////////////////////////
	/// \brief Implement cef locking mechanism.
	///
	////////////////////////////////////////////////////////////
	IMPLEMENT_LOCKING(WebInterface);
};

////////////////////////////////////////////////////////////
/// \brief CefV8Handler implementation which executes bound javascript callbacks.
///
/// This must reside in its own class, as Cef will delete its instances.
///
////////////////////////////////////////////////////////////
class WebV8Handler : public CefV8Handler
{
	friend class WebSystem;

public:
	WebV8Handler() {}

	////////////////////////////////////////////////////////////
	/// \brief Called by Cef when a bound function has been called in javascript.
	///
	/// Function will always return as if the function has succeeded.
	/// This is due to the nature of javascript callbacks in Cef3, which must be asynchronous.
	///
	/// \return Boolean success of the call.
	///
	////////////////////////////////////////////////////////////
	virtual bool Execute(const CefString& name,
		CefRefPtr<CefV8Value> object,
		const CefV8ValueList& arguments,
		CefRefPtr<CefV8Value>& retval,
		CefString& exception) override;

private:
	////////////////////////////////////////////////////////////
	/// \brief Implement cef reference counting.
	///
	////////////////////////////////////////////////////////////
	IMPLEMENT_REFCOUNTING(WebV8Handler);
};
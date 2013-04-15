//Custom schemes

#pragma once

#include <string>
#include <fstream>

#include "include/cef_browser.h"
#include "include/cef_scheme.h"

#undef min

//Scheme for loading local files.
class LocalSchemeHandler : public CefSchemeHandler
{
public:
	LocalSchemeHandler() :mOffset(0) {}

	virtual bool ProcessRequest(CefRefPtr<CefRequest> request, 
								CefRefPtr<CefSchemeHandlerCallback> callback) override
	{
		bool handled = false;

		AutoLock lock_scope(this);

		std::string url = request->GetURL();

		char path[1024];
		sscanf_s(url.c_str(), "local://%s", path);

		std::string relativePath;// = "../";
		relativePath += path;

		std::ifstream in(relativePath.c_str(), std::ios::in | std::ios::binary);
		if(in)
		{
			in.seekg(0, std::ios::end);
			mData.resize((unsigned int)in.tellg());
			in.seekg(0, std::ios::beg);
			in.read(&mData[0], mData.size());
			in.close();
		}

		handled = true;

		mMimeType = "text/html";
		if(strstr(url.c_str(), ".htm") != NULL || strstr(url.c_str(), ".html") != NULL)
		{
			mMimeType = "text/html";
		}
		else if(strstr(url.c_str(), ".css") != NULL)
		{
			mMimeType = "text/css";
		}
		else if(strstr(url.c_str(), ".js") != NULL)
		{
			mMimeType = "text/js";
		}

		if(handled)
		{
			callback->HeadersAvailable();
			return true;
		}

		return false;
	}

	virtual void GetResponseHeaders(CefRefPtr<CefResponse> response,
									int64& response_length,
									CefString& redirectUrl) override
	{
		response->SetMimeType(mMimeType);
		response->SetStatus(200);

		response_length = mData.length();
	}

	virtual void Cancel() override
	{

	}

	virtual bool ReadResponse(void* data_out,
							  int bytes_to_read,
							  int& bytes_read,
							  CefRefPtr<CefSchemeHandlerCallback> callback) override
	{
		bool has_data = false;
		bytes_read = 0;

		AutoLock lock_scope(this);

		if(mOffset < mData.length())
		{
			//Copy the next block of data into the buffer.
			int transfer_size = std::min(bytes_to_read, static_cast<int>(mData.length() - mOffset));
			memcpy(data_out, mData.c_str() + mOffset, transfer_size);
			mOffset += transfer_size;

			bytes_read = transfer_size;
			has_data = true;
		}

		return has_data;
	}

private:
	std::string mData;
	std::string mMimeType;
	size_t mOffset;

	IMPLEMENT_REFCOUNTING(LocalSchemeHandler);
	IMPLEMENT_LOCKING(LocalSchemeHandler);
};

//Factory for creating custom scheme handlers.  
class LocalSchemeHandlerFactory : public CefSchemeHandlerFactory
{
public:
	virtual CefRefPtr<CefSchemeHandler> Create(CefRefPtr<CefBrowser> browser,
											   const CefString& scheme_name,
											   CefRefPtr<CefRequest> request) override
	{
		return new LocalSchemeHandler();
	}

	IMPLEMENT_REFCOUNTING(LocalSchemeHandlerFactory);
};
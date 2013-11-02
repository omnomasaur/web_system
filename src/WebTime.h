
#pragma once

#include <string>

#include <SFML/Network.hpp>

class WebTime
{
public:
	static std::string GetFormattedTime()
	{
		sf::Http http;
		http.setHost("http://www.timeapi.org");

		sf::Http::Request request;
		request.setUri("utc/now?format=%25H%20%25M%20%25S");

		sf::Http::Response response = http.sendRequest(request);
		
		sf::Http::Response::Status status = response.getStatus();
		if(status == sf::Http::Response::Ok)
		{
			std::string timeString = response.getBody();
			return timeString;
		}

		return "ERROR";
	}
};
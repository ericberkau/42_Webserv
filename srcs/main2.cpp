// this file is only to test the current (01/08/26) RequestHandler
// (just temporary, no response builder yet, so output in strings)

#include "RequestHandler.hpp"
#include "HttpRequestContainer.hpp"
#include "ConfigStructures.hpp"
#include <iostream>

void runHandlerTest(const std::string& testName, const ServerConfig& config, const HttpRequestContainer& req)
{
    std::cout << "--- Handler Test: " << testName << " ---\n";
    RequestHandler handler(config);
    std::string rawResponse = handler.processRequest(req);
    std::cout << rawResponse << "\n\n";
}

int main()
{
    // 1. Create a mock global configuration
    ServerConfig mockConfig;
    mockConfig.root = "./public_html";
    mockConfig.index = "index.html";
    mockConfig.clientMaxBodySize = 1000000;

    // 2. Create a location block for a strict GET-only route
    LocationConfig secureRoute;
    secureRoute.path = "/secure";
    secureRoute.root = "./public_html/protected";
    secureRoute.allowedMethods.insert("GET"); // 🚨 Only GET is allowed here!
    secureRoute.index = "secret.html";
    mockConfig.locations.push_back(secureRoute);

    // ---------------------------------------------------------
    // TEST 1: Standard GET request matching default root
    // ---------------------------------------------------------
    {
        HttpRequestContainer req;
        req.setMethod("GET");
        req.setUri("/");
        req.setVersion("HTTP/1.1");
        req.addHeader("host", "localhost");
        
        runHandlerTest("Valid Global GET Request", mockConfig, req);
    }

    // ---------------------------------------------------------
    // TEST 2: Triggering 405 Method Not Allowed via routing
    // ---------------------------------------------------------
    {
        HttpRequestContainer req;
        req.setMethod("POST"); // ❌ Trying to POST to a GET-only route!
        req.setUri("/secure");
        req.setVersion("HTTP/1.1");
        req.addHeader("host", "localhost");
        req.setBody("username=admin");
        
        runHandlerTest("Blocked POST on GET-Only Route (405 Check)", mockConfig, req);
    }

    // ---------------------------------------------------------
    // TEST 3: Triggering 404 Not Found on missing asset
    // ---------------------------------------------------------
    {
        HttpRequestContainer req;
        req.setMethod("GET");
        req.setUri("/ghost-file.txt"); // ❌ File does not exist on disk
        req.setVersion("HTTP/1.1");
        req.addHeader("host", "localhost");
        
        runHandlerTest("Missing File Request (404 Check)", mockConfig, req);
    }

    return 0;
}

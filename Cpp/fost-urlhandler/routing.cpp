/**
    Copyright 2008-2019 Red Anchor Trading Co. Ltd.

    Distributed under the Boost Software License, Version 1.0.
    See <http://www.boost.org/LICENSE_1_0.txt>
 */


#include <fost/crypto>
#include "fost-urlhandler.hpp"
#include <fost/urlhandler.hpp>
#include <fost/log>
#include <iostream>

bool fostlib::urlhandler::service(fostlib::http::server::request &req) {
    // Before doing anything else run some sanity checks on the request
    std::cout << "sanity check "<< std::endl;
    auto const rfsw = static_cast<std::string>(
            req.file_spec()); // TODO Should be string view
    if (rfsw[0] != '/' || rfsw.find("/..") != std::string::npos) {
        auto log = fostlib::log::error(c_fost_web_urlhandler);
        log("",
            "fostlib::urlhandler::service -- Bad request received, could "
            "not be parsed");
        log("req", "method", req.method());
        log("req", "file_spec", req.file_spec());
        log("req", "query",
            req.query_string().as_string().value_or(ascii_printable_string()));
        fostlib::text_body response(
                fostlib::string("400 Bad Request\n"),
                fostlib::mime::mime_headers(), L"text/plain");
        req(response, 400);
        return true;
    }

    // Now process it
    fostlib::string hostname(req.data()->headers()["Host"].value());

    // Fost-Request-ID
    if (!req.data()->headers().exists("Fost-Request-ID")) {
        req.data()->headers().set(
                "Fost-Request-ID",
                fostlib::timestamp_nonce24b64u().underlying().underlying());
    }
    std::cout << "parse config "<< std::endl;
    fostlib::json host_config = c_hosts.value();
    std::cout << "parse done "<< std::endl;
    if (host_config.has_key(hostname)
        || host_config.has_key(fostlib::string())) {
        try {
            // Route the request to the right handler
            std::cout << "try  "<< std::endl;
            auto view_conf = host_config
                    [host_config.has_key(hostname) ? hostname
                                                   : fostlib::string()];
            std::cout << "view config "<< std::endl;
            auto path = coerce<string>(req.file_spec().underlying()).substr(1);
            std::cout << "path:  "<< path << " view_conf " << view_conf << std::endl;
            auto resource = view::execute(view_conf, path, req, host(hostname));
            std::cout << "view execute done  "<< std::endl;
            if  (resource.first == nullptr) {
                fostlib::text_body response{
                f5::u8view{"<html><body>Some internal error "
                            "for request</body></html>"},
                fostlib::mime::mime_headers(), "text/html"};
                req(response, 500);
            }
            else {
                std::cout << "send response "<< std::endl;
                req(*resource.first, resource.second);
            }
            std::cout << "try done "<< std::endl;
        } catch (fostlib::exceptions::exception &e) {
            std::cout << "catch 1"<< std::endl;
            fostlib::log::error(c_fost_web_urlhandler)(
                    "",
                    "fostlib::urlhandler::service -- "
                    "fostlib::exceptions::exception")(
                    "exception", coerce<json>(e));
            fostlib::text_body response(
                    utf8_string("<html><body>An error occurred in the "
                                "request</body></html>"),
                    fostlib::mime::mime_headers(), L"text/html");
            req(response, 500);
        } catch (std::exception &e) {
            std::cout << "catch 2"<< std::endl;
            fostlib::log::error(c_fost_web_urlhandler)(
                    "", "fostlib::urlhandler::service -- std::exception")(
                    "exception", "message",
                    e.what())("exception", "type", typeid(e).name());
            fostlib::text_body response(
                    utf8_string("<html><body>An error occurred in the "
                                "request</body></html>"),
                    fostlib::mime::mime_headers(), L"text/html");
            req(response, 500);
        }
    } else {
        std::cout << "else "<< std::endl;
        fostlib::log::warning(c_fost_web_urlhandler)(
                "", "fostlib::urlhandler::service -- No configured web site")(
                "hostname", hostname)("config", host_config);
        std::cout << "body "<< std::endl;
        fostlib::text_body response(
                L"<html><body>No site found to service request</body></html>",
                fostlib::mime::mime_headers(), L"text/html");
        std::cout << "500 "<< std::endl;  
        req(response, 500);
    }
    std::cout << "url handler done "<< std::endl;
    return true;
}

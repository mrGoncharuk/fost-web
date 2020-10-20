/**
    Copyright 2011-2019 Red Anchor Trading Co. Ltd.

    Distributed under the Boost Software License, Version 1.0.
    See <http://www.boost.org/LICENSE_1_0.txt>
 */


#include "fost-urlhandler.hpp"
#include <fost/insert>
#include <fost/threading>
#include <fost/urlhandler.hpp>
#include <fost/exception/unexpected_nil.hpp>
#include <iostream>

namespace {
    typedef fostlib::threadsafe_store<fostlib::urlhandler::view *> view_store;
    view_store &views() {
        static view_store store;
        return store;
    }
}

fostlib::urlhandler::view::view(const fostlib::string &name) {
    views().add(name, this);
}

fostlib::urlhandler::view::~view() {
    // Allow the view to leak as they'll only be removed on termination
}


namespace {
    fostlib::string view_name(const fostlib::json &obj) {
        if (obj["view"].isnull()) {
            throw fostlib::exceptions::not_implemented(
                    __func__, "No view name was given in the configuration",
                    obj);
        }
        return fostlib::coerce<fostlib::string>(obj["view"]);
        ;
    }
    bool next_view(
            const fostlib::json &views,
            std::pair<fostlib::string, fostlib::json> &current) {
        if (views.has_key(current.first)) {
            current = std::make_pair(
                    view_name(views[current.first]),
                    views[current.first]["configuration"]);
            return true;
        } else {
            fostlib::nullable<fostlib::json> view_setting(
                    fostlib::setting<fostlib::json>::value(
                            fostlib::urlhandler::c_views.section(),
                            fostlib::urlhandler::c_views.name() + "/"
                                    + current.first,
                            fostlib::null));
            if (view_setting) {
                current = std::make_pair(
                        view_name(view_setting.value()),
                        view_setting.value()["configuration"]);
                return true;
            } else {
                return false;
            }
        }
    }
}
std::pair<fostlib::string, fostlib::json> fostlib::urlhandler::view::find_view(
        const fostlib::string &view, const fostlib::json &view_config) {
    fostlib::json views(c_views.value());
    std::pair<fostlib::string, fostlib::json> final(view, view_config);
    while (next_view(views, final)) { ; }
    return final;
}


const fostlib::urlhandler::view &
        fostlib::urlhandler::view::view_for(const fostlib::string &name) {
    view_store::found_t found(views().find(name));
    if (found.size() == 1) {
        return *found[0];
    } else {
        fostlib::exceptions::unexpected_nil exception(
                "Where zero or more than 1 views are found");
        insert(exception.data(), "view-name", name);
        insert(exception.data(), "found", found.size());
        throw exception;
    }
}


std::pair<boost::shared_ptr<fostlib::mime>, int>
        fostlib::urlhandler::view::execute(
                const fostlib::json &configuration,
                const fostlib::string &path,
                fostlib::http::server::request &request,
                const fostlib::host &host) {
    try {

        std::pair<boost::shared_ptr<fostlib::mime>, int> response;
        std::cout << "try view::execute "<< std::endl;
        if (configuration.isobject()) {
            std::cout << "try find_view for isobject  configuration: "<< configuration << std::endl;  
            auto view_fn = find_view(
                    view_name(configuration), configuration["configuration"]);
            std::cout << "find_view done "<< configuration << std::endl; 
            response = view_for(view_fn.first)(
                    view_fn.second, path, request, host);
            std::cout << "view_for done in isobject " << configuration << std::endl;
        } else {
            std::cout << "try else view::execute "<< std::endl;
            auto view_name = coerce<string>(configuration);
            std::cout << "try find_view for else configuration: "<< configuration << std::endl;
            auto to_exec = find_view(view_name);
            std::cout << "response in else view::execute "<< std::endl;
            response = view_for(to_exec.first)(
                    to_exec.second, path, request, host);
            std::cout << "view_for done in else "<< configuration << std::endl;
        }
        std::cout << "try Preserve Fost-Request-ID from request"<< std::endl;

        if (response.first == nullptr) {
            std::cout << "nullptr at responce pair for "<< path << " " <<  " " <<  host  << std::endl;
            return  response;
        }
        // Preserve Fost-Request-ID from request
        response.first->headers().set(
                "Fost-Request-ID",
                request.data()->headers()["Fost-Request-ID"]);
        std::cout << "try return responce"<< std::endl;
        return response;
    } catch (fostlib::exceptions::exception &e) {
        std::cout << "catch in view::execute"<< std::endl;
        push_back(e.data(), "fost-web", "execute", "stacktrace", configuration);
        throw;
    }
}

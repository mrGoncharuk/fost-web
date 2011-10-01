/*
    Copyright 2011 Felspar Co Ltd. http://support.felspar.com/
    Distributed under the Boost Software License, Version 1.0.
    See accompanying file LICENSE_1_0.txt or copy at
        http://www.boost.org/LICENSE_1_0.txt
*/


#include <fost/internet>
#include <fost/http.server.hpp>


/// URL routing for requests
namespace urlhandler {


    /// The prime routing for web sites
    bool service( fostlib::http::server::request &req );

    /// A view class
    class view : boost::noncopyable {
        protected:
            /// The name of the configuration that the handler should tie to
            view(const fostlib::string &name);
            /// Allow sub-classing to work properly
            virtual ~view();
        public:
            /// Handle the request.
            virtual void operator () (fostlib::http::server::request &,
                const fostlib::host &) const = 0;
    };


}

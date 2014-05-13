#include <fost/log>
#include <proxy/cache.hpp>
#include <proxy/views.hpp>
#include <boost/filesystem/fstream.hpp>


namespace {


    const class proxy_view : public fostlib::urlhandler::view {
    public:
        proxy_view()
        : view("proxy") {
        }


        std::pair<boost::shared_ptr<fostlib::mime>, int >
            operator () (const fostlib::json &configuration,
                const fostlib::string &path,
                fostlib::http::server::request &request,
                const fostlib::host &host) const {
            auto info = fostlib::log::info()("id", fostlib::guid());

            fostlib::url base("http://www.kirit.com/");
            fostlib::url location(base, request.file_spec());
            info("location", location);

            fostlib::http::user_agent ua(base);
            std::auto_ptr< fostlib::http::user_agent::response >
                response = ua.get(location);
            info("response", "status", response->status());

            boost::filesystem::wpath pathname = proxy::save_entry(*response);
            info("response", "size", response->body()->data().size());
            if ( response->body()->data().size() ) {
                boost::filesystem::ofstream(pathname,
                        std::ios_base::out | std::ios_base::binary).
                    write(reinterpret_cast<const char *>(
                            response->body()->data().data()),
                        response->body()->data().size());
            }

            fostlib::mime::mime_headers headers;
            headers.set("Content-Type", response->headers()["Content-Type"]);
            boost::shared_ptr<fostlib::mime> body =
                boost::make_shared<fostlib::binary_body>(
                    response->body()->data(), headers);

            return std::make_pair(body, response->status());
        }

    } c_proxy_view;


}


const fostlib::urlhandler::view &proxy::view::c_proxy = c_proxy_view;


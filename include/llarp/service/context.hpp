#ifndef LLARP_SERVICE_CONTEXT_HPP
#define LLARP_SERVICE_CONTEXT_HPP
#include <handlers/tun.hpp>
#include <llarp/net.hpp>
#include <llarp/service/config.hpp>
#include <llarp/service/endpoint.hpp>

#include <unordered_map>

namespace llarp
{
  namespace service
  {
    /// holds all the hidden service endpoints we own
    struct Context
    {
      Context(llarp::Router *r);
      ~Context();

      void
      Tick(llarp_time_t now);

      bool
      hasEndpoints();

      /// DRY refactor
      llarp::service::Endpoint *
      getFirstEndpoint();

      bool
      FindBestAddressFor(const byte_t *addr, bool isSNode, huint32_t &);

      /// DRY refactor
      llarp::handlers::TunEndpoint *
      getFirstTun();

      /// punch a hole to get ip range from first tun endpoint
      llarp_tun_io *
      getRange();

      struct mapAddressAll_context
      {
        llarp::service::Address serviceAddr;
        llarp::Addr localPrivateIpAddr;
      };

      struct endpoint_iter
      {
        void *user;
        llarp::service::Endpoint *endpoint;
        size_t index;
        bool (*visit)(struct endpoint_iter *);
      };

      bool
      iterate(struct endpoint_iter &i);

      /// hint at possible path usage and trigger building early
      bool
      Prefetch(const llarp::service::Address &addr);

      bool
      MapAddressAll(const llarp::service::Address &addr,
                    llarp::Addr &localPrivateIpAddr);

      bool
      AddDefaultEndpoint(
          const std::unordered_multimap< std::string, std::string > &opts);

      bool
      AddEndpoint(const Config::section_t &conf, bool autostart = false);

      bool
      StartAll();

     private:
      llarp::Router *m_Router;
      std::unordered_map< std::string, std::unique_ptr< Endpoint > >
          m_Endpoints;
    };
  }  // namespace service
}  // namespace llarp
#endif

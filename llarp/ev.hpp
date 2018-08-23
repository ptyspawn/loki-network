#ifndef LLARP_EV_HPP
#define LLARP_EV_HPP
#include <llarp/ev.h>

#ifndef _MSC_VER
#include <unistd.h>
#endif
#include <llarp/buffer.h>
#include <list>
#include <llarp/codel.hpp>
#include <vector>
#ifndef MAX_WRITE_QUEUE_SIZE
#define MAX_WRITE_QUEUE_SIZE 1024
#endif

namespace llarp
{
  struct ev_io
  {
#ifndef _WIN32
    int fd;
    ev_io(int f) : fd(f), m_writeq("writequeue"){};
#else
    SOCKET fd;
    // the unique completion key that helps us to
    // identify the object instance for which we receive data
    // Here, we'll use the address of the udp_listener instance, converted to
    // its literal int/int64 representation.
    ULONG_PTR listener_id = 0;
    ev_io(SOCKET f) : fd(f), m_writeq("writequeue"){};
#endif
    virtual int
    read(void* buf, size_t sz) = 0;

    virtual int
    sendto(const sockaddr* dst, const void* data, size_t sz) = 0;

    /// used for tun interface
    bool
    queue_write(const void* data, size_t sz)
    {
      return m_writeq.EmplaceIf(
          [&](WriteBuffer* pkt) -> bool {
            return m_writeq.Size() < MAX_WRITE_QUEUE_SIZE
                && sz <= sizeof(pkt->buf);
          },
          data, sz);
    }

    /// called in event loop when fd is ready for writing
    /// requeues anything not written
    /// this assumes fd is set to non blocking
    virtual void
    flush_write()
    {
      m_writeq.ProcessIf([&](WriteBuffer* buffer) -> bool {
      // todo: wtf???
#ifndef _WIN32
        if(write(fd, buffer->buf, buffer->bufsz) == -1)
        {
          // if we would block we save the entries for later
          return errno == EWOULDBLOCK || errno == EAGAIN;
        }
        // discard entry
        return true;
#else
      // writefile
#endif
      });
      /// reset errno
      errno = 0;
    }

    struct WriteBuffer
    {
      llarp_time_t timestamp = 0;
      size_t bufsz;
      byte_t buf[1500];

      WriteBuffer(const void* ptr, size_t sz)
      {
        if(sz <= sizeof(buf))
        {
          bufsz = sz;
          memcpy(buf, ptr, bufsz);
        }
        else
          bufsz = 0;
      }

      struct GetTime
      {
        llarp_time_t
        operator()(const WriteBuffer* w) const
        {
          return w->timestamp;
        }
      };

      struct PutTime
      {
        void
        operator()(WriteBuffer* w) const
        {
          w->timestamp = llarp_time_now_ms();
        }
      };

      struct Compare
      {
        bool
        operator()(const WriteBuffer* left, const WriteBuffer* right) const
        {
          return left->timestamp < right->timestamp;
        }
      };
    };

    llarp::util::CoDelQueue< WriteBuffer, WriteBuffer::GetTime,
                             WriteBuffer::PutTime, WriteBuffer::Compare,
                             llarp::util::NullMutex, llarp::util::NullLock >
        m_writeq;

    virtual ~ev_io()
    {
#ifndef _WIN32
      ::close(fd);
#else
      closesocket(fd);
#endif
    };
  };

};  // namespace llarp

struct llarp_ev_loop
{
  virtual bool
  init() = 0;
  virtual int
  run() = 0;

  virtual int
  tick(int ms) = 0;

  virtual void
  stop() = 0;

  bool
  udp_listen(llarp_udp_io* l, const sockaddr* src)
  {
    auto ev = create_udp(l, src);
    return ev && add_ev(ev, false);
  }

  virtual llarp::ev_io*
  create_udp(llarp_udp_io* l, const sockaddr* src) = 0;

  virtual bool
  udp_close(llarp_udp_io* l) = 0;
  virtual bool
  close_ev(llarp::ev_io* ev) = 0;

  virtual llarp::ev_io*
  create_tun(llarp_tun_io* tun) = 0;

  virtual bool
  add_ev(llarp::ev_io* ev, bool write = false) = 0;

  virtual bool
  running() const = 0;

  virtual ~llarp_ev_loop(){};

  std::list< llarp_udp_io* > udp_listeners;
  std::list< llarp_tun_io* > tun_listeners;

  void
  tick_listeners()
  {
    for(auto& l : udp_listeners)
      if(l->tick)
        l->tick(l);
    for(auto& l : tun_listeners)
    {
      if(l->tick)
        l->tick(l);
      if(l->before_write)
        l->before_write(l);
      static_cast< llarp::ev_io* >(l->impl)->flush_write();
    }
  }
};

#endif

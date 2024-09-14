#include "pch.h"
#include "io_event.h"

#include "io/io_service.h"
#include "net/socket.h"
#include "net/packet.h"
#include "util/deferred_call.h"
#include "logging/logger.h"

namespace io
{
    bool IoAcceptEvent::post_overlapped_io(win::Socket sock)
    {
        accepted_socket = net::create_windows_socket(net::SocketProtocol::TCPv4Rio);
        if (accepted_socket == INVALID_SOCKET)
            return false;

        return net::Socket::accept(sock, accepted_socket, event_data()->begin(), &overlapped);
    }

    void IoAcceptEvent::on_event_complete(void* completion_key, DWORD transferred_bytes_or_signal)
    {
        auto event_handler = static_cast<IoEventHandler*>(completion_key);
        event_handler->handle_io_event(this);
        event_handler->on_complete(this);
    }

    void IoRecvEvent::on_event_complete(void* completion_key, DWORD transferred_bytes_or_signal)
    {
        auto event_handler = static_cast<IoEventHandler*>(completion_key);

        // pre-processing
        if (transferred_bytes_or_signal == EOF_SIGNAL) {
            event_handler->on_error();
            return;
        }

        if (transferred_bytes_or_signal != RETRY_SIGNAL)
            event_data()->push(nullptr, transferred_bytes_or_signal); // pass nullptr because data was already appended by I/O. just update size only.

        // deliver events to the owner.
        auto processed_bytes = event_handler->handle_io_event(this);

        // post-processing
        if (processed_bytes)
            event_data()->pop(processed_bytes);

        event_handler->on_complete(this);
    }

    bool IoRecvEvent::post_rio_event(io::RegisteredIO& rio, unsigned connection_id)
    {
        if (is_processing || event_data()->unused_size() < net::PacketStructure::max_size_of_packet_struct())
            return false;

        is_processing = true;

        if (not rio.recv(connection_id, event_data()->end(), event_data()->unused_size(), this))
            return is_processing = false;

        return true;
    }

    void IoSendEvent::on_event_complete(void* completion_key, DWORD transferred_bytes_or_signal)
    {
        auto event_handler = static_cast<IoEventHandler*>(completion_key);

        // pre-processing
        if (transferred_bytes_or_signal == EOF_SIGNAL) {
            event_handler->on_error();
            return;
        }

        event_data()->pop(transferred_bytes_or_signal);

        event_handler->on_complete(this, transferred_bytes_or_signal);
    }

    bool IoSendEvent::post_overlapped_io(win::Socket sock)
    {
        return net::Socket::send(sock, event_data()->begin(), event_data()->size(), &overlapped);
    }

    bool IoSendEvent::post_rio_event(io::RegisteredIO& rio, unsigned connection_id)
    {
        if (is_processing || event_data()->size() == 0)
            return false;

        is_processing = true;

        if (not rio.send(connection_id, event_data()->begin(), event_data()->size(), this))
            return is_processing = false;

        return true;
    }

    bool IoMulticastSendEvent::post_rio_event(io::RegisteredIO& rio, unsigned connection_id)
    {
        if (is_processing || event_data()->size() == 0)
            return false;

        is_processing = true;

        assert(multicast_data != nullptr);
        if (not rio.multicast_send(connection_id, multicast_data->buffer_id(), event_data()->size(), this))
            return is_processing = false;

        return true;
    }

    void IoMulticastSendEvent::on_event_complete(void* completion_key, DWORD transferred_bytes_or_signal)
    {
        auto event_handler = static_cast<IoEventHandler*>(completion_key);

        // pre-processing
        if (transferred_bytes_or_signal == EOF_SIGNAL) {
            event_handler->on_error();
            return;
        }

        event_handler->on_complete(this, transferred_bytes_or_signal);
    }

    void RioEvent::on_event_complete(void* completion_key, DWORD transferred_bytes_or_signal)
    {
        thread_local io::RioEventResult event_results[io::RegisteredIO::MAX_RIO_EVENT_RESULTS];

        auto rio = static_cast<io::RegisteredIO*>(completion_key);

        auto num_dequeued_results = rio->dequeue_rio_event_result(event_results, std::size(event_results));
        if (num_dequeued_results <= 0) {
            CONSOLE_LOG(error) << "RIODequeueCompletion failed with:" << ::GetLastError();
            return;
        }

        rio->notify_completion();

        for (int i = 0; i < num_dequeued_results; i++) {
            auto io_event = reinterpret_cast<io::IoEvent*>(event_results[i].RequestContext);
            auto completion_key = reinterpret_cast<IoEventHandler*>(event_results[i].SocketContext);

            io_event->on_event_complete(completion_key, event_results[i].BytesTransferred);
        }
    }

    bool IoRecvEventDataImpl::push(const std::byte* data, std::size_t n)
    {
        if (data)
            std::memcpy(begin(), data, n);

        _size += n;
        assert(_size <= _capacity);
        return true;
    }

    void IoRecvEventDataImpl::pop(std::size_t n)
    {
        assert(_size >= n);
        if (_size -= n)
            std::memmove(_data, _data + n, _size); // move remaining data ahead.
    }
}
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
        accepted_socket = net::create_windows_socket(net::socket_protocol_id::tcp_rio_v4);
        if (accepted_socket == INVALID_SOCKET)
            return false;

        return net::Socket::accept(sock, accepted_socket, event_data()->begin(), &overlapped);
    }

    bool IoConnectEvent::post_overlapped_io(win::Socket sock, std::string_view ip, int port)
    {
        connected_socket = sock;

        return net::Socket::connect(sock, ip, port, &overlapped);
    }

    bool IoSendEvent::post_overlapped_io(win::Socket sock)
    {
        return net::Socket::send(sock, event_data()->begin(), event_data()->size(), &overlapped);
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
        if (not rio.multicast_send(connection_id, multicast_data->registered_buffer_id(), event_data()->size(), this))
            return is_processing = false;

        return true;
    }

    void IoAcceptEvent::on_event_complete(io::IoEventHandler* connection, DWORD transferred_bytes_or_signal)
    {
        if (transferred_bytes_or_signal != io::iocp_signal::event_failed)
            connection->handle_io_event(this);

        connection->on_complete(this);
    }

    void IoConnectEvent::on_event_complete(io::IoEventHandler* connection, DWORD transferred_bytes_or_signal)
    {
        if (transferred_bytes_or_signal != io::iocp_signal::event_failed)
            connection->handle_io_event(this);

        connection->on_complete(this);
    }

    void IoRecvEvent::on_event_complete(io::IoEventHandler* connection, DWORD transferred_bytes_or_signal)
    {
        // pre-processing
        if (transferred_bytes_or_signal == io::iocp_signal::eof) {
            connection->on_error();
            return;
        }

        else if (transferred_bytes_or_signal == io::iocp_signal::event_failed) {
            connection->on_complete(this);
            return;
        }

        else if (transferred_bytes_or_signal != io::iocp_signal::retry_packet_process)
            event_data()->push(nullptr, transferred_bytes_or_signal); // pass nullptr because data was already appended by I/O. just update size only.

        // deliver events to the owner.
        auto processed_bytes = connection->handle_io_event(this);

        // post-processing
        if (processed_bytes)
            event_data()->pop(processed_bytes);

        connection->on_complete(this);
    }

    void IoSendEvent::on_event_complete(io::IoEventHandler* connection, DWORD transferred_bytes_or_signal)
    {
        // pre-processing
        if (transferred_bytes_or_signal == io::iocp_signal::eof) {
            connection->on_error();
            return;
        }

        else if (transferred_bytes_or_signal == io::iocp_signal::event_failed) {
            connection->on_complete(this, 0);
            return;
        }

        event_data()->pop(transferred_bytes_or_signal);

        connection->on_complete(this, transferred_bytes_or_signal);
    }

    void IoMulticastSendEvent::on_event_complete(io::IoEventHandler* connection, DWORD transferred_bytes_or_signal)
    {
        // pre-processing
        if (transferred_bytes_or_signal == io::iocp_signal::eof) {
            connection->on_error();
            return;
        }

        connection->on_complete(this, transferred_bytes_or_signal);
    }

    void RioEvent::on_event_complete(io::IoEventHandler* completion_key, DWORD transferred_bytes_or_signal)
    {
        thread_local io::RioEventResult event_results[io::RegisteredIO::max_dequeuing_rio_event_results];

        auto rio = reinterpret_cast<io::RegisteredIO*>(completion_key);

        auto num_dequeued_results = rio->dequeue_rio_event_result(event_results, std::size(event_results));
        if (num_dequeued_results <= 0) {
            CONSOLE_LOG(error) << "RIODequeueCompletion failed with:" << ::GetLastError();
            return;
        }

        rio->notify_completion();

        for (int i = 0; i < num_dequeued_results; i++) {
            auto io_event = reinterpret_cast<io::IoEvent*>(event_results[i].RequestContext);
            auto connection = reinterpret_cast<io::IoEventHandler*>(event_results[i].SocketContext);

            io_event->on_event_complete(connection, event_results[i].Status == S_OK ?
                event_results[i].BytesTransferred : io::iocp_signal::event_failed);
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
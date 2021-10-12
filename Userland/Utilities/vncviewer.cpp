#include <LibGUI/Application.h>
#include <LibGUI/Widget.h>
#include <LibGUI/Window.h>

#include <AK/Variant.h>
#include <LibCore/Promise.h>

#include <AK/IPv4Address.h>
#include <LibCore/TCPSocket.h>

using namespace Core;

// https://datatracker.ietf.org/doc/html/rfc6143
class VNCClient {

    enum eState {
        // Connecting
        eConnecting,

        // Init
        eProtocolVersionHandshake,
        eSecurityHandshake,
        eSecurityResultHandshake,
        eClientInit,
        eServerInit,

        // Connected

        // Error/Disconnecting
        eFailedWaitingForReason,
        eFailedWaitingForClose,

        // Disconnected
        eDisconnected,
    };

public:
    VNCClient(StringView host, unsigned port)
        : m_host(host)
        , m_port(port)
        , m_current_state(eDisconnected)
    {
        m_socket = Core::TCPSocket::construct();
    }

    RefPtr<Promise<Empty>> connect()
    {

        m_current_state = eConnecting;
        m_buffer.clear();

        m_socket->on_ready_to_read = [&] {
            on_ready_to_receive();
        };

        auto success = m_socket->connect(m_host, m_port);
        dbgln("connecting to {}:{} {}", m_host, m_port, success);

        if (!success)
            return {};

        m_connect_pending = Promise<Empty>::construct();
        return m_connect_pending;
    }

private:
    void on_ready_to_receive()
    {
        if (m_current_state == eConnecting)
            m_current_state = eProtocolVersionHandshake;

        if (!m_socket->can_read())
            return;

        m_buffer += m_socket->read_all();

        dbgln("Recv data {}", m_buffer.size());
        processes_data();
    }

    void processes_data()
    {
        while (m_buffer.size()) {
            dbgln("Got data {}", m_buffer.size());
            switch (m_current_state) {
            case eProtocolVersionHandshake: {
                // We do not have enough data
                if (m_buffer.size() < 12)
                    return;

                auto response = m_buffer.slice(0, 12);
                StringView protocol_version(response);

                if (protocol_version != "RFB 003.008\n") {
                    dbgln("Unsupported protocol version sent from server: {}", protocol_version);
                    m_current_state = eFailedWaitingForClose;
                    return;
                }

                dbgln("Recieved protocol version from server {}", protocol_version);
                dbgln("Responding");
                m_socket->write(response);

                m_current_state = eSecurityHandshake;

                // TODO: We want some sort of ring buffer here with an API that lets us
                //       "read" N bytes into a location.
                m_buffer = m_buffer.slice(12, m_buffer.size() - 12);
                break;
            }
            case eSecurityHandshake: {
                u8 const* security_data = m_buffer.data();
                u8 const number_of_security_types = security_data[0];
                size_t const data_size = number_of_security_types + 1;

                if (number_of_security_types == 0) {
                    m_current_state = eFailedWaitingForReason;
                    break;
                }

                //Not enough data recieved
                if (m_buffer.size() < data_size)
                    return;

                dbgln("Number of security types {}", number_of_security_types);
                for (u8 idx = 1; idx <= number_of_security_types; idx++) {
                    dbgln("- Security type {} = {}", idx, security_data[idx]);
                }

                m_buffer = m_buffer.slice(data_size, m_buffer.size() - data_size);
                break;
            }
            default:
                m_buffer.clear();
                break;
            }
        }
    }

private:
    StringView m_host;
    unsigned m_port;

    eState m_current_state;

    RefPtr<Core::Socket> m_socket;
    RefPtr<Promise<Empty>> m_connect_pending {};

    ByteBuffer m_buffer;
};

int main(int argc, char** argv)
{
    auto app = GUI::Application::construct(argc, argv);
    auto window = GUI::Window::construct();

    VNCClient client("192.168.1.102", 5901);
    client.connect();

    window->show();
    return app->exec();

    warnln("Connection closed");

    return 0;
}
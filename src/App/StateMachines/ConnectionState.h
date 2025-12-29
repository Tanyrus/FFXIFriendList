
#ifndef APP_CONNECTION_STATE_H
#define APP_CONNECTION_STATE_H

namespace XIFriendList {
namespace App {

enum class ConnectionState {
    Disconnected,
    Connecting,
    Connected,
    Reconnecting,
    Failed
};

class ConnectionStateMachine {
public:
    ConnectionStateMachine() : state_(ConnectionState::Disconnected) {}
    
    ConnectionState getState() const { return state_; }
    void startConnecting() {
        if (state_ == ConnectionState::Disconnected || 
            state_ == ConnectionState::Failed) {
            state_ = ConnectionState::Connecting;
        }
    }
    
    void setConnected() {
        if (state_ == ConnectionState::Connecting || 
            state_ == ConnectionState::Reconnecting) {
            state_ = ConnectionState::Connected;
        }
    }
    
    void setDisconnected() {
        state_ = ConnectionState::Disconnected;
    }
    void startReconnecting() {
        if (state_ == ConnectionState::Connected) {
            state_ = ConnectionState::Reconnecting;
        }
    }
    
    void setFailed() {
        state_ = ConnectionState::Failed;
    }
    bool isConnected() const {
        return state_ == ConnectionState::Connected;
    }
    bool isConnecting() const {
        return state_ == ConnectionState::Connecting || 
               state_ == ConnectionState::Reconnecting;
    }
    bool canConnect() const {
        return state_ == ConnectionState::Disconnected || 
               state_ == ConnectionState::Failed;
    }

private:
    ConnectionState state_;
};

} // namespace App
} // namespace XIFriendList

#endif // APP_CONNECTION_STATE_H


//
//  SuperTuxKart - a fun racing game with go-kart
//  Copyright (C) 2013-2015 SuperTuxKart-Team
//
//  This program is free software; you can redistribute it and/or
//  modify it under the terms of the GNU General Public License
//  as published by the Free Software Foundation; either version 3
//  of the License, or (at your option) any later version.
//
//  This program is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU General Public License for more details.
//
//  You should have received a copy of the GNU General Public License
//  along with this program; if not, write to the Free Software
//  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

/*! \file event.hpp
 *  \brief Contains an interface to store network events, like connections,
 *  disconnections and messages.
 */

#ifndef EVENT_HPP
#define EVENT_HPP

#include "network/network_string.hpp"
#include "utils/leak_check.hpp"
#include "utils/types.hpp"

#include "enet/enet.h"

#include <memory>

class STKPeer;

/*!
 * \enum EVENT_TYPE
 * \brief Represents a network event type.
 */
enum EVENT_TYPE
{
    EVENT_TYPE_CONNECTED,   //!< A peer is connected
    EVENT_TYPE_DISCONNECTED,//!< A peer is disconnected
    EVENT_TYPE_MESSAGE      //!< A message between server and client protocols
};

/*!
 * \enum EVENT_CHANNEL
 * \brief Represents a list of channels stk used.
 */
enum EVENT_CHANNEL : uint8_t
{
    EVENT_CHANNEL_NORMAL = 0,   //!< Normal channel (encrypted if supported)
    EVENT_CHANNEL_UNENCRYPTED = 1,//!< Unencrypted channel
    EVENT_CHANNEL_DATA_TRANSFER = 2,//!< Data transfer channel (like game replay)
    EVENT_CHANNEL_COUNT = 3
};

enum PeerDisconnectInfo : unsigned int;

/*!
 * \class Event
 * \brief Class representing an event that need to pass trough the system.
 * This is used to remove ENet dependency in the network.
 * It interfaces the ENetEvent structure.
 * The user has to be extremely careful about the peer.
 * Indeed, when packets are logged, the state of the peer cannot be stored at
 * all times, and then the user of this class can rely only on the address/port
 * of the peer, and not on values that might change over time.
 */
class Event
{
private:
    LEAK_CHECK()

    /** Copy of the data passed by the event. */
    NetworkString *m_data;

    /**  Type of the event. */
    EVENT_TYPE m_type;

    /** Pointer to the peer that triggered that event. */
    std::shared_ptr<STKPeer> m_peer;

    /** Arrivial time of the event, for timeouts. */
    uint64_t m_arrival_time;

    /** For disconnection event, a bit more info is provided. */
    PeerDisconnectInfo m_pdi;

public:
         Event(ENetEvent* event, std::shared_ptr<STKPeer> peer);
        ~Event();

    // ------------------------------------------------------------------------
    /** Returns the type of this event. */
    EVENT_TYPE getType() const { return m_type; }
    // ------------------------------------------------------------------------
    /** Returns the peer of this event (shared pointer). */
    std::shared_ptr<STKPeer> getPeerSP() const { return m_peer; }
    // ------------------------------------------------------------------------
    /** Returns the peer of this event. */
    STKPeer* getPeer() const { return m_peer.get(); }
    // ------------------------------------------------------------------------
    /** \brief Get a const reference to the received data.
     *  This is empty for events like connection or disconnections. 
     */
    const NetworkString& data() const { return *m_data; }
    // ------------------------------------------------------------------------
    /** \brief Get a non-const reference to the received data.
     *  A copy of the message data. This is empty for events like
     *  connection or disconnections. */
    NetworkString& data() { return *m_data; }
    // ------------------------------------------------------------------------
    /** Determines if this event should be delivered synchronous or not.
     *  Only messages can be delivered synchronous. */
    bool isSynchronous() const { return m_type==EVENT_TYPE_MESSAGE &&
                                        m_data->isSynchronous();     }
    // ------------------------------------------------------------------------
    /** Returns the arrival time of this event. */
    uint64_t getArrivalTime() const { return m_arrival_time; }
    // ------------------------------------------------------------------------
    PeerDisconnectInfo getPeerDisconnectInfo() const { return m_pdi; }
    // ------------------------------------------------------------------------

};   // class Event

#endif // EVENT_HPP

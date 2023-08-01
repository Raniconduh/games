//
//  SuperTuxKart - a fun racing game with go-kart
//  Copyright (C) 2013-2015 Glenn De Jonghe
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

#include "online/online_player_profile.hpp"

#include "achievements/achievements_manager.hpp"
#include "config/player_manager.hpp"
#include "config/user_config.hpp"
#include "guiengine/message_queue.hpp"
#include "guiengine/screen.hpp"
#include "network/network_config.hpp"
#include "online/online_profile.hpp"
#include "online/profile_manager.hpp"
#include "online/request_manager.hpp"
#include "online/xml_request.hpp"
#include "states_screens/main_menu_screen.hpp"
#include "states_screens/online/online_profile_friends.hpp"
#include "states_screens/options/user_screen.hpp"
#include "states_screens/dialogs/change_password_dialog.hpp"
#include "states_screens/dialogs/user_info_dialog.hpp"
#include "utils/log.hpp"
#include "utils/translation.hpp"

#include <sstream>
#include <stdlib.h>
#include <assert.h>
#include <algorithm>

using namespace irr;
using namespace Online;

namespace Online
{
    namespace PrivateRequest
    {
        // ----------------------------------------------------------------
        class SignInRequest : public XMLRequest
        {
            virtual void callback();
        public:
            SignInRequest()
                : XMLRequest(/*priority*/10) {}
        };   // SignInRequest

        // ----------------------------------------------------------------
        class PollRequest : public XMLRequest
        {
            virtual void callback();
        public:
            PollRequest();
        };   // PollRequest
    }
    // ------------------------------------------------------------------------
    /** Adds the login credential to a http request. It sets the name of
     *  the script to invokce, token, and user id.
     *  \param request The http request.
     *  \param action the action performed
     */
    void OnlinePlayerProfile::setUserDetails(std::shared_ptr<HTTPRequest> request,
                                             const std::string &action,
                                             const std::string &url_path) const
    {
        if (url_path.size())
        {
            request->setApiURL(url_path, action);
        }
        else // default path
        {
            request->setApiURL(API::USER_PATH, action);
        }

        if (m_profile)
            request->addParameter("userid", m_profile->getID());
        if (m_online_state == OS_SIGNED_IN)
            request->addParameter("token", m_token);
    }   // setUserDetails

    // ========================================================================
    OnlinePlayerProfile::OnlinePlayerProfile(const XMLNode *player)
               : PlayerProfile(player)
    {
        m_online_state = OS_SIGNED_OUT;
        m_token        = "";
        m_profile      = NULL;
    }   // OnlinePlayerProfile

    // ------------------------------------------------------------------------
    OnlinePlayerProfile::OnlinePlayerProfile(const core::stringw &name, bool is_guest)
        : PlayerProfile(name, is_guest)
    {
        m_online_state = OS_SIGNED_OUT;
        m_token        = "";
        m_profile      = NULL;

    }   // OnlinePlayerProfile

    // ------------------------------------------------------------------------
    /** Request a login using the saved credentials of the user. */
    void OnlinePlayerProfile::requestSavedSession()
    {
        if (m_online_state == OS_SIGNED_OUT && hasSavedSession())
        {
            auto request = std::make_shared<PrivateRequest::SignInRequest>();
            setUserDetails(request, "saved-session");

            // The userid must be taken from the saved data,
            // setUserDetails takes it from current data.
            request->addParameter("userid", getSavedUserId());
            request->addParameter("token",  getSavedToken() );
            request->queue();
            m_online_state = OS_SIGNING_IN;
        }
    }   // requestSavedSession

    // ------------------------------------------------------------------------
    /** Create a signin request.
     *  \param username Name of user.
     *  \param password Password.
     */
    void OnlinePlayerProfile::requestSignIn(const core::stringw &username,
                                           const core::stringw &password)
    {
        // If the player changes the online account, there can be a
        // logout stil happening.
        assert(m_online_state == OS_SIGNED_OUT ||
               m_online_state == OS_SIGNING_OUT);
        auto request = std::make_shared<PrivateRequest::SignInRequest>();

        // We can't use setUserDetail here, since there is no token yet
        request->setApiURL(API::USER_PATH, "connect");
        request->addParameter("username", username);
        request->addParameter("password", password);
        request->addParameter("save-session",
                              rememberPassword() ? "true"
                                                 : "false");
        request->queue();
        m_online_state = OS_SIGNING_IN;
    }   // requestSignIn

    // ------------------------------------------------------------------------
    /** Called when the signin request is finished.
     */
    void PrivateRequest::SignInRequest::callback()
    {
        PlayerManager::getCurrentPlayer()->signIn(isSuccess(), getXMLData());
        GUIEngine::Screen *screen = GUIEngine::getCurrentScreen();

        // If the login is successful, reset any saved session of other
        // local players using the same online account (which are now invalid)
        if (isSuccess())
        {
            PlayerProfile *current = PlayerManager::getCurrentPlayer();
            for (unsigned int i = 0; i < PlayerManager::get()->getNumPlayers(); i++)
            {
                PlayerProfile *player = PlayerManager::get()->getPlayer(i);
                if(player != current &&
                    player->hasSavedSession() &&
                    player->getLastOnlineName() == current->getLastOnlineName())
                {
                    player->clearSession();
                }
            }
        }

        // Test if failure while showing user login screen
        BaseUserScreen *login = dynamic_cast<BaseUserScreen*>(screen);
        if (login)
        {
            if(isSuccess())
                login->loginSuccessful();
            else
                login->loginError(getInfo(), !hadDownloadError());
        }   // if dialog

        // Check if failure happened during automatic (saved) signin.
        else if (!isSuccess())
        {
            if (GUIEngine::getCurrentScreen() != MainMenuScreen::getInstance() ||
                GUIEngine::ModalDialog::isADialogActive())
            {
                // User has already opened another menu, so use message queue
                // to inform user that login failed.
                // Same thing if a dialog is active, can't navigate to other
                // screen when a dialog is active
                MessageQueue::add(MessageQueue::MT_ERROR, getInfo());
                return;
            }

            // User still at main menu screen, push user screen. Note that
            // this function is called from the main thread, so we can 
            // push screens without synchronisations.
            UserScreen::getInstance()->push();
            UserScreen::getInstance()->loginError(getInfo(),
                !hadDownloadError());
        }


    }   // SignInRequest::callback

    // ------------------------------------------------------------------------
    /** Checks the server respond after a login attempt. If the login
     *  was successful, it marks the user as logged in, and (if requested)
     *  saves data to be able to login next time.
     *  \param success If the answer from the server indicated a
     *         successful login attemp.
     *  \param input Xml tree with the complete server response.
     */
    void OnlinePlayerProfile::signIn(bool success, const XMLNode * input)
    {
        if (success)
        {
            core::stringw username("");
            uint32_t userid(0);

#ifdef DEBUG
            int token_fetched       = input->get("token", &m_token);
            int username_fetched    = input->get("username", &username);
            int userid_fetched      = input->get("userid", &userid);
            assert(token_fetched && username_fetched && userid_fetched);
#else
            input->get("token", &m_token);
            input->get("username", &username);
            input->get("userid", &userid);
#endif
            if (NetworkConfig::get()->getServerIdFile().empty())
            {
                NetworkConfig::get()->setCurrentUserId(userid);
                NetworkConfig::get()->setCurrentUserToken(m_token);
            }
            setLastOnlineName(username);

            OnlineProfile* profile = new OnlineProfile(userid, username, true);
            // Note that addPersistent might decide to merge profile with an
            // existing profile, and then delete profile. Only the returned
            // pointer is save to use.
            m_profile = ProfileManager::get()->addPersistent(profile);
            m_online_state = OS_SIGNED_IN;
            if(rememberPassword())
            {
                setWasOnlineLastTime(true);
                saveSession(getOnlineId(), getToken());
            }

            std::string achieved_string("");

            // Even if no achievements were sent, we have to call sync
            // in order to upload local achievements to the server
            input->get("achieved", &achieved_string);
            std::vector<uint32_t> achieved_ids =
                            StringUtils::splitToUInt(achieved_string, ' ');
            PlayerManager::getCurrentAchievementsStatus()->sync(achieved_ids);
            m_profile->fetchFriends();
        }   // if success
        else
        {
            m_online_state = OS_SIGNED_OUT;
        }
    }   // signIn

    // ------------------------------------------------------------------------
    /** Requests a sign out from the server. If the user should be remembered,
     *  a 'client-quit' request is sent (which will log the user out, but
     *  remember the token), otherwise a 'disconnect' is sent.
     */
    void OnlinePlayerProfile::requestSignOut()
    {
        assert(m_online_state == OS_SIGNED_IN || m_online_state == OS_GUEST);
        // ----------------------------------------------------------------
        class SignOutRequest : public XMLRequest
        {
        private:
            PlayerProfile *m_player;
            virtual void callback()
            {
                m_player->signOut(isSuccess(), getXMLData(), getInfo());
            }
        public:
            /** Sign out request, which have the highest priority (same as
             *  quit-stk request). This allows the final logout request at
             *  the end of STK to be handled, even if a quit request gets
             *  added (otherwise if quit has higher priority, the quit can
             *  be executed before signout, resulting in players not being
             *  logged out properly). It also guarantees that the logout
             *  happens before a following logout.
             */
            SignOutRequest(PlayerProfile *player)
                        : XMLRequest(/*priority*/RequestManager::HTTP_MAX_PRIORITY)
            {
                m_player = player;
                setAbortable(false);
            }   // SignOutRequest
        };   // SignOutRequest
        // ----------------------------------------------------------------

        auto request = std::make_shared<SignOutRequest>(this);
        setUserDetails(request,
            rememberPassword() ? "client-quit" : "disconnect");
        request->queue();
        m_online_state = OS_SIGNING_OUT;
    }   // requestSignOut

    // ------------------------------------------------------------------------
    /** Callback once the logout event has been processed.
     *  \param success If the request was successful.
     *  \param input
     */
    void OnlinePlayerProfile::signOut(bool success, const XMLNode *input,
                                      const irr::core::stringw &info)
    {
        GUIEngine::Screen *screen = GUIEngine::getCurrentScreen();
        BaseUserScreen *user_screen = dynamic_cast<BaseUserScreen*>(screen);

        // We can't do much of error handling here, no screen waits for
        // a logout to finish, so we can only log the message to screen,
        // and otherwise mark the player logged out internally.
        if (!success)
        {
            Log::warn("OnlinePlayerProfile::signOut",
                      "There were some connection issues while logging out. "
                      "Report a bug if this caused issues.");
            Log::warn("OnlinePlayerProfile::signOut", core::stringc(info.c_str()).c_str());
            if (user_screen)
                user_screen->logoutError(info);
        }
        else
        {
            if (user_screen)
                user_screen->logoutSuccessful();
        }

        ProfileManager::get()->clearPersistent();
        m_profile = NULL;
        m_online_state = OS_SIGNED_OUT;

        // Discard token if session should not be saved.
        if (!rememberPassword())
            clearSession();
    }   // signOut

    // ------------------------------------------------------------------------
    /** Sends a request to the server to see if any new information is
     *  available. (online friends, notifications, etc.).
     */
    void OnlinePlayerProfile::requestPoll() const
    {
        assert(m_online_state == OS_SIGNED_IN);

        auto request = std::make_shared<PrivateRequest::PollRequest>();
        setUserDetails(request, "poll");
        request->queue();
    }   // requestPoll()

    // ------------------------------------------------------------------------
    PrivateRequest::PollRequest::PollRequest()
                       : XMLRequest()
    {
    }   // PollRequest

    // ------------------------------------------------------------------------
    /** Callback for the poll request. Parses the information and spawns
     *  notifications accordingly.
     */
    void PrivateRequest::PollRequest::callback()
    {
        // connection error
        if (!isSuccess())
        {
            Log::error("Online Player Profile", "Poll request failed");
            return;
        }

        if (!PlayerManager::getCurrentPlayer()->isLoggedIn())
            return;
        float f;
        if(getXMLData()->get("menu-polling-interval", &f))
            RequestManager::get()->setMenuPollingInterval(f);
        if(getXMLData()->get("game-polling-interval", &f))
            RequestManager::get()->setGamePollingInterval(f);

        if (PlayerManager::getCurrentPlayer()->getProfile()->hasFetchedFriends())
        {
            std::string online_friends_string("");
            if (getXMLData()->get("online", &online_friends_string) == 1)
            {
                std::vector<uint32_t> online_friends =
                      StringUtils::splitToUInt(online_friends_string, ' ');

                // flag that indicates if a current online friend went offline
                bool went_offline = false;

                // iterate over all friends and find out if they come online or not
                // filling the notification messages
                std::vector<uint32_t> friends =
                    PlayerManager::getCurrentPlayer()->getProfile()->getFriends();
                std::vector<core::stringw> to_notify;
                for (unsigned int i = 0; i < friends.size(); ++i)
                {
                     bool now_online = false;
                     std::vector<uint32_t>::iterator found_friend =
                         std::find(online_friends.begin(),
                                   online_friends.end(), friends[i]);
                     if (found_friend != online_friends.end())
                     {
                         now_online = true;
                         online_friends.erase(found_friend);
                     }

                     OnlineProfile * profile =
                         ProfileManager::get()->getProfileByID(friends[i]);
                     OnlineProfile::RelationInfo * relation_info =
                                                profile->getRelationInfo();

                     if (relation_info && relation_info->isOnline())
                     {
                         if (!now_online) // the friend went offline
                         {
                             relation_info->setOnline(false);
                             went_offline = true;
                         }
                     }
                     else if (relation_info)
                     {
                         if (now_online) // friend came online
                         {
                             relation_info->setOnline(true);

                             // Do this because a user might have accepted
                             // a pending friend request.
                             profile->setFriend();
                             to_notify.push_back(profile->getUserName());
                         }
                     }
                }

                if (to_notify.size() > 0)
                {
                    core::stringw message("");
                    if(to_notify.size() == 1)
                    {
                        message = _("%s is now online.", to_notify[0]);
                    }
                    else if(to_notify.size() == 2)
                    {
                        message = _("%s and %s are now online.",
                                    to_notify[0], to_notify[1]    );
                    }
                    else if(to_notify.size() == 3)
                    {
                        message = _("%s, %s and %s are now online.",
                                 to_notify[0], to_notify[1], to_notify[2]);
                    }
                    else if(to_notify.size() > 3)
                    {
                        //I18N: Only used for count > 3
                        message = _P("%d friend is now online.",
                                     "%d friends are now online.",
                                     (int)to_notify.size());
                    }
                    MessageQueue::add(MessageQueue::MT_FRIEND, message);
                }
                else if (went_offline)
                {
                    OnlineProfileFriends::getInstance()->refreshFriendsList();
                }
            }

            std::map<uint32_t, core::stringw> friend_server_map;
            for (unsigned int i = 0; i < getXMLData()->getNumNodes(); i++)
            {
                const XMLNode * node = getXMLData()->getNode(i);
                if (node->getName() == "friend-in-server")
                {
                    uint32_t id = 0;
                    std::string server_name;
                    node->get("id", &id);
                    node->get("name", &server_name);
                    friend_server_map[id] =
                        StringUtils::xmlDecode(server_name);
                }
            }
            std::vector<std::pair<core::stringw, core::stringw> >
                friend_server_notify;
            std::map<uint32_t, core::stringw>& cur_friend_server_map =
                PlayerManager::getCurrentPlayer()->getProfile()
                ->getFriendServerMap();
            for (auto& p : friend_server_map)
            {
                if (friend_server_notify.size() > 3)
                    break;
                uint32_t uid = p.first;
                const core::stringw& server_name = p.second;
                if ((cur_friend_server_map.find(uid) ==
                    cur_friend_server_map.end() ||
                    cur_friend_server_map.at(uid) != server_name) &&
                    ProfileManager::get()->getProfileByID(uid) != NULL)
                {
                    friend_server_notify.emplace_back(ProfileManager::get()
                        ->getProfileByID(uid)->getUserName(), server_name);
                }
            }
            for (auto& p : friend_server_notify)
            {
                // I18N: Tell your friend if he is on any server in game
                core::stringw message = _("%s is now on server \"%s\".",
                    p.first, p.second);
                MessageQueue::add(MessageQueue::MT_FRIEND, message);
            }
            std::swap(cur_friend_server_map, friend_server_map);
        }
        else
        {
            PlayerManager::getCurrentPlayer()->getProfile()->fetchFriends();
        }

        int friend_request_count = 0;
        for (unsigned int i = 0; i < getXMLData()->getNumNodes(); i++)
        {
            const XMLNode * node = getXMLData()->getNode(i);
            if (node->getName() == "new_friend_request")
            {
                OnlineProfile::RelationInfo * ri =
                    new OnlineProfile::RelationInfo("New", false, true, true);
                OnlineProfile * p = new OnlineProfile(node);
                p->setRelationInfo(ri);
                ProfileManager::get()->addPersistent(p);
                friend_request_count++;
            }
        }

        if (friend_request_count > 0)
        {
            core::stringw message("");
            if (friend_request_count > 1)
            {
                message = _P("You have %d new friend request!",
                             "You have %d new friend requests!",
                             friend_request_count);
            }
            else
            {
                message = _("You have a new friend request!");
            }

            MessageQueue::add(MessageQueue::MT_FRIEND, message);
            OnlineProfileFriends::getInstance()->refreshFriendsList();
            PlayerManager::getCurrentPlayer()->getProfile()
                ->unsetHasFetchedFriends();
        }
    }   // PollRequest::callback

    // ------------------------------------------------------------------------
    /** \return the online id, or 0 if the user is not signed in.
     */
    uint32_t OnlinePlayerProfile::getOnlineId() const
    {
        if (m_online_state == OS_SIGNED_IN)
        {
            assert(m_profile != NULL);
            return m_profile->getID();
        }

        return 0;
    }   // getOnlineId

} // namespace Online

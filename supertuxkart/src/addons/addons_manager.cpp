//  SuperTuxKart - a fun racing game with go-kart
//  Copyright (C) 2010-2015 Lucas Baudin
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
/**
  \page addons Addons
  */

#ifndef SERVER_ONLY

#include "addons/addons_manager.hpp"

#include "addons/news_manager.hpp"
#include "addons/zip.hpp"
#include "config/user_config.hpp"
#include "io/file_manager.hpp"
#include "io/xml_node.hpp"
#include "karts/kart_properties.hpp"
#include "karts/kart_properties_manager.hpp"
#include "online/http_request.hpp"
#include "online/request_manager.hpp"
#include "states_screens/dialogs/addons_pack.hpp"
#include "states_screens/kart_selection.hpp"
#include "tracks/track.hpp"
#include "tracks/track_manager.hpp"
#include "utils/file_utils.hpp"
#include "utils/string_utils.hpp"
#include "utils/translation.hpp"

#include <fstream>
#include <iostream>
#include <map>
#include <sstream>
#include <string.h>
#include <vector>

using namespace Online;

AddonsManager* addons_manager = 0;

// ----------------------------------------------------------------------------
/** Initialises the non-online component of the addons manager (i.e. handling
 *  the list of already installed addons). The online component is initialised
 *  later from a separate thread started from the news manager (see
 *  NewsManager::init()  ).
 */
AddonsManager::AddonsManager() : m_addons_list(std::vector<Addon>() ),
                                 m_state(STATE_INIT)
{
    m_has_new_addons = false;
    m_downloaded_icons = false;
    // Clean .part file which may be left behind
    std::string addons_part = file_manager->getAddonsFile("addons.xml.part");
    if (file_manager->fileExists(addons_part))
        file_manager->removeFile(addons_part);

    m_file_installed = file_manager->getAddonsFile("addons_installed.xml");

    // Load the addons list (even if internet is disabled)
    m_addons_list.lock();
    // Clear the list in case that a reinit is being done.
    m_addons_list.getData().clear();
    loadInstalledAddons();
    m_addons_list.unlock();
}   // AddonsManager

// ----------------------------------------------------------------------------
/** The destructor saves the installed addons file again. This is necessary
 *  so that information about downloaded icons is saved for the next run.
 */
AddonsManager::~AddonsManager()
{
    saveInstalled();
}   // ~AddonsManager

// ----------------------------------------------------------------------------
/** This init function is called from a separate thread (started in
 *  news_manager, since the news.xml file contains the address of the
 *  addons.xml URL).
 *  \param xml The news.xml file, which inclues the data about the addons.xml
 *         file (in the 'include' node).
 *  \param force_refresh Download addons.xml, even if the usual waiting period
 *         between downloads hasn't passed yet.
 */
void AddonsManager::init(const XMLNode *xml,
                         bool force_refresh)
{
    m_has_new_addons = false;
    std::string    addon_list_url("");
    StkTime::TimeType mtime(0);
    const XMLNode *include = xml->getNode("include");
    std::string filename=file_manager->getAddonsFile("addons.xml");
    // Prevent downloading when .part file created, which is already downloaded
    std::string filename_part=file_manager->getAddonsFile("addons.xml.part");
    if(!include)
    {
        file_manager->removeFile(filename);
        setErrorState();
        NewsManager::get()->addNewsMessage(_("Failed to connect to the SuperTuxKart add-ons server."));
        return;
    }

    include->get("file",  &addon_list_url);
    int frequency = 0;
    include->get("frequency", &frequency);

    int64_t tmp;
    include->get("mtime", &tmp);
    mtime = tmp;

    bool download =
        ( mtime > UserConfigParams::m_addons_last_updated +frequency ||
          force_refresh                                              ||
          !file_manager->fileExists(filename)                          )
       && UserConfigParams::m_internet_status == RequestManager::IPERM_ALLOWED
       && !file_manager->fileExists(filename_part);

    if (download)
    {
        Log::info("addons", "Downloading updated addons.xml.");
        auto download_request = std::make_shared<Online::HTTPRequest>("addons.xml");
        download_request->setURL(addon_list_url);
        download_request->executeNow();
        if(download_request->hadDownloadError())
        {
            Log::error("addons", "Error on download addons.xml: %s.",
                       download_request->getDownloadErrorMessage());
            return;
        }
        UserConfigParams::m_addons_last_updated=StkTime::getTimeSinceEpoch();
    }
    else
        Log::info("addons", "Using cached addons.xml.");

    const XMLNode* xml_addons = NULL;
    try
    {
        xml_addons = new XMLNode(filename);
    }
    catch (std::exception& e)
    {
        Log::error("addons", "Error %s", e.what());
    }
    if (!xml_addons)
        return;
    addons_manager->initAddons(xml_addons);   // will free xml_addons
    if(UserConfigParams::logAddons())
        Log::info("addons", "Addons manager list downloaded.");
}   // init

// ----------------------------------------------------------------------------
/** This initialises the online portion of the addons manager. It uses the
 *  downloaded list of available addons. It is called from init(), which is
 *  called from a separate thread, so blocking download requests can be used
 *  without blocking the GUI. This function will update the state variable.
 *  \param xml The xml tree of addons.xml with information about all available
 *         addons.
 */
void AddonsManager::initAddons(const XMLNode *xml)
{
    m_addons_list.lock();
    // Clear the list in case that a reinit is being done.
    m_addons_list.getData().clear();
    loadInstalledAddons();
    m_addons_list.unlock();

    for(unsigned int i=0; i<xml->getNumNodes(); i++)
    {
        const XMLNode *node = xml->getNode(i);
        const std::string &name = node->getName();
        // Ignore news/redirect, which is handled by the NewsManager
        if(name=="include" || name=="message")
            continue;
        if(node->getName()=="track" || node->getName()=="kart" ||
            node->getName()=="arena"                                 )
        {
            Addon addon(*node);
            if (addon.testStatus(Addon::AS_APPROVED) &&
                addon.getDate() > UserConfigParams::m_latest_addon_time)
            {
                m_has_new_addons = true;
                UserConfigParams::m_latest_addon_time = addon.getDate();
            }
            int index = getAddonIndex(addon.getId());

            int stk_version=0;
            node->get("format", &stk_version);
            int   testing=-1;
            node->get("testing", &testing);

            bool wrong_version=false;

            if(addon.getType()=="kart")
                wrong_version = stk_version <stk_config->m_min_kart_version ||
                                stk_version >stk_config->m_max_kart_version   ;
            else
                wrong_version = stk_version <stk_config->m_min_track_version ||
                                stk_version >stk_config->m_max_track_version   ;
            // If the add-on is included, behave like it is a wrong version
            if (addon.testIncluded(addon.getMinIncludeVer(), addon.getMaxIncludeVer()))
                wrong_version = true;

            // Check which version to use: only for this stk version,
            // and not addons that are marked as hidden (testing=0)
            if(wrong_version|| testing==0)
            {
                // If the version is too old (e.g. after an update of stk)
                // remove a cached icon.
                std::string full_path =
                    file_manager->getAddonsFile("icons/"
                                                +addon.getIconBasename());
                if(file_manager->fileExists(full_path))
                {
                    if(UserConfigParams::logAddons())
                        Log::warn("addons", "Removing cached icon '%s'.",
                               addon.getIconBasename().c_str());
                    file_manager->removeFile(full_path);
                }
                continue;
            }

            m_addons_list.lock();
            if(index>=0)
            {
                Addon& tmplist_addon = m_addons_list.getData()[index];

                // Only copy the data if a newer revision is found (ignore unapproved
                // revisions unless player is in the mode to see them)
                if (tmplist_addon.getRevision() < addon.getRevision() &&
                    (addon.testStatus(Addon::AS_APPROVED) || UserConfigParams::m_artist_debug_mode))
                {
                    m_addons_list.getData()[index].copyInstallData(addon);
                }
            }
            else
            {
                m_addons_list.getData().push_back(addon);
                index = (int) m_addons_list.getData().size()-1;
            }
            // Mark that this addon still exists on the server
            m_addons_list.getData()[index].setStillExists();
            m_addons_list.unlock();
        }
        else
        {
            Log::error("addons", "Found invalid node '%s' while downloading addons.",
                    node->getName().c_str());
            Log::error("addons", "Ignored.");
        }
    }   // for i<xml->getNumNodes
    delete xml;

    // Now remove all items from the addons-installed list, that are not
    // on the server anymore (i.e. not in the addons.xml file), and not
    // installed. If found, remove the icon cached for this addon.
    // Note that if (due to a bug) an icon is shared (i.e. same icon on
    // an addon that's still on the server and an invalid entry in the
    // addons installed file), it will be re-downloaded later.
    m_addons_list.lock();
    unsigned int count = (unsigned int) m_addons_list.getData().size();

    for(unsigned int i=0; i<count;)
    {
        // if installed addon needs an update, set flag
        if (m_addons_list.getData()[i].isInstalled() &&
            m_addons_list.getData()[i].needsUpdate())
        {
            m_has_new_addons = true;
        }
        if(m_addons_list.getData()[i].getStillExists() ||
            m_addons_list.getData()[i].isInstalled())
        {
            i++;
            continue;
        }
        // This addon is not on the server anymore, and not installed. Remove
        // it from the list.
        if(UserConfigParams::logAddons())
            Log::warn(
                "addons", "Removing '%s' which is not on the server anymore.",
                m_addons_list.getData()[i].getId().c_str() );
        const std::string &icon = m_addons_list.getData()[i].getIconBasename();
        std::string icon_file =file_manager->getAddonsFile("icons/"+icon);
        if(file_manager->fileExists(icon_file))
        {
            file_manager->removeFile(icon_file);
            // Ignore errors silently.
        }
        m_addons_list.getData()[i] = m_addons_list.getData()[count-1];
        m_addons_list.getData().pop_back();
        count--;
    }
    m_addons_list.unlock();

    for (unsigned int i = 0; i < m_addons_list.getData().size(); i++)
    {
        Addon& addon = m_addons_list.getData()[i];
        const std::string& icon = addon.getIconBasename();
        const std::string& icon_full =
            file_manager->getAddonsFile("icons/" + icon);
        if (!addon.iconNeedsUpdate() && file_manager->fileExists(icon_full))
            addon.setIconReady();
    }   // for i < m_addons_list.size()

    m_state.setAtomic(STATE_READY);
}   // initAddons

// ----------------------------------------------------------------------------
/** Reinitialises the addon manager, which happens when the user selects
 *  'reload' in the addon manager.
 */
void AddonsManager::reInit()
{
    m_state.setAtomic(STATE_INIT);
}   // reInit

// ----------------------------------------------------------------------------
/** This function checks if the information in the installed addons file is
 *  consistent with what is actually available. This avoids e.g. that an
 *  addon is installed, but not marked here (and therefore shows up as
 *  not installed in the addons GUI), see bug #455.
 */
void AddonsManager::checkInstalledAddons()
{
    bool something_was_changed = false;

    // Lock the whole addons list to make sure a consistent view is
    // written back to disk. The network thread might still be
    // downloading icons and modify content
    m_addons_list.lock();

    // First karts
    // -----------
    for(unsigned int i=0; i<kart_properties_manager->getNumberOfKarts(); i++)
    {
        const KartProperties *kp = kart_properties_manager->getKartById(i);
        const std::string &dir=kp->getKartDir();
        if(dir.find(file_manager->getAddonsDir())==std::string::npos)
            continue;
        int n = getAddonIndex(kp->getIdent());
        if(n<0) continue;
        if(!m_addons_list.getData()[n].isInstalled())
        {
            Log::info("addons", "Marking '%s' as being installed.",
                   kp->getIdent().c_str());
            m_addons_list.getData()[n].setInstalled(true);
            something_was_changed = true;
        }
    }

    // Then tracks
    // -----------
    for(unsigned int i=0; i<track_manager->getNumberOfTracks(); i++)
    {
        const Track *track = track_manager->getTrack(i);
        const std::string &dir=track->getFilename();
        if(dir.find(file_manager->getAddonsDir())==std::string::npos)
            continue;
        int n = getAddonIndex(track->getIdent());
        if(n<0) continue;
        if(!m_addons_list.getData()[n].isInstalled())
        {
            Log::info("addons", "Marking '%s' as being installed.",
                   track->getIdent().c_str());
            m_addons_list.getData()[n].setInstalled(true);
            something_was_changed = true;
        }
    }
    if(something_was_changed)
        saveInstalled();
    m_addons_list.unlock();
}   // checkInstalledAddons

// ----------------------------------------------------------------------------
/** Download icon for specific addon */
void AddonsManager::downloadIconForAddon(const std::string& addon_id,
                                         std::weak_ptr<bool> result)
{
    Addon* addon = getAddon(addon_id);
    if (!addon)
        return;
    const std::string &icon = addon->getIconBasename();
    const std::string &icon_full =
        file_manager->getAddonsFile("icons/" + icon);
    if (addon->iconNeedsUpdate() ||
        !file_manager->fileExists(icon_full))
    {
        const std::string& url = addon->getIconURL();
        const std::string& icon = addon->getIconBasename();
        if (icon.empty())
        {
            if (UserConfigParams::logAddons())
            {
                Log::error("addons", "No icon or image specified for '%s'.",
                    addon->getId().c_str());
            }
            return;
        }
        // A simple class that will notify the addon via a callback
        class IconRequest : public Online::HTTPRequest
        {
            std::weak_ptr<bool> m_result;
            Addon* m_addon;  // stores this addon object
            void callback()
            {
                m_addon->setIconReady();
                if (std::shared_ptr<bool> result = m_result.lock())
                    *result = true;
                if (!hadDownloadError())
                    addons_manager->m_downloaded_icons = true;
            }   // callback
        public:
            IconRequest(const std::string& filename,
                        const std::string& url,
                        Addon* addon, std::weak_ptr<bool> result)
                : HTTPRequest(filename,/*priority*/1)
            {
                m_addon = addon;
                m_result = result;
                setURL(url);
            }   // IconRequest
        };
        auto r =
            std::make_shared<IconRequest>("icons/"+icon, url, addon, result);
        r->queue();
    }
}   // downloadIconForAddon

// ----------------------------------------------------------------------------
/** Loads the installed addons from .../addons/addons_installed.xml.
 */
void AddonsManager::loadInstalledAddons()
{
    /* checking for installed addons */
    if(UserConfigParams::logAddons())
    {
        Log::info("addons", "Loading an xml file for installed addons: %s.",
                    m_file_installed.c_str());
    }
    auto xml = std::unique_ptr<XMLNode>(file_manager->createXMLTree(m_file_installed));
    if(!xml)
        return;

    for(unsigned int i=0; i<xml->getNumNodes(); i++)
    {
        const XMLNode *node=xml->getNode(i);
        if(node->getName()=="kart"   || node->getName()=="arena" ||
            node->getName()=="track"    )
        {
            Addon addon(*node);
            m_addons_list.getData().push_back(addon);
        }
    }   // for i <= xml->getNumNodes()
}   // loadInstalledAddons

// ----------------------------------------------------------------------------
/** Returns an addon with a given id. Raises an assertion if the id is not
 *  found!
 *  \param id The id to search for.
 */
Addon* AddonsManager::getAddon(const std::string &id)
{
    int i = getAddonIndex(id);
    return (i<0) ? NULL : &(m_addons_list.getData()[i]);
}   // getAddon

// ----------------------------------------------------------------------------
/** Returns the index of the addon with the given id, or -1 if no such
 *  addon exist.
 *  \param id The (unique) identifier of the addon.
 */
int AddonsManager::getAddonIndex(const std::string &id) const
{
    for(unsigned int i = 0; i < m_addons_list.getData().size(); i++)
    {
        if(m_addons_list.getData()[i].getId()== id)
        {
            return i;
        }
    }
    return -1;
}   // getAddonIndex
// ----------------------------------------------------------------------------
bool AddonsManager::anyAddonsInstalled() const
{
    for(unsigned int i=0; i<m_addons_list.getData().size(); i++)
        if(m_addons_list.getData()[i].isInstalled())
            return true;
    return false;
}   // anyAddonsInstalled

// ----------------------------------------------------------------------------
/** Installs or updates (i.e. remove old and then install a new) an addon.
 *  It checks for the directories and then unzips the file (which must already
 *  have been downloaded).
 *  \param addon Addon data for the addon to install.
 *  \return true if installation was successful.
 */
bool AddonsManager::install(const Addon &addon)
{

    //extract the zip in the addons folder called like the addons name
    std::string base_name = StringUtils::getBasename(addon.getZipFileName());
    std::string from      = file_manager->getAddonsFile("tmp/"+base_name);
    std::string to        = addon.getDataDir();

    // Remove old addon first (including non official way to install addons)
    AddonsPack::uninstallByName(addon.getDirName(), true/*force_clear*/);
    if (file_manager->isDirectory(to))
        file_manager->removeDirectory(to);

    file_manager->checkAndCreateDirForAddons(to);

    bool success = extract_zip(from, to, true/*recursive*/);
    if (!success)
    {
        // TODO: show a message in the interface
        Log::error("addons", "Failed to unzip '%s' to '%s'.",
                    from.c_str(), to.c_str());
    }

    if(!file_manager->removeFile(from))
    {
        Log::error("addons", "Problems removing temporary file '%s'.",
                    from.c_str());
    }
    if (!success)
        return false;

    int index = getAddonIndex(addon.getId());
    assert(index>=0 && index < (int)m_addons_list.getData().size());
    m_addons_list.getData()[index].setInstalled(true);

    if(addon.getType()=="kart")
    {
        // We have to remove the mesh of the kart since otherwise it remains
        // cashed (if a kart is updated), and will therefore be found again
        // when reloading the karts. This is important on one hand since we
        // reload all karts (this function is easily available) and existing
        // karts will not reload their meshes.
        const KartProperties *prop =
            kart_properties_manager->getKart(addon.getId());
        // If the model already exist, first remove the old kart
        if(prop)
            kart_properties_manager->removeKart(addon.getId());
        kart_properties_manager->loadKart(addon.getDataDir());
    }
    else if (addon.getType()=="track" || addon.getType()=="arena")
    {
        Track *track = track_manager->getTrack(addon.getId());
        if(track)
            track_manager->removeTrack(addon.getId());

        try
        {
            track_manager->loadTrack(addon.getDataDir());
        }
        catch (std::exception& e)
        {
            Log::error("addons", "Cannot load track '%s' : %s.",
                        addon.getDataDir().c_str(), e.what());
        }
    }
    // if we have installed/updated at least one addon
    // we remove the notification in main menu
    m_has_new_addons = false;
    // check if there are still addons that need an update
    for (unsigned int i=0; i<addons_manager->getNumAddons() && !m_has_new_addons; i++)
    {
        const Addon & addon = addons_manager->getAddon(i);
        if (addon.isInstalled() && addon.needsUpdate())
            m_has_new_addons = true;
    }
    saveInstalled();
    return true;
}   // install

// ----------------------------------------------------------------------------
/** Removes all files froma login.
 *  \param addon The addon to be removed.
 *  \return True if uninstallation was successful.
 */
bool AddonsManager::uninstall(const Addon &addon)
{
    Log::info("addons", "Uninstalling '%s'.",
               core::stringc(addon.getName()).c_str());

    // addon is a const reference, and to avoid removing the const, we
    // find the proper index again to modify the installed state
    int index = getAddonIndex(addon.getId());
    assert(index>=0 && index < (int)m_addons_list.getData().size());
    m_addons_list.getData()[index].setInstalled(false);

    //remove the addons directory
    bool error = false;
    // if the user deleted the data directory for an add-on with
    // filesystem tools, removeTrack/removeKart will trigger an assert
    // because the kart/track was never added in the first place
    if (file_manager->fileExists(addon.getDataDir()))
    {
        error = !file_manager->removeDirectory(addon.getDataDir());

        // Even if an error happened when removing the data files
        // still remove the addon, since it is unknown if e.g. only
        // some files were successfully removed. Since we can not
        // know if the addon is still functional, better remove it.
        // On the other hand, in case of a problem the user will have
        // the option in the GUI to try again. In this case
        // removeTrack/Kart would not find the addon and assert. So
        // check first if the track is still known.
        if(addon.getType()=="kart")
        {
            if(kart_properties_manager->getKart(addon.getId()))
               kart_properties_manager->removeKart(addon.getId());
        }
        else if(addon.getType()=="track" || addon.getType()=="arena")
        {
            if(track_manager->getTrack(addon.getId()))
               track_manager->removeTrack(addon.getId());
        }
    }
    saveInstalled();
    return !error;
}   // uninstall

// ----------------------------------------------------------------------------
/** Saves the information about installed addons and cached icons to
 *  addons_installed.xml. If this is not called, information about downloaded
 *  icons is lost (and will trigger a complete redownload when STK is started
 *  next time).
 */
void AddonsManager::saveInstalled()
{
    // Put the addons in the xml file
    // Manually because the irrlicht xml writer doesn't seem finished, FIXME ?
    std::ofstream xml_installed(
        FileUtils::getPortableWritingPath(m_file_installed));

    // Write the header of the xml file
    xml_installed << "<?xml version=\"1.0\"?>" << std::endl;

    // Get server address from config
    const std::string server = stk_config->m_server_addons;

    // Find the third slash (end of the domain)
    std::string::size_type index = server.find('/');
    index = server.find('/', index + 2) + 1; // Omit one slash
    xml_installed << "<addons  xmlns='" << server.substr(0, index) << "'>"
                    << std::endl;

    for(unsigned int i = 0; i < m_addons_list.getData().size(); i++)
    {
        m_addons_list.getData()[i].writeXML(&xml_installed);
    }
    xml_installed << "</addons>" << std::endl;
    xml_installed.close();
    m_downloaded_icons = false;
}   // saveInstalled

#endif

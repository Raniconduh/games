//
//  SuperTuxKart - a fun racing game with go-kart
//  Copyright (C) 2010-2015 SuperTuxKart-Team
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

#include "states_screens/grand_prix_win.hpp"

#include "audio/music_manager.hpp"
#include "audio/sfx_manager.hpp"
#include "challenges/unlock_manager.hpp"
#include "config/player_manager.hpp"
#include "config/stk_config.hpp"
#include "graphics/irr_driver.hpp"
#include "graphics/lod_node.hpp"
#include <ge_render_info.hpp>
#include "guiengine/engine.hpp"
#include "guiengine/scalable_font.hpp"
#include "guiengine/widgets/button_widget.hpp"
#include "guiengine/widgets/label_widget.hpp"
#include "io/file_manager.hpp"
#include "items/item_manager.hpp"
#include "karts/kart_properties.hpp"
#include "karts/kart_properties_manager.hpp"
#include "modes/cutscene_world.hpp"
#include "modes/world.hpp"
#include "race/race_manager.hpp"
#include "states_screens/feature_unlocked.hpp"
#include "states_screens/state_manager.hpp"
#include "tracks/track.hpp"
#include "tracks/track_object.hpp"
#include "tracks/track_object_manager.hpp"
#include "utils/string_utils.hpp"
#include "utils/translation.hpp"

#include <ICameraSceneNode.h>
#include <IGUIEnvironment.h>
#include <IGUIImage.h>
#include <ILightSceneNode.h>
#include <IMeshSceneNode.h>
#include <ISceneManager.h>
#include <SColor.h>

#include <iostream>


using namespace irr::core;
using namespace irr::gui;
using namespace irr::video;
using namespace GUIEngine;

/*
 * FIXME: Some of these are not yet fully implemented
 */

// { 2nd, 1st, 3rd }

// The hight of the podiums' tops above y=0, used to put the kart on top
const float PODIUMS_HEIGHT_FROM_Y0 = 0.78f;

// The hight of the podiums
const float PODIUMS_HEIGHT = 1.56f;

// Initial locations and rotations (Begin Stage 1)
const float KARTS_AND_PODIUMS_INITIAL_ROTATION[3] = { 90.0f, 90.0f, 90.0f };
const float PODIUMS_INITIAL_Y[3] = { -2.51f, -2.61f, -2.51f };
const float KARTS_INITIAL_X[3] = { 18.0f, 18.0f, 18.0f };
//const float KARTS_INITIAL_Y[3] = { -1.73f, -1.83f, -1.73f };
const float KARTS_INITIAL_Y[3] = { 
            PODIUMS_INITIAL_Y[0] + PODIUMS_HEIGHT_FROM_Y0,
            PODIUMS_INITIAL_Y[1] + PODIUMS_HEIGHT_FROM_Y0,
            PODIUMS_INITIAL_Y[2] + PODIUMS_HEIGHT_FROM_Y0
};
const float KARTS_INITIAL_Z[3] = { 44.9f, 40.9f, 36.9f };

// Locations for karts to go to, should be the locations of the podiums (End Stage 1)
const float KARTS_PODIUM_X[3] = { 20.5f, 20.75f, 21.0f };

// Rotations for karts to go to in Stage 2
const float KARTS_AND_PODIUMS_FINAL_ROTATION[3] = { 270.0f, 270.0f, 270.0f };

// Locations for karts and podiums to go to in Stage 3
//const float KARTS_FINAL_X[3] = KARTS_PODIUM_X; // Not yet implemented
const float KARTS_FINAL_Y[3] = {
            PODIUMS_INITIAL_Y[0] + (PODIUMS_HEIGHT * 0.6f) + PODIUMS_HEIGHT_FROM_Y0,
            PODIUMS_INITIAL_Y[1] + (PODIUMS_HEIGHT * 0.9f) + PODIUMS_HEIGHT_FROM_Y0,
            PODIUMS_INITIAL_Y[2] + (PODIUMS_HEIGHT * 0.3f) + PODIUMS_HEIGHT_FROM_Y0
};
//const float KARTS_FINAL_Z[3] = KARTS_INITIAL_Z; // Not yet implemented
const float PODIUMS_FINAL_Y[3] = {
            PODIUMS_INITIAL_Y[0] + PODIUMS_HEIGHT * 0.6f,
            PODIUMS_INITIAL_Y[1] + PODIUMS_HEIGHT * 0.9f,
            PODIUMS_INITIAL_Y[2] + PODIUMS_HEIGHT * 0.3f
};

const float PODIUMS_AND_KARTS_SPEED_Y[3] = { 0.9f, 1.35f, 0.45f };

// -------------------------------------------------------------------------------------

GrandPrixWin::GrandPrixWin() : GrandPrixCutscene("grand_prix_win.stkgui")
{
    for (int i = 0; i < 3; i++)
    {
        m_kart_node[i] = NULL;
        m_podium_steps[i] = NULL;
    }
    m_player_won = false;
}   // GrandPrixWin

// -------------------------------------------------------------------------------------

void GrandPrixWin::onCutsceneEnd()
{
    for (unsigned int i = 0; i<m_all_kart_models.size(); i++)
        delete m_all_kart_models[i];
    m_all_kart_models.clear();

    if (m_unlocked_label != NULL)
    {
        manualRemoveWidget(m_unlocked_label);
        delete m_unlocked_label;
        m_unlocked_label = NULL;
    }

    for (int i = 0; i < 3; i++)
    {
        if (m_kart_node[i] != NULL)
            m_kart_node[i]->getPresentation<TrackObjectPresentationSceneNode>()->getNode()->remove();
        m_kart_node[i] = NULL;
        m_podium_steps[i] = NULL;
    }
}

// -------------------------------------------------------------------------------------

void GrandPrixWin::init()
{
    std::vector<std::string> parts;
    parts.push_back("gpwin");
    ((CutsceneWorld*)World::getWorld())->setParts(parts);
    CutsceneWorld::setUseDuration(false);

    Screen::init();

    World::getWorld()->setPhase(WorldStatus::RACE_PHASE);


    saveGPButton();
    if (PlayerManager::getCurrentPlayer()->getRecentlyCompletedChallenges().size() > 0)
    {
        const core::dimension2d<u32>& frame_size = GUIEngine::getDriver()->getCurrentRenderTargetSize();


        core::stringw message = _("You completed a challenge!");
        const int message_width = GUIEngine::getFont()->getDimension(message.c_str()).Width + 30;

        const int label_height = GUIEngine::getFontHeight() + 15;

        const int y_from       = frame_size.Height - label_height*2 - GUIEngine::getFontHeight();
        const int y_to         = frame_size.Height - label_height - GUIEngine::getFontHeight();

        const int label_x_from = frame_size.Width/2 - message_width/2;
        const int label_x_to   = frame_size.Width/2 + message_width/2;

        // button_h is used in the x coordinates not by mistake, but because the icon is square and
        // scaled according to the available height.
        core::rect< s32 > iconarea(label_x_from - label_height, y_from,
                                   label_x_from,                y_to);
        IGUIImage* img = GUIEngine::getGUIEnv()->addImage( iconarea );
        img->setImage( irr_driver->getTexture( FileManager::GUI_ICON, "cup_gold.png") );
        img->setScaleImage(true);
        img->setTabStop(false);
        img->setUseAlphaChannel(true);

        core::rect< s32 > icon2area(label_x_to,                y_from,
                                    label_x_to + label_height, y_to);
        img = GUIEngine::getGUIEnv()->addImage( icon2area );
        img->setImage( irr_driver->getTexture( FileManager::GUI_ICON,"cup_gold.png") );
        img->setScaleImage(true);
        img->setTabStop(false);
        img->setUseAlphaChannel(true);

        m_unlocked_label = new GUIEngine::LabelWidget();
        m_unlocked_label->m_properties[GUIEngine::PROP_ID] = "label";
        m_unlocked_label->m_properties[GUIEngine::PROP_TEXT_ALIGN] = "center";
        m_unlocked_label->m_x = label_x_from;
        m_unlocked_label->m_y = y_from;
        m_unlocked_label->m_w = message_width;
        m_unlocked_label->m_h = label_height;
        m_unlocked_label->setText(message, false);

        m_unlocked_label->add();
        manualAddWidget(m_unlocked_label);
        m_unlocked_label->setColor(video::SColor(255, 255, 255, 255));
    }
    else
    {
        m_unlocked_label = NULL;
    }

    m_global_time = 0.0f;
    m_phase = 1;

    SFXManager::get()->quickSound("gp_end");
    getWidget<ButtonWidget>("continue")->setFocusForPlayer(PLAYER_ID_GAME_MASTER);
}   // init

// -------------------------------------------------------------------------------------

void GrandPrixWin::onUpdate(float dt)
{
    m_global_time += dt;

    // ---- karts move
    if (m_phase == 1)
    {
        assert(m_kart_node[0] != NULL || m_kart_node[1] != NULL || m_kart_node[2] != NULL);

        int karts_not_yet_done = 0;
        for (int k=0; k<3; k++)
        {
            if (m_kart_node[k] != NULL)
            {
                if (m_kart_x[k] != KARTS_PODIUM_X[k])
                {
                    if (m_kart_x[k] < KARTS_PODIUM_X[k] - dt)
                        m_kart_x[k] += dt;
                    else if (m_kart_x[k] > KARTS_PODIUM_X[k] + dt)
                        m_kart_x[k] -= dt;
                    else
                        m_kart_x[k] = KARTS_PODIUM_X[k];
                    karts_not_yet_done++;
                }

                core::vector3df kart_pos(m_kart_x[k], m_kart_y[k], m_kart_z[k]);
                core::vector3df kart_rot(0, m_kart_rotation[k], 0);
                core::vector3df kart_scale(1.0f, 1.0f, 1.0f);
                m_kart_node[k]->move(kart_pos, kart_rot, kart_scale, false, true);
            }
        } // end for

        if (karts_not_yet_done == 0)
            m_phase = 2;
    }

    // ---- Karts Rotate
    else if (m_phase == 2)
    {
        int karts_not_yet_done = 0;
        for (int k=0; k<3; k++)
        {
            if (m_kart_node[k] != NULL)
            {
                if (m_kart_rotation[k] < KARTS_AND_PODIUMS_FINAL_ROTATION[k])
                {
                    m_kart_rotation[k] += 25.0f*dt;

                    core::vector3df kart_pos(m_kart_x[k], m_kart_y[k], m_kart_z[k]);
                    core::vector3df kart_rot(0, m_kart_rotation[k], 0);
                    core::vector3df kart_scale(1.0f, 1.0f, 1.0f);
                    m_kart_node[k]->move(kart_pos, kart_rot, kart_scale, false, true);

                    core::vector3df podium_pos = m_podium_steps[k]->getInitXYZ();
                    core::vector3df podium_rot(0, m_kart_rotation[k], 0);
                    m_podium_steps[k]->move(podium_pos, podium_rot, core::vector3df(1.0f, 1.0f, 1.0f), false, true);

                    karts_not_yet_done++;
                }
            }
        } // end for

        if (karts_not_yet_done == 0)
            m_phase = 3;
    }

    // ---- Podium Rises
    else if (m_phase == 3)
    {
        for (int k=0; k<3; k++)
        {
            if (m_kart_node[k] != NULL)
            {
                if (m_kart_y[k] < KARTS_FINAL_Y[k])
                {
                    m_kart_y[k] += dt * PODIUMS_AND_KARTS_SPEED_Y[k];
                    core::vector3df kart_pos(m_kart_x[k], m_kart_y[k], m_kart_z[k]);
                    core::vector3df kart_rot(0, m_kart_rotation[k], 0);
                    core::vector3df kart_scale(1.0f, 1.0f, 1.0f);
                    m_kart_node[k]->move(kart_pos, kart_rot, kart_scale, false, true);
                }

                if (m_podium_y[k] < PODIUMS_FINAL_Y[k]) {
                    core::vector3df podium_pos(m_podium_steps[k]->getInitXYZ().X, m_podium_y[k], m_podium_steps[k]->getInitXYZ().Z);
                    core::vector3df podium_rot(0, m_kart_rotation[k], 0);
                    m_podium_y[k] += dt * PODIUMS_AND_KARTS_SPEED_Y[k];
                    m_podium_steps[k]->move(core::vector3df(m_podium_x[k], m_podium_y[k], m_podium_z[k]), podium_rot, core::vector3df(1.0f, 1.0f, 1.0f), false, true);
                }
            }
        } // end for
    }


    // ---- title
    static const int w = irr_driver->getFrameSize().Width;
    static const int h = irr_driver->getFrameSize().Height;
    const irr::video::SColor color(255, 255, 255, 255);

    static int test_y = 0;

    irr::core::stringw message = (m_player_won) ? _("You won the Grand Prix!") :
                                                  _("You completed the Grand Prix!");

    GUIEngine::getTitleFont()->draw(message, core::rect< s32 >( 0, test_y, w, h/10 ),
                                    color, true/* center h */, true /* center v */ );
}   // onUpdate


// -------------------------------------------------------------------------------------

void GrandPrixWin::setKarts(const std::pair<std::string, float> idents_arg[3])
{
    TrackObjectManager* tobjman = Track::getCurrentTrack()->getTrackObjectManager();

    // reorder in "podium order" (i.e. second player to the left, first player
    // in the middle, last at the right)
    std::pair<std::string, float> idents[3];
    idents[0] = idents_arg[1];
    idents[1] = idents_arg[0];
    idents[2] = idents_arg[2];

    for (int i = 0; i < 3; i++)
    {
        const KartProperties* kp = kart_properties_manager->getKart(idents[i].first);
        if (kp == NULL) continue;

        KartModel* kart_model = kp->getKartModelCopy(std::make_shared<GE::GERenderInfo>(idents[i].second));
        m_all_kart_models.push_back(kart_model);
        scene::ISceneNode* kart_main_node = kart_model->attachModel(true, false);
        LODNode* lnode = dynamic_cast<LODNode*>(kart_main_node);
        if (lnode)
        {
            // Lod node has to be animated
            auto* a_node = static_cast<scene::IAnimatedMeshSceneNode*>
                (lnode->getAllNodes()[0]);
            const unsigned start_frame =
                kart_model->getFrame(KartModel::AF_WIN_LOOP_START) > -1 ?
                kart_model->getFrame(KartModel::AF_WIN_LOOP_START) :
                kart_model->getFrame(KartModel::AF_WIN_START) > -1 ?
                kart_model->getFrame(KartModel::AF_WIN_START) :
                kart_model->getFrame(KartModel::AF_STRAIGHT);
            const unsigned end_frame =
                kart_model->getFrame(KartModel::AF_WIN_END) > -1 ?
                kart_model->getFrame(KartModel::AF_WIN_END) :
                kart_model->getFrame(KartModel::AF_STRAIGHT);
            a_node->setLoopMode(true);
            a_node->setFrameLoop(start_frame, end_frame);
        }

        m_kart_x[i] = KARTS_INITIAL_X[i];
        m_kart_y[i] = KARTS_INITIAL_Y[i];
        m_kart_z[i] = KARTS_INITIAL_Z[i];
        m_kart_rotation[i] = KARTS_AND_PODIUMS_INITIAL_ROTATION[i];

        core::vector3df kart_pos(m_kart_x[i], m_kart_y[i], m_kart_z[i]);
        core::vector3df kart_rot(0, 0, 0);
        core::vector3df kart_scale(1.0f, 1.0f, 1.0f);

        //FIXME: it's not ideal that both the track object and the presentation
        // know the initial coordinates of the object
        TrackObjectPresentationSceneNode* presentation = 
            new TrackObjectPresentationSceneNode(kart_pos, kart_rot, kart_scale,
                                                 kart_main_node);
        TrackObject* tobj = new TrackObject(kart_pos, kart_rot, kart_scale,
            "ghost", presentation, false /* isDynamic */, NULL /* physics settings */);
        tobjman->insertObject(tobj);

        m_kart_node[i] = tobj;
    }

    TrackObject* currObj;
    PtrVector<TrackObject>& objects = tobjman->getObjects();
    for_in(currObj, objects)
    {
        TrackObjectPresentationMesh* meshPresentation = currObj->getPresentation<TrackObjectPresentationMesh>();
        if (meshPresentation != NULL)
        {
            if (meshPresentation->getModelFile() == "gpwin_podium1.spm")
                m_podium_steps[0] = currObj;
            else if (meshPresentation->getModelFile() == "gpwin_podium2.spm")
                m_podium_steps[1] = currObj;
            else if (meshPresentation->getModelFile() == "gpwin_podium3.spm")
                m_podium_steps[2] = currObj;
        }
    }

    for (int k=0; k<3; k++)
    {
        m_podium_x[k] = m_podium_steps[k]->getInitXYZ().X;
        m_podium_y[k] = PODIUMS_INITIAL_Y[k];
        m_podium_z[k] = m_podium_steps[k]->getInitXYZ().Z;

        core::vector3df podium_pos(m_podium_x[k], m_podium_y[k], m_podium_z[k]);

        m_podium_steps[k]->move(podium_pos, core::vector3df(0, 0, 0), core::vector3df(1.0f, 1.0f, 1.0f), false, true);
    }

    assert(m_podium_steps[0] != NULL);
    assert(m_podium_steps[1] != NULL);
    assert(m_podium_steps[2] != NULL);
}   // setKarts

// -------------------------------------------------------------------------------------

MusicInformation* GrandPrixWin::getInGameMenuMusic() const
{
    MusicInformation* mi = stk_config->m_gp_win_music;
    return mi;
}

// -------------------------------------------------------------------------------------


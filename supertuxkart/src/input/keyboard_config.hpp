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

#ifndef HEADER_KEYBOARD_CONFIG_HPP
#define HEADER_KEYBOARD_CONFIG_HPP

#include "input/binding.hpp"
#include "input/device_config.hpp"
#include "input/input.hpp"
#include "utils/no_copy.hpp"
#include "utils/cpp2011.hpp"

#include <iosfwd>

/**
  * \brief specialisation of DeviceConfig for keyboard type devices
  * \ingroup config
  */
class KeyboardConfig : public DeviceConfig
{

public:

                KeyboardConfig();
    virtual    ~KeyboardConfig() {}

    virtual void setDefaultBinds();
    virtual void save(std::ofstream& stream);

    // ------------------------------------------------------------------------
    virtual bool isGamePad()  const { return false; }
    // ------------------------------------------------------------------------
    virtual bool isKeyboard() const { return true; }

};   // class KeyboardConfig

#endif

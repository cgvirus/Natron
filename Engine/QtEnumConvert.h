/* ***** BEGIN LICENSE BLOCK *****
 * This file is part of Natron <https://natrongithub.github.io/>,
 * (C) 2018-2020 The Natron developers
 * (C) 2013-2018 INRIA and Alexandre Gauthier-Foichat
 *
 * Natron is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * Natron is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Natron.  If not, see <http://www.gnu.org/licenses/gpl-2.0.html>
 * ***** END LICENSE BLOCK ***** */

#ifndef Natron_Engine_QtEnumConvert_h
#define Natron_Engine_QtEnumConvert_h

// ***** BEGIN PYTHON BLOCK *****
// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>
// ***** END PYTHON BLOCK *****

#include "Global/Macros.h"

#include <vector>

#include "Global/KeySymbols.h"
#include "Global/Enums.h"

#include "Engine/EngineFwd.h"

NATRON_NAMESPACE_ENTER

namespace QtEnumConvert
{

    Key fromQtKey(Qt::Key k);
    KeyboardModifierEnum fromQtModifier(Qt::KeyboardModifier m);
    KeyboardModifiers fromQtModifiers(Qt::KeyboardModifiers m);
    Qt::Key toQtKey(Key k);
    Qt::KeyboardModifiers fromOfxtoQtModifiers(const std::list<int>& modifiers);
    bool fromOfxtoQtModifier(int modifier, Qt::KeyboardModifier* mod);
    Qt::KeyboardModifier  toQtModifier(KeyboardModifierEnum m);
    Qt::KeyboardModifiers toQtModifiers(const KeyboardModifiers& modifiers);
    bool toQtCursor(CursorEnum c, Qt::CursorShape* ret);
}

NATRON_NAMESPACE_EXIT

#endif // Natron_Engine_QtEnumConvert_h

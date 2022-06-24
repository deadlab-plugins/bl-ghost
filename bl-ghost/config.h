/* Copyright (C) 2022 Nicolas Dittlo <deadlab.plugins@gmail.com>
 *
 * This software is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 3 of the License, or (at your option) any later version.
 *
 * This software is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General
 * Public License along with this software; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place, Suite 330,
 * Boston, MA 02111-1307, USA.
 */
 
#define PLUG_NAME "bl-ghost"

#define PLUG_MFR "BlueLab"
#define PLUG_VERSION_HEX 0x00060306
#define PLUG_VERSION_STR "6.3.6"
// http://service.steinberg.de/databases/plugin.nsf/plugIn?openForm
// Full
#define PLUG_UNIQUE_ID 'baUw'

#define PLUG_MFR_ID 'BlLa'
#define PLUG_URL_STR "http://www.bluelab-plugs.com"
#define PLUG_EMAIL_STR "contact@bluelab-plugs.com"
#define PLUG_COPYRIGHT_STR "Copyright 2022 BlueLab | Audio Plugins"
#define PLUG_CLASS_NAME Ghost

#define BUNDLE_NAME "bl-ghost"
#define BUNDLE_MFR "BlueLab"
#define BUNDLE_DOMAIN "com"

#define SHARED_RESOURCES_SUBPATH "bl-ghost"

#ifndef APP_API
#define PLUG_CHANNEL_IO "1-1 2-2"
#else
// Fix for Mac 2020 (mic has only 1 channel, which prevents from initializing audio)
// Force 1 input channel only for App version.
#define PLUG_CHANNEL_IO "1-1 1-2"
#endif

#define PLUG_LATENCY 2048
#define PLUG_TYPE 0
#define PLUG_DOES_MIDI_IN 0
#define PLUG_DOES_MIDI_OUT 0
#define PLUG_DOES_MPE 0
#define PLUG_DOES_STATE_CHUNKS 0
#define PLUG_HAS_UI 1
#define PLUG_WIDTH 848
#define PLUG_HEIGHT 600
// Origin IPlug2: 60:
// Origin Rebalance: 25
// For Windows + VSynch
#define PLUG_FPS 60
#define PLUG_SHARED_RESOURCES 0
#define PLUG_HOST_RESIZE 0

#define AUV2_ENTRY Ghost_Entry
#define AUV2_ENTRY_STR "Ghost_Entry"
#define AUV2_FACTORY Ghost_Factory
#define AUV2_VIEW_CLASS Ghost_View
#define AUV2_VIEW_CLASS_STR "Ghost_View"

#define AAX_TYPE_IDS 'IEF1', 'IEF2'
#define AAX_TYPE_IDS_AUDIOSUITE 'IEA1', 'IEA2'
#define AAX_PLUG_MFR_STR "BlLa"

#define AAX_PLUG_NAME_STR "bl ghost\nbl ghost"

#define AAX_PLUG_CATEGORY_STR "Effect"
#define AAX_DOES_AUDIOSUITE 1

#define VST3_SUBCATEGORY "Fx|Restoration"

#define APP_NUM_CHANNELS 2
#define APP_N_VECTOR_WAIT 0
#define APP_MULT 1
#define APP_COPY_AUV3 0
#define APP_SIGNAL_VECTOR_SIZE 64

// Necessary!
#define ROBOTO_FN "Roboto-Regular.ttf"

#define FONT_REGULAR_FN "font-regular.ttf"
#define FONT_LIGHT_FN "font-light.ttf"
#define FONT_BOLD_FN "font-bold.ttf"

#define FONT_OPENSANS_EXTRA_BOLD_FN "OpenSans-ExtraBold.ttf"
#define FONT_ROBOTO_BOLD_FN "Roboto-Bold.ttf"

#define MANUAL_FN "bl-ghost_manual-EN.pdf"
// For WIN32
#define MANUAL_FULLPATH_FN "manual\bl-ghost_manual-EN.pdf"

#define DUMMY_RESOURCE_FN "dummy.txt"

// For linux installer
#define BL_ICON_FN "bl-ghost.png"

// Image resource locations for this plug.
#define BACKGROUND_FN "background.png"
#define PLUGNAME_FN "plugname.png"
#define HELP_BUTTON_FN "help-button.png"
#define TEXTFIELD_FN "textfield.png"
#define RADIOBUTTON_FN "radiobutton.png"
#define CHECKBOX_FN "checkbox.png"

#define KNOB_SMALL_FN "knob-small.svg"

#define GRAPH_FN "graph.png"

// GUI resize
#define BUTTON_RESIZE_SMALL_FN "button-small-gui.png"
#define BUTTON_RESIZE_MEDIUM_FN "button-med-gui.png"
#define BUTTON_RESIZE_BIG_FN "button-big-gui.png"
#define BUTTON_RESIZE_HUGE_FN "button-huge-gui.png"

#define PLUGNAME_FN "plugname.png"

#define BUTTON_PLAY_FN "button-play.png"

#define BUTTON_UNDO_FN "button-undo.png"
#define BUTTON_CUT_FN "button-cut.png"
#define BUTTON_COPY_FN "button-copy.png"
#define BUTTON_PASTE_FN "button-paste.png"
#define BUTTON_APPLY_GAIN_FN "button-applygain.png"
#define BUTTON_INPAINT_FN "button-inpaint.png"

#define BUTTON_INPAINT_BOTH_FN "button-inpdir-both.png"
#define BUTTON_INPAINT_HORIZ_FN "button-inpdir-horiz.png"
#define BUTTON_INPAINT_VERT_FN "button-inpdir-vert.png"

#define BUTTON_SELDIR_BOTH_FN "button-seldir-both.png"
#define BUTTON_SELDIR_HORIZ_FN "button-seldir-horiz.png"
#define BUTTON_SELDIR_VERT_FN "button-seldir-vert.png"

// App
#define BACKGROUND_MED_FN "background-med.png"
#define BACKGROUND_BIG_FN "background-big.png"
#define BACKGROUND_HUGE_FN "background-huge.png"

// Plug
#define BACKGROUND_PLUG_FN "background-plug.png"
#define BACKGROUND_PLUG_MED_FN "background-plug-med.png"
#define BACKGROUND_PLUG_BIG_FN "background-plug-big.png"
#define BACKGROUND_PLUG_HUGE_FN "background-plug-huge.png"

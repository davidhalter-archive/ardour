/*
    Copyright (C) 2000-2009 Paul Davis

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.

*/

/* Note: public Editor methods are documented in public_editor.h */

#include <stdint.h>
#include <unistd.h>
#include <cstdlib>
#include <cmath>
#include <string>
#include <algorithm>
#include <map>

#include "ardour_ui.h"
/*
 * ardour_ui.h include was moved to the top of the list
 * due to a conflicting definition of 'Style' between
 * Apple's MacTypes.h and BarController.
 */

#include <boost/none.hpp>

#include <sigc++/bind.h>

#include "pbd/convert.h"
#include "pbd/error.h"
#include "pbd/enumwriter.h"
#include "pbd/memento_command.h"
#include "pbd/unknown_type.h"

#include <glibmm/miscutils.h>
#include <gtkmm/image.h>
#include <gdkmm/color.h>
#include <gdkmm/bitmap.h>

#include <gtkmm2ext/grouped_buttons.h>
#include <gtkmm2ext/gtk_ui.h>
#include <gtkmm2ext/tearoff.h>
#include <gtkmm2ext/utils.h>
#include <gtkmm2ext/window_title.h>
#include <gtkmm2ext/choice.h>
#include <gtkmm2ext/cell_renderer_pixbuf_toggle.h>

#include "ardour/audio_diskstream.h"
#include "ardour/audio_track.h"
#include "ardour/audioplaylist.h"
#include "ardour/audioregion.h"
#include "ardour/location.h"
#include "ardour/midi_region.h"
#include "ardour/plugin_manager.h"
#include "ardour/profile.h"
#include "ardour/route_group.h"
#include "ardour/session_directory.h"
#include "ardour/session_route.h"
#include "ardour/session_state_utils.h"
#include "ardour/tempo.h"
#include "ardour/utils.h"
#include "ardour/session_playlists.h"
#include "ardour/audioengine.h"

#include "control_protocol/control_protocol.h"

#include "editor.h"
#include "keyboard.h"
#include "marker.h"
#include "playlist_selector.h"
#include "audio_region_view.h"
#include "rgb_macros.h"
#include "selection.h"
#include "audio_streamview.h"
#include "time_axis_view.h"
#include "audio_time_axis.h"
#include "utils.h"
#include "crossfade_view.h"
#include "canvas-noevent-text.h"
#include "editing.h"
#include "public_editor.h"
#include "crossfade_edit.h"
#include "canvas_impl.h"
#include "actions.h"
#include "sfdb_ui.h"
#include "gui_thread.h"
#include "simpleline.h"
#include "rhythm_ferret.h"
#include "actions.h"
#include "tempo_lines.h"
#include "analysis_window.h"
#include "bundle_manager.h"
#include "global_port_matrix.h"
#include "editor_drag.h"
#include "editor_group_tabs.h"
#include "automation_time_axis.h"
#include "editor_routes.h"
#include "midi_time_axis.h"
#include "mixer_strip.h"
#include "editor_route_groups.h"
#include "editor_regions.h"
#include "editor_locations.h"
#include "editor_snapshots.h"
#include "editor_summary.h"
#include "region_layering_order_editor.h"
#include "mouse_cursors.h"
#include "editor_cursors.h"

#include "i18n.h"

#ifdef WITH_CMT
#include "imageframe_socket_handler.h"
#endif

using namespace std;
using namespace ARDOUR;
using namespace PBD;
using namespace Gtk;
using namespace Glib;
using namespace Gtkmm2ext;
using namespace Editing;

using PBD::internationalize;
using PBD::atoi;
using Gtkmm2ext::Keyboard;

const double Editor::timebar_height = 15.0;

static const gchar *_snap_type_strings[] = {
	N_("CD Frames"),
	N_("Timecode Frames"),
	N_("Timecode Seconds"),
	N_("Timecode Minutes"),
	N_("Seconds"),
	N_("Minutes"),
	N_("Beats/32"),
	N_("Beats/28"),
	N_("Beats/24"),
	N_("Beats/20"),
	N_("Beats/16"),
	N_("Beats/14"),
	N_("Beats/12"),
	N_("Beats/10"),
	N_("Beats/8"),
	N_("Beats/7"),
	N_("Beats/6"),
	N_("Beats/5"),
	N_("Beats/4"),
	N_("Beats/3"),
	N_("Beats/2"),
	N_("Beats"),
	N_("Bars"),
	N_("Marks"),
	N_("Region starts"),
	N_("Region ends"),
	N_("Region syncs"),
	N_("Region bounds"),
	0
};

static const gchar *_snap_mode_strings[] = {
	N_("No Grid"),
	N_("Grid"),
	N_("Magnetic"),
	0
};

static const gchar *_edit_point_strings[] = {
	N_("Playhead"),
	N_("Marker"),
	N_("Mouse"),
	0
};

static const gchar *_zoom_focus_strings[] = {
	N_("Left"),
	N_("Right"),
	N_("Center"),
	N_("Playhead"),
 	N_("Mouse"),
 	N_("Edit point"),
	0
};

#ifdef USE_RUBBERBAND
static const gchar *_rb_opt_strings[] = {
	N_("Mushy"),
	N_("Smooth"),
	N_("Balanced multitimbral mixture"),
	N_("Unpitched percussion with stable notes"),
	N_("Crisp monophonic instrumental"),
	N_("Unpitched solo percussion"),
	N_("Resample without preserving pitch"),
	0
};
#endif

void
show_me_the_size (Requisition* r, const char* what)
{
	cerr << "size of " << what << " = " << r->width << " x " << r->height << endl;
}

#ifdef GTKOSX
static void
pane_size_watcher (Paned* pane)
{
	/* if the handle of a pane vanishes into (at least) the tabs of a notebook,
	   it is no longer accessible. so stop that. this doesn't happen on X11,
	   just the quartz backend.

	   ugh.
	*/

	int max_width_of_lhs = GTK_WIDGET(pane->gobj())->allocation.width - 25;

	gint pos = pane->get_position ();

	if (pos > max_width_of_lhs) {
		pane->set_position (max_width_of_lhs);
	}
}
#endif

Editor::Editor ()
	: _join_object_range_state (JOIN_OBJECT_RANGE_NONE)

	  /* time display buttons */
	, minsec_label (_("Mins:Secs"))
	, bbt_label (_("Bars:Beats"))
	, timecode_label (_("Timecode"))
	, frame_label (_("Samples"))
	, tempo_label (_("Tempo"))
	, meter_label (_("Meter"))
	, mark_label (_("Location Markers"))
	, range_mark_label (_("Range Markers"))
	, transport_mark_label (_("Loop/Punch Ranges"))
	, cd_mark_label (_("CD Markers"))
	, edit_packer (4, 4, true)

	  /* the values here don't matter: layout widgets
	     reset them as needed.
	  */

	, vertical_adjustment (0.0, 0.0, 10.0, 400.0)

	  /* tool bar related */

	, zoom_range_clock (X_("zoomrange"), false, X_("ZoomRangeClock"), true, false, true)

	, toolbar_selection_clock_table (2,3)

	, automation_mode_button (_("mode"))
	, global_automation_button (_("automation"))

	, midi_panic_button (_("Panic"))

#ifdef WITH_CMT
	, image_socket_listener(0)
#endif

	  /* nudge */

	, nudge_clock (X_("nudge"), false, X_("NudgeClock"), true, false, true)
	, meters_running(false)
	, _pending_locate_request (false)
	, _pending_initial_locate (false)
	, _last_cut_copy_source_track (0)

	, _region_selection_change_updates_region_list (true)
{
	constructed = false;

	/* we are a singleton */

	PublicEditor::_instance = this;

	_have_idled = false;

	selection = new Selection (this);
	cut_buffer = new Selection (this);

	clicked_regionview = 0;
	clicked_axisview = 0;
	clicked_routeview = 0;
	clicked_crossfadeview = 0;
	clicked_control_point = 0;
	last_update_frame = 0;
        pre_press_cursor = 0;
	_drags = new DragManager (this);
	current_mixer_strip = 0;
	current_bbt_points = 0;
	tempo_lines = 0;

	snap_type_strings =  I18N (_snap_type_strings);
	snap_mode_strings =  I18N (_snap_mode_strings);
	zoom_focus_strings = I18N (_zoom_focus_strings);
	edit_point_strings = I18N (_edit_point_strings);
#ifdef USE_RUBBERBAND
	rb_opt_strings = I18N (_rb_opt_strings);
	rb_current_opt = 4;
#endif

	snap_threshold = 5.0;
	bbt_beat_subdivision = 4;
	_canvas_width = 0;
	_canvas_height = 0;
	last_autoscroll_x = 0;
	last_autoscroll_y = 0;
	autoscroll_active = false;
	autoscroll_timeout_tag = -1;
	logo_item = 0;

	analysis_window = 0;

	current_interthread_info = 0;
	_show_measures = true;
	show_gain_after_trim = false;
	verbose_cursor_on = true;
	last_item_entered = 0;

	have_pending_keyboard_selection = false;
	_follow_playhead = true;
        _stationary_playhead = false;
	_xfade_visibility = true;
	editor_ruler_menu = 0;
	no_ruler_shown_update = false;
	marker_menu = 0;
	session_range_marker_menu = 0;
	range_marker_menu = 0;
	marker_menu_item = 0;
	tempo_or_meter_marker_menu = 0;
	transport_marker_menu = 0;
	new_transport_marker_menu = 0;
	editor_mixer_strip_width = Wide;
	show_editor_mixer_when_tracks_arrive = false;
	region_edit_menu_split_multichannel_item = 0;
	region_edit_menu_split_item = 0;
	temp_location = 0;
	leftmost_frame = 0;
	current_stepping_trackview = 0;
	entered_track = 0;
	entered_regionview = 0;
	entered_marker = 0;
	clear_entered_track = false;
	current_timefx = 0;
	playhead_cursor = 0;
	button_release_can_deselect = true;
	_dragging_playhead = false;
	_dragging_edit_point = false;
	select_new_marker = false;
	rhythm_ferret = 0;
	layering_order_editor = 0;
	_bundle_manager = 0;
	no_save_visual = false;
	resize_idle_id = -1;

	scrubbing_direction = 0;

	sfbrowser = 0;

	location_marker_color = ARDOUR_UI::config()->canvasvar_LocationMarker.get();
	location_range_color = ARDOUR_UI::config()->canvasvar_LocationRange.get();
	location_cd_marker_color = ARDOUR_UI::config()->canvasvar_LocationCDMarker.get();
	location_loop_color = ARDOUR_UI::config()->canvasvar_LocationLoop.get();
	location_punch_color = ARDOUR_UI::config()->canvasvar_LocationPunch.get();

	_edit_point = EditAtMouse;
	_internal_editing = false;
	current_canvas_cursor = 0;

	frames_per_unit = 2048; /* too early to use reset_zoom () */

	_scroll_callbacks = 0;

	zoom_focus = ZoomFocusLeft;
	set_zoom_focus (ZoomFocusLeft);
	zoom_range_clock.ValueChanged.connect (sigc::mem_fun(*this, &Editor::zoom_adjustment_changed));

	bbt_label.set_name ("EditorTimeButton");
	bbt_label.set_size_request (-1, (int)timebar_height);
	bbt_label.set_alignment (1.0, 0.5);
	bbt_label.set_padding (5,0);
	bbt_label.hide ();
	bbt_label.set_no_show_all();
	minsec_label.set_name ("EditorTimeButton");
	minsec_label.set_size_request (-1, (int)timebar_height);
	minsec_label.set_alignment (1.0, 0.5);
	minsec_label.set_padding (5,0);
	minsec_label.hide ();
	minsec_label.set_no_show_all();
	timecode_label.set_name ("EditorTimeButton");
	timecode_label.set_size_request (-1, (int)timebar_height);
	timecode_label.set_alignment (1.0, 0.5);
	timecode_label.set_padding (5,0);
	timecode_label.hide ();
	timecode_label.set_no_show_all();
	frame_label.set_name ("EditorTimeButton");
	frame_label.set_size_request (-1, (int)timebar_height);
	frame_label.set_alignment (1.0, 0.5);
	frame_label.set_padding (5,0);
	frame_label.hide ();
	frame_label.set_no_show_all();

	tempo_label.set_name ("EditorTimeButton");
	tempo_label.set_size_request (-1, (int)timebar_height);
	tempo_label.set_alignment (1.0, 0.5);
	tempo_label.set_padding (5,0);
	tempo_label.hide();
	tempo_label.set_no_show_all();
	meter_label.set_name ("EditorTimeButton");
	meter_label.set_size_request (-1, (int)timebar_height);
	meter_label.set_alignment (1.0, 0.5);
	meter_label.set_padding (5,0);
	meter_label.hide();
	meter_label.set_no_show_all();
	mark_label.set_name ("EditorTimeButton");
	mark_label.set_size_request (-1, (int)timebar_height);
	mark_label.set_alignment (1.0, 0.5);
	mark_label.set_padding (5,0);
	mark_label.hide();
	mark_label.set_no_show_all();
	cd_mark_label.set_name ("EditorTimeButton");
	cd_mark_label.set_size_request (-1, (int)timebar_height);
	cd_mark_label.set_alignment (1.0, 0.5);
	cd_mark_label.set_padding (5,0);
	cd_mark_label.hide();
	cd_mark_label.set_no_show_all();
	range_mark_label.set_name ("EditorTimeButton");
	range_mark_label.set_size_request (-1, (int)timebar_height);
	range_mark_label.set_alignment (1.0, 0.5);
	range_mark_label.set_padding (5,0);
	range_mark_label.hide();
	range_mark_label.set_no_show_all();
	transport_mark_label.set_name ("EditorTimeButton");
	transport_mark_label.set_size_request (-1, (int)timebar_height);
	transport_mark_label.set_alignment (1.0, 0.5);
	transport_mark_label.set_padding (5,0);
	transport_mark_label.hide();
	transport_mark_label.set_no_show_all();

	initialize_rulers ();
	initialize_canvas ();
	_summary = new EditorSummary (this);

	selection->TimeChanged.connect (sigc::mem_fun(*this, &Editor::time_selection_changed));
	selection->TracksChanged.connect (sigc::mem_fun(*this, &Editor::track_selection_changed));
	editor_regions_selection_changed_connection = selection->RegionsChanged.connect (sigc::mem_fun(*this, &Editor::region_selection_changed));
	selection->PointsChanged.connect (sigc::mem_fun(*this, &Editor::point_selection_changed));
	selection->MarkersChanged.connect (sigc::mem_fun(*this, &Editor::marker_selection_changed));

	edit_controls_vbox.set_spacing (0);
	vertical_adjustment.signal_value_changed().connect (sigc::mem_fun(*this, &Editor::tie_vertical_scrolling), true);
	track_canvas->signal_map_event().connect (sigc::mem_fun (*this, &Editor::track_canvas_map_handler));

	HBox* h = manage (new HBox);
	_group_tabs = new EditorGroupTabs (this);
	h->pack_start (*_group_tabs, PACK_SHRINK);
	h->pack_start (edit_controls_vbox);
	controls_layout.add (*h);

	controls_layout.set_name ("EditControlsBase");
	controls_layout.add_events (Gdk::SCROLL_MASK);
	controls_layout.signal_scroll_event().connect (sigc::mem_fun(*this, &Editor::control_layout_scroll), false);

	controls_layout.add_events (Gdk::BUTTON_PRESS_MASK|Gdk::BUTTON_RELEASE_MASK|Gdk::ENTER_NOTIFY_MASK|Gdk::LEAVE_NOTIFY_MASK);
	controls_layout.signal_button_release_event().connect (sigc::mem_fun(*this, &Editor::edit_controls_button_release));
	controls_layout_size_request_connection = controls_layout.signal_size_request().connect (sigc::mem_fun (*this, &Editor::controls_layout_size_request));

	_cursors = new MouseCursors;

	ArdourCanvas::Canvas* time_pad = manage(new ArdourCanvas::Canvas());
	ArdourCanvas::SimpleLine* pad_line_1 = manage(new ArdourCanvas::SimpleLine(*time_pad->root(),
			0.0, 1.0, 100.0, 1.0));
	pad_line_1->property_color_rgba() = 0xFF0000FF;
	pad_line_1->show();
	time_pad->show();

	time_canvas_vbox.set_size_request (-1, (int)(timebar_height * visible_timebars) + 2);
	time_canvas_vbox.set_size_request (-1, -1);

	ruler_label_event_box.add (ruler_label_vbox);
	ruler_label_event_box.set_events (Gdk::BUTTON_PRESS_MASK|Gdk::BUTTON_RELEASE_MASK);
	ruler_label_event_box.signal_button_release_event().connect (sigc::mem_fun(*this, &Editor::ruler_label_button_release));

	time_button_event_box.add (time_button_vbox);
	time_button_event_box.set_events (Gdk::BUTTON_PRESS_MASK|Gdk::BUTTON_RELEASE_MASK);
	time_button_event_box.signal_button_release_event().connect (sigc::mem_fun(*this, &Editor::ruler_label_button_release));

	/* these enable us to have a dedicated window (for cursor setting, etc.)
	   for the canvas areas.
	*/

	track_canvas_event_box.add (*track_canvas);

	time_canvas_event_box.add (time_canvas_vbox);
	time_canvas_event_box.set_events (Gdk::BUTTON_PRESS_MASK|Gdk::BUTTON_RELEASE_MASK|Gdk::POINTER_MOTION_MASK);

	edit_packer.set_col_spacings (0);
	edit_packer.set_row_spacings (0);
	edit_packer.set_homogeneous (false);
	edit_packer.set_border_width (0);
	edit_packer.set_name ("EditorWindow");

	/* labels for the rulers */
	edit_packer.attach (ruler_label_event_box,   1, 2, 0, 1,    FILL,        SHRINK, 0, 0);
	/* labels for the marker "tracks" */
	edit_packer.attach (time_button_event_box,   1, 2, 1, 2,    FILL,        SHRINK, 0, 0);
	/* the rulers */
	edit_packer.attach (time_canvas_event_box,   2, 3, 0, 1,    FILL|EXPAND, FILL, 0, 0);
	/* track controls */
	edit_packer.attach (controls_layout,         0, 2, 2, 3,    FILL,        FILL|EXPAND, 0, 0);
	/* main canvas */
	edit_packer.attach (track_canvas_event_box,  2, 3, 1, 3,    FILL|EXPAND, FILL|EXPAND, 0, 0);

	bottom_hbox.set_border_width (2);
	bottom_hbox.set_spacing (3);

	_route_groups = new EditorRouteGroups (this);
	_routes = new EditorRoutes (this);
	_regions = new EditorRegions (this);
	_snapshots = new EditorSnapshots (this);
	_locations = new EditorLocations (this);

	Gtk::Label* nlabel;

	nlabel = manage (new Label (_("Regions")));
	nlabel->set_angle (-90);
	the_notebook.append_page (_regions->widget (), *nlabel);
	nlabel = manage (new Label (_("Tracks & Busses")));
	nlabel->set_angle (-90);
	the_notebook.append_page (_routes->widget (), *nlabel);
	nlabel = manage (new Label (_("Snapshots")));
	nlabel->set_angle (-90);
	the_notebook.append_page (_snapshots->widget (), *nlabel);
	nlabel = manage (new Label (_("Route Groups")));
	nlabel->set_angle (-90);
	the_notebook.append_page (_route_groups->widget (), *nlabel);
	nlabel = manage (new Label (_("Ranges & Marks")));
	nlabel->set_angle (-90);
	the_notebook.append_page (_locations->widget (), *nlabel);

	the_notebook.set_show_tabs (true);
	the_notebook.set_scrollable (true);
	the_notebook.popup_disable ();
	the_notebook.set_tab_pos (Gtk::POS_RIGHT);
	the_notebook.show_all ();
	
	post_maximal_editor_width = 0;
	post_maximal_horizontal_pane_position = 0;
	post_maximal_editor_height = 0;
	post_maximal_vertical_pane_position = 0;

	editor_summary_pane.pack1(edit_packer);

	Button* summary_arrows_left_left = manage (new Button);
	summary_arrows_left_left->add (*manage (new Arrow (ARROW_LEFT, SHADOW_NONE)));
	summary_arrows_left_left->signal_pressed().connect (sigc::hide_return (sigc::mem_fun (*this, &Editor::horizontal_scroll_left_press)));
	summary_arrows_left_left->signal_released().connect (sigc::mem_fun (*this, &Editor::horizontal_scroll_left_release));
	Button* summary_arrows_left_right = manage (new Button);
	summary_arrows_left_right->add (*manage (new Arrow (ARROW_RIGHT, SHADOW_NONE)));
	summary_arrows_left_right->signal_pressed().connect (sigc::hide_return (sigc::mem_fun (*this, &Editor::horizontal_scroll_right_press)));
	summary_arrows_left_right->signal_released().connect (sigc::mem_fun (*this, &Editor::horizontal_scroll_right_release));
	VBox* summary_arrows_left = manage (new VBox);
	summary_arrows_left->pack_start (*summary_arrows_left_left);
	summary_arrows_left->pack_start (*summary_arrows_left_right);

	Button* summary_arrows_right_left = manage (new Button);
	summary_arrows_right_left->add (*manage (new Arrow (ARROW_LEFT, SHADOW_NONE)));
	summary_arrows_right_left->signal_pressed().connect (sigc::hide_return (sigc::mem_fun (*this, &Editor::horizontal_scroll_left_press)));
	summary_arrows_right_left->signal_released().connect (sigc::mem_fun (*this, &Editor::horizontal_scroll_left_release));
	Button* summary_arrows_right_right = manage (new Button);
	summary_arrows_right_right->add (*manage (new Arrow (ARROW_RIGHT, SHADOW_NONE)));
	summary_arrows_right_right->signal_pressed().connect (sigc::hide_return (sigc::mem_fun (*this, &Editor::horizontal_scroll_right_press)));
	summary_arrows_right_right->signal_released().connect (sigc::mem_fun (*this, &Editor::horizontal_scroll_right_release));
	VBox* summary_arrows_right = manage (new VBox);
	summary_arrows_right->pack_start (*summary_arrows_right_left);
	summary_arrows_right->pack_start (*summary_arrows_right_right);

	Frame* summary_frame = manage (new Frame);
	summary_frame->set_shadow_type (Gtk::SHADOW_ETCHED_IN);
	summary_frame->add (*_summary);
	summary_frame->show ();

	_summary_hbox.pack_start (*summary_arrows_left, false, false);
	_summary_hbox.pack_start (*summary_frame, true, true);
	_summary_hbox.pack_start (*summary_arrows_right, false, false);
	
	editor_summary_pane.pack2 (_summary_hbox);

	edit_pane.pack1 (editor_summary_pane, true, true);
	edit_pane.pack2 (the_notebook, false, true);

	editor_summary_pane.signal_size_allocate().connect (sigc::bind (sigc::mem_fun (*this, &Editor::pane_allocation_handler), static_cast<Paned*> (&editor_summary_pane)));

	/* XXX: editor_summary_pane might need similar special OS X treatment to the edit_pane */

	edit_pane.signal_size_allocate().connect (sigc::bind (sigc::mem_fun(*this, &Editor::pane_allocation_handler), static_cast<Paned*> (&edit_pane)));
#ifdef GTKOSX
	Glib::PropertyProxy<int> proxy = edit_pane.property_position();
	proxy.signal_changed().connect (bind (sigc::ptr_fun (pane_size_watcher), static_cast<Paned*> (&edit_pane)));
#endif
	top_hbox.pack_start (toolbar_frame, false, true);

	HBox *hbox = manage (new HBox);
	hbox->pack_start (edit_pane, true, true);

	global_vpacker.pack_start (top_hbox, false, false);
	global_vpacker.pack_start (*hbox, true, true);

	global_hpacker.pack_start (global_vpacker, true, true);

	set_name ("EditorWindow");
	add_accel_group (ActionManager::ui_manager->get_accel_group());

	status_bar_hpacker.show ();

	vpacker.pack_end (status_bar_hpacker, false, false);
	vpacker.pack_end (global_hpacker, true, true);

	/* register actions now so that set_state() can find them and set toggles/checks etc */

	register_actions ();

	setup_toolbar ();
	setup_midi_toolbar ();

	_snap_type = SnapToBeat;
	set_snap_to (_snap_type);
	_snap_mode = SnapOff;
	set_snap_mode (_snap_mode);
	set_mouse_mode (MouseObject, true);
	set_edit_point_preference (EditAtMouse, true);

	_playlist_selector = new PlaylistSelector();
	_playlist_selector->signal_delete_event().connect (sigc::bind (sigc::ptr_fun (just_hide_it), static_cast<Window *> (_playlist_selector)));

	RegionView::RegionViewGoingAway.connect (*this, invalidator (*this),  ui_bind (&Editor::catch_vanishing_regionview, this, _1), gui_context());

	/* nudge stuff */

	nudge_forward_button.add (*(manage (new Image (::get_icon("nudge_right")))));
	nudge_backward_button.add (*(manage (new Image (::get_icon("nudge_left")))));

	nudge_forward_button.set_name ("TransportButton");
	nudge_backward_button.set_name ("TransportButton");

	fade_context_menu.set_name ("ArdourContextMenu");

	/* icons, titles, WM stuff */

	list<Glib::RefPtr<Gdk::Pixbuf> > window_icons;
	Glib::RefPtr<Gdk::Pixbuf> icon;

	if ((icon = ::get_icon ("ardour_icon_16px")) != 0) {
		window_icons.push_back (icon);
	}
	if ((icon = ::get_icon ("ardour_icon_22px")) != 0) {
		window_icons.push_back (icon);
	}
	if ((icon = ::get_icon ("ardour_icon_32px")) != 0) {
		window_icons.push_back (icon);
	}
	if ((icon = ::get_icon ("ardour_icon_48px")) != 0) {
		window_icons.push_back (icon);
	}
	if (!window_icons.empty()) {
		set_icon_list (window_icons);
		set_default_icon_list (window_icons);
	}

	WindowTitle title(Glib::get_application_name());
	title += _("Editor");
	set_title (title.get_string());
	set_wmclass (X_("ardour_editor"), PROGRAM_NAME);

	add (vpacker);
	add_events (Gdk::KEY_PRESS_MASK|Gdk::KEY_RELEASE_MASK);

	signal_configure_event().connect (sigc::mem_fun (*ARDOUR_UI::instance(), &ARDOUR_UI::configure_handler));
	signal_delete_event().connect (sigc::mem_fun (*ARDOUR_UI::instance(), &ARDOUR_UI::exit_on_main_window_close));

	/* allow external control surfaces/protocols to do various things */

	ControlProtocol::ZoomToSession.connect (*this, invalidator (*this), boost::bind (&Editor::temporal_zoom_session, this), gui_context());
	ControlProtocol::ZoomIn.connect (*this, invalidator (*this), boost::bind (&Editor::temporal_zoom_step, this, false), gui_context());
	ControlProtocol::ZoomOut.connect (*this, invalidator (*this), boost::bind (&Editor::temporal_zoom_step, this, true), gui_context());
	ControlProtocol::ScrollTimeline.connect (*this, invalidator (*this), ui_bind (&Editor::control_scroll, this, _1), gui_context());
	BasicUI::AccessAction.connect (*this, invalidator (*this), ui_bind (&Editor::access_action, this, _1, _2), gui_context());
	
	/* problematic: has to return a value and thus cannot be x-thread */

	Session::AskAboutPlaylistDeletion.connect_same_thread (*this, boost::bind (&Editor::playlist_deletion_dialog, this, _1));

	Config->ParameterChanged.connect (*this, invalidator (*this), ui_bind (&Editor::parameter_changed, this, _1), gui_context());

	TimeAxisView::CatchDeletion.connect (*this, invalidator (*this), ui_bind (&Editor::timeaxisview_deleted, this, _1), gui_context());

	_ignore_region_action = false;
	_last_region_menu_was_main = false;
	_popup_region_menu_item = 0;

	_show_marker_lines = false;
	_over_region_trim_target = false;

	constructed = true;
	instant_save ();

	setup_fade_images ();
}

Editor::~Editor()
{
#ifdef WITH_CMT
	if(image_socket_listener) {
		if(image_socket_listener->is_connected())
		{
			image_socket_listener->close_connection() ;
		}

		delete image_socket_listener ;
		image_socket_listener = 0 ;
	}
#endif
        
	delete _routes;
	delete _route_groups;
	delete track_canvas;
	delete _drags;
}

void
Editor::add_toplevel_controls (Container& cont)
{
	vpacker.pack_start (cont, false, false);
	cont.show_all ();
}

void
Editor::catch_vanishing_regionview (RegionView *rv)
{
	/* note: the selection will take care of the vanishing
	   audioregionview by itself.
	*/

	if (_drags->active() && _drags->have_item (rv->get_canvas_group()) && !_drags->ending()) {
		_drags->abort ();
	}

	if (clicked_regionview == rv) {
		clicked_regionview = 0;
	}

	if (entered_regionview == rv) {
		set_entered_regionview (0);
	}

	if (!_all_region_actions_sensitized) {
		sensitize_all_region_actions (true);
	}
}

void
Editor::set_entered_regionview (RegionView* rv)
{
	if (rv == entered_regionview) {
		return;
	}

	if (entered_regionview) {
		entered_regionview->exited ();
	}

	if ((entered_regionview = rv) != 0) {
		entered_regionview->entered (internal_editing ());
	}

	if (!_all_region_actions_sensitized && _last_region_menu_was_main) {
		/* This RegionView entry might have changed what region actions
		   are allowed, so sensitize them all in case a key is pressed.
		*/
		sensitize_all_region_actions (true);
	}
}

void
Editor::set_entered_track (TimeAxisView* tav)
{
	if (entered_track) {
		entered_track->exited ();
	}

	if ((entered_track = tav) != 0) {
		entered_track->entered ();
	}
}

void
Editor::show_window ()
{
	if (! is_visible ()) {
		show_all ();

		/* re-hide editor list if necessary */
		editor_list_button_toggled ();

		/* re-hide summary widget if necessary */
		parameter_changed ("show-summary");

		parameter_changed ("show-edit-group-tabs");

		/* now reset all audio_time_axis heights, because widgets might need
		   to be re-hidden
		*/

		TimeAxisView *tv;

		for (TrackViewList::iterator i = track_views.begin(); i != track_views.end(); ++i) {
			tv = (static_cast<TimeAxisView*>(*i));
			tv->reset_height ();
		}
	}

	present ();
}

void
Editor::instant_save ()
{
	if (!constructed || !ARDOUR_UI::instance()->session_loaded) {
		return;
	}

	if (_session) {
		_session->add_instant_xml(get_state());
	} else {
		Config->add_instant_xml(get_state());
	}
}

void
Editor::zoom_adjustment_changed ()
{
	if (_session == 0) {
		return;
	}

	double fpu = zoom_range_clock.current_duration() / _canvas_width;

	if (fpu < 1.0) {
		fpu = 1.0;
		zoom_range_clock.set ((framepos_t) floor (fpu * _canvas_width));
	} else if (fpu > _session->current_end_frame() / _canvas_width) {
		fpu = _session->current_end_frame() / _canvas_width;
		zoom_range_clock.set ((framepos_t) floor (fpu * _canvas_width));
	}

	temporal_zoom (fpu);
}

void
Editor::control_scroll (float fraction)
{
	ENSURE_GUI_THREAD (*this, &Editor::control_scroll, fraction)

	if (!_session) {
		return;
	}

	double step = fraction * current_page_frames();

	/*
		_control_scroll_target is an optional<T>

		it acts like a pointer to an framepos_t, with
		a operator conversion to boolean to check
		that it has a value could possibly use
		playhead_cursor->current_frame to store the
		value and a boolean in the class to know
		when it's out of date
	*/

	if (!_control_scroll_target) {
		_control_scroll_target = _session->transport_frame();
		_dragging_playhead = true;
	}

	if ((fraction < 0.0f) && (*_control_scroll_target < (framepos_t) fabs(step))) {
		*_control_scroll_target = 0;
	} else if ((fraction > 0.0f) && (max_framepos - *_control_scroll_target < step)) {
		*_control_scroll_target = max_framepos - (current_page_frames()*2); // allow room for slop in where the PH is on the screen
	} else {
		*_control_scroll_target += (framepos_t) floor (step);
	}

	/* move visuals, we'll catch up with it later */

	playhead_cursor->set_position (*_control_scroll_target);
	UpdateAllTransportClocks (*_control_scroll_target);

	if (*_control_scroll_target > (current_page_frames() / 2)) {
		/* try to center PH in window */
		reset_x_origin (*_control_scroll_target - (current_page_frames()/2));
	} else {
		reset_x_origin (0);
	}

	/*
		Now we do a timeout to actually bring the session to the right place
		according to the playhead. This is to avoid reading disk buffers on every
		call to control_scroll, which is driven by ScrollTimeline and therefore
		probably by a control surface wheel which can generate lots of events.
	*/
	/* cancel the existing timeout */

	control_scroll_connection.disconnect ();

	/* add the next timeout */

	control_scroll_connection = Glib::signal_timeout().connect (sigc::bind (sigc::mem_fun (*this, &Editor::deferred_control_scroll), *_control_scroll_target), 250);
}

bool
Editor::deferred_control_scroll (framepos_t /*target*/)
{
	_session->request_locate (*_control_scroll_target, _session->transport_rolling());
	// reset for next stream
	_control_scroll_target = boost::none;
	_dragging_playhead = false;
	return false;
}

void
Editor::access_action (std::string action_group, std::string action_item)
{
	if (!_session) {
		return;
	}

	ENSURE_GUI_THREAD (*this, &Editor::access_action, action_group, action_item)

	RefPtr<Action> act;
	act = ActionManager::get_action( action_group.c_str(), action_item.c_str() );

	if (act) {
		act->activate();
	}
}

void
Editor::on_realize ()
{
	Window::on_realize ();
	Realized ();
}

void
Editor::map_position_change (framepos_t frame)
{
	ENSURE_GUI_THREAD (*this, &Editor::map_position_change, frame)

	if (_session == 0) {
		return;
	}

	if (_follow_playhead) {
		center_screen (frame);
	}

	playhead_cursor->set_position (frame);
}

void
Editor::center_screen (framepos_t frame)
{
	double page = _canvas_width * frames_per_unit;

	/* if we're off the page, then scroll.
	 */

	if (frame < leftmost_frame || frame >= leftmost_frame + page) {
		center_screen_internal (frame, page);
	}
}

void
Editor::center_screen_internal (framepos_t frame, float page)
{
	page /= 2;

	if (frame > page) {
		frame -= (framepos_t) page;
	} else {
		frame = 0;
	}

	reset_x_origin (frame);
}


void
Editor::update_title ()
{
	ENSURE_GUI_THREAD (*this, &Editor::update_title)

	if (_session) {
		bool dirty = _session->dirty();

		string session_name;

		if (_session->snap_name() != _session->name()) {
			session_name = _session->snap_name();
		} else {
			session_name = _session->name();
		}

		if (dirty) {
			session_name = "*" + session_name;
		}

		WindowTitle title(session_name);
		title += Glib::get_application_name();
		set_title (title.get_string());
	}
}

void
Editor::set_session (Session *t)
{
	SessionHandlePtr::set_session (t);

	if (!_session) {
		return;
	}

	zoom_range_clock.set_session (_session);
	_playlist_selector->set_session (_session);
	nudge_clock.set_session (_session);
	_summary->set_session (_session);
	_group_tabs->set_session (_session);
	_route_groups->set_session (_session);
	_regions->set_session (_session);
	_snapshots->set_session (_session);
	_routes->set_session (_session);
	_locations->set_session (_session);

	if (rhythm_ferret) {
		rhythm_ferret->set_session (_session);
	}

	if (analysis_window) {
		analysis_window->set_session (_session);
	}

	if (sfbrowser) {
		sfbrowser->set_session (_session);
	}

	compute_fixed_ruler_scale ();

	XMLNode* node = ARDOUR_UI::instance()->editor_settings();
	set_state (*node, Stateful::loading_state_version);

	/* catch up with the playhead */

	_session->request_locate (playhead_cursor->current_frame);
	_pending_initial_locate = true;

	update_title ();

	/* These signals can all be emitted by a non-GUI thread. Therefore the
	   handlers for them must not attempt to directly interact with the GUI,
	   but use Gtkmm2ext::UI::instance()->call_slot();
	*/

	_session->StepEditStatusChange.connect (_session_connections, invalidator (*this), ui_bind(&Editor::step_edit_status_change, this, _1), gui_context());
	_session->TransportStateChange.connect (_session_connections, invalidator (*this), boost::bind (&Editor::map_transport_state, this), gui_context());
	_session->PositionChanged.connect (_session_connections, invalidator (*this), ui_bind (&Editor::map_position_change, this, _1), gui_context());
	_session->RouteAdded.connect (_session_connections, invalidator (*this), ui_bind (&Editor::handle_new_route, this, _1), gui_context());
	_session->DirtyChanged.connect (_session_connections, invalidator (*this), boost::bind (&Editor::update_title, this), gui_context());
	_session->TimecodeOffsetChanged.connect (_session_connections, invalidator (*this), boost::bind (&Editor::update_just_timecode, this), gui_context());
	_session->tempo_map().PropertyChanged.connect (_session_connections, invalidator (*this), ui_bind (&Editor::tempo_map_changed, this, _1), gui_context());
	_session->Located.connect (_session_connections, invalidator (*this), boost::bind (&Editor::located, this), gui_context());
	_session->config.ParameterChanged.connect (_session_connections, invalidator (*this), ui_bind (&Editor::parameter_changed, this, _1), gui_context());
	_session->StateSaved.connect (_session_connections, invalidator (*this), ui_bind (&Editor::session_state_saved, this, _1), gui_context());
	_session->locations()->added.connect (_session_connections, invalidator (*this), ui_bind (&Editor::add_new_location, this, _1), gui_context());
	_session->locations()->removed.connect (_session_connections, invalidator (*this), ui_bind (&Editor::location_gone, this, _1), gui_context());
	_session->locations()->changed.connect (_session_connections, invalidator (*this), boost::bind (&Editor::refresh_location_display, this), gui_context());
	_session->locations()->StateChanged.connect (_session_connections, invalidator (*this), ui_bind (&Editor::refresh_location_display_s, this, _1), gui_context());
	_session->history().Changed.connect (_session_connections, invalidator (*this), boost::bind (&Editor::history_changed, this), gui_context());

	if (Profile->get_sae()) {
		BBT_Time bbt;
		bbt.bars = 0;
		bbt.beats = 0;
		bbt.ticks = 120;
		nframes_t pos = _session->tempo_map().bbt_duration_at (0, bbt, 1);
		nudge_clock.set_mode(AudioClock::BBT);
		nudge_clock.set (pos, true, 0, AudioClock::BBT);

	} else {
		nudge_clock.set (_session->frame_rate() * 5, true, 0, AudioClock::Timecode); // default of 5 seconds
	}

	playhead_cursor->canvas_item.show ();

	Location* loc = _session->locations()->auto_loop_location();
	if (loc == 0) {
		loc = new Location (*_session, 0, _session->current_end_frame(), _("Loop"),(Location::Flags) (Location::IsAutoLoop | Location::IsHidden));
		if (loc->start() == loc->end()) {
			loc->set_end (loc->start() + 1);
		}
		_session->locations()->add (loc, false);
		_session->set_auto_loop_location (loc);
	} else {
		// force name
		loc->set_name (_("Loop"));
	}

	loc = _session->locations()->auto_punch_location();
	if (loc == 0) {
		loc = new Location (*_session, 0, _session->current_end_frame(), _("Punch"), (Location::Flags) (Location::IsAutoPunch | Location::IsHidden));
		if (loc->start() == loc->end()) {
			loc->set_end (loc->start() + 1);
		}
		_session->locations()->add (loc, false);
		_session->set_auto_punch_location (loc);
	} else {
		// force name
		loc->set_name (_("Punch"));
	}

	boost::function<void (string)> pc (boost::bind (&Editor::parameter_changed, this, _1));
	Config->map_parameters (pc);
	_session->config.map_parameters (pc);

	refresh_location_display ();

	restore_ruler_visibility ();
	//tempo_map_changed (PropertyChange (0));
	_session->tempo_map().apply_with_metrics (*this, &Editor::draw_metric_marks);

	for (TrackViewList::iterator i = track_views.begin(); i != track_views.end(); ++i) {
		(static_cast<TimeAxisView*>(*i))->set_samples_per_unit (frames_per_unit);
	}

	super_rapid_screen_update_connection = ARDOUR_UI::instance()->SuperRapidScreenUpdate.connect (
		sigc::mem_fun (*this, &Editor::super_rapid_screen_update)
		);
	
	switch (_snap_type) {
	case SnapToRegionStart:
	case SnapToRegionEnd:
	case SnapToRegionSync:
	case SnapToRegionBoundary:
		build_region_boundary_cache ();
		break;

	default:
		break;
	}

	/* register for undo history */
	_session->register_with_memento_command_factory(_id, this);

	ActionManager::ui_manager->signal_pre_activate().connect (sigc::mem_fun (*this, &Editor::action_pre_activated));

	start_updating_meters ();
}

void
Editor::action_pre_activated (Glib::RefPtr<Action> const & a)
{
	if (a->get_name() == "RegionMenu") {
		/* When the main menu's region menu is opened, we setup the actions so that they look right
		   in the menu.  I can't find a way of getting a signal when this menu is subsequently closed,
		   so we resensitize all region actions when the entered regionview or the region selection
		   changes.  HOWEVER we can't always resensitize on entered_regionview change because that
		   happens after the region context menu is opened.  So we set a flag here, too.

		   What a carry on :(
		*/
		sensitize_the_right_region_actions ();
		_last_region_menu_was_main = true;
	}
}

/** Pop up a context menu for when the user clicks on a fade in or fade out */
void
Editor::popup_fade_context_menu (int button, int32_t time, ArdourCanvas::Item* item, ItemType item_type)
{
	using namespace Menu_Helpers;
	AudioRegionView* arv = static_cast<AudioRegionView*> (item->get_data ("regionview"));

	if (arv == 0) {
		fatal << _("programming error: fade in canvas item has no regionview data pointer!") << endmsg;
		/*NOTREACHED*/
	}

	MenuList& items (fade_context_menu.items());

	items.clear ();

	switch (item_type) {
	case FadeInItem:
	case FadeInHandleItem:
		if (arv->audio_region()->fade_in_active()) {
			items.push_back (MenuElem (_("Deactivate"), sigc::bind (sigc::mem_fun (*this, &Editor::set_fade_in_active), false)));
		} else {
			items.push_back (MenuElem (_("Activate"), sigc::bind (sigc::mem_fun (*this, &Editor::set_fade_in_active), true)));
		}

		items.push_back (SeparatorElem());

		if (Profile->get_sae()) {
			
			items.push_back (MenuElem (_("Linear"), sigc::bind (sigc::mem_fun (*this, &Editor::set_fade_in_shape), FadeLinear)));
			items.push_back (MenuElem (_("Slowest"), sigc::bind (sigc::mem_fun (*this, &Editor::set_fade_in_shape), FadeFast)));
			
		} else {

			items.push_back (
				ImageMenuElem (
					_("Linear"),
					*_fade_in_images[FadeLinear],
					sigc::bind (sigc::mem_fun (*this, &Editor::set_fade_in_shape), FadeLinear)
					)
				);

			dynamic_cast<ImageMenuItem*>(&items.back())->set_always_show_image ();

			items.push_back (
				ImageMenuElem (
					_("Slowest"),
					*_fade_in_images[FadeFast],
					sigc::bind (sigc::mem_fun (*this, &Editor::set_fade_in_shape), FadeFast)
					));
			
			dynamic_cast<ImageMenuItem*>(&items.back())->set_always_show_image ();
			
			items.push_back (
				ImageMenuElem (
					_("Slow"),
					*_fade_in_images[FadeLogB],
					sigc::bind (sigc::mem_fun (*this, &Editor::set_fade_in_shape), FadeLogB)
					));
			
			dynamic_cast<ImageMenuItem*>(&items.back())->set_always_show_image ();
			
			items.push_back (
				ImageMenuElem (
					_("Fast"),
					*_fade_in_images[FadeLogA],
					sigc::bind (sigc::mem_fun (*this, &Editor::set_fade_in_shape), FadeLogA)
					));
			
			dynamic_cast<ImageMenuItem*>(&items.back())->set_always_show_image ();
			
			items.push_back (
				ImageMenuElem (
					_("Fastest"),
					*_fade_in_images[FadeSlow],
					sigc::bind (sigc::mem_fun (*this, &Editor::set_fade_in_shape), FadeSlow)
					));
			
			dynamic_cast<ImageMenuItem*>(&items.back())->set_always_show_image ();
		}

		break;

	case FadeOutItem:
	case FadeOutHandleItem:
		if (arv->audio_region()->fade_out_active()) {
			items.push_back (MenuElem (_("Deactivate"), sigc::bind (sigc::mem_fun (*this, &Editor::set_fade_out_active), false)));
		} else {
			items.push_back (MenuElem (_("Activate"), sigc::bind (sigc::mem_fun (*this, &Editor::set_fade_out_active), true)));
		}

		items.push_back (SeparatorElem());

		if (Profile->get_sae()) {
			items.push_back (MenuElem (_("Linear"), sigc::bind (sigc::mem_fun (*this, &Editor::set_fade_out_shape), FadeLinear)));
			items.push_back (MenuElem (_("Slowest"), sigc::bind (sigc::mem_fun (*this, &Editor::set_fade_out_shape), FadeSlow)));
		} else {

			items.push_back (
				ImageMenuElem (
					_("Linear"),
					*_fade_out_images[FadeLinear],
					sigc::bind (sigc::mem_fun (*this, &Editor::set_fade_out_shape), FadeLinear)
					)
				);

			dynamic_cast<ImageMenuItem*>(&items.back())->set_always_show_image ();

			items.push_back (
				ImageMenuElem (
					_("Slowest"),
					*_fade_out_images[FadeFast],
					sigc::bind (sigc::mem_fun (*this, &Editor::set_fade_out_shape), FadeSlow)
					));
			
			dynamic_cast<ImageMenuItem*>(&items.back())->set_always_show_image ();
			
			items.push_back (
				ImageMenuElem (
					_("Slow"),
					*_fade_out_images[FadeLogB],
					sigc::bind (sigc::mem_fun (*this, &Editor::set_fade_out_shape), FadeLogA)
					));
			
			dynamic_cast<ImageMenuItem*>(&items.back())->set_always_show_image ();
			
			items.push_back (
				ImageMenuElem (
					_("Fast"),
					*_fade_out_images[FadeLogA],
					sigc::bind (sigc::mem_fun (*this, &Editor::set_fade_out_shape), FadeLogB)
					));
			
			dynamic_cast<ImageMenuItem*>(&items.back())->set_always_show_image ();
			
			items.push_back (
				ImageMenuElem (
					_("Fastest"),
					*_fade_out_images[FadeSlow],
					sigc::bind (sigc::mem_fun (*this, &Editor::set_fade_out_shape), FadeFast)
					));
			
			dynamic_cast<ImageMenuItem*>(&items.back())->set_always_show_image ();
		}

		break;

	default:
		fatal << _("programming error: ")
		      << X_("non-fade canvas item passed to popup_fade_context_menu()")
		      << endmsg;
		/*NOTREACHED*/
	}

	fade_context_menu.popup (button, time);
}

void
Editor::popup_track_context_menu (int button, int32_t time, ItemType item_type, bool with_selection)
{
	using namespace Menu_Helpers;
	Menu* (Editor::*build_menu_function)();
	Menu *menu;

	switch (item_type) {
	case RegionItem:
	case RegionViewName:
	case RegionViewNameHighlight:
	case LeftFrameHandle:
	case RightFrameHandle:
		if (with_selection) {
			build_menu_function = &Editor::build_track_selection_context_menu;
		} else {
			build_menu_function = &Editor::build_track_region_context_menu;
		}
		break;

	case SelectionItem:
		if (with_selection) {
			build_menu_function = &Editor::build_track_selection_context_menu;
		} else {
			build_menu_function = &Editor::build_track_context_menu;
		}
		break;

	case CrossfadeViewItem:
		build_menu_function = &Editor::build_track_crossfade_context_menu;
		break;

	case StreamItem:
		if (clicked_routeview->track()) {
			build_menu_function = &Editor::build_track_context_menu;
		} else {
			build_menu_function = &Editor::build_track_bus_context_menu;
		}
		break;

	default:
		/* probably shouldn't happen but if it does, we don't care */
		return;
	}

	menu = (this->*build_menu_function)();
	menu->set_name ("ArdourContextMenu");

	/* now handle specific situations */

	switch (item_type) {
	case RegionItem:
	case RegionViewName:
	case RegionViewNameHighlight:
	case LeftFrameHandle:
	case RightFrameHandle:
		if (!with_selection) {
			if (region_edit_menu_split_item) {
				if (clicked_regionview && clicked_regionview->region()->covers (get_preferred_edit_position())) {
					ActionManager::set_sensitive (ActionManager::edit_point_in_region_sensitive_actions, true);
				} else {
					ActionManager::set_sensitive (ActionManager::edit_point_in_region_sensitive_actions, false);
				}
			}
			if (region_edit_menu_split_multichannel_item) {
				if (clicked_regionview && clicked_regionview->region()->n_channels() > 1) {
					region_edit_menu_split_multichannel_item->set_sensitive (true);
				} else {
					region_edit_menu_split_multichannel_item->set_sensitive (false);
				}
			}
		}
		break;

	case SelectionItem:
		break;

	case CrossfadeViewItem:
		break;

	case StreamItem:
		break;

	default:
		/* probably shouldn't happen but if it does, we don't care */
		return;
	}

	if (item_type != SelectionItem && clicked_routeview && clicked_routeview->audio_track()) {

		/* Bounce to disk */

		using namespace Menu_Helpers;
		MenuList& edit_items  = menu->items();

		edit_items.push_back (SeparatorElem());

		switch (clicked_routeview->audio_track()->freeze_state()) {
		case AudioTrack::NoFreeze:
			edit_items.push_back (MenuElem (_("Freeze"), sigc::mem_fun(*this, &Editor::freeze_route)));
			break;

		case AudioTrack::Frozen:
			edit_items.push_back (MenuElem (_("Unfreeze"), sigc::mem_fun(*this, &Editor::unfreeze_route)));
			break;

		case AudioTrack::UnFrozen:
			edit_items.push_back (MenuElem (_("Freeze"), sigc::mem_fun(*this, &Editor::freeze_route)));
			break;
		}

	}

	if (item_type == StreamItem && clicked_routeview) {
		clicked_routeview->build_underlay_menu(menu);
	}

	/* When the region menu is opened, we setup the actions so that they look right
	   in the menu.
	*/
	sensitize_the_right_region_actions ();
	_last_region_menu_was_main = false;

	menu->signal_hide().connect (sigc::bind (sigc::mem_fun (*this, &Editor::sensitize_all_region_actions), true));
	menu->popup (button, time);
}

Menu*
Editor::build_track_context_menu ()
{
	using namespace Menu_Helpers;

 	MenuList& edit_items = track_context_menu.items();
	edit_items.clear();

	add_dstream_context_items (edit_items);
	return &track_context_menu;
}

Menu*
Editor::build_track_bus_context_menu ()
{
	using namespace Menu_Helpers;

 	MenuList& edit_items = track_context_menu.items();
	edit_items.clear();

	add_bus_context_items (edit_items);
	return &track_context_menu;
}

Menu*
Editor::build_track_region_context_menu ()
{
	using namespace Menu_Helpers;
	MenuList& edit_items  = track_region_context_menu.items();
	edit_items.clear();

	/* we've just cleared the track region context menu, so the menu that these
	   two items were on will have disappeared; stop them dangling.
	*/
	region_edit_menu_split_item = 0;
	region_edit_menu_split_multichannel_item = 0;

	RouteTimeAxisView* rtv = dynamic_cast<RouteTimeAxisView*> (clicked_axisview);

	if (rtv) {
		boost::shared_ptr<Track> tr;
		boost::shared_ptr<Playlist> pl;

		/* Don't offer a region submenu if we are in internal edit mode, as we don't select regions in this
		   mode and so offering region context is somewhat confusing.
		*/
		if ((tr = rtv->track()) && ((pl = tr->playlist())) && !internal_editing()) {
			framepos_t const framepos = (framepos_t) floor ((double) get_preferred_edit_position() * tr->speed());
			uint32_t regions_at = pl->count_regions_at (framepos);
			add_region_context_items (edit_items, regions_at > 1);
		}
	}

	add_dstream_context_items (edit_items);

	return &track_region_context_menu;
}

Menu*
Editor::build_track_crossfade_context_menu ()
{
	using namespace Menu_Helpers;
	MenuList& edit_items  = track_crossfade_context_menu.items();
	edit_items.clear ();

	AudioTimeAxisView* atv = dynamic_cast<AudioTimeAxisView*> (clicked_axisview);

	if (atv) {
		boost::shared_ptr<Track> tr;
		boost::shared_ptr<Playlist> pl;
		boost::shared_ptr<AudioPlaylist> apl;

		if ((tr = atv->track()) && ((pl = tr->playlist()) != 0) && ((apl = boost::dynamic_pointer_cast<AudioPlaylist> (pl)) != 0)) {

			AudioPlaylist::Crossfades xfades;

			apl->crossfades_at (get_preferred_edit_position (), xfades);

			bool many = xfades.size() > 1;

			for (AudioPlaylist::Crossfades::iterator i = xfades.begin(); i != xfades.end(); ++i) {
				add_crossfade_context_items (atv->audio_view(), (*i), edit_items, many);
			}

			framepos_t framepos = (framepos_t) floor ((double) get_preferred_edit_position() * tr->speed());
			uint32_t regions_at = pl->count_regions_at (framepos);
			add_region_context_items (edit_items, regions_at > 1);
		}
	}

	add_dstream_context_items (edit_items);

	return &track_crossfade_context_menu;
}

void
Editor::analyze_region_selection ()
{
	if (analysis_window == 0) {
		analysis_window = new AnalysisWindow();

		if (_session != 0)
			analysis_window->set_session(_session);

		analysis_window->show_all();
	}

	analysis_window->set_regionmode();
	analysis_window->analyze();

	analysis_window->present();
}

void
Editor::analyze_range_selection()
{
	if (analysis_window == 0) {
		analysis_window = new AnalysisWindow();

		if (_session != 0)
			analysis_window->set_session(_session);

		analysis_window->show_all();
	}

	analysis_window->set_rangemode();
	analysis_window->analyze();

	analysis_window->present();
}

Menu*
Editor::build_track_selection_context_menu ()
{
	using namespace Menu_Helpers;
	MenuList& edit_items  = track_selection_context_menu.items();
	edit_items.clear ();

	add_selection_context_items (edit_items);
	// edit_items.push_back (SeparatorElem());
	// add_dstream_context_items (edit_items);

	return &track_selection_context_menu;
}

/** Add context menu items relevant to crossfades.
 * @param edit_items List to add the items to.
 */
void
Editor::add_crossfade_context_items (AudioStreamView* /*view*/, boost::shared_ptr<Crossfade> xfade, Menu_Helpers::MenuList& edit_items, bool many)
{
	using namespace Menu_Helpers;
	Menu     *xfade_menu = manage (new Menu);
	MenuList& items       = xfade_menu->items();
	xfade_menu->set_name ("ArdourContextMenu");
	string str;

	if (xfade->active()) {
		str = _("Mute");
	} else {
		str = _("Unmute");
	}

	items.push_back (MenuElem (str, sigc::bind (sigc::mem_fun(*this, &Editor::toggle_xfade_active), boost::weak_ptr<Crossfade> (xfade))));
	items.push_back (MenuElem (_("Edit..."), sigc::bind (sigc::mem_fun(*this, &Editor::edit_xfade), boost::weak_ptr<Crossfade> (xfade))));

	if (xfade->can_follow_overlap()) {

		if (xfade->following_overlap()) {
			str = _("Convert to Short");
		} else {
			str = _("Convert to Full");
		}

		items.push_back (MenuElem (str, sigc::bind (sigc::mem_fun(*this, &Editor::toggle_xfade_length), xfade)));
	}

	if (many) {
		str = xfade->out()->name();
		str += "->";
		str += xfade->in()->name();
	} else {
		str = _("Crossfade");
	}

	edit_items.push_back (MenuElem (str, *xfade_menu));
	edit_items.push_back (SeparatorElem());
}

void
Editor::xfade_edit_left_region ()
{
	if (clicked_crossfadeview) {
		clicked_crossfadeview->left_view.show_region_editor ();
	}
}

void
Editor::xfade_edit_right_region ()
{
	if (clicked_crossfadeview) {
		clicked_crossfadeview->right_view.show_region_editor ();
	}
}

void
Editor::add_region_context_items (Menu_Helpers::MenuList& edit_items, bool multiple_regions_at_position)
{
	using namespace Menu_Helpers;
	
	/* OK, stick the region submenu at the top of the list, and then add
	   the standard items.
	*/

	/* we have to hack up the region name because "_" has a special
	   meaning for menu titles.
	*/

	RegionSelection rs = get_regions_from_selection_and_entered ();
	
	string::size_type pos = 0;
	string menu_item_name = (rs.size() == 1) ? rs.front()->region()->name() : _("Selected Regions");

	while ((pos = menu_item_name.find ("_", pos)) != string::npos) {
		menu_item_name.replace (pos, 1, "__");
		pos += 2;
	}

	if (_popup_region_menu_item == 0) {
		_popup_region_menu_item = new MenuItem (menu_item_name);
		_popup_region_menu_item->set_submenu (*dynamic_cast<Menu*> (ActionManager::get_widget (X_("/PopupRegionMenu"))));
		_popup_region_menu_item->show ();
	} else {
		_popup_region_menu_item->set_label (menu_item_name);
	}

	edit_items.push_back (*_popup_region_menu_item);
	if (multiple_regions_at_position && (layering_order_editor == 0 || !layering_order_editor->is_visible ())) {
		edit_items.push_back (*manage (_region_actions->get_action ("choose-top-region")->create_menu_item ()));
	}
	edit_items.push_back (SeparatorElem());
}

/** Add context menu items relevant to selection ranges.
 * @param edit_items List to add the items to.
 */
void
Editor::add_selection_context_items (Menu_Helpers::MenuList& edit_items)
{
	using namespace Menu_Helpers;

	edit_items.push_back (MenuElem (_("Play Range"), sigc::mem_fun(*this, &Editor::play_selection)));
	edit_items.push_back (MenuElem (_("Loop Range"), sigc::bind (sigc::mem_fun(*this, &Editor::set_loop_from_selection), true)));

	edit_items.push_back (SeparatorElem());
	edit_items.push_back (MenuElem (_("Spectral Analysis"), sigc::mem_fun(*this, &Editor::analyze_range_selection)));

	if (!selection->regions.empty()) {
		edit_items.push_back (SeparatorElem());
		edit_items.push_back (MenuElem (_("Extend Range to End of Region"), sigc::bind (sigc::mem_fun(*this, &Editor::extend_selection_to_end_of_region), false)));
		edit_items.push_back (MenuElem (_("Extend Range to Start of Region"), sigc::bind (sigc::mem_fun(*this, &Editor::extend_selection_to_start_of_region), false)));
	}

	edit_items.push_back (SeparatorElem());
	edit_items.push_back (MenuElem (_("Convert to region in-place"), mem_fun(*this, &Editor::separate_region_from_selection)));
	edit_items.push_back (MenuElem (_("Convert to Region in Region List"), sigc::mem_fun(*this, &Editor::new_region_from_selection)));

	edit_items.push_back (SeparatorElem());
	edit_items.push_back (MenuElem (_("Select All in Range"), sigc::mem_fun(*this, &Editor::select_all_selectables_using_time_selection)));

	edit_items.push_back (SeparatorElem());
	edit_items.push_back (MenuElem (_("Set Loop from Range"), sigc::bind (sigc::mem_fun(*this, &Editor::set_loop_from_selection), false)));
	edit_items.push_back (MenuElem (_("Set Punch from Range"), sigc::mem_fun(*this, &Editor::set_punch_from_selection)));

	edit_items.push_back (SeparatorElem());
	edit_items.push_back (MenuElem (_("Add Range Markers"), sigc::mem_fun (*this, &Editor::add_location_from_selection)));

	edit_items.push_back (SeparatorElem());
	edit_items.push_back (MenuElem (_("Crop Region to Range"), sigc::mem_fun(*this, &Editor::crop_region_to_selection)));
	edit_items.push_back (MenuElem (_("Fill Range with Region"), sigc::mem_fun(*this, &Editor::region_fill_selection)));
	edit_items.push_back (MenuElem (_("Duplicate Range"), sigc::bind (sigc::mem_fun(*this, &Editor::duplicate_dialog), false)));

	edit_items.push_back (SeparatorElem());
	edit_items.push_back (MenuElem (_("Consolidate Range"), sigc::bind (sigc::mem_fun(*this, &Editor::bounce_range_selection), true, false)));
	edit_items.push_back (MenuElem (_("Consolidate Range With Processing"), sigc::bind (sigc::mem_fun(*this, &Editor::bounce_range_selection), true, true)));
	edit_items.push_back (MenuElem (_("Bounce Range to Region List"), sigc::bind (sigc::mem_fun(*this, &Editor::bounce_range_selection), false, false)));
	edit_items.push_back (MenuElem (_("Bounce Range to Region List With Processing"), sigc::bind (sigc::mem_fun(*this, &Editor::bounce_range_selection), false, true)));
	edit_items.push_back (MenuElem (_("Export Range"), sigc::mem_fun(*this, &Editor::export_selection)));
}


void
Editor::add_dstream_context_items (Menu_Helpers::MenuList& edit_items)
{
	using namespace Menu_Helpers;

	/* Playback */

	Menu *play_menu = manage (new Menu);
	MenuList& play_items = play_menu->items();
	play_menu->set_name ("ArdourContextMenu");

	play_items.push_back (MenuElem (_("Play From Edit Point"), sigc::mem_fun(*this, &Editor::play_from_edit_point)));
	play_items.push_back (MenuElem (_("Play From Start"), sigc::mem_fun(*this, &Editor::play_from_start)));
	play_items.push_back (MenuElem (_("Play Region"), sigc::mem_fun(*this, &Editor::play_selected_region)));
	play_items.push_back (SeparatorElem());
	play_items.push_back (MenuElem (_("Loop Region"), sigc::bind (sigc::mem_fun (*this, &Editor::set_loop_from_region), true)));

	edit_items.push_back (MenuElem (_("Play"), *play_menu));

	/* Selection */

	Menu *select_menu = manage (new Menu);
	MenuList& select_items = select_menu->items();
	select_menu->set_name ("ArdourContextMenu");

	select_items.push_back (MenuElem (_("Select All in Track"), sigc::bind (sigc::mem_fun(*this, &Editor::select_all_in_track), Selection::Set)));
	select_items.push_back (MenuElem (_("Select All"), sigc::bind (sigc::mem_fun(*this, &Editor::select_all), Selection::Set)));
	select_items.push_back (MenuElem (_("Invert Selection in Track"), sigc::mem_fun(*this, &Editor::invert_selection_in_track)));
	select_items.push_back (MenuElem (_("Invert Selection"), sigc::mem_fun(*this, &Editor::invert_selection)));
	select_items.push_back (SeparatorElem());
	select_items.push_back (MenuElem (_("Set Range to Loop Range"), sigc::mem_fun(*this, &Editor::set_selection_from_loop)));
	select_items.push_back (MenuElem (_("Set Range to Punch Range"), sigc::mem_fun(*this, &Editor::set_selection_from_punch)));
	select_items.push_back (SeparatorElem());
	select_items.push_back (MenuElem (_("Select All After Edit Point"), sigc::bind (sigc::mem_fun(*this, &Editor::select_all_selectables_using_edit), true)));
	select_items.push_back (MenuElem (_("Select All Before Edit Point"), sigc::bind (sigc::mem_fun(*this, &Editor::select_all_selectables_using_edit), false)));
	select_items.push_back (MenuElem (_("Select All After Playhead"), sigc::bind (sigc::mem_fun(*this, &Editor::select_all_selectables_using_cursor), playhead_cursor, true)));
	select_items.push_back (MenuElem (_("Select All Before Playhead"), sigc::bind (sigc::mem_fun(*this, &Editor::select_all_selectables_using_cursor), playhead_cursor, false)));
	select_items.push_back (MenuElem (_("Select All Between Playhead and Edit Point"), sigc::bind (sigc::mem_fun(*this, &Editor::select_all_selectables_between), false)));
	select_items.push_back (MenuElem (_("Select All Within Playhead and Edit Point"), sigc::bind (sigc::mem_fun(*this, &Editor::select_all_selectables_between), true)));
	select_items.push_back (MenuElem (_("Select Range Between Playhead and Edit Point"), sigc::mem_fun(*this, &Editor::select_range_between)));

	edit_items.push_back (MenuElem (_("Select"), *select_menu));

	/* Cut-n-Paste */

	Menu *cutnpaste_menu = manage (new Menu);
	MenuList& cutnpaste_items = cutnpaste_menu->items();
	cutnpaste_menu->set_name ("ArdourContextMenu");

	cutnpaste_items.push_back (MenuElem (_("Cut"), sigc::mem_fun(*this, &Editor::cut)));
	cutnpaste_items.push_back (MenuElem (_("Copy"), sigc::mem_fun(*this, &Editor::copy)));
	cutnpaste_items.push_back (MenuElem (_("Paste"), sigc::bind (sigc::mem_fun(*this, &Editor::paste), 1.0f)));

	cutnpaste_items.push_back (SeparatorElem());

	cutnpaste_items.push_back (MenuElem (_("Align"), sigc::bind (sigc::mem_fun (*this, &Editor::align_regions), ARDOUR::SyncPoint)));
	cutnpaste_items.push_back (MenuElem (_("Align Relative"), sigc::bind (sigc::mem_fun (*this, &Editor::align_regions_relative), ARDOUR::SyncPoint)));

	edit_items.push_back (MenuElem (_("Edit"), *cutnpaste_menu));

	/* Adding new material */

	edit_items.push_back (SeparatorElem());
	edit_items.push_back (MenuElem (_("Insert Selected Region"), sigc::bind (sigc::mem_fun(*this, &Editor::insert_region_list_selection), 1.0f)));
	edit_items.push_back (MenuElem (_("Insert Existing Media"), sigc::bind (sigc::mem_fun(*this, &Editor::add_external_audio_action), ImportToTrack)));

	/* Nudge track */

	Menu *nudge_menu = manage (new Menu());
	MenuList& nudge_items = nudge_menu->items();
	nudge_menu->set_name ("ArdourContextMenu");

	edit_items.push_back (SeparatorElem());
	nudge_items.push_back (MenuElem (_("Nudge Entire Track Forward"), (sigc::bind (sigc::mem_fun(*this, &Editor::nudge_track), false, true))));
	nudge_items.push_back (MenuElem (_("Nudge Track After Edit Point Forward"), (sigc::bind (sigc::mem_fun(*this, &Editor::nudge_track), true, true))));
	nudge_items.push_back (MenuElem (_("Nudge Entire Track Backward"), (sigc::bind (sigc::mem_fun(*this, &Editor::nudge_track), false, false))));
	nudge_items.push_back (MenuElem (_("Nudge Track After Edit Point Backward"), (sigc::bind (sigc::mem_fun(*this, &Editor::nudge_track), true, false))));

	edit_items.push_back (MenuElem (_("Nudge"), *nudge_menu));
}

void
Editor::add_bus_context_items (Menu_Helpers::MenuList& edit_items)
{
	using namespace Menu_Helpers;

	/* Playback */

	Menu *play_menu = manage (new Menu);
	MenuList& play_items = play_menu->items();
	play_menu->set_name ("ArdourContextMenu");

	play_items.push_back (MenuElem (_("Play From Edit Point"), sigc::mem_fun(*this, &Editor::play_from_edit_point)));
	play_items.push_back (MenuElem (_("Play From Start"), sigc::mem_fun(*this, &Editor::play_from_start)));
	edit_items.push_back (MenuElem (_("Play"), *play_menu));

	/* Selection */

	Menu *select_menu = manage (new Menu);
	MenuList& select_items = select_menu->items();
	select_menu->set_name ("ArdourContextMenu");

	select_items.push_back (MenuElem (_("Select All in Track"), sigc::bind (sigc::mem_fun(*this, &Editor::select_all_in_track), Selection::Set)));
	select_items.push_back (MenuElem (_("Select All"), sigc::bind (sigc::mem_fun(*this, &Editor::select_all), Selection::Set)));
	select_items.push_back (MenuElem (_("Invert Selection in Track"), sigc::mem_fun(*this, &Editor::invert_selection_in_track)));
	select_items.push_back (MenuElem (_("Invert Selection"), sigc::mem_fun(*this, &Editor::invert_selection)));
	select_items.push_back (SeparatorElem());
	select_items.push_back (MenuElem (_("Select All After Edit Point"), sigc::bind (sigc::mem_fun(*this, &Editor::select_all_selectables_using_edit), true)));
	select_items.push_back (MenuElem (_("Select All Before Edit Point"), sigc::bind (sigc::mem_fun(*this, &Editor::select_all_selectables_using_edit), false)));
	select_items.push_back (MenuElem (_("Select All After Playhead"), sigc::bind (sigc::mem_fun(*this, &Editor::select_all_selectables_using_cursor), playhead_cursor, true)));
	select_items.push_back (MenuElem (_("Select All Before Playhead"), sigc::bind (sigc::mem_fun(*this, &Editor::select_all_selectables_using_cursor), playhead_cursor, false)));

	edit_items.push_back (MenuElem (_("Select"), *select_menu));

	/* Cut-n-Paste */

	Menu *cutnpaste_menu = manage (new Menu);
	MenuList& cutnpaste_items = cutnpaste_menu->items();
	cutnpaste_menu->set_name ("ArdourContextMenu");

	cutnpaste_items.push_back (MenuElem (_("Cut"), sigc::mem_fun(*this, &Editor::cut)));
	cutnpaste_items.push_back (MenuElem (_("Copy"), sigc::mem_fun(*this, &Editor::copy)));
	cutnpaste_items.push_back (MenuElem (_("Paste"), sigc::bind (sigc::mem_fun(*this, &Editor::paste), 1.0f)));

	Menu *nudge_menu = manage (new Menu());
	MenuList& nudge_items = nudge_menu->items();
	nudge_menu->set_name ("ArdourContextMenu");

	edit_items.push_back (SeparatorElem());
	nudge_items.push_back (MenuElem (_("Nudge Entire Track Forward"), (sigc::bind (sigc::mem_fun(*this, &Editor::nudge_track), false, true))));
	nudge_items.push_back (MenuElem (_("Nudge Track After Edit Point Forward"), (sigc::bind (sigc::mem_fun(*this, &Editor::nudge_track), true, true))));
	nudge_items.push_back (MenuElem (_("Nudge Entire Track Backward"), (sigc::bind (sigc::mem_fun(*this, &Editor::nudge_track), false, false))));
	nudge_items.push_back (MenuElem (_("Nudge Track After Edit Point Backward"), (sigc::bind (sigc::mem_fun(*this, &Editor::nudge_track), true, false))));

	edit_items.push_back (MenuElem (_("Nudge"), *nudge_menu));
}

SnapType
Editor::snap_type() const
{
	return _snap_type;
}

SnapMode
Editor::snap_mode() const
{
	return _snap_mode;
}

void
Editor::set_snap_to (SnapType st)
{
	unsigned int snap_ind = (unsigned int)st;

	_snap_type = st;

	if (snap_ind > snap_type_strings.size() - 1) {
		snap_ind = 0;
		_snap_type = (SnapType)snap_ind;
	}

	string str = snap_type_strings[snap_ind];

	if (str != snap_type_selector.get_active_text()) {
		snap_type_selector.set_active_text (str);
	}

	instant_save ();

	switch (_snap_type) {
	case SnapToBeatDiv32:
	case SnapToBeatDiv28:
	case SnapToBeatDiv24:
	case SnapToBeatDiv20:
	case SnapToBeatDiv16:
	case SnapToBeatDiv14:
	case SnapToBeatDiv12:
	case SnapToBeatDiv10:
	case SnapToBeatDiv8:
	case SnapToBeatDiv7:
	case SnapToBeatDiv6:
	case SnapToBeatDiv5:
	case SnapToBeatDiv4:
	case SnapToBeatDiv3:
	case SnapToBeatDiv2:
		compute_bbt_ruler_scale (leftmost_frame, leftmost_frame + current_page_frames());
		update_tempo_based_rulers ();
		break;

	case SnapToRegionStart:
	case SnapToRegionEnd:
	case SnapToRegionSync:
	case SnapToRegionBoundary:
		build_region_boundary_cache ();
		break;

	default:
		/* relax */
		break;
	}

	SnapChanged (); /* EMIT SIGNAL */
}

void
Editor::set_snap_mode (SnapMode mode)
{
	_snap_mode = mode;
	string str = snap_mode_strings[(int)mode];

	if (str != snap_mode_selector.get_active_text ()) {
		snap_mode_selector.set_active_text (str);
	}

	instant_save ();
}
void
Editor::set_edit_point_preference (EditPoint ep, bool force)
{
	bool changed = (_edit_point != ep);

	_edit_point = ep;
	string str = edit_point_strings[(int)ep];

	if (str != edit_point_selector.get_active_text ()) {
		edit_point_selector.set_active_text (str);
	}

	set_canvas_cursor ();

	if (!force && !changed) {
		return;
	}

	const char* action=NULL;

	switch (_edit_point) {
	case EditAtPlayhead:
		action = "edit-at-playhead";
		break;
	case EditAtSelectedMarker:
		action = "edit-at-marker";
		break;
	case EditAtMouse:
		action = "edit-at-mouse";
		break;
	}

	Glib::RefPtr<Action> act = ActionManager::get_action ("Editor", action);
	if (act) {
		Glib::RefPtr<RadioAction>::cast_dynamic(act)->set_active (true);
	}

	framepos_t foo;
	bool in_track_canvas;

	if (!mouse_frame (foo, in_track_canvas)) {
		in_track_canvas = false;
	}

	reset_canvas_action_sensitivity (in_track_canvas);

	instant_save ();
}

int
Editor::set_state (const XMLNode& node, int /*version*/)
{
	const XMLProperty* prop;
	XMLNode* geometry;
	int x, y, xoff, yoff;
	Gdk::Geometry g;

	if ((prop = node.property ("id")) != 0) {
		_id = prop->value ();
	}

	g.base_width = default_width;
	g.base_height = default_height;
	x = 1;
	y = 1;
	xoff = 0;
	yoff = 21;

	if ((geometry = find_named_node (node, "geometry")) != 0) {

		XMLProperty* prop;

		if ((prop = geometry->property("x_size")) == 0) {
			prop = geometry->property ("x-size");
		}
		if (prop) {
			g.base_width = atoi(prop->value());
		}
		if ((prop = geometry->property("y_size")) == 0) {
			prop = geometry->property ("y-size");
		}
		if (prop) {
			g.base_height = atoi(prop->value());
		}

		if ((prop = geometry->property ("x_pos")) == 0) {
			prop = geometry->property ("x-pos");
		}
		if (prop) {
			x = atoi (prop->value());

		}
		if ((prop = geometry->property ("y_pos")) == 0) {
			prop = geometry->property ("y-pos");
		}
		if (prop) {
			y = atoi (prop->value());
		}

		if ((prop = geometry->property ("x_off")) == 0) {
			prop = geometry->property ("x-off");
		}
		if (prop) {
			xoff = atoi (prop->value());
		}
		if ((prop = geometry->property ("y_off")) == 0) {
			prop = geometry->property ("y-off");
		}
		if (prop) {
			yoff = atoi (prop->value());
		}
	}

	set_default_size (g.base_width, g.base_height);
	move (x, y);

	if (_session && (prop = node.property ("playhead"))) {
		framepos_t pos;
		sscanf (prop->value().c_str(), "%" PRIi64, &pos);
		playhead_cursor->set_position (pos);
	} else {
		playhead_cursor->set_position (0);
	}
	
	if ((prop = node.property ("mixer-width"))) {
		editor_mixer_strip_width = Width (string_2_enum (prop->value(), editor_mixer_strip_width));
	}

	if ((prop = node.property ("zoom-focus"))) {
		set_zoom_focus ((ZoomFocus) atoi (prop->value()));
	}

	if ((prop = node.property ("zoom"))) {
		reset_zoom (PBD::atof (prop->value()));
	} else {
		reset_zoom (frames_per_unit);
	}

	if ((prop = node.property ("snap-to"))) {
		set_snap_to ((SnapType) atoi (prop->value()));
	}

	if ((prop = node.property ("snap-mode"))) {
		set_snap_mode ((SnapMode) atoi (prop->value()));
	}

	if ((prop = node.property ("mouse-mode"))) {
		MouseMode m = str2mousemode(prop->value());
		set_mouse_mode (m, true);
	} else {
		set_mouse_mode (MouseObject, true);
	}

	if ((prop = node.property ("left-frame")) != 0){
		framepos_t pos;
		if (sscanf (prop->value().c_str(), "%" PRId64, &pos) == 1) {
			reset_x_origin (pos);
		}
	}

	if ((prop = node.property ("y-origin")) != 0) {
		reset_y_origin (atof (prop->value ()));
	}

	if ((prop = node.property ("internal-edit"))) {
		bool yn = string_is_affirmative (prop->value());
		RefPtr<Action> act = ActionManager::get_action (X_("MouseMode"), X_("toggle-internal-edit"));
		if (act) {
			RefPtr<ToggleAction> tact = RefPtr<ToggleAction>::cast_dynamic(act);
			tact->set_active (!yn);
			tact->set_active (yn);
		}
	}

	if ((prop = node.property ("join-object-range"))) {
		join_object_range_button.set_active (string_is_affirmative (prop->value ()));
	}

	if ((prop = node.property ("edit-point"))) {
		set_edit_point_preference ((EditPoint) string_2_enum (prop->value(), _edit_point), true);
	}

	if ((prop = node.property ("show-measures"))) {
		bool yn = string_is_affirmative (prop->value());
		_show_measures = yn;
		RefPtr<Action> act = ActionManager::get_action (X_("Editor"), X_("ToggleMeasureVisibility"));
		if (act) {
			RefPtr<ToggleAction> tact = RefPtr<ToggleAction>::cast_dynamic(act);
			/* do it twice to force the change */
			tact->set_active (!yn);
			tact->set_active (yn);
		}
	}

	if ((prop = node.property ("follow-playhead"))) {
		bool yn = string_is_affirmative (prop->value());
		set_follow_playhead (yn);
		RefPtr<Action> act = ActionManager::get_action (X_("Editor"), X_("toggle-follow-playhead"));
		if (act) {
			RefPtr<ToggleAction> tact = RefPtr<ToggleAction>::cast_dynamic(act);
			if (tact->get_active() != yn) {
				tact->set_active (yn);
			}
		}
	}

	if ((prop = node.property ("stationary-playhead"))) {
		bool yn = (prop->value() == "yes");
		set_stationary_playhead (yn);
		RefPtr<Action> act = ActionManager::get_action (X_("Editor"), X_("toggle-stationary-playhead"));
		if (act) {
			RefPtr<ToggleAction> tact = RefPtr<ToggleAction>::cast_dynamic(act);
			if (tact->get_active() != yn) {
				tact->set_active (yn);
			}
		}
	}
        
	if ((prop = node.property ("region-list-sort-type"))) {
		RegionListSortType st;
		_regions->reset_sort_type ((RegionListSortType) string_2_enum (prop->value(), st), true);
	}

	if ((prop = node.property ("xfades-visible"))) {
		bool yn = string_is_affirmative (prop->value());
		_xfade_visibility = !yn;
		// set_xfade_visibility (yn);
	}

	if ((prop = node.property ("show-editor-mixer"))) {

		Glib::RefPtr<Action> act = ActionManager::get_action (X_("Editor"), X_("show-editor-mixer"));
		assert (act);

		Glib::RefPtr<ToggleAction> tact = Glib::RefPtr<ToggleAction>::cast_dynamic(act);
		bool yn = string_is_affirmative (prop->value());
		
		/* do it twice to force the change */
		
		tact->set_active (!yn);
		tact->set_active (yn);
	}

	if ((prop = node.property ("show-editor-list"))) {

		Glib::RefPtr<Action> act = ActionManager::get_action (X_("Editor"), X_("show-editor-list"));
		assert (act);

		Glib::RefPtr<ToggleAction> tact = Glib::RefPtr<ToggleAction>::cast_dynamic(act);
		bool yn = string_is_affirmative (prop->value());
		
		/* do it twice to force the change */
		
		tact->set_active (!yn);
		tact->set_active (yn);
	}

	if ((prop = node.property (X_("editor-list-page")))) {
		the_notebook.set_current_page (atoi (prop->value ()));
	}

	if ((prop = node.property (X_("show-marker-lines")))) {
		Glib::RefPtr<Action> act = ActionManager::get_action (X_("Editor"), X_("show-marker-lines"));
		assert (act);
		Glib::RefPtr<ToggleAction> tact = Glib::RefPtr<ToggleAction>::cast_dynamic (act);
		bool yn = string_is_affirmative (prop->value ());

		tact->set_active (!yn);
		tact->set_active (yn);
	}

	XMLNodeList children = node.children ();
	for (XMLNodeList::const_iterator i = children.begin(); i != children.end(); ++i) {
		selection->set_state (**i, Stateful::current_state_version);
		_regions->set_state (**i);
	}

	return 0;
}

XMLNode&
Editor::get_state ()
{
	XMLNode* node = new XMLNode ("Editor");
	char buf[32];

	_id.print (buf, sizeof (buf));
	node->add_property ("id", buf);

	if (is_realized()) {
		Glib::RefPtr<Gdk::Window> win = get_window();

		int x, y, xoff, yoff, width, height;
		win->get_root_origin(x, y);
		win->get_position(xoff, yoff);
		win->get_size(width, height);

		XMLNode* geometry = new XMLNode ("geometry");

		snprintf(buf, sizeof(buf), "%d", width);
		geometry->add_property("x-size", string(buf));
		snprintf(buf, sizeof(buf), "%d", height);
		geometry->add_property("y-size", string(buf));
		snprintf(buf, sizeof(buf), "%d", x);
		geometry->add_property("x-pos", string(buf));
		snprintf(buf, sizeof(buf), "%d", y);
		geometry->add_property("y-pos", string(buf));
		snprintf(buf, sizeof(buf), "%d", xoff);
		geometry->add_property("x-off", string(buf));
		snprintf(buf, sizeof(buf), "%d", yoff);
		geometry->add_property("y-off", string(buf));
		snprintf(buf,sizeof(buf), "%d",gtk_paned_get_position (static_cast<Paned*>(&edit_pane)->gobj()));
		geometry->add_property("edit-horizontal-pane-pos", string(buf));
		snprintf(buf,sizeof(buf), "%d",gtk_paned_get_position (static_cast<Paned*>(&editor_summary_pane)->gobj()));
		geometry->add_property("edit-vertical-pane-pos", string(buf));

		node->add_child_nocopy (*geometry);
	}

	maybe_add_mixer_strip_width (*node);

	snprintf (buf, sizeof(buf), "%d", (int) zoom_focus);
	node->add_property ("zoom-focus", buf);
	snprintf (buf, sizeof(buf), "%f", frames_per_unit);
	node->add_property ("zoom", buf);
	snprintf (buf, sizeof(buf), "%d", (int) _snap_type);
	node->add_property ("snap-to", buf);
	snprintf (buf, sizeof(buf), "%d", (int) _snap_mode);
	node->add_property ("snap-mode", buf);

	node->add_property ("edit-point", enum_2_string (_edit_point));

	snprintf (buf, sizeof (buf), "%" PRIi64, playhead_cursor->current_frame);
	node->add_property ("playhead", buf);
	snprintf (buf, sizeof (buf), "%" PRIi64, leftmost_frame);
	node->add_property ("left-frame", buf);
	snprintf (buf, sizeof (buf), "%f", vertical_adjustment.get_value ());
	node->add_property ("y-origin", buf);

	node->add_property ("show-measures", _show_measures ? "yes" : "no");
	node->add_property ("follow-playhead", _follow_playhead ? "yes" : "no");
	node->add_property ("stationary-playhead", _stationary_playhead ? "yes" : "no");
	node->add_property ("xfades-visible", _xfade_visibility ? "yes" : "no");
	node->add_property ("region-list-sort-type", enum_2_string (_regions->sort_type ()));
	node->add_property ("mouse-mode", enum2str(mouse_mode));
	node->add_property ("internal-edit", _internal_editing ? "yes" : "no");
	node->add_property ("join-object-range", join_object_range_button.get_active () ? "yes" : "no");

	Glib::RefPtr<Action> act = ActionManager::get_action (X_("Editor"), X_("show-editor-mixer"));
	if (act) {
		Glib::RefPtr<ToggleAction> tact = Glib::RefPtr<ToggleAction>::cast_dynamic(act);
		node->add_property (X_("show-editor-mixer"), tact->get_active() ? "yes" : "no");
	}

	act = ActionManager::get_action (X_("Editor"), X_("show-editor-list"));
	if (act) {
		Glib::RefPtr<ToggleAction> tact = Glib::RefPtr<ToggleAction>::cast_dynamic(act);
		node->add_property (X_("show-editor-list"), tact->get_active() ? "yes" : "no");
	}

	snprintf (buf, sizeof (buf), "%d", the_notebook.get_current_page ());
	node->add_property (X_("editor-list-page"), buf);

	node->add_property (X_("show-marker-lines"), _show_marker_lines ? "yes" : "no");

	node->add_child_nocopy (selection->get_state ());
	node->add_child_nocopy (_regions->get_state ());
	
	return *node;
}



/** @param y y offset from the top of all trackviews.
 *  @return pair: TimeAxisView that y is over, layer index.
 *  TimeAxisView may be 0.  Layer index is the layer number if the TimeAxisView is valid and is
 *  in stacked region display mode, otherwise 0.
 */
std::pair<TimeAxisView *, layer_t>
Editor::trackview_by_y_position (double y)
{
	for (TrackViewList::iterator iter = track_views.begin(); iter != track_views.end(); ++iter) {

		std::pair<TimeAxisView*, int> const r = (*iter)->covers_y_position (y);
		if (r.first) {
			return r;
		}
	}

	return std::make_pair ( (TimeAxisView *) 0, 0);
}

/** Snap a position to the grid, if appropriate, taking into account current
 *  grid settings and also the state of any snap modifier keys that may be pressed.
 *  @param start Position to snap.
 *  @param event Event to get current key modifier information from, or 0.
 */
void
Editor::snap_to_with_modifier (framepos_t& start, GdkEvent const * event, int32_t direction, bool for_mark)
{
	if (!_session || !event) {
		return;
	}

	if (Keyboard::modifier_state_contains (event->button.state, Keyboard::snap_modifier())) {
		if (_snap_mode == SnapOff) {
			snap_to_internal (start, direction, for_mark);
		}
	} else {
		if (_snap_mode != SnapOff) {
			snap_to_internal (start, direction, for_mark);
		}
	}
}

void
Editor::snap_to (framepos_t& start, int32_t direction, bool for_mark)
{
	if (!_session || _snap_mode == SnapOff) {
		return;
	}

	snap_to_internal (start, direction, for_mark);
}

void
Editor::timecode_snap_to_internal (framepos_t& start, int32_t direction, bool /*for_mark*/)
{
	const framepos_t one_timecode_second = (framepos_t)(rint(_session->timecode_frames_per_second()) * _session->frames_per_timecode_frame());
	framepos_t one_timecode_minute = (framepos_t)(rint(_session->timecode_frames_per_second()) * _session->frames_per_timecode_frame() * 60);

	switch (_snap_type) {
	case SnapToTimecodeFrame:
		if (((direction == 0) && (fmod((double)start, (double)_session->frames_per_timecode_frame()) > (_session->frames_per_timecode_frame() / 2))) || (direction > 0)) {
			start = (framepos_t) (ceil ((double) start / _session->frames_per_timecode_frame()) * _session->frames_per_timecode_frame());
		} else {
			start = (framepos_t) (floor ((double) start / _session->frames_per_timecode_frame()) *  _session->frames_per_timecode_frame());
		}
		break;

	case SnapToTimecodeSeconds:
		if (_session->timecode_offset_negative())
		{
			start += _session->timecode_offset ();
		} else {
			start -= _session->timecode_offset ();
		}
		if (((direction == 0) && (start % one_timecode_second > one_timecode_second / 2)) || direction > 0) {
			start = (framepos_t) ceil ((double) start / one_timecode_second) * one_timecode_second;
		} else {
			start = (framepos_t) floor ((double) start / one_timecode_second) * one_timecode_second;
		}

		if (_session->timecode_offset_negative())
		{
			start -= _session->timecode_offset ();
		} else {
			start += _session->timecode_offset ();
		}
		break;

	case SnapToTimecodeMinutes:
		if (_session->timecode_offset_negative())
		{
			start += _session->timecode_offset ();
		} else {
			start -= _session->timecode_offset ();
		}
		if (((direction == 0) && (start % one_timecode_minute > one_timecode_minute / 2)) || direction > 0) {
			start = (framepos_t) ceil ((double) start / one_timecode_minute) * one_timecode_minute;
		} else {
			start = (framepos_t) floor ((double) start / one_timecode_minute) * one_timecode_minute;
		}
		if (_session->timecode_offset_negative())
		{
			start -= _session->timecode_offset ();
		} else {
			start += _session->timecode_offset ();
		}
		break;
	default:
		fatal << "Editor::smpte_snap_to_internal() called with non-timecode snap type!" << endmsg;
		/*NOTREACHED*/
	}
}

void
Editor::snap_to_internal (framepos_t& start, int32_t direction, bool for_mark)
{
	const framepos_t one_second = _session->frame_rate();
	const framepos_t one_minute = _session->frame_rate() * 60;
	framepos_t presnap = start;
	framepos_t before;
	framepos_t after;

	switch (_snap_type) {
	case SnapToTimecodeFrame:
	case SnapToTimecodeSeconds:
	case SnapToTimecodeMinutes:
		return timecode_snap_to_internal (start, direction, for_mark);

	case SnapToCDFrame:
		if (((direction == 0) && (start % (one_second/75) > (one_second/75) / 2)) || (direction > 0)) {
			start = (framepos_t) ceil ((double) start / (one_second / 75)) * (one_second / 75);
		} else {
			start = (framepos_t) floor ((double) start / (one_second / 75)) * (one_second / 75);
		}
		break;

	case SnapToSeconds:
		if (((direction == 0) && (start % one_second > one_second / 2)) || (direction > 0)) {
			start = (framepos_t) ceil ((double) start / one_second) * one_second;
		} else {
			start = (framepos_t) floor ((double) start / one_second) * one_second;
		}
		break;

	case SnapToMinutes:
		if (((direction == 0) && (start % one_minute > one_minute / 2)) || (direction > 0)) {
			start = (framepos_t) ceil ((double) start / one_minute) * one_minute;
		} else {
			start = (framepos_t) floor ((double) start / one_minute) * one_minute;
		}
		break;

	case SnapToBar:
		start = _session->tempo_map().round_to_bar (start, direction);
		break;

	case SnapToBeat:
		start = _session->tempo_map().round_to_beat (start, direction);
		break;

	case SnapToBeatDiv32:
		start = _session->tempo_map().round_to_beat_subdivision (start, 32, direction);
		break;
	case SnapToBeatDiv28:
		start = _session->tempo_map().round_to_beat_subdivision (start, 28, direction);
		break;
	case SnapToBeatDiv24:
		start = _session->tempo_map().round_to_beat_subdivision (start, 24, direction);
		break;
	case SnapToBeatDiv20:
		start = _session->tempo_map().round_to_beat_subdivision (start, 20, direction);
		break;
	case SnapToBeatDiv16:
		start = _session->tempo_map().round_to_beat_subdivision (start, 16, direction);
		break;
	case SnapToBeatDiv14:
		start = _session->tempo_map().round_to_beat_subdivision (start, 14, direction);
		break;
	case SnapToBeatDiv12:
		start = _session->tempo_map().round_to_beat_subdivision (start, 12, direction);
		break;
	case SnapToBeatDiv10:
		start = _session->tempo_map().round_to_beat_subdivision (start, 10, direction);
		break;
	case SnapToBeatDiv8:
		start = _session->tempo_map().round_to_beat_subdivision (start, 8, direction);
		break;
	case SnapToBeatDiv7:
		start = _session->tempo_map().round_to_beat_subdivision (start, 7, direction);
		break;
	case SnapToBeatDiv6:
		start = _session->tempo_map().round_to_beat_subdivision (start, 6, direction);
		break;
	case SnapToBeatDiv5:
		start = _session->tempo_map().round_to_beat_subdivision (start, 5, direction);
		break;
	case SnapToBeatDiv4:
		start = _session->tempo_map().round_to_beat_subdivision (start, 4, direction);
		break;
	case SnapToBeatDiv3:
		start = _session->tempo_map().round_to_beat_subdivision (start, 3, direction);
		break;
	case SnapToBeatDiv2:
		start = _session->tempo_map().round_to_beat_subdivision (start, 2, direction);
		break;

	case SnapToMark:
		if (for_mark) {
			return;
		}

		_session->locations()->marks_either_side (start, before, after);

		if (before == max_framepos) {
			start = after;
		} else if (after == max_framepos) {
			start = before;
		} else if (before != max_framepos && after != max_framepos) {
			/* have before and after */
			if ((start - before) < (after - start)) {
				start = before;
			} else {
				start = after;
			}
		}

		break;

	case SnapToRegionStart:
	case SnapToRegionEnd:
	case SnapToRegionSync:
	case SnapToRegionBoundary:
		if (!region_boundary_cache.empty()) {

			vector<framepos_t>::iterator prev = region_boundary_cache.end ();
			vector<framepos_t>::iterator next = region_boundary_cache.end ();

			if (direction > 0) {
				next = std::upper_bound (region_boundary_cache.begin(), region_boundary_cache.end(), start);
			} else {
				next = std::lower_bound (region_boundary_cache.begin(), region_boundary_cache.end(), start);
			}

			if (next != region_boundary_cache.begin ()) {
				prev = next;
				prev--;
			}

			framepos_t const p = (prev == region_boundary_cache.end()) ? region_boundary_cache.front () : *prev;
			framepos_t const n = (next == region_boundary_cache.end()) ? region_boundary_cache.back () : *next;

			if (start > (p + n) / 2) {
				start = n;
			} else {
				start = p;
			}
		}
		break;
	}

	switch (_snap_mode) {
	case SnapNormal:
		return;

	case SnapMagnetic:

		if (presnap > start) {
			if (presnap > (start + unit_to_frame(snap_threshold))) {
				start = presnap;
			}

		} else if (presnap < start) {
			if (presnap < (start - unit_to_frame(snap_threshold))) {
				start = presnap;
			}
		}

	default:
		/* handled at entry */
		return;

	}
}


void
Editor::setup_toolbar ()
{
	string pixmap_path;

	/* Mode Buttons (tool selection) */

	mouse_move_button.set_relief(Gtk::RELIEF_NONE);
	mouse_select_button.set_relief(Gtk::RELIEF_NONE);
	mouse_gain_button.set_relief(Gtk::RELIEF_NONE);
	mouse_zoom_button.set_relief(Gtk::RELIEF_NONE);
	mouse_timefx_button.set_relief(Gtk::RELIEF_NONE);
	mouse_audition_button.set_relief(Gtk::RELIEF_NONE);
	// internal_edit_button.set_relief(Gtk::RELIEF_NONE);
	join_object_range_button.set_relief(Gtk::RELIEF_NONE);

	HBox* mode_box = manage(new HBox);
	mode_box->set_border_width (2);
	mode_box->set_spacing(4);

	/* table containing mode buttons */

	HBox* mouse_mode_button_box = manage (new HBox ());

	if (Profile->get_sae()) {
		mouse_mode_button_box->pack_start (mouse_move_button);
	} else {
		mouse_mode_button_box->pack_start (mouse_move_button);
		mouse_mode_button_box->pack_start (join_object_range_button);
		mouse_mode_button_box->pack_start (mouse_select_button);
	}

	mouse_mode_button_box->pack_start (mouse_zoom_button);

	if (!Profile->get_sae()) {
		mouse_mode_button_box->pack_start (mouse_gain_button);
	}

	mouse_mode_button_box->pack_start (mouse_timefx_button);
	mouse_mode_button_box->pack_start (mouse_audition_button);
	mouse_mode_button_box->pack_start (internal_edit_button);

	vector<string> edit_mode_strings;
	edit_mode_strings.push_back (edit_mode_to_string (Slide));
	if (!Profile->get_sae()) {
		edit_mode_strings.push_back (edit_mode_to_string (Splice));
	}
	edit_mode_strings.push_back (edit_mode_to_string (Lock));

	edit_mode_selector.set_name ("EditModeSelector");
	set_popdown_strings (edit_mode_selector, edit_mode_strings, true);
	edit_mode_selector.signal_changed().connect (sigc::mem_fun(*this, &Editor::edit_mode_selection_done));

	mode_box->pack_start (edit_mode_selector);
	mode_box->pack_start (*mouse_mode_button_box);

	_mouse_mode_tearoff = manage (new TearOff (*mode_box));
	_mouse_mode_tearoff->set_name ("MouseModeBase");
	_mouse_mode_tearoff->tearoff_window().signal_key_press_event().connect (sigc::bind (sigc::ptr_fun (relay_key_press), &_mouse_mode_tearoff->tearoff_window()), false);

	if (Profile->get_sae()) {
		_mouse_mode_tearoff->set_can_be_torn_off (false);
	}

	_mouse_mode_tearoff->Detach.connect (sigc::bind (sigc::mem_fun(*this, &Editor::detach_tearoff), static_cast<Box*>(&toolbar_hbox),
	                                                 &_mouse_mode_tearoff->tearoff_window()));
	_mouse_mode_tearoff->Attach.connect (sigc::bind (sigc::mem_fun(*this, &Editor::reattach_tearoff), static_cast<Box*> (&toolbar_hbox),
	                                                 &_mouse_mode_tearoff->tearoff_window(), 1));
	_mouse_mode_tearoff->Hidden.connect (sigc::bind (sigc::mem_fun(*this, &Editor::detach_tearoff), static_cast<Box*>(&toolbar_hbox),
	                                                 &_mouse_mode_tearoff->tearoff_window()));
	_mouse_mode_tearoff->Visible.connect (sigc::bind (sigc::mem_fun(*this, &Editor::reattach_tearoff), static_cast<Box*> (&toolbar_hbox),
	                                                  &_mouse_mode_tearoff->tearoff_window(), 1));

	mouse_move_button.set_mode (false);
	mouse_select_button.set_mode (false);
	mouse_gain_button.set_mode (false);
	mouse_zoom_button.set_mode (false);
	mouse_timefx_button.set_mode (false);
	mouse_audition_button.set_mode (false);
	join_object_range_button.set_mode (false);

	mouse_move_button.set_name ("MouseModeButton");
	mouse_select_button.set_name ("MouseModeButton");
	mouse_gain_button.set_name ("MouseModeButton");
	mouse_zoom_button.set_name ("MouseModeButton");
	mouse_timefx_button.set_name ("MouseModeButton");
	mouse_audition_button.set_name ("MouseModeButton");
	internal_edit_button.set_name ("MouseModeButton");
	join_object_range_button.set_name ("MouseModeButton");

	mouse_move_button.unset_flags (CAN_FOCUS);
	mouse_select_button.unset_flags (CAN_FOCUS);
	mouse_gain_button.unset_flags (CAN_FOCUS);
	mouse_zoom_button.unset_flags (CAN_FOCUS);
	mouse_timefx_button.unset_flags (CAN_FOCUS);
	mouse_audition_button.unset_flags (CAN_FOCUS);
	internal_edit_button.unset_flags (CAN_FOCUS);
	join_object_range_button.unset_flags (CAN_FOCUS);

	/* Zoom */

	_zoom_box.set_spacing (1);
	_zoom_box.set_border_width (0);

	zoom_in_button.set_name ("EditorTimeButton");
	zoom_in_button.set_image (*(manage (new Image (Stock::ZOOM_IN, Gtk::ICON_SIZE_MENU))));
	zoom_in_button.signal_clicked().connect (sigc::bind (sigc::mem_fun(*this, &Editor::temporal_zoom_step), false));

	zoom_out_button.set_name ("EditorTimeButton");
	zoom_out_button.set_image (*(manage (new Image (Stock::ZOOM_OUT, Gtk::ICON_SIZE_MENU))));
	zoom_out_button.signal_clicked().connect (sigc::bind (sigc::mem_fun(*this, &Editor::temporal_zoom_step), true));

	zoom_out_full_button.set_name ("EditorTimeButton");
	zoom_out_full_button.set_image (*(manage (new Image (Stock::ZOOM_100, Gtk::ICON_SIZE_MENU))));
	zoom_out_full_button.signal_clicked().connect (sigc::mem_fun(*this, &Editor::temporal_zoom_session));

	zoom_focus_selector.set_name ("ZoomFocusSelector");
	set_popdown_strings (zoom_focus_selector, zoom_focus_strings, true);
	zoom_focus_selector.signal_changed().connect (sigc::mem_fun(*this, &Editor::zoom_focus_selection_done));

	_zoom_box.pack_start (zoom_out_button, false, false);
	_zoom_box.pack_start (zoom_in_button, false, false);
	_zoom_box.pack_start (zoom_out_full_button, false, false);

	_zoom_box.pack_start (zoom_focus_selector);
	
	/* Track zoom buttons */
	tav_expand_button.set_name ("TrackHeightButton");
	tav_expand_button.set_size_request(-1,20);
	tav_expand_button.add (*(manage (new Image (::get_icon("tav_exp")))));
	tav_expand_button.signal_clicked().connect (sigc::bind (sigc::mem_fun(*this, &Editor::tav_zoom_step), true));

	tav_shrink_button.set_name ("TrackHeightButton");
	tav_shrink_button.set_size_request(-1,20);
	tav_shrink_button.add (*(manage (new Image (::get_icon("tav_shrink")))));
	tav_shrink_button.signal_clicked().connect (sigc::bind (sigc::mem_fun(*this, &Editor::tav_zoom_step), false));

	_zoom_box.pack_start (tav_shrink_button);
	_zoom_box.pack_start (tav_expand_button);
	
	_zoom_tearoff = manage (new TearOff (_zoom_box));

	_zoom_tearoff->Detach.connect (sigc::bind (sigc::mem_fun(*this, &Editor::detach_tearoff), static_cast<Box*>(&toolbar_hbox),
						   &_zoom_tearoff->tearoff_window()));
	_zoom_tearoff->Attach.connect (sigc::bind (sigc::mem_fun(*this, &Editor::reattach_tearoff), static_cast<Box*> (&toolbar_hbox),
						   &_zoom_tearoff->tearoff_window(), 0));
	_zoom_tearoff->Hidden.connect (sigc::bind (sigc::mem_fun(*this, &Editor::detach_tearoff), static_cast<Box*>(&toolbar_hbox),
						   &_zoom_tearoff->tearoff_window()));
	_zoom_tearoff->Visible.connect (sigc::bind (sigc::mem_fun(*this, &Editor::reattach_tearoff), static_cast<Box*> (&toolbar_hbox),
						    &_zoom_tearoff->tearoff_window(), 0));
	
	snap_box.set_spacing (1);
	snap_box.set_border_width (2);

	snap_type_selector.set_name ("SnapTypeSelector");
	set_popdown_strings (snap_type_selector, snap_type_strings, true);
	snap_type_selector.signal_changed().connect (sigc::mem_fun(*this, &Editor::snap_type_selection_done));

	snap_mode_selector.set_name ("SnapModeSelector");
	set_popdown_strings (snap_mode_selector, snap_mode_strings, true);
	snap_mode_selector.signal_changed().connect (sigc::mem_fun(*this, &Editor::snap_mode_selection_done));

	edit_point_selector.set_name ("EditPointSelector");
	set_popdown_strings (edit_point_selector, edit_point_strings, true);
	edit_point_selector.signal_changed().connect (sigc::mem_fun(*this, &Editor::edit_point_selection_done));

	snap_box.pack_start (snap_mode_selector, false, false);
	snap_box.pack_start (snap_type_selector, false, false);
	snap_box.pack_start (edit_point_selector, false, false);

	/* Nudge */

	HBox *nudge_box = manage (new HBox);
	nudge_box->set_spacing(1);
	nudge_box->set_border_width (2);

	nudge_forward_button.signal_button_release_event().connect (sigc::mem_fun(*this, &Editor::nudge_forward_release), false);
	nudge_backward_button.signal_button_release_event().connect (sigc::mem_fun(*this, &Editor::nudge_backward_release), false);

	nudge_box->pack_start (nudge_backward_button, false, false);
	nudge_box->pack_start (nudge_forward_button, false, false);
	nudge_box->pack_start (nudge_clock, false, false);


	/* Pack everything in... */

	HBox* hbox = manage (new HBox);
	hbox->set_spacing(10);

	_tools_tearoff = manage (new TearOff (*hbox));
	_tools_tearoff->set_name ("MouseModeBase");
	_tools_tearoff->tearoff_window().signal_key_press_event().connect (sigc::bind (sigc::ptr_fun (relay_key_press), &_tools_tearoff->tearoff_window()), false);
        
	if (Profile->get_sae()) {
		_tools_tearoff->set_can_be_torn_off (false);
	}

	_tools_tearoff->Detach.connect (sigc::bind (sigc::mem_fun(*this, &Editor::detach_tearoff), static_cast<Box*>(&toolbar_hbox),
	                                            &_tools_tearoff->tearoff_window()));
	_tools_tearoff->Attach.connect (sigc::bind (sigc::mem_fun(*this, &Editor::reattach_tearoff), static_cast<Box*> (&toolbar_hbox),
	                                            &_tools_tearoff->tearoff_window(), 0));
	_tools_tearoff->Hidden.connect (sigc::bind (sigc::mem_fun(*this, &Editor::detach_tearoff), static_cast<Box*>(&toolbar_hbox),
	                                            &_tools_tearoff->tearoff_window()));
	_tools_tearoff->Visible.connect (sigc::bind (sigc::mem_fun(*this, &Editor::reattach_tearoff), static_cast<Box*> (&toolbar_hbox),
	                                             &_tools_tearoff->tearoff_window(), 0));

	toolbar_hbox.set_spacing (10);
	toolbar_hbox.set_border_width (1);

	toolbar_hbox.pack_start (*_mouse_mode_tearoff, false, false);
	toolbar_hbox.pack_start (*_zoom_tearoff, false, false);
	toolbar_hbox.pack_start (*_tools_tearoff, false, false);

	hbox->pack_start (snap_box, false, false);
	hbox->pack_start (*nudge_box, false, false);
	hbox->pack_start (panic_box, false, false);

	hbox->show_all ();

	toolbar_base.set_name ("ToolBarBase");
	toolbar_base.add (toolbar_hbox);

	toolbar_frame.set_shadow_type (SHADOW_OUT);
	toolbar_frame.set_name ("BaseFrame");
	toolbar_frame.add (toolbar_base);
}

void
Editor::setup_tooltips ()
{
	ARDOUR_UI::instance()->set_tip (mouse_move_button, _("Select/Move Objects"));
	ARDOUR_UI::instance()->set_tip (mouse_gain_button, _("Draw Region Gain"));
	ARDOUR_UI::instance()->set_tip (mouse_zoom_button, _("Select Zoom Range"));
	ARDOUR_UI::instance()->set_tip (mouse_timefx_button, _("Stretch/Shrink Regions and MIDI Notes"));
	ARDOUR_UI::instance()->set_tip (mouse_audition_button, _("Listen to Specific Regions"));
	ARDOUR_UI::instance()->set_tip (join_object_range_button, _("Select/Move Objects or Ranges"));
	ARDOUR_UI::instance()->set_tip (internal_edit_button, _("Edit Region Contents (e.g. notes)"));
	ARDOUR_UI::instance()->set_tip (*_group_tabs, _("Groups: click to (de)activate\nContext-click for other operations"));
	ARDOUR_UI::instance()->set_tip (nudge_forward_button, _("Nudge Region/Selection Forwards"));
	ARDOUR_UI::instance()->set_tip (nudge_backward_button, _("Nudge Region/Selection Backwards"));
	ARDOUR_UI::instance()->set_tip (zoom_in_button, _("Zoom In"));
	ARDOUR_UI::instance()->set_tip (zoom_out_button, _("Zoom Out"));
	ARDOUR_UI::instance()->set_tip (zoom_out_full_button, _("Zoom to Session"));
	ARDOUR_UI::instance()->set_tip (zoom_focus_selector, _("Zoom focus"));
	ARDOUR_UI::instance()->set_tip (tav_expand_button, _("Expand Tracks"));
	ARDOUR_UI::instance()->set_tip (tav_shrink_button, _("Shrink Tracks"));
	ARDOUR_UI::instance()->set_tip (snap_type_selector, _("Snap/Grid Units"));
	ARDOUR_UI::instance()->set_tip (snap_mode_selector, _("Snap/Grid Mode"));
	ARDOUR_UI::instance()->set_tip (edit_point_selector, _("Edit point"));
	ARDOUR_UI::instance()->set_tip (midi_sound_notes, _("Sound Notes"));
	ARDOUR_UI::instance()->set_tip (midi_panic_button, _("Send note off and reset controller messages on all MIDI channels"));
	ARDOUR_UI::instance()->set_tip (edit_mode_selector, _("Edit Mode"));
}

void
Editor::midi_panic ()
{
	cerr << "MIDI panic\n";

	if (_session) {
		_session->midi_panic();
	}
}

void
Editor::setup_midi_toolbar ()
{
	RefPtr<Action> act;

	/* Midi sound notes */
	midi_sound_notes.add (*(manage (new Image (::get_icon("midi_sound_notes")))));
	midi_sound_notes.set_relief(Gtk::RELIEF_NONE);
	midi_sound_notes.unset_flags (CAN_FOCUS);

	/* Panic */

	act = ActionManager::get_action (X_("MIDI"), X_("panic"));
	midi_panic_button.set_name("MidiPanicButton");
	act->connect_proxy (midi_panic_button);

	panic_box.pack_start (midi_sound_notes , true, true);
	panic_box.pack_start (midi_panic_button, true, true);
}

int
Editor::convert_drop_to_paths (
		vector<string>&                paths,
		const RefPtr<Gdk::DragContext>& /*context*/,
		gint                            /*x*/,
		gint                            /*y*/,
		const SelectionData&            data,
		guint                           /*info*/,
		guint                           /*time*/)
{
	if (_session == 0) {
		return -1;
	}
        
	vector<string> uris = data.get_uris();

	if (uris.empty()) {

		/* This is seriously fucked up. Nautilus doesn't say that its URI lists
		   are actually URI lists. So do it by hand.
		*/

		if (data.get_target() != "text/plain") {
			return -1;
		}

		/* Parse the "uri-list" format that Nautilus provides,
		   where each pathname is delimited by \r\n.

		   THERE MAY BE NO NULL TERMINATING CHAR!!!
		*/

		string txt = data.get_text();
		const char* p;
		const char* q;

		p = (const char *) malloc (txt.length() + 1);
		txt.copy ((char *) p, txt.length(), 0);
		((char*)p)[txt.length()] = '\0';

		while (p)
		{
			if (*p != '#')
			{
				while (g_ascii_isspace (*p))
					p++;

				q = p;
				while (*q && (*q != '\n') && (*q != '\r')) {
					q++;
				}

				if (q > p)
				{
					q--;
					while (q > p && g_ascii_isspace (*q))
						q--;

					if (q > p)
					{
						uris.push_back (string (p, q - p + 1));
					}
				}
			}
			p = strchr (p, '\n');
			if (p)
				p++;
		}

		free ((void*)p);

		if (uris.empty()) {
			return -1;
		}
	}

	for (vector<string>::iterator i = uris.begin(); i != uris.end(); ++i) {

		if ((*i).substr (0,7) == "file://") {

			string p = *i;
			PBD::url_decode (p);

			// scan forward past three slashes

			string::size_type slashcnt = 0;
			string::size_type n = 0;
			string::iterator x = p.begin();

			while (slashcnt < 3 && x != p.end()) {
				if ((*x) == '/') {
					slashcnt++;
				} else if (slashcnt == 3) {
					break;
				}
				++n;
				++x;
			}

			if (slashcnt != 3 || x == p.end()) {
				error << _("malformed URL passed to drag-n-drop code") << endmsg;
				continue;
			}

			paths.push_back (p.substr (n - 1));
		}
	}

	return 0;
}

void
Editor::new_tempo_section ()

{
}

void
Editor::map_transport_state ()
{
	ENSURE_GUI_THREAD (*this, &Editor::map_transport_state)

	if (_session && _session->transport_stopped()) {
		have_pending_keyboard_selection = false;
	}

	update_loop_range_view (true);
}

/* UNDO/REDO */

Editor::State::State (PublicEditor const * e)
{
	selection = new Selection (e);
}

Editor::State::~State ()
{
	delete selection;
}

void
Editor::begin_reversible_command (string name)
{
	if (_session) {
		_session->begin_reversible_command (name);
	}
}

void
Editor::commit_reversible_command ()
{
	if (_session) {
		_session->commit_reversible_command ();
	}
}

void
Editor::set_route_group_solo (Route& route, bool yn)
{
	RouteGroup *route_group;

	if ((route_group = route.route_group()) != 0) {
		route_group->apply (&Route::set_solo, yn, this);
	} else {
		route.set_solo (yn, this);
	}
}

void
Editor::set_route_group_mute (Route& route, bool yn)
{
	RouteGroup *route_group = 0;

	if ((route_group = route.route_group()) != 0) {
		route_group->apply (&Route::set_mute, yn, this);
	} else {
		route.set_mute (yn, this);
	}
}

void
Editor::history_changed ()
{
	string label;

	if (undo_action && _session) {
		if (_session->undo_depth() == 0) {
			label = _("Undo");
		} else {
			label = string_compose(_("Undo (%1)"), _session->next_undo());
		}
		undo_action->property_label() = label;
	}

	if (redo_action && _session) {
		if (_session->redo_depth() == 0) {
			label = _("Redo");
		} else {
			label = string_compose(_("Redo (%1)"), _session->next_redo());
		}
		redo_action->property_label() = label;
	}
}

void
Editor::duplicate_dialog (bool with_dialog)
{
	float times = 1.0f;

	if (mouse_mode == MouseRange) {
		if (selection->time.length() == 0) {
			return;
		}
	}

	RegionSelection rs = get_regions_from_selection_and_entered ();

	if (mouse_mode != MouseRange && rs.empty()) {
		return;
	}

	if (with_dialog) {

		ArdourDialog win (_("Duplicate"));
		Label label (_("Number of duplications:"));
		Adjustment adjustment (1.0, 1.0, 1000000.0, 1.0, 5.0);
		SpinButton spinner (adjustment, 0.0, 1);
		HBox hbox;

		win.get_vbox()->set_spacing (12);
		win.get_vbox()->pack_start (hbox);
		hbox.set_border_width (6);
		hbox.pack_start (label, PACK_EXPAND_PADDING, 12);

		/* dialogs have ::add_action_widget() but that puts the spinner in the wrong
		   place, visually. so do this by hand.
		*/

		hbox.pack_start (spinner, PACK_EXPAND_PADDING, 12);
		spinner.signal_activate().connect (sigc::bind (sigc::mem_fun (win, &ArdourDialog::response), RESPONSE_ACCEPT));
		spinner.grab_focus();

		hbox.show ();
		label.show ();
		spinner.show ();

		win.add_button (Stock::CANCEL, RESPONSE_CANCEL);
		win.add_button (_("Duplicate"), RESPONSE_ACCEPT);
		win.set_default_response (RESPONSE_ACCEPT);

		win.set_position (WIN_POS_MOUSE);

		spinner.grab_focus ();

		switch (win.run ()) {
		case RESPONSE_ACCEPT:
			break;
		default:
			return;
		}

		times = adjustment.get_value();
	}

	if (mouse_mode == MouseRange) {
		duplicate_selection (times);
	} else {
		duplicate_some_regions (rs, times);
	}
}

void
Editor::show_verbose_canvas_cursor ()
{
	verbose_canvas_cursor->raise_to_top();
	verbose_canvas_cursor->show();
	verbose_cursor_visible = true;
}

void
Editor::hide_verbose_canvas_cursor ()
{
	verbose_canvas_cursor->hide();
	verbose_cursor_visible = false;
}

double
Editor::clamp_verbose_cursor_x (double x)
{
	if (x < 0) {
		x = 0;
	} else {
		x = min (_canvas_width - 200.0, x);
	}
	return x;
}

double
Editor::clamp_verbose_cursor_y (double y)
{
	if (y < canvas_timebars_vsize) {
		y = canvas_timebars_vsize;
	} else {
		y = min (_canvas_height - 50, y);
	}
	return y;
}

void
Editor::show_verbose_canvas_cursor_with (const string & txt, int32_t xoffset, int32_t yoffset)
{
	verbose_canvas_cursor->property_text() = txt.c_str();

	int x, y;
	double wx, wy;

	track_canvas->get_pointer (x, y);
	track_canvas->window_to_world (x, y, wx, wy);

	wx += xoffset;
	wy += yoffset;

	/* don't get too close to the edge */
	verbose_canvas_cursor->property_x() = clamp_verbose_cursor_x (wx);
	verbose_canvas_cursor->property_y() = clamp_verbose_cursor_y (wy);

	show_verbose_canvas_cursor ();
}

void
Editor::set_verbose_canvas_cursor (const string & txt, double x, double y)
{
	verbose_canvas_cursor->property_text() = txt.c_str();
	/* don't get too close to the edge */
	verbose_canvas_cursor->property_x() = clamp_verbose_cursor_x (x);
	verbose_canvas_cursor->property_y() = clamp_verbose_cursor_y (y);
}

void
Editor::set_verbose_canvas_cursor_text (const string & txt)
{
	verbose_canvas_cursor->property_text() = txt.c_str();
}

void
Editor::set_edit_mode (EditMode m)
{
	Config->set_edit_mode (m);
}

void
Editor::cycle_edit_mode ()
{
	switch (Config->get_edit_mode()) {
	case Slide:
		if (Profile->get_sae()) {
			Config->set_edit_mode (Lock);
		} else {
			Config->set_edit_mode (Splice);
		}
		break;
	case Splice:
		Config->set_edit_mode (Lock);
		break;
	case Lock:
		Config->set_edit_mode (Slide);
		break;
	}
}

void
Editor::edit_mode_selection_done ()
{
	Config->set_edit_mode (string_to_edit_mode (edit_mode_selector.get_active_text ()));
}

void
Editor::snap_type_selection_done ()
{
	string choice = snap_type_selector.get_active_text();
	SnapType snaptype = SnapToBeat;

	if (choice == _("Beats/2")) {
		snaptype = SnapToBeatDiv2;
	} else if (choice == _("Beats/3")) {
		snaptype = SnapToBeatDiv3;
	} else if (choice == _("Beats/4")) {
		snaptype = SnapToBeatDiv4;
	} else if (choice == _("Beats/5")) {
		snaptype = SnapToBeatDiv5;
	} else if (choice == _("Beats/6")) {
		snaptype = SnapToBeatDiv6;
	} else if (choice == _("Beats/7")) {
		snaptype = SnapToBeatDiv7;
	} else if (choice == _("Beats/8")) {
		snaptype = SnapToBeatDiv8;
	} else if (choice == _("Beats/10")) {
		snaptype = SnapToBeatDiv10;
	} else if (choice == _("Beats/12")) {
		snaptype = SnapToBeatDiv12;
	} else if (choice == _("Beats/14")) {
		snaptype = SnapToBeatDiv14;
	} else if (choice == _("Beats/16")) {
		snaptype = SnapToBeatDiv16;
	} else if (choice == _("Beats/20")) {
		snaptype = SnapToBeatDiv20;
	} else if (choice == _("Beats/24")) {
		snaptype = SnapToBeatDiv24;
	} else if (choice == _("Beats/28")) {
		snaptype = SnapToBeatDiv28;
	} else if (choice == _("Beats/32")) {
		snaptype = SnapToBeatDiv32;
	} else if (choice == _("Beats")) {
		snaptype = SnapToBeat;
	} else if (choice == _("Bars")) {
		snaptype = SnapToBar;
	} else if (choice == _("Marks")) {
		snaptype = SnapToMark;
	} else if (choice == _("Region starts")) {
		snaptype = SnapToRegionStart;
	} else if (choice == _("Region ends")) {
		snaptype = SnapToRegionEnd;
	} else if (choice == _("Region bounds")) {
		snaptype = SnapToRegionBoundary;
	} else if (choice == _("Region syncs")) {
		snaptype = SnapToRegionSync;
	} else if (choice == _("CD Frames")) {
		snaptype = SnapToCDFrame;
	} else if (choice == _("Timecode Frames")) {
		snaptype = SnapToTimecodeFrame;
	} else if (choice == _("Timecode Seconds")) {
		snaptype = SnapToTimecodeSeconds;
	} else if (choice == _("Timecode Minutes")) {
		snaptype = SnapToTimecodeMinutes;
	} else if (choice == _("Seconds")) {
		snaptype = SnapToSeconds;
	} else if (choice == _("Minutes")) {
		snaptype = SnapToMinutes;
	}

	RefPtr<RadioAction> ract = snap_type_action (snaptype);
	if (ract) {
		ract->set_active ();
	}
}

void
Editor::snap_mode_selection_done ()
{
	string choice = snap_mode_selector.get_active_text();
	SnapMode mode = SnapNormal;

	if (choice == _("No Grid")) {
		mode = SnapOff;
	} else if (choice == _("Grid")) {
		mode = SnapNormal;
	} else if (choice == _("Magnetic")) {
		mode = SnapMagnetic;
	}

	RefPtr<RadioAction> ract = snap_mode_action (mode);

	if (ract) {
		ract->set_active (true);
	}
}

void
Editor::cycle_edit_point (bool with_marker)
{
	switch (_edit_point) {
	case EditAtMouse:
		set_edit_point_preference (EditAtPlayhead);
		break;
	case EditAtPlayhead:
		if (with_marker) {
			set_edit_point_preference (EditAtSelectedMarker);
		} else {
			set_edit_point_preference (EditAtMouse);
		}
		break;
	case EditAtSelectedMarker:
		set_edit_point_preference (EditAtMouse);
		break;
	}
}

void
Editor::edit_point_selection_done ()
{
	string choice = edit_point_selector.get_active_text();
	EditPoint ep = EditAtSelectedMarker;

	if (choice == _("Marker")) {
		set_edit_point_preference (EditAtSelectedMarker);
	} else if (choice == _("Playhead")) {
		set_edit_point_preference (EditAtPlayhead);
	} else {
		set_edit_point_preference (EditAtMouse);
	}

	RefPtr<RadioAction> ract = edit_point_action (ep);

	if (ract) {
		ract->set_active (true);
	}
}

void
Editor::zoom_focus_selection_done ()
{
	string choice = zoom_focus_selector.get_active_text();
	ZoomFocus focus_type = ZoomFocusLeft;

	if (choice == _("Left")) {
		focus_type = ZoomFocusLeft;
	} else if (choice == _("Right")) {
		focus_type = ZoomFocusRight;
	} else if (choice == _("Center")) {
		focus_type = ZoomFocusCenter;
	} else if (choice == _("Playhead")) {
		focus_type = ZoomFocusPlayhead;
	} else if (choice == _("Mouse")) {
		focus_type = ZoomFocusMouse;
	} else if (choice == _("Edit point")) {
		focus_type = ZoomFocusEdit;
	}

	RefPtr<RadioAction> ract = zoom_focus_action (focus_type);

	if (ract) {
		ract->set_active ();
	}
}

gint
Editor::edit_controls_button_release (GdkEventButton* ev)
{
	if (Keyboard::is_context_menu_event (ev)) {
		ARDOUR_UI::instance()->add_route (this);
	}
	return TRUE;
}

gint
Editor::mouse_select_button_release (GdkEventButton* ev)
{
	/* this handles just right-clicks */

	if (ev->button != 3) {
		return false;
	}

	return true;
}

void
Editor::set_zoom_focus (ZoomFocus f)
{
	string str = zoom_focus_strings[(int)f];

	if (str != zoom_focus_selector.get_active_text()) {
		zoom_focus_selector.set_active_text (str);
	}

	if (zoom_focus != f) {
		zoom_focus = f;

		ZoomFocusChanged (); /* EMIT_SIGNAL */

		instant_save ();
	}
}

void
Editor::ensure_float (Window& win)
{
	win.set_transient_for (*this);
}

void
Editor::pane_allocation_handler (Allocation &alloc, Paned* which)
{
	/* recover or initialize pane positions. do this here rather than earlier because
	   we don't want the positions to change the child allocations, which they seem to do.
	 */

	int pos;
	XMLProperty* prop;
	char buf[32];
	XMLNode* node = ARDOUR_UI::instance()->editor_settings();
	int width, height;

	enum Pane {
		Horizontal = 0x1,
		Vertical = 0x2
	};

	static Pane done;
	
	XMLNode* geometry;

	width = default_width;
	height = default_height;

	if ((geometry = find_named_node (*node, "geometry")) != 0) {

		prop = geometry->property ("x-size");
		if (prop) {
			width = atoi (prop->value());
		}
		prop = geometry->property ("y-size");
		if (prop) {
			height = atoi (prop->value());
		}
	}

	if (which == static_cast<Paned*> (&edit_pane)) {

		if (done & Horizontal) {
			return;
		}

		if (!geometry || (prop = geometry->property ("edit-horizontal-pane-pos")) == 0) {
			/* initial allocation is 90% to canvas, 10% to notebook */
			pos = (int) floor (alloc.get_width() * 0.90f);
			snprintf (buf, sizeof(buf), "%d", pos);
		} else {
			pos = atoi (prop->value());
		}

		if (GTK_WIDGET(edit_pane.gobj())->allocation.width > pos) {
			edit_pane.set_position (pos);
			pre_maximal_horizontal_pane_position = pos;
		}

		done = (Pane) (done | Horizontal);
		
	} else if (which == static_cast<Paned*> (&editor_summary_pane)) {

		if (done & Vertical) {
			return;
		}

		if (!geometry || (prop = geometry->property ("edit-vertical-pane-pos")) == 0) {
			/* initial allocation is 90% to canvas, 10% to summary */
			pos = (int) floor (alloc.get_height() * 0.90f);
			snprintf (buf, sizeof(buf), "%d", pos);
		} else {
			pos = atoi (prop->value());
		}

		if (GTK_WIDGET(editor_summary_pane.gobj())->allocation.height > pos) {
			editor_summary_pane.set_position (pos);
			pre_maximal_vertical_pane_position = pos;
		}

		done = (Pane) (done | Vertical);
	}
}

void
Editor::detach_tearoff (Box* /*b*/, Window* /*w*/)
{
	if (_tools_tearoff->torn_off() && _mouse_mode_tearoff->torn_off()) {
		top_hbox.remove (toolbar_frame);
	}
}

void
Editor::reattach_tearoff (Box* /*b*/, Window* /*w*/, int32_t /*n*/)
{
	if (toolbar_frame.get_parent() == 0) {
		top_hbox.pack_end (toolbar_frame);
	}
}

void
Editor::set_show_measures (bool yn)
{
	if (_show_measures != yn) {
		hide_measures ();

		if ((_show_measures = yn) == true) {
			if (tempo_lines)
				tempo_lines->show();
			draw_measures ();
		}
		instant_save ();
	}
}

void
Editor::toggle_follow_playhead ()
{
	RefPtr<Action> act = ActionManager::get_action (X_("Editor"), X_("toggle-follow-playhead"));
	if (act) {
		RefPtr<ToggleAction> tact = RefPtr<ToggleAction>::cast_dynamic(act);
		set_follow_playhead (tact->get_active());
	}
}

void
Editor::set_follow_playhead (bool yn)
{
	if (_follow_playhead != yn) {
		if ((_follow_playhead = yn) == true) {
			/* catch up */
			reset_x_origin_to_follow_playhead ();
		}
		instant_save ();
	}
}

void
Editor::toggle_stationary_playhead ()
{
	RefPtr<Action> act = ActionManager::get_action (X_("Editor"), X_("toggle-stationary-playhead"));
	if (act) {
		RefPtr<ToggleAction> tact = RefPtr<ToggleAction>::cast_dynamic(act);
		set_stationary_playhead (tact->get_active());
	}
}

void
Editor::set_stationary_playhead (bool yn)
{
	if (_stationary_playhead != yn) {
		if ((_stationary_playhead = yn) == true) {
			/* catch up */
			// FIXME need a 3.0 equivalent of this 2.X call
			// update_current_screen ();
		}
		instant_save ();
	}
}

void
Editor::toggle_xfade_active (boost::weak_ptr<Crossfade> wxfade)
{
	boost::shared_ptr<Crossfade> xfade (wxfade.lock());
	if (xfade) {
		xfade->set_active (!xfade->active());
	}
}

void
Editor::toggle_xfade_length (boost::weak_ptr<Crossfade> wxfade)
{
	boost::shared_ptr<Crossfade> xfade (wxfade.lock());
	if (xfade) {
		xfade->set_follow_overlap (!xfade->following_overlap());
	}
}

void
Editor::edit_xfade (boost::weak_ptr<Crossfade> wxfade)
{
	boost::shared_ptr<Crossfade> xfade (wxfade.lock());

	if (!xfade) {
		return;
	}

	CrossfadeEditor cew (_session, xfade, xfade->fade_in().get_min_y(), 1.0);

	ensure_float (cew);

	switch (cew.run ()) {
	case RESPONSE_ACCEPT:
		break;
	default:
		return;
	}

	cew.apply ();
	PropertyChange all_crossfade_properties;
	all_crossfade_properties.add (ARDOUR::Properties::active);
	all_crossfade_properties.add (ARDOUR::Properties::follow_overlap);
	xfade->PropertyChanged (all_crossfade_properties);
}

PlaylistSelector&
Editor::playlist_selector () const
{
	return *_playlist_selector;
}

Evoral::MusicalTime
Editor::get_grid_type_as_beats (bool& success, framepos_t position)
{
	success = true;

	switch (_snap_type) {
	case SnapToBeat:
		return 1.0;
		break;

	case SnapToBeatDiv32:
		return 1.0/32.0;
		break;
	case SnapToBeatDiv28:
		return 1.0/28.0;
		break;
	case SnapToBeatDiv24:
		return 1.0/24.0;
		break;
	case SnapToBeatDiv20:
		return 1.0/20.0;
		break;
	case SnapToBeatDiv16:
		return 1.0/16.0;
		break;
	case SnapToBeatDiv14:
		return 1.0/14.0;
		break;
	case SnapToBeatDiv12:
		return 1.0/12.0;
		break;
	case SnapToBeatDiv10:
		return 1.0/10.0;
		break;
	case SnapToBeatDiv8:
		return 1.0/8.0;
		break;
	case SnapToBeatDiv7:
		return 1.0/7.0;
		break;
	case SnapToBeatDiv6:
		return 1.0/6.0;
		break;
	case SnapToBeatDiv5:
		return 1.0/5.0;
		break;
	case SnapToBeatDiv4:
		return 1.0/4.0;
		break;
	case SnapToBeatDiv3:
		return 1.0/3.0;
		break;
	case SnapToBeatDiv2:
		return 1.0/2.0;
		break;

	case SnapToBar:
		if (_session) {
			return _session->tempo_map().meter_at (position).beats_per_bar();
		}
		break;

	case SnapToCDFrame:
	case SnapToTimecodeFrame:
	case SnapToTimecodeSeconds:
	case SnapToTimecodeMinutes:
	case SnapToSeconds:
	case SnapToMinutes:
	case SnapToRegionStart:
	case SnapToRegionEnd:
	case SnapToRegionSync:
	case SnapToRegionBoundary:
	default:
		success = false;
		break;
	}

	return 0.0;
}

framecnt_t
Editor::get_nudge_distance (framepos_t pos, framecnt_t& next)
{
	framecnt_t ret;

	ret = nudge_clock.current_duration (pos);
	next = ret + 1; /* XXXX fix me */

	return ret;
}

int
Editor::playlist_deletion_dialog (boost::shared_ptr<Playlist> pl)
{
	ArdourDialog dialog (_("Playlist Deletion"));
	Label  label (string_compose (_("Playlist %1 is currently unused.\n"
					"If left alone, no audio files used by it will be cleaned.\n"
					"If deleted, audio files used by it alone by will cleaned."),
				      pl->name()));

	dialog.set_position (WIN_POS_CENTER);
	dialog.get_vbox()->pack_start (label);

	label.show ();

	dialog.add_button (_("Delete playlist"), RESPONSE_ACCEPT);
	dialog.add_button (_("Keep playlist"), RESPONSE_REJECT);
	dialog.add_button (_("Cancel"), RESPONSE_CANCEL);

	switch (dialog.run ()) {
	case RESPONSE_ACCEPT:
		/* delete the playlist */
		return 0;
		break;

	case RESPONSE_REJECT:
		/* keep the playlist */
		return 1;
		break;

	default:
		break;
	}

	return -1;
}

bool
Editor::audio_region_selection_covers (framepos_t where)
{
	for (RegionSelection::iterator a = selection->regions.begin(); a != selection->regions.end(); ++a) {
		if ((*a)->region()->covers (where)) {
			return true;
		}
	}

	return false;
}

void
Editor::prepare_for_cleanup ()
{
	cut_buffer->clear_regions ();
	cut_buffer->clear_playlists ();

	selection->clear_regions ();
	selection->clear_playlists ();

	_regions->suspend_redisplay ();
}

void
Editor::finish_cleanup ()
{
	_regions->resume_redisplay ();
}

Location*
Editor::transport_loop_location()
{
	if (_session) {
		return _session->locations()->auto_loop_location();
	} else {
		return 0;
	}
}

Location*
Editor::transport_punch_location()
{
	if (_session) {
		return _session->locations()->auto_punch_location();
	} else {
		return 0;
	}
}

bool
Editor::control_layout_scroll (GdkEventScroll* ev)
{
	if (Keyboard::some_magic_widget_has_focus()) {
		return false;
	}

	switch (ev->direction) {
	case GDK_SCROLL_UP:
		scroll_tracks_up_line ();
		return true;
		break;

	case GDK_SCROLL_DOWN:
		scroll_tracks_down_line ();
		return true;

	default:
		/* no left/right handling yet */
		break;
	}

	return false;
}

void
Editor::session_state_saved (string)
{
	update_title ();	
	_snapshots->redisplay ();
}

void
Editor::maximise_editing_space ()
{
	_mouse_mode_tearoff->set_visible (false);
	_tools_tearoff->set_visible (false);
	_zoom_tearoff->set_visible (false);

	pre_maximal_horizontal_pane_position = edit_pane.get_position ();
	pre_maximal_vertical_pane_position = editor_summary_pane.get_position ();
	pre_maximal_editor_width = this->get_width ();
	pre_maximal_editor_height = this->get_height ();

	if (post_maximal_horizontal_pane_position == 0) {
		post_maximal_horizontal_pane_position = edit_pane.get_width();
	}

	if (post_maximal_vertical_pane_position == 0) {
		post_maximal_vertical_pane_position = editor_summary_pane.get_height();
	}
	
	fullscreen ();

	if (post_maximal_editor_width) {
		edit_pane.set_position (post_maximal_horizontal_pane_position -
			abs(post_maximal_editor_width - pre_maximal_editor_width));
	} else {
		edit_pane.set_position (post_maximal_horizontal_pane_position);
	}

	if (post_maximal_editor_height) {
		editor_summary_pane.set_position (post_maximal_vertical_pane_position -
			abs(post_maximal_editor_height - pre_maximal_editor_height));
	} else {
		editor_summary_pane.set_position (post_maximal_vertical_pane_position);
	}

	if (Config->get_keep_tearoffs()) {
		_mouse_mode_tearoff->set_visible (true);
		_tools_tearoff->set_visible (true);
		_zoom_tearoff->set_visible (true);
	}

}

void
Editor::restore_editing_space ()
{
	// user changed width/height of panes during fullscreen

	if (post_maximal_horizontal_pane_position != edit_pane.get_position()) {
		post_maximal_horizontal_pane_position = edit_pane.get_position();
	}

	if (post_maximal_vertical_pane_position != editor_summary_pane.get_position()) {
		post_maximal_vertical_pane_position = editor_summary_pane.get_position();
	}
	
	unfullscreen();

	_mouse_mode_tearoff->set_visible (true);
	_tools_tearoff->set_visible (true);
	_zoom_tearoff->set_visible (true);
	post_maximal_editor_width = this->get_width();
	post_maximal_editor_height = this->get_height();

	edit_pane.set_position (pre_maximal_horizontal_pane_position + abs(this->get_width() - pre_maximal_editor_width));
	editor_summary_pane.set_position (pre_maximal_vertical_pane_position + abs(this->get_height() - pre_maximal_editor_height));
}

/**
 *  Make new playlists for a given track and also any others that belong
 *  to the same active route group with the `edit' property.
 *  @param v Track.
 */

void
Editor::new_playlists (TimeAxisView* v)
{
	begin_reversible_command (_("new playlists"));
	vector<boost::shared_ptr<ARDOUR::Playlist> > playlists;
	_session->playlists->get (playlists);
	mapover_tracks (sigc::bind (sigc::mem_fun (*this, &Editor::mapped_use_new_playlist), playlists), v, ARDOUR::Properties::edit.property_id);
	commit_reversible_command ();
}

/**
 *  Use a copy of the current playlist for a given track and also any others that belong
 *  to the same active route group with the `edit' property.
 *  @param v Track.
 */

void
Editor::copy_playlists (TimeAxisView* v)
{
	begin_reversible_command (_("copy playlists"));
	vector<boost::shared_ptr<ARDOUR::Playlist> > playlists;
	_session->playlists->get (playlists);
	mapover_tracks (sigc::bind (sigc::mem_fun (*this, &Editor::mapped_use_copy_playlist), playlists), v, ARDOUR::Properties::edit.property_id);
	commit_reversible_command ();
}

/** Clear the current playlist for a given track and also any others that belong
 *  to the same active route group with the `edit' property.
 *  @param v Track.
 */

void
Editor::clear_playlists (TimeAxisView* v)
{
	begin_reversible_command (_("clear playlists"));
	vector<boost::shared_ptr<ARDOUR::Playlist> > playlists;
	_session->playlists->get (playlists);
	mapover_tracks (sigc::mem_fun (*this, &Editor::mapped_clear_playlist), v, ARDOUR::Properties::edit.property_id);
	commit_reversible_command ();
}

void
Editor::mapped_use_new_playlist (RouteTimeAxisView& atv, uint32_t sz, vector<boost::shared_ptr<ARDOUR::Playlist> > const & playlists)
{
	atv.use_new_playlist (sz > 1 ? false : true, playlists);
}

void
Editor::mapped_use_copy_playlist (RouteTimeAxisView& atv, uint32_t sz, vector<boost::shared_ptr<ARDOUR::Playlist> > const & playlists)
{
	atv.use_copy_playlist (sz > 1 ? false : true, playlists);
}

void
Editor::mapped_clear_playlist (RouteTimeAxisView& atv, uint32_t /*sz*/)
{
	atv.clear_playlist ();
}

bool
Editor::on_key_press_event (GdkEventKey* ev)
{
	return key_press_focus_accelerator_handler (*this, ev);
}

bool
Editor::on_key_release_event (GdkEventKey* ev)
{
	return Gtk::Window::on_key_release_event (ev);
	// return key_press_focus_accelerator_handler (*this, ev);
}

/** Queue up a change to the viewport x origin.
 *  @param frame New x origin.
 */
void
Editor::reset_x_origin (framepos_t frame)
{
	queue_visual_change (frame);
}

void
Editor::reset_y_origin (double y)
{
	queue_visual_change_y (y);
}

void
Editor::reset_zoom (double fpu)
{
	queue_visual_change (fpu);
}

void
Editor::reposition_and_zoom (framepos_t frame, double fpu)
{
	reset_x_origin (frame);
	reset_zoom (fpu);

	if (!no_save_visual) {
		undo_visual_stack.push_back (current_visual_state(false));
	}
}

Editor::VisualState*
Editor::current_visual_state (bool with_tracks)
{
	VisualState* vs = new VisualState;
	vs->y_position = vertical_adjustment.get_value();
	vs->frames_per_unit = frames_per_unit;
	vs->leftmost_frame = leftmost_frame;
	vs->zoom_focus = zoom_focus;

	if (with_tracks) {
		for (TrackViewList::iterator i = track_views.begin(); i != track_views.end(); ++i) {
			vs->track_states.push_back (TAVState ((*i), &(*i)->get_state()));
		}
	}

	return vs;
}

void
Editor::undo_visual_state ()
{
	if (undo_visual_stack.empty()) {
		return;
	}

	redo_visual_stack.push_back (current_visual_state());

	VisualState* vs = undo_visual_stack.back();
	undo_visual_stack.pop_back();
	use_visual_state (*vs);
}

void
Editor::redo_visual_state ()
{
	if (redo_visual_stack.empty()) {
		return;
	}

	undo_visual_stack.push_back (current_visual_state());

	VisualState* vs = redo_visual_stack.back();
	redo_visual_stack.pop_back();
	use_visual_state (*vs);
}

void
Editor::swap_visual_state ()
{
	if (undo_visual_stack.empty()) {
		redo_visual_state ();
	} else {
		undo_visual_state ();
	}
}

void
Editor::use_visual_state (VisualState& vs)
{
	no_save_visual = true;

	_routes->suspend_redisplay ();

	vertical_adjustment.set_value (vs.y_position);

	set_zoom_focus (vs.zoom_focus);
	reposition_and_zoom (vs.leftmost_frame, vs.frames_per_unit);

	for (list<TAVState>::iterator i = vs.track_states.begin(); i != vs.track_states.end(); ++i) {
		TrackViewList::iterator t;

		/* check if the track still exists - it could have been deleted */

		if ((t = find (track_views.begin(), track_views.end(), i->first)) != track_views.end()) {
			(*t)->set_state (*(i->second), Stateful::loading_state_version);
		}
	}


	if (!vs.track_states.empty()) {
		_routes->update_visibility ();
	}

	_routes->resume_redisplay ();

	no_save_visual = false;
}

void
Editor::set_frames_per_unit (double fpu)
{
	/* this is the core function that controls the zoom level of the canvas. it is called
	   whenever one or more calls are made to reset_zoom(). it executes in an idle handler.
	*/

	if (fpu == frames_per_unit) {
		return;
	}

	if (fpu < 2.0) {
		fpu = 2.0;
	}


	/* don't allow zooms that fit more than the maximum number
	   of frames into an 800 pixel wide space.
	*/

	if (max_framepos / fpu < 800.0) {
		return;
	}

	if (tempo_lines)
		tempo_lines->tempo_map_changed();

	frames_per_unit = fpu;
	post_zoom ();
}

void
Editor::post_zoom ()
{
	// convert fpu to frame count

	framepos_t frames = (framepos_t) floor (frames_per_unit * _canvas_width);

	if (frames_per_unit != zoom_range_clock.current_duration()) {
		zoom_range_clock.set (frames);
	}

	if (mouse_mode == MouseRange && selection->time.start () != selection->time.end_frame ()) {
		for (TrackViewList::iterator i = selection->tracks.begin(); i != selection->tracks.end(); ++i) {
			(*i)->reshow_selection (selection->time);
		}
	}

	ZoomChanged (); /* EMIT_SIGNAL */

	//reset_scrolling_region ();

	if (playhead_cursor) {
		playhead_cursor->set_position (playhead_cursor->current_frame);
	}

	refresh_location_display();
	_summary->set_overlays_dirty ();

	update_marker_labels ();

	instant_save ();
}

void
Editor::queue_visual_change (framepos_t where)
{
	pending_visual_change.add (VisualChange::TimeOrigin);
	pending_visual_change.time_origin = where;
	ensure_visual_change_idle_handler ();
}

void
Editor::queue_visual_change (double fpu)
{
	pending_visual_change.add (VisualChange::ZoomLevel);
	pending_visual_change.frames_per_unit = fpu;

	ensure_visual_change_idle_handler ();
}

void
Editor::queue_visual_change_y (double y)
{
	pending_visual_change.add (VisualChange::YOrigin);
	pending_visual_change.y_origin = y;

	ensure_visual_change_idle_handler ();
}

void
Editor::ensure_visual_change_idle_handler ()
{
	if (pending_visual_change.idle_handler_id < 0) {
		pending_visual_change.idle_handler_id = g_idle_add (_idle_visual_changer, this);
	}
}

int
Editor::_idle_visual_changer (void* arg)
{
	return static_cast<Editor*>(arg)->idle_visual_changer ();
}

int
Editor::idle_visual_changer ()
{
	VisualChange::Type p = pending_visual_change.pending;
	pending_visual_change.pending = (VisualChange::Type) 0;

	double const last_time_origin = horizontal_position ();

	if (p & VisualChange::TimeOrigin) {
		/* This is a bit of a hack, but set_frames_per_unit
		   below will (if called) end up with the
		   CrossfadeViews looking at Editor::leftmost_frame,
		   and if we're changing origin and zoom in the same
		   operation it will be the wrong value unless we
		   update it here.
		*/

		leftmost_frame = pending_visual_change.time_origin;
	}

	if (p & VisualChange::ZoomLevel) {
		set_frames_per_unit (pending_visual_change.frames_per_unit);

		compute_fixed_ruler_scale ();
		compute_current_bbt_points(pending_visual_change.time_origin, pending_visual_change.time_origin + current_page_frames());
		compute_bbt_ruler_scale (pending_visual_change.time_origin, pending_visual_change.time_origin + current_page_frames());
		update_tempo_based_rulers ();
	}
	if (p & VisualChange::TimeOrigin) {
		set_horizontal_position (pending_visual_change.time_origin / frames_per_unit);
	}
	if (p & VisualChange::YOrigin) {
		vertical_adjustment.set_value (pending_visual_change.y_origin);
	}

	if (last_time_origin == horizontal_position ()) {
		/* changed signal not emitted */
		update_fixed_rulers ();
		redisplay_tempo (true);
	}

	_summary->set_overlays_dirty ();

	pending_visual_change.idle_handler_id = -1;
	return 0; /* this is always a one-shot call */
}

struct EditorOrderTimeAxisSorter {
    bool operator() (const TimeAxisView* a, const TimeAxisView* b) const {
	    return a->order () < b->order ();
    }
};

void
Editor::sort_track_selection (TrackViewList* sel)
{
	EditorOrderTimeAxisSorter cmp;

	if (sel) {
		sel->sort (cmp);
	} else {
		selection->tracks.sort (cmp);
	}
}

framepos_t
Editor::get_preferred_edit_position (bool ignore_playhead)
{
	bool ignored;
	framepos_t where = 0;
	EditPoint ep = _edit_point;

	if (entered_marker) {
		return entered_marker->position();
	}

	if (ignore_playhead && ep == EditAtPlayhead) {
		ep = EditAtSelectedMarker;
	}

	switch (ep) {
	case EditAtPlayhead:
		where = _session->audible_frame();
		break;

	case EditAtSelectedMarker:
		if (!selection->markers.empty()) {
			bool is_start;
			Location* loc = find_location_from_marker (selection->markers.front(), is_start);
			if (loc) {
				if (is_start) {
					where =  loc->start();
				} else {
					where = loc->end();
				}
				break;
			}
		}
		/* fallthru */

	default:
	case EditAtMouse:
		if (!mouse_frame (where, ignored)) {
			/* XXX not right but what can we do ? */
			return 0;
		}
		snap_to (where);
		break;
	}

	return where;
}

void
Editor::set_loop_range (framepos_t start, framepos_t end, string cmd)
{
	if (!_session) return;

	begin_reversible_command (cmd);

	Location* tll;

	if ((tll = transport_loop_location()) == 0) {
		Location* loc = new Location (*_session, start, end, _("Loop"),  Location::IsAutoLoop);
		XMLNode &before = _session->locations()->get_state();
		_session->locations()->add (loc, true);
		_session->set_auto_loop_location (loc);
		XMLNode &after = _session->locations()->get_state();
		_session->add_command (new MementoCommand<Locations>(*(_session->locations()), &before, &after));
	} else {
		XMLNode &before = tll->get_state();
		tll->set_hidden (false, this);
		tll->set (start, end);
		XMLNode &after = tll->get_state();
		_session->add_command (new MementoCommand<Location>(*tll, &before, &after));
	}

	commit_reversible_command ();
}

void
Editor::set_punch_range (framepos_t start, framepos_t end, string cmd)
{
	if (!_session) return;

	begin_reversible_command (cmd);

	Location* tpl;

	if ((tpl = transport_punch_location()) == 0) {
		Location* loc = new Location (*_session, start, end, _("Loop"),  Location::IsAutoPunch);
		XMLNode &before = _session->locations()->get_state();
		_session->locations()->add (loc, true);
		_session->set_auto_loop_location (loc);
		XMLNode &after = _session->locations()->get_state();
		_session->add_command (new MementoCommand<Locations>(*(_session->locations()), &before, &after));
	}
	else {
		XMLNode &before = tpl->get_state();
		tpl->set_hidden (false, this);
		tpl->set (start, end);
		XMLNode &after = tpl->get_state();
		_session->add_command (new MementoCommand<Location>(*tpl, &before, &after));
	}

	commit_reversible_command ();
}

/** Find regions which exist at a given time, and optionally on a given list of tracks.
 *  @param rs List to which found regions are added.
 *  @param where Time to look at.
 *  @param ts Tracks to look on; if this is empty, all tracks are examined.
 */
void
Editor::get_regions_at (RegionSelection& rs, framepos_t where, const TrackViewList& ts) const
{
	const TrackViewList* tracks;

	if (ts.empty()) {
		tracks = &track_views;
	} else {
		tracks = &ts;
	}

	for (TrackViewList::const_iterator t = tracks->begin(); t != tracks->end(); ++t) {
		RouteTimeAxisView* rtv = dynamic_cast<RouteTimeAxisView*>(*t);
		if (rtv) {
			boost::shared_ptr<Track> tr;
			boost::shared_ptr<Playlist> pl;

			if ((tr = rtv->track()) && ((pl = tr->playlist()))) {

				Playlist::RegionList* regions = pl->regions_at (
						(framepos_t) floor ( (double)where * tr->speed()));

				for (Playlist::RegionList::iterator i = regions->begin(); i != regions->end(); ++i) {
					RegionView* rv = rtv->view()->find_view (*i);
					if (rv) {
						rs.add (rv);
					}
				}

				delete regions;
			}
		}
	}
}

void
Editor::get_regions_after (RegionSelection& rs, framepos_t where, const TrackViewList& ts) const
{
	const TrackViewList* tracks;

	if (ts.empty()) {
		tracks = &track_views;
	} else {
		tracks = &ts;
	}

	for (TrackViewList::const_iterator t = tracks->begin(); t != tracks->end(); ++t) {
		RouteTimeAxisView* rtv = dynamic_cast<RouteTimeAxisView*>(*t);
		if (rtv) {
			boost::shared_ptr<Track> tr;
			boost::shared_ptr<Playlist> pl;

			if ((tr = rtv->track()) && ((pl = tr->playlist()))) {

				Playlist::RegionList* regions = pl->regions_touched (
					(framepos_t) floor ( (double)where * tr->speed()), max_framepos);

				for (Playlist::RegionList::iterator i = regions->begin(); i != regions->end(); ++i) {

					RegionView* rv = rtv->view()->find_view (*i);

					if (rv) {
						rs.push_back (rv);
					}
				}

				delete regions;
			}
		}
	}
}

/** Get regions using the following conditions:
 *    1.  If the edit point is `mouse':
 *          if the mouse is over a selected region, or no region, return all selected regions.
 *          if the mouse is over an unselected region, return just that region.
 *    2.  For all other edit points:
 *          return the selected regions AND those that are both under the edit position
 *          AND on a selected track, or on a track which is in the same active edit-enabled route group
 *          as a selected region.
 *
 *  The rationale here is that the mouse edit point is special in that its position describes
 *  both a time and a track; the other edit modes only describe a time.
 *
 *  @param rs Returned region list.
 */

RegionSelection
Editor::get_regions_from_selection_and_edit_point ()
{
	if (_edit_point == EditAtMouse) {
		if (entered_regionview == 0 || selection->regions.contains (entered_regionview)) {
			return selection->regions;
		} else {
			RegionSelection rs;
			rs.add (entered_regionview);
			return rs;
		}
	}

	/* We're using the edit point, but its not EditAtMouse */

	/* Start with selected regions */
	RegionSelection rs = selection->regions;

	TrackViewList tracks = selection->tracks;

	/* Tracks is currently the set of selected tracks; add any other tracks that
	   have regions that are in the same edit-activated route group as one of
	   our regions.
	 */
	for (RegionSelection::iterator i = rs.begin (); i != rs.end(); ++i) {
		
		RouteGroup* g = (*i)->get_time_axis_view().route_group ();
		if (g && g->is_active() && g->is_edit()) {
			tracks.add (axis_views_from_routes (g->route_list()));
		}
		
	}
	
	if (!tracks.empty()) {
		/* now find regions that are at the edit position on those tracks */
		framepos_t const where = get_preferred_edit_position ();
		get_regions_at (rs, where, tracks);
	}

	return rs;
}


RegionSelection
Editor::get_regions_from_selection_and_entered ()
{
	RegionSelection rs = selection->regions;
	
	if (rs.empty() && entered_regionview) {
		rs.add (entered_regionview);
	}

	return rs;
}

void
Editor::get_regions_corresponding_to (boost::shared_ptr<Region> region, vector<RegionView*>& regions)
{
	for (TrackViewList::iterator i = track_views.begin(); i != track_views.end(); ++i) {

		RouteTimeAxisView* tatv;

		if ((tatv = dynamic_cast<RouteTimeAxisView*> (*i)) != 0) {

			boost::shared_ptr<Playlist> pl;
			vector<boost::shared_ptr<Region> > results;
			RegionView* marv;
			boost::shared_ptr<Track> tr;

			if ((tr = tatv->track()) == 0) {
				/* bus */
				continue;
			}

			if ((pl = (tr->playlist())) != 0) {
				pl->get_region_list_equivalent_regions (region, results);
			}

			for (vector<boost::shared_ptr<Region> >::iterator ir = results.begin(); ir != results.end(); ++ir) {
				if ((marv = tatv->view()->find_view (*ir)) != 0) {
					regions.push_back (marv);
				}
			}

		}
	}
}

void
Editor::show_rhythm_ferret ()
{
	if (rhythm_ferret == 0) {
		rhythm_ferret = new RhythmFerret(*this);
	}

	rhythm_ferret->set_session (_session);
	rhythm_ferret->show ();
	rhythm_ferret->present ();
}

void
Editor::first_idle ()
{
	MessageDialog* dialog = 0;

	if (track_views.size() > 1) {
		dialog = new MessageDialog (*this,
					    string_compose (_("Please wait while %1 loads visual data"), PROGRAM_NAME),
					    true,
					    Gtk::MESSAGE_INFO,
					    Gtk::BUTTONS_NONE);
		dialog->present ();
		ARDOUR_UI::instance()->flush_pending ();
	}

	for (TrackViewList::iterator t = track_views.begin(); t != track_views.end(); ++t) {
		(*t)->first_idle();
	}

	// first idle adds route children (automation tracks), so we need to redisplay here
	_routes->redisplay ();

	delete dialog;

	_have_idled = true;
}

gboolean
Editor::_idle_resize (gpointer arg)
{
	return ((Editor*)arg)->idle_resize ();
}

void
Editor::add_to_idle_resize (TimeAxisView* view, int32_t h)
{
	if (resize_idle_id < 0) {
		resize_idle_id = g_idle_add (_idle_resize, this);
		_pending_resize_amount = 0;
	}

	/* make a note of the smallest resulting height, so that we can clamp the
	   lower limit at TimeAxisView::hSmall */

	int32_t min_resulting = INT32_MAX;

	_pending_resize_amount += h;
	_pending_resize_view = view;

	min_resulting = min (min_resulting, int32_t (_pending_resize_view->current_height()) + _pending_resize_amount);

	if (selection->tracks.contains (_pending_resize_view)) {
		for (TrackViewList::iterator i = selection->tracks.begin(); i != selection->tracks.end(); ++i) {
			min_resulting = min (min_resulting, int32_t ((*i)->current_height()) + _pending_resize_amount);
		}
	}

	if (min_resulting < 0) {
		min_resulting = 0;
	}

	/* clamp */
	if (uint32_t (min_resulting) < TimeAxisView::preset_height (HeightSmall)) {
		_pending_resize_amount += TimeAxisView::preset_height (HeightSmall) - min_resulting;
	}
}

/** Handle pending resizing of tracks */
bool
Editor::idle_resize ()
{
	_pending_resize_view->idle_resize (_pending_resize_view->current_height() + _pending_resize_amount);

	if (dynamic_cast<AutomationTimeAxisView*> (_pending_resize_view) == 0 &&
	    selection->tracks.contains (_pending_resize_view)) {

		for (TrackViewList::iterator i = selection->tracks.begin(); i != selection->tracks.end(); ++i) {
			if (*i != _pending_resize_view) {
				(*i)->idle_resize ((*i)->current_height() + _pending_resize_amount);
			}
		}
	}

	_pending_resize_amount = 0;
	flush_canvas ();
	_group_tabs->set_dirty ();
	resize_idle_id = -1;

	return false;
}

void
Editor::located ()
{
	ENSURE_GUI_THREAD (*this, &Editor::located);

	playhead_cursor->set_position (_session->audible_frame ());
	if (_follow_playhead && !_pending_initial_locate) {
		reset_x_origin_to_follow_playhead ();
	}

	_pending_locate_request = false;
	_pending_initial_locate = false;
}

void
Editor::region_view_added (RegionView *)
{
	_summary->set_dirty ();
}

TimeAxisView*
Editor::axis_view_from_route (boost::shared_ptr<Route> r) const
{
	TrackViewList::const_iterator j = track_views.begin ();
	while (j != track_views.end()) {
		RouteTimeAxisView* rtv = dynamic_cast<RouteTimeAxisView*> (*j);
		if (rtv && rtv->route() == r) {
			return rtv;
		}
		++j;
	}

	return 0;
}


TrackViewList
Editor::axis_views_from_routes (boost::shared_ptr<RouteList> r) const
{
	TrackViewList t;

	for (RouteList::const_iterator i = r->begin(); i != r->end(); ++i) {
		TimeAxisView* tv = axis_view_from_route (*i);
		if (tv) {
			t.push_back (tv);
		}
	}

	return t;
}


void
Editor::handle_new_route (RouteList& routes)
{
	ENSURE_GUI_THREAD (*this, &Editor::handle_new_route, routes)

	RouteTimeAxisView *rtv;
	list<RouteTimeAxisView*> new_views;

	for (RouteList::iterator x = routes.begin(); x != routes.end(); ++x) {
		boost::shared_ptr<Route> route = (*x);

		if (route->is_hidden() || route->is_monitor()) {
			continue;
		}

		DataType dt = route->input()->default_type();

		if (dt == ARDOUR::DataType::AUDIO) {
			rtv = new AudioTimeAxisView (*this, _session, route, *track_canvas);
		} else if (dt == ARDOUR::DataType::MIDI) {
			rtv = new MidiTimeAxisView (*this, _session, route, *track_canvas);
		} else {
			throw unknown_type();
		}

		new_views.push_back (rtv);
		track_views.push_back (rtv);

		rtv->effective_gain_display ();

		rtv->view()->RegionViewAdded.connect (sigc::mem_fun (*this, &Editor::region_view_added));
	}

	_routes->routes_added (new_views);

	if (show_editor_mixer_when_tracks_arrive) {
		show_editor_mixer (true);
	}

	editor_list_button.set_sensitive (true);

	_summary->set_dirty ();
}

void
Editor::timeaxisview_deleted (TimeAxisView *tv)
{
	if (_session && _session->deletion_in_progress()) {
		/* the situation is under control */
		return;
	}

	ENSURE_GUI_THREAD (*this, &Editor::timeaxisview_deleted, tv);

	RouteTimeAxisView* rtav = dynamic_cast<RouteTimeAxisView*> (tv);
	
	_routes->route_removed (tv);

	if (tv == entered_track) {
		entered_track = 0;
	}

	TimeAxisView::Children c = tv->get_child_list ();
	for (TimeAxisView::Children::const_iterator i = c.begin(); i != c.end(); ++i) {
		if (entered_track == i->get()) {
			entered_track = 0;
		}
	}

	/* remove it from the list of track views */

	TrackViewList::iterator i;

	if ((i = find (track_views.begin(), track_views.end(), tv)) != track_views.end()) {
		i = track_views.erase (i);
	}

	/* update whatever the current mixer strip is displaying, if revelant */

	boost::shared_ptr<Route> route;

	if (rtav) {
		route = rtav->route ();
	} 

	if (current_mixer_strip && current_mixer_strip->route() == route) {

		TimeAxisView* next_tv;

		if (track_views.empty()) {
			next_tv = 0;
		} else if (i == track_views.end()) {
			next_tv = track_views.front();
		} else {
			next_tv = (*i);
		}
		
		
		if (next_tv) {
			set_selected_mixer_strip (*next_tv);
		} else {
			/* make the editor mixer strip go away setting the
			 * button to inactive (which also unticks the menu option)
			 */
			
			ActionManager::uncheck_toggleaction ("<Actions>/Editor/show-editor-mixer");
		}
	} 
}

void
Editor::hide_track_in_display (TimeAxisView* tv, bool /*temponly*/)
{
	RouteTimeAxisView* rtv = dynamic_cast<RouteTimeAxisView*> (tv);

	if (rtv && current_mixer_strip && (rtv->route() == current_mixer_strip->route())) {
		// this will hide the mixer strip
		set_selected_mixer_strip (*tv);
	}

	_routes->hide_track_in_display (*tv);
}

bool
Editor::sync_track_view_list_and_routes ()
{
	track_views = TrackViewList (_routes->views ());

	_summary->set_dirty ();
	_group_tabs->set_dirty ();

	return false; // do not call again (until needed)
}

void
Editor::foreach_time_axis_view (sigc::slot<void,TimeAxisView&> theslot)
{
	for (TrackViewList::iterator i = track_views.begin(); i != track_views.end(); ++i) {
		theslot (**i);
	}
}

/** Find a RouteTimeAxisView by the ID of its route */
RouteTimeAxisView*
Editor::get_route_view_by_route_id (PBD::ID& id) const
{
	RouteTimeAxisView* v;

	for (TrackViewList::const_iterator i = track_views.begin(); i != track_views.end(); ++i) {
		if((v = dynamic_cast<RouteTimeAxisView*>(*i)) != 0) {
			if(v->route()->id() == id) {
				return v;
			}
		}
	}

	return 0;
}

void
Editor::fit_route_group (RouteGroup *g)
{
	TrackViewList ts = axis_views_from_routes (g->route_list ());
	fit_tracks (ts);
}

void
Editor::consider_auditioning (boost::shared_ptr<Region> region)
{
	boost::shared_ptr<AudioRegion> r = boost::dynamic_pointer_cast<AudioRegion> (region);

	if (r == 0) {
		_session->cancel_audition ();
		return;
	}

	if (_session->is_auditioning()) {
		_session->cancel_audition ();
		if (r == last_audition_region) {
			return;
		}
	}

	_session->audition_region (r);
	last_audition_region = r;
}


void
Editor::hide_a_region (boost::shared_ptr<Region> r)
{
	r->set_hidden (true);
}

void
Editor::show_a_region (boost::shared_ptr<Region> r)
{
	r->set_hidden (false);
}

void
Editor::audition_region_from_region_list ()
{
	_regions->selection_mapover (sigc::mem_fun (*this, &Editor::consider_auditioning));
}

void
Editor::hide_region_from_region_list ()
{
	_regions->selection_mapover (sigc::mem_fun (*this, &Editor::hide_a_region));
}

void
Editor::show_region_in_region_list ()
{
	_regions->selection_mapover (sigc::mem_fun (*this, &Editor::show_a_region));
}

void
Editor::step_edit_status_change (bool yn)
{
	if (yn) {
		start_step_editing ();
	} else {
		stop_step_editing ();
	}
}

void
Editor::start_step_editing ()
{
	step_edit_connection = Glib::signal_timeout().connect (sigc::mem_fun (*this, &Editor::check_step_edit), 20);
}

void
Editor::stop_step_editing ()
{
	step_edit_connection.disconnect ();
}

bool
Editor::check_step_edit ()
{
	for (TrackViewList::iterator i = track_views.begin(); i != track_views.end(); ++i) {
		MidiTimeAxisView* mtv = dynamic_cast<MidiTimeAxisView*> (*i);
		if (mtv) {
			mtv->check_step_edit ();
		}
	}

	return true; // do it again, till we stop
}

bool
Editor::horizontal_scroll_left_press ()
{
	++_scroll_callbacks;
	
	if (_scroll_connection.connected() && _scroll_callbacks < 5) {
		/* delay the first auto-repeat */
		return true;
	}
		
	double x = leftmost_position() - current_page_frames() / 5;
	if (x < 0) {
		x = 0;
	}
	
	reset_x_origin (x);

	/* do hacky auto-repeat */
	if (!_scroll_connection.connected ()) {
		_scroll_connection = Glib::signal_timeout().connect (sigc::mem_fun (*this, &Editor::horizontal_scroll_left_press), 100);
		_scroll_callbacks = 0;
	}

	return true;
}

void
Editor::horizontal_scroll_left_release ()
{
	_scroll_connection.disconnect ();
}

bool
Editor::horizontal_scroll_right_press ()
{
	++_scroll_callbacks;
	
	if (_scroll_connection.connected() && _scroll_callbacks < 5) {
		/* delay the first auto-repeat */
		return true;
	}

	reset_x_origin (leftmost_position() + current_page_frames() / 5);

	/* do hacky auto-repeat */
	if (!_scroll_connection.connected ()) {
		_scroll_connection = Glib::signal_timeout().connect (sigc::mem_fun (*this, &Editor::horizontal_scroll_right_press), 100);
		_scroll_callbacks = 0;
	}

	return true;
}

void
Editor::horizontal_scroll_right_release ()
{
	_scroll_connection.disconnect ();
}

/** Queue a change for the Editor viewport x origin to follow the playhead */
void
Editor::reset_x_origin_to_follow_playhead ()
{
	framepos_t const frame = playhead_cursor->current_frame;

	if (frame < leftmost_frame || frame > leftmost_frame + current_page_frames()) {

		if (_session->transport_speed() < 0) {
			
			if (frame > (current_page_frames() / 2)) {
				center_screen (frame-(current_page_frames()/2));
			} else {
				center_screen (current_page_frames()/2);
			}
			
		} else {
						
			if (frame < leftmost_frame) {
				/* moving left */
				framepos_t l = 0;
				if (_session->transport_rolling()) {
					/* rolling; end up with the playhead at the right of the page */
					l = frame - current_page_frames ();
				} else {
					/* not rolling: end up with the playhead 3/4 of the way along the page */
					l = frame - (3 * current_page_frames() / 4);
				}
				
				if (l < 0) {
					l = 0;
				}
				
				center_screen_internal (l + (current_page_frames() / 2), current_page_frames ());
			} else {
				/* moving right */
				if (_session->transport_rolling()) {
					/* rolling: end up with the playhead on the left of the page */
					center_screen_internal (frame + (current_page_frames() / 2), current_page_frames ());
				} else {
					/* not rolling: end up with the playhead 1/4 of the way along the page */
					center_screen_internal (frame + (current_page_frames() / 4), current_page_frames ());
				}
			}
		}
	}
}

void
Editor::super_rapid_screen_update ()
{
	if (!_session || !_session->engine().running()) {
		return;
	}

	/* METERING / MIXER STRIPS */

	/* update track meters, if required */
	if (is_mapped() && meters_running) {
		RouteTimeAxisView* rtv;
		for (TrackViewList::iterator i = track_views.begin(); i != track_views.end(); ++i) {
			if ((rtv = dynamic_cast<RouteTimeAxisView*>(*i)) != 0) {
				rtv->fast_update ();
			}
		}
	}

	/* and any current mixer strip */
	if (current_mixer_strip) {
		current_mixer_strip->fast_update ();
	}

	/* PLAYHEAD AND VIEWPORT */

	framepos_t const frame = _session->audible_frame();

	/* There are a few reasons why we might not update the playhead / viewport stuff:
	 *
	 * 1.  we don't update things when there's a pending locate request, otherwise
	 *     when the editor requests a locate there is a chance that this method
	 *     will move the playhead before the locate request is processed, causing
	 *     a visual glitch.
	 * 2.  if we're not rolling, there's nothing to do here (locates are handled elsewhere).
	 * 3.  if we're still at the same frame that we were last time, there's nothing to do.
	 */

	if (!_pending_locate_request && _session->transport_speed() != 0 && frame != last_update_frame) {

		last_update_frame = frame;

		if (!_dragging_playhead) {
			playhead_cursor->set_position (frame);
		}

		if (!_stationary_playhead) {

			if (!_dragging_playhead && _follow_playhead && _session->requested_return_frame() < 0) {
				reset_x_origin_to_follow_playhead ();
			}

		} else {
			
			/* don't do continuous scroll till the new position is in the rightmost quarter of the
			   editor canvas
			*/
#if 0                        
			// FIXME DO SOMETHING THAT WORKS HERE - this is 2.X code                         
			double target = ((double)frame - (double)current_page_frames()/2.0) / frames_per_unit;
			if (target <= 0.0) {
				target = 0.0;
			}
			if (fabs(target - current) < current_page_frames() / frames_per_unit) {
				target = (target * 0.15) + (current * 0.85);
			} else {
				/* relax */
			}
                        
			current = target;
			set_horizontal_position (current);
#endif
		}
		
	}
}


void
Editor::session_going_away ()
{
	_have_idled = false;

	_session_connections.drop_connections ();

	super_rapid_screen_update_connection.disconnect ();
	
	selection->clear ();
	cut_buffer->clear ();

	clicked_regionview = 0;
	clicked_axisview = 0;
	clicked_routeview = 0;
	clicked_crossfadeview = 0;
	entered_regionview = 0;
	entered_track = 0;
	last_update_frame = 0;
	_drags->abort ();

	playhead_cursor->canvas_item.hide ();

	/* rip everything out of the list displays */

	_regions->clear ();
	_routes->clear ();
	_route_groups->clear ();

	/* do this first so that deleting a track doesn't reset cms to null
	   and thus cause a leak.
	*/

	if (current_mixer_strip) {
		if (current_mixer_strip->get_parent() != 0) {
			global_hpacker.remove (*current_mixer_strip);
		}
		delete current_mixer_strip;
		current_mixer_strip = 0;
	}

	/* delete all trackviews */

	for (TrackViewList::iterator i = track_views.begin(); i != track_views.end(); ++i) {
		delete *i;
	}
	track_views.clear ();

	zoom_range_clock.set_session (0);
	nudge_clock.set_session (0);

	editor_list_button.set_active(false);
	editor_list_button.set_sensitive(false);

	/* clear tempo/meter rulers */
	remove_metric_marks ();
	hide_measures ();
	clear_marker_display ();

	delete current_bbt_points;
	current_bbt_points = 0;

	/* get rid of any existing editor mixer strip */

	WindowTitle title(Glib::get_application_name());
	title += _("Editor");

	set_title (title.get_string());

	SessionHandlePtr::session_going_away ();
}


void
Editor::show_editor_list (bool yn)
{
	if (yn) {
		the_notebook.show();
	} else {
		the_notebook.hide();
	}
}

void
Editor::change_region_layering_order ()
{
	framepos_t const position = get_preferred_edit_position ();
	
	if (!clicked_routeview) {
		if (layering_order_editor) {
			layering_order_editor->hide ();
		}
		return;
	}

	boost::shared_ptr<Track> track = boost::dynamic_pointer_cast<Track> (clicked_routeview->route());

	if (!track) {
		return;
	}

	boost::shared_ptr<Playlist> pl = track->playlist();

	if (!pl) {
		return;
	}
                
	if (layering_order_editor == 0) {
		layering_order_editor = new RegionLayeringOrderEditor(*this);
	}

	layering_order_editor->set_context (clicked_routeview->name(), _session, pl, position);
	layering_order_editor->maybe_present ();
}

void
Editor::update_region_layering_order_editor ()
{
	if (layering_order_editor && layering_order_editor->is_visible ()) {
		change_region_layering_order ();
	}
}

void
Editor::setup_fade_images ()
{
	_fade_in_images[FadeLinear] = new Gtk::Image (get_icon_path (X_("crossfade-in-linear")));
	_fade_in_images[FadeFast] = new Gtk::Image (get_icon_path (X_("crossfade-in-short-cut")));
	_fade_in_images[FadeLogB] = new Gtk::Image (get_icon_path (X_("crossfade-in-slow-cut")));
	_fade_in_images[FadeLogA] = new Gtk::Image (get_icon_path (X_("crossfade-in-fast-cut")));
	_fade_in_images[FadeSlow] = new Gtk::Image (get_icon_path (X_("crossfade-in-long-cut")));

	_fade_out_images[FadeLinear] = new Gtk::Image (get_icon_path (X_("crossfade-out-linear")));
	_fade_out_images[FadeFast] = new Gtk::Image (get_icon_path (X_("crossfade-out-short-cut")));
	_fade_out_images[FadeLogB] = new Gtk::Image (get_icon_path (X_("crossfade-out-slow-cut")));
	_fade_out_images[FadeLogA] = new Gtk::Image (get_icon_path (X_("crossfade-out-fast-cut")));
	_fade_out_images[FadeSlow] = new Gtk::Image (get_icon_path (X_("crossfade-out-long-cut")));
}


/** @return Gtk::manage()d menu item for a given action from `editor_actions' */
Gtk::MenuItem&
Editor::action_menu_item (std::string const & name)
{
	Glib::RefPtr<Action> a = editor_actions->get_action (name);
	assert (a);
	
	return *manage (a->create_menu_item ());
}

